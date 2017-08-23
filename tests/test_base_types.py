# -*- coding: utf-8 -*-
# :Project:   python-rapidjson -- Basic tests
# :Author:    John Anderson <sontek@gmail.com>
# :License:   MIT License
# :Copyright: © 2015 John Anderson
# :Copyright: © 2016, 2017 Lele Gaifax
#

import random
import sys

import pytest

import rapidjson as rj


@pytest.mark.unit
@pytest.mark.parametrize(
    'value', (
        'A', 'cruel\x00world', 1, -1, 2.3,
        {'foo': 'bar', '\x00': 'issue57', 'issue57': '\x00'},
        [1, 2, 'a', 1.2, {'foo': 'bar'},],
        sys.maxsize, sys.maxsize**2,
))
def test_base_values(value, dumps, loads):
    dumped = dumps(value)
    loaded = loads(dumped)
    assert loaded == value and type(loaded) is type(value)


@pytest.mark.unit
def test_bytes_value(dumps):
    dumped = dumps(b'cruel\x00world')
    assert dumped == r'"cruel\u0000world"'


@pytest.mark.unit
def test_larger_structure(dumps, loads):
    value = {
        'words': """
            Lorem ipsum dolor sit amet, consectetur adipiscing
            elit. Mauris adipiscing adipiscing placerat.
            Vestibulum augue augue,
            pellentesque quis sollicitudin id, adipiscing.
            """,
        'list': list(range(200)),
        'dict': dict((str(i),'a') for i in list(range(200))),
        'int': 100100100,
        'float': 100999.123456
    }

    dumped = dumps(value)
    loaded = loads(dumped)
    assert loaded == value


@pytest.mark.unit
def test_object_hook():
    def as_complex(dct):
        if '__complex__' in dct:
            return complex(dct['real'], dct['imag'])

        return dct

    result = rj.loads(
        '{"__complex__": true, "real": 1, "imag": 2}',
        object_hook=as_complex
    )

    assert result == (1+2j)


@pytest.mark.unit
def test_end_object():
    class ComplexDecoder(rj.Decoder):
        def end_object(self, dct):
            if '__complex__' in dct:
                return complex(dct['real'], dct['imag'])

            return dct

    loads = ComplexDecoder()
    result = loads('{"__complex__": true, "real": 1, "imag": 2}')
    assert result == (1+2j)


@pytest.mark.unit
def test_dumps_default():
    def encode_complex(obj):
        if isinstance(obj, complex):
            return [obj.real, obj.imag]
        raise TypeError(repr(obj) + " is not JSON serializable")

    result = rj.dumps(2 + 1j, default=encode_complex)
    assert result == '[2.0,1.0]'


@pytest.mark.unit
def test_encoder_default():
    class ComplexEncoder(rj.Encoder):
        def default(self, obj):
            if isinstance(obj, complex):
                return [obj.real, obj.imag]
            raise TypeError(repr(obj) + " is not JSON serializable")

    dumps = ComplexEncoder()
    result = dumps(2 + 1j)
    assert result == '[2.0,1.0]'


@pytest.mark.unit
def test_doubles(dumps, loads):
    for x in range(100000):
        d = sys.maxsize * random.random()
        dumped = dumps(d)
        loaded = loads(dumped)
        assert loaded == d


@pytest.mark.unit
def test_unicode(dumps, loads):
   arabic='بينهم ان يكون مسلما رشيدا عاقلا ًوابنا شرعيا لابوين عمانيين'
   chinese='本站所提供的資料和服務都不收費，因此網站所需要的資金全來自廣告及捐款。若您願意捐款補助'
   for text in (arabic, chinese):
       dumped = dumps(text)
       loaded = loads(dumped)
       assert text == loaded


@pytest.mark.unit
def test_serialize_sets_dumps():
    def default_iterable(obj):
        if isinstance(obj, set):
            return list(obj)
        raise TypeError(repr(obj) + " is not JSON serializable")

    rj.dumps([set()], default=default_iterable)

    with pytest.raises(TypeError):
        rj.dumps([set()])


@pytest.mark.unit
def test_serialize_sets_encoder():
    class SetsEncoder(rj.Encoder):
        def default(self, obj):
            if isinstance(obj, set):
                return list(obj)
            raise TypeError(repr(obj) + " is not JSON serializable")

    dumps = SetsEncoder()
    dumps([set()])

    with pytest.raises(TypeError):
        rj.Encoder()([set()])


@pytest.mark.unit
def test_constants(dumps, loads):
    for c in [None, True, False]:
        assert loads(dumps(c)) is c
        assert loads(dumps([c]))[0] is c
        assert loads(dumps({'a': c}))['a'] is c


# TODO: Figure out what we want to do here
bad_tests = """
@pytest.mark.unit
def test_true_false():
    dumped1 = sorted(rj.dumps({True: False, False: True}))
    dumped2 = sorted(rj.dumps({
        2: 3.0,
        4.0: long_type(5),
        False: 1,
        long_type(6): True,
        "7": 0
    }))

    assert dumped1 == '{"false": true, "true": false}'
    expected = '{"2": 3.0, "4.0": 5, "6": true, "7": 0, "false": 1}'

    assert dumped2 == expected"""
