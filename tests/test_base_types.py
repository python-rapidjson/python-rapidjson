import pytest
import sys
import rapidjson


@pytest.mark.unit
@pytest.mark.parametrize(
    'value', [
        'A', 1, -1, 2.3, {'foo': 'bar'}, [1, 2, 'a', 1.2, {'foo': 'bar'},],
        sys.maxsize
])
def test_base_values(value):
    dumped = rapidjson.dumps(value)
    loaded = rapidjson.loads(dumped)
    assert loaded == value


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
    import simplejson as json

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
