from datetime import date, datetime, time
import math
import uuid

import pytest
import rapidjson


@pytest.mark.unit
def test_skipkeys():
    o = {True: False, -1: 1, 1.1: 1.1, (1,2): "foo", b"asdf": 1, None: None}

    with pytest.raises(TypeError):
        rapidjson.dumps(o)

    with pytest.raises(TypeError):
        rapidjson.dumps(o, skipkeys=False)

    assert rapidjson.dumps(o, skipkeys=True) == '{}'


@pytest.mark.unit
def test_ensure_ascii():
    s = '\N{GREEK SMALL LETTER ALPHA}\N{GREEK CAPITAL LETTER OMEGA}'
    assert rapidjson.dumps(s) == '"\\u03B1\\u03A9"'
    assert rapidjson.dumps(s, ensure_ascii=True) == '"\\u03B1\\u03A9"'
    assert rapidjson.dumps(s, ensure_ascii=False) == '"%s"' % s


@pytest.mark.unit
def test_allow_nan():
    f = [1.1, float("inf"), 2.2, float("nan"), 3.3, float("-inf"), 4.4]
    expected = '[1.1,Infinity,2.2,NaN,3.3,-Infinity,4.4]'
    assert rapidjson.dumps(f) == expected
    assert rapidjson.dumps(f, allow_nan=True) == expected

    with pytest.raises(ValueError):
        rapidjson.dumps(f, allow_nan=False)

    s = "NaN"
    assert math.isnan(rapidjson.loads(s))
    assert math.isnan(rapidjson.loads(s, allow_nan=True))

    with pytest.raises(ValueError):
        rapidjson.loads(s, allow_nan=False)

    s = "Infinity"
    assert rapidjson.loads(s) == float("inf")
    assert rapidjson.loads(s, allow_nan=True) == float("inf")

    with pytest.raises(ValueError):
        rapidjson.loads(s, allow_nan=False)

    s = "-Infinity"
    assert rapidjson.loads(s) == float("-inf")
    assert rapidjson.loads(s, allow_nan=True) == float("-inf")

    with pytest.raises(ValueError):
        rapidjson.loads(s, allow_nan=False)


@pytest.mark.unit
def test_native_numbers():
    f = [-1, 1, 1.1, -2.2]
    expected = '[-1,1,1.1,-2.2]'
    assert rapidjson.dumps(f, native_numbers=True) == expected
    assert rapidjson.dumps(f, native_numbers=False) == expected
    assert rapidjson.loads(expected, native_numbers=True) == f
    assert rapidjson.loads(expected, native_numbers=False) == f


@pytest.mark.unit
def test_indent():
    o = {"a": 1, "z": 2, "b": 3}
    expected1 = '{\n    "a": 1,\n    "z": 2,\n    "b": 3\n}'
    expected2 = '{\n    "a": 1,\n    "b": 3,\n    "z": 2\n}'
    expected3 = '{\n    "b": 3,\n    "a": 1,\n    "z": 2\n}'
    expected4 = '{\n    "b": 3,\n    "z": 2,\n    "a": 1\n}'
    expected5 = '{\n    "z": 2,\n    "a": 1,\n    "b": 3\n}'
    expected6 = '{\n    "z": 2,\n    "b": 3,\n    "a": 1\n}'
    expected = (
        expected1,
        expected2,
        expected3,
        expected4,
        expected5,
        expected6)

    assert rapidjson.dumps(o, indent=4) in expected

    with pytest.raises(TypeError):
        rapidjson.dumps(o, indent="\t")

    with pytest.raises(TypeError):
        rapidjson.dumps(o, indent=-1)


@pytest.mark.unit
def test_sort_keys():
    o = {"a": 1, "z": 2, "b": 3}
    expected0 = '{\n"a": 1,\n"b": 3,\n"z": 2\n}'
    expected1 = '{"a":1,"b":3,"z":2}'
    expected2 = '{\n    "a": 1,\n    "b": 3,\n    "z": 2\n}'

    assert rapidjson.dumps(o, sort_keys=True) == expected1
    assert rapidjson.dumps(o, sort_keys=True, indent=4) == expected2
    assert rapidjson.dumps(o, sort_keys=True, indent=0) == expected0


