import io
from concurrent.futures import ThreadPoolExecutor
from threading import Barrier

import pytest

from rapidjson import Decoder, JSONDecodeError, dumps


@pytest.mark.parametrize('source', [
    lambda s: s,
    lambda s: s.encode(),
    lambda s: bytearray(s.encode()),
    io.StringIO,
    lambda s: io.BytesIO(s.encode()),
])
def test_callback_pointers(source):
    events = []

    class RecordingDecoder(Decoder):
        def start_object(self):
            events.append(('start_object', self.pointer))
            return {}

        def end_object(self, obj):
            events.append(('end_object', self.pointer))
            return obj

        def end_array(self, obj):
            events.append(('end_array', self.pointer))
            return tuple(obj)

        def string(self, value):
            events.append(('string', self.pointer))
            return value.upper()

    decoder = RecordingDecoder()
    assert decoder.pointer is None
    data = {'hello': 'world', 'obj': {'~a/b': [
        {}, [], 'str', None, 42, {'': '\u20ac'},
    ]}, 'arr': [2, 4, 'six'], 'empty': {}}
    assert decoder(source(dumps(data)), chunk_size=4) == {
        'hello': 'WORLD', 'obj': {'~a/b': (
            {}, (), 'STR', None, 42, {'': '\u20ac'},
        )}, 'arr': (2, 4, 'SIX'), 'empty': {},
    }
    assert events == [
        ('start_object', ''),
        ('string', '/hello'),
        ('start_object', '/obj'),
        ('start_object', '/obj/~0a~1b/0'),
        ('end_object', '/obj/~0a~1b/0'),
        ('end_array', '/obj/~0a~1b/1'),
        ('string', '/obj/~0a~1b/2'),
        ('start_object', '/obj/~0a~1b/5'),
        ('string', '/obj/~0a~1b/5/'),
        ('end_object', '/obj/~0a~1b/5'),
        ('end_array', '/obj/~0a~1b'),
        ('end_object', '/obj'),
        ('string', '/arr/2'),
        ('end_array', '/arr'),
        ('start_object', '/empty'),
        ('end_object', '/empty'),
        ('end_object', ''),
    ]
    assert decoder.pointer is None


@pytest.mark.parametrize('value', ['root', [], {}, ['root'], [[{}, 'root']]])
def test_root_and_nested_arrays(value):
    pointers = []

    class RecordingDecoder(Decoder):
        def start_object(self):
            pointers.append(self.pointer)
            return {}

        def end_array(self, obj):
            pointers.append(self.pointer)
            return obj

        def string(self, value):
            pointers.append(self.pointer)
            return value

    expected = {
        '"root"': [''], '[]': [''], '{}': [''],
        '["root"]': ['/0', ''],
        '[[{},"root"]]': ['/0/0', '/0/1', '/0', ''],
    }
    decoder = RecordingDecoder()
    assert decoder(dumps(value)) == value
    assert pointers == expected[dumps(value)]


@pytest.mark.parametrize('key', ['', '/', '~', '~1', '~/~', '\u20ac\U0001f600', '\x00'])
@pytest.mark.parametrize('stream', [False, True])
def test_pointer_key_escaping(key, stream):
    class PointerDecoder(Decoder):
        def string(self, value):
            return self.pointer

    pointer = '/' + key.replace('~', '~0').replace('/', '~1')
    encoded = dumps({key: 'value'}, ensure_ascii=False)
    source = io.BytesIO(encoded.encode()) if stream else encoded
    assert PointerDecoder()(source, chunk_size=4) == {key: pointer}


def test_pointer_selects_discarding_mapping():
    events = []

    class Discard(dict):
        def __setitem__(self, key, value):
            pass

    class SelectingDecoder(Decoder):
        def start_object(self):
            events.append(self.pointer)
            return Discard() if self.pointer.startswith('/ignored') else {}

        def string(self, value):
            events.append(self.pointer)
            return value

    decoder = SelectingDecoder()
    assert decoder('{"ignored":{"nested":{"key":"value"},"array":["item"]},'
                   '"kept":"yes"}') == {
        'ignored': {}, 'kept': 'yes',
    }
    assert events == ['', '/ignored', '/ignored/nested', '/ignored/nested/key',
                      '/ignored/array/0', '/kept']


def test_pointer_with_object_pairs_and_duplicate_keys():
    class PairsDecoder(Decoder):
        def start_object(self):
            return []

        def string(self, value):
            return self.pointer

    assert PairsDecoder()('{"a":"one","a":"two","child":{"":"three"}}') == [
        ('a', '/a'), ('a', '/a'), ('child', [('', '/child/')]),
    ]


def test_reentrant_pointer_and_callback_failure():
    events = []
    other = Decoder()

    class ReentrantDecoder(Decoder):
        def string(self, value):
            assert other.pointer is None
            events.append((value, self.pointer))
            if value == 'outer':
                assert self('{"inner":"value"}') == {'inner': 'value'}
                assert self.pointer == '/outer'
                with pytest.raises(ValueError, match='callback failure'):
                    self('{"inner":"error"}')
                assert self.pointer == '/outer'
            elif value == 'error':
                raise ValueError('callback failure')
            return value

    decoder = ReentrantDecoder()
    assert decoder('{"outer":"outer"}') == {'outer': 'outer'}
    assert events == [('outer', '/outer'), ('value', '/inner'), ('error', '/inner')]
    assert decoder.pointer is None
    with pytest.raises(ValueError, match='callback failure'):
        decoder('"error"')
    assert decoder.pointer is None
    with pytest.raises(JSONDecodeError):
        decoder('{"broken":')
    assert decoder.pointer is None
    assert decoder('"again"') == 'again'
    assert events[-1] == ('again', '')


def test_pointer_is_local_to_the_calling_thread():
    entered = Barrier(3)
    resume = Barrier(3)

    class ConcurrentDecoder(Decoder):
        def string(self, value):
            before = self.pointer
            entered.wait(timeout=10)
            resume.wait(timeout=10)
            return before, self.pointer

    decoder = ConcurrentDecoder()
    assert decoder.pointer is None
    with ThreadPoolExecutor(max_workers=2) as pool:
        left = pool.submit(decoder, '{"left":"value"}')
        right = pool.submit(decoder, '{"right":"value"}')
        entered.wait(timeout=10)
        try:
            assert decoder.pointer is None
        finally:
            resume.wait(timeout=10)
        assert left.result(timeout=10) == {'left': ('/left', '/left')}
        assert right.result(timeout=10) == {'right': ('/right', '/right')}
    assert decoder.pointer is None


def test_pointer_is_read_only():
    decoder = Decoder()
    assert decoder.pointer is None
    with pytest.raises(AttributeError):
        decoder.pointer = '/other'
    with pytest.raises(AttributeError):
        del decoder.pointer
