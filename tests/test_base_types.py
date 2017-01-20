import pytest
import sys
import rapidjson
import random


@pytest.mark.unit
@pytest.mark.parametrize(
    'value', [
        'A', 'cruel\x00world', 1, -1, 2.3,
        {'foo': 'bar', '\x00': 'issue57', 'issue57': '\x00'},
        [1, 2, 'a', 1.2, {'foo': 'bar'},],
        sys.maxsize, sys.maxsize**2
])
def test_base_values(value):
    dumped = rapidjson.dumps(value)
    loaded = rapidjson.loads(dumped)
    assert loaded == value and type(loaded) is type(value)


@pytest.mark.unit
def test_bytes_value():
    dumped = rapidjson.dumps(b'cruel\x00world')
    assert dumped == r'"cruel\u0000world"'


@pytest.mark.unit
def test_larger_structure():
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

    dumped = rapidjson.dumps(value)
    loaded = rapidjson.loads(dumped)

    assert loaded == value


@pytest.mark.unit
def test_object_hook():
    def as_complex(dct):
        if '__complex__' in dct:
            return complex(dct['real'], dct['imag'])

        return dct

    result = rapidjson.loads(
        '{"__complex__": true, "real": 1, "imag": 2}',
        object_hook=as_complex
    )

    assert result == (1+2j)


@pytest.mark.unit
def test_default():
    def encode_complex(obj):
        if isinstance(obj, complex):
            return [obj.real, obj.imag]

        raise TypeError(repr(obj) + " is not JSON serializable")

    result = rapidjson.dumps(2 + 1j, default=encode_complex)
    assert result == '[2.0,1.0]'


@pytest.mark.unit
def test_doubles():
    for x in range(100000):
        d = sys.maxsize * random.random()
        dumped = rapidjson.dumps(d)
        loaded = rapidjson.loads(dumped)
        assert loaded == d


@pytest.mark.unit
def test_unicode():
   arabic='بينهم ان يكون مسلما رشيدا عاقلا ًوابنا شرعيا لابوين عمانيين'
   chinese='本站所提供的資料和服務都不收費，因此網站所需要的資金全來自廣告及捐款。若您願意捐款補助'

   for text in [arabic, chinese]:
       dumped = rapidjson.dumps(text)
       loaded = rapidjson.loads(dumped)
       assert text == loaded


@pytest.mark.unit
def test_serialize_sets():
    def default_iterable(obj):
        return list(obj)

    rapidjson.dumps([set()], default=default_iterable)

    with pytest.raises(TypeError):
        rapidjson.dumps([set()])


@pytest.mark.unit
def test_constants():
    for c in [None, True, False]:
        assert rapidjson.loads(rapidjson.dumps(c)) is c
        assert rapidjson.loads(rapidjson.dumps([c]))[0] is c
        assert rapidjson.loads(rapidjson.dumps({'a': c}))['a'] is c


# TODO: Figure out what we want to do here
bad_tests = """
@pytest.mark.unit
def test_true_false():
    dumped1 = sorted(rapidjson.dumps({True: False, False: True}))
    dumped2 = sorted(rapidjson.dumps({
        2: 3.0,
        4.0: long_type(5),
        False: 1,
        long_type(6): True,
        "7": 0
    }))

    assert dumped1 == '{"false": true, "true": false}'
    expected = '{"2": 3.0, "4.0": 5, "6": true, "7": 0, "false": 1}'

    assert dumped2 == expected"""