@pytest.mark.unit
def test_default():
    class Bar:
        pass

    class Foo:
        def __init__(self):
            self.foo = "bar"

    def default(obj):
        if isinstance(obj, Foo):
            return {"foo": obj.foo}

        raise TypeError("default error")

    o = {"asdf": Foo()}
    assert rapidjson.dumps(o, default=default) == '{"asdf":{"foo":"bar"}}'

    o = {"asdf": Foo(), "qwer": Bar()}
    with pytest.raises(TypeError):
        rapidjson.dumps(o, default=default)

    with pytest.raises(TypeError):
        rapidjson.dumps(o)


@pytest.mark.unit
def test_use_decimal():
    import math
    from decimal import Decimal

    dstr = "2.7182818284590452353602874713527"
    d = Decimal(dstr)

    with pytest.raises(TypeError):
        rapidjson.dumps(d)

    assert rapidjson.dumps(float(dstr)) == str(math.e)
    assert rapidjson.dumps(d, use_decimal=True) == dstr
    assert rapidjson.dumps({"foo": d}, use_decimal=True) == '{"foo":%s}' % dstr

    assert rapidjson.loads(
        rapidjson.dumps(d, use_decimal=True),
        use_decimal=True) == d

    assert rapidjson.loads(rapidjson.dumps(d, use_decimal=True)) == float(dstr)


@pytest.mark.unit
def test_max_recursion_depth():
    a = {'a': {'b': {'c': 1}}}

    assert rapidjson.dumps(a) == '{"a":{"b":{"c":1}}}'

    with pytest.raises(OverflowError):
        rapidjson.dumps(a, max_recursion_depth=2)


@pytest.mark.unit
def test_datetime_mode_dumps():
    import pytz

    assert rapidjson.DATETIME_MODE_NONE == 0
    assert rapidjson.DATETIME_MODE_ISO8601 == 1
    assert rapidjson.DATETIME_MODE_ISO8601_IGNORE_TZ == 2
    assert rapidjson.DATETIME_MODE_ISO8601_UTC == 3

    d = datetime.utcnow()
    dstr = d.isoformat()

    with pytest.raises(TypeError):
        rapidjson.dumps(d)

    with pytest.raises(ValueError):
        rapidjson.dumps(d, datetime_mode=42)

    with pytest.raises(ValueError):
        rapidjson.loads('""', datetime_mode=42)

    with pytest.raises(TypeError):
        rapidjson.dumps(d, datetime_mode=rapidjson.DATETIME_MODE_NONE)

    assert rapidjson.dumps(d, datetime_mode=rapidjson.DATETIME_MODE_ISO8601) == '"%s"' % dstr
    assert rapidjson.dumps(d, datetime_mode=rapidjson.DATETIME_MODE_ISO8601_IGNORE_TZ) == '"%s"' % dstr

    d = d.replace(tzinfo=pytz.utc)
    dstr = utcstr = d.isoformat()

    assert rapidjson.dumps(d, datetime_mode=rapidjson.DATETIME_MODE_ISO8601) == '"%s"' % dstr
    assert rapidjson.dumps(d, datetime_mode=rapidjson.DATETIME_MODE_ISO8601_IGNORE_TZ) == '"%s"' % dstr[:-6]

    d = d.astimezone(pytz.timezone('Pacific/Chatham'))
    dstr = d.isoformat()

    assert rapidjson.dumps(d, datetime_mode=rapidjson.DATETIME_MODE_ISO8601) == '"%s"' % dstr
    assert rapidjson.dumps(d, datetime_mode=rapidjson.DATETIME_MODE_ISO8601_IGNORE_TZ) == '"%s"' % dstr[:-6]

    d = d.astimezone(pytz.timezone('Asia/Kathmandu'))
    dstr = d.isoformat()

    assert rapidjson.dumps(d, datetime_mode=rapidjson.DATETIME_MODE_ISO8601) == '"%s"' % dstr
    assert rapidjson.dumps(d, datetime_mode=rapidjson.DATETIME_MODE_ISO8601_IGNORE_TZ) == '"%s"' % dstr[:-6]

    d = d.astimezone(pytz.timezone('America/New_York'))
    dstr = d.isoformat()

    assert rapidjson.dumps(d, datetime_mode=rapidjson.DATETIME_MODE_ISO8601) == '"%s"' % dstr
    assert rapidjson.dumps(d, datetime_mode=rapidjson.DATETIME_MODE_ISO8601_IGNORE_TZ) == '"%s"' % dstr[:-6]
    assert rapidjson.dumps(d, datetime_mode=rapidjson.DATETIME_MODE_ISO8601_UTC) == '"%s"' % utcstr


@pytest.mark.unit
def test_datetime_mode_loads():
    import pytz

    utc = datetime.now(pytz.utc)
    utcstr = utc.isoformat()

    jsond = rapidjson.dumps(utc, datetime_mode=rapidjson.DATETIME_MODE_ISO8601)

    assert jsond == '"%s"' % utcstr
    assert rapidjson.loads(jsond, datetime_mode=rapidjson.DATETIME_MODE_ISO8601) == utc

    local = utc.astimezone(pytz.timezone('Europe/Rome'))
    locstr = local.isoformat()

    jsond = rapidjson.dumps(local, datetime_mode=rapidjson.DATETIME_MODE_ISO8601)

    assert jsond == '"%s"' % locstr
    assert rapidjson.loads(jsond) == locstr
    assert rapidjson.loads(jsond, datetime_mode=rapidjson.DATETIME_MODE_ISO8601) == local

    load_as_utc = rapidjson.loads(jsond, datetime_mode=rapidjson.DATETIME_MODE_ISO8601_UTC)

    assert load_as_utc == utc
    assert not load_as_utc.utcoffset()

    load_as_naive = rapidjson.loads(jsond, datetime_mode=rapidjson.DATETIME_MODE_ISO8601_IGNORE_TZ)

    assert load_as_naive == local.replace(tzinfo=None)


@pytest.mark.unit
@pytest.mark.parametrize(
    'value', [date.today(), datetime.now(), time(10,20,30)])
def test_datetime_values(value):
    with pytest.raises(TypeError):
        rapidjson.dumps(value)

    dumped = rapidjson.dumps(value, datetime_mode=rapidjson.DATETIME_MODE_ISO8601)
    loaded = rapidjson.loads(dumped, datetime_mode=rapidjson.DATETIME_MODE_ISO8601)
    assert loaded == value


@pytest.mark.unit
def test_uuid_mode():
    assert rapidjson.UUID_MODE_NONE == 0
    assert rapidjson.UUID_MODE_CANONICAL == 1
    assert rapidjson.UUID_MODE_HEX == 2

    value = uuid.uuid1()
    with pytest.raises(TypeError):
        rapidjson.dumps(value)

    with pytest.raises(ValueError):
        rapidjson.dumps(value, uuid_mode=42)

    with pytest.raises(ValueError):
        rapidjson.loads('""', uuid_mode=42)

    dumped = rapidjson.dumps(value, uuid_mode=rapidjson.UUID_MODE_CANONICAL)
    loaded = rapidjson.loads(dumped, uuid_mode=rapidjson.UUID_MODE_CANONICAL)
    assert loaded == value

    # When loading, hex mode implies canonical format
    loaded = rapidjson.loads(dumped, uuid_mode=rapidjson.UUID_MODE_HEX)
    assert loaded == value

    dumped = rapidjson.dumps(value, uuid_mode=rapidjson.UUID_MODE_HEX)
    loaded = rapidjson.loads(dumped, uuid_mode=rapidjson.UUID_MODE_HEX)
    assert loaded == value


@pytest.mark.unit
def test_uuid_and_datetime_mode_together():
    value = [date.today(), uuid.uuid1()]
    dumped = rapidjson.dumps(value,
                             datetime_mode=rapidjson.DATETIME_MODE_ISO8601,
                             uuid_mode=rapidjson.UUID_MODE_CANONICAL)
    loaded = rapidjson.loads(dumped,
                             datetime_mode=rapidjson.DATETIME_MODE_ISO8601,
                             uuid_mode=rapidjson.UUID_MODE_CANONICAL)
    assert loaded == value


@pytest.mark.unit
@pytest.mark.parametrize(
    'value,cls', [
        ('x999-02-03', str),
        ('1999 02 03', str),
        ('x0:02:20', str),
        ('20.02:20', str),
        ('x999-02-03T10:20:30', str),
        ('1999-02-03t10:20:30', str),

        ('0000-01-01', str),
        ('0001-99-99', str),
        ('0001-01-32', str),
        ('0001-02-29', str),

        ('24:02:20', str),
        ('23:62:20', str),
        ('23:02:62', str),
        ('20:02:20.123-25:00', str),
        ('20:02:20.123-05:61', str),

        ('1968-02-29', date),
        ('1999-02-03', date),

        ('20:02:20', time),
        ('20:02:20Z', time),
        ('20:02:20.123', time),
        ('20:02:20.123Z', time),
        ('20:02:20-05:00', time),
        ('20:02:20.123456', time),
        ('20:02:20.123456Z', time),
        ('20:02:20.123-05:00', time),
        ('20:02:20.123456-05:00', time),

        ('1999-02-03T10:20:30', datetime),
        ('1999-02-03T10:20:30Z', datetime),
        ('1999-02-03T10:20:30.123', datetime),
        ('1999-02-03T10:20:30.123Z', datetime),
        ('1999-02-03T10:20:30-05:00', datetime),
        ('1999-02-03T10:20:30.123456', datetime),
        ('1999-02-03T10:20:30.123456Z', datetime),
        ('1999-02-03T10:20:30.123-05:00', datetime),
        ('1999-02-03T10:20:30.123456-05:00', datetime),
    ])
def test_datetime_iso8601(value, cls):
    result = rapidjson.loads('"%s"' % value, datetime_mode=rapidjson.DATETIME_MODE_ISO8601)
    assert isinstance(result, cls)


@pytest.mark.unit
@pytest.mark.parametrize(
    'value,cls', [
        ('7a683da49aa011e5972e3085a99ccac7', str),
        ('7a683da4 9aa0-11e5-972e-3085a99ccac7', str),
        ('za683da4-9aa0-11e5-972e-3085a99ccac7', str),

        ('7a683da4-9aa0-11e5-972e-3085a99ccac7', uuid.UUID),
    ])
def test_uuid_canonical(value, cls):
    result = rapidjson.loads('"%s"' % value, uuid_mode=rapidjson.UUID_MODE_CANONICAL)
    assert isinstance(result, cls), type(result)


@pytest.mark.unit
@pytest.mark.parametrize(
    'value,cls', [
        ('za683da49aa011e5972e3085a99ccac7', str),

        ('7a683da49aa011e5972e3085a99ccac7', uuid.UUID),
        ('7a683da4-9aa0-11e5-972e-3085a99ccac7', uuid.UUID),
    ])
def test_uuid_hex(value, cls):
    result = rapidjson.loads('"%s"' % value, uuid_mode=rapidjson.UUID_MODE_HEX)
    assert isinstance(result, cls), type(result)


@pytest.mark.unit
def test_object_hook():
    class Foo:
        def __init__(self, foo):
            self.foo = foo

    def hook(d):
        if 'foo' in d:
            return Foo(d['foo'])

        return d

    def default(obj):
        return {'foo': obj.foo}

    res = rapidjson.loads('{"foo": 1}', object_hook=hook)
    assert isinstance(res, Foo)
    assert res.foo == 1

    assert rapidjson.dumps(rapidjson.loads('{"foo": 1}', object_hook=hook),
            default=default) == '{"foo":1}'
    res = rapidjson.loads(rapidjson.dumps(Foo(foo="bar"), default=default),
            object_hook=hook)
    assert isinstance(res, Foo)
    assert res.foo == "bar"


@pytest.mark.unit
@pytest.mark.parametrize(
    'posargs,kwargs', (
        ( (), {} ),
        ( (None,), {} ),
        ( (True,), {} ),
        ( ('{}',), { 'this_keyword_arg_shall_never_exist': True } ),
        ( ('[]',), { 'object_hook': True } ),
        ( ('[]',), { 'datetime_mode': 'no' } ),
        ( ('[]',), { 'datetime_mode': -100 } ),
        ( ('[]',), { 'datetime_mode': 100 } ),
        ( ('[]',), { 'uuid_mode': 'no' } ),
        ( ('[]',), { 'uuid_mode': -100 } ),
        ( ('[]',), { 'uuid_mode': 100 } ),
    ))
def test_invalid_loads_params(posargs, kwargs):
    try:
        rapidjson.loads(*posargs, **kwargs)
    except (TypeError, ValueError) as e:
        pass
    else:
        assert False, "Expected either a TypeError or a ValueError"


@pytest.mark.unit
@pytest.mark.parametrize(
    'posargs,kwargs', (
        ( (), {} ),
        ( ([],), { 'this_keyword_arg_shall_never_exist': True } ),
        ( ([],), { 'default': True } ),
        ( ([],), { 'indent': -1 }),
        ( ([],), { 'datetime_mode': 'no' } ),
        ( ([],), { 'datetime_mode': -100 } ),
        ( ([],), { 'datetime_mode': 100 } ),
        ( ([],), { 'uuid_mode': 'no' } ),
        ( ([],), { 'uuid_mode': -100 } ),
        ( ([],), { 'uuid_mode': 100 } ),
    ))
def test_invalid_dumps_params(posargs, kwargs):
    try:
        rapidjson.dumps(*posargs, **kwargs)
    except (TypeError, ValueError) as e:
        pass
    else:
        assert False, "Expected either a TypeError or a ValueError"
