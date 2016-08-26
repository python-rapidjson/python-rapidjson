import json
import pytest
import rapidjson


@pytest.mark.unit
def test_unicode_1():
    u = '\N{GREEK SMALL LETTER ALPHA}\N{GREEK CAPITAL LETTER OMEGA}'
    s = u.encode('utf-8')
    ju = rapidjson.dumps(u)
    js = rapidjson.dumps(s)
    assert ju == js
    assert ju.lower() == json.dumps(u).lower()
    assert rapidjson.dumps(u, ensure_ascii=False) == json.dumps(u, ensure_ascii=False)


@pytest.mark.unit
def test_unicode_2():
    u = '\U0010ffff'
    s = u.encode('utf-8')
    ju = rapidjson.dumps(u)
    js = rapidjson.dumps(s)
    assert ju == js
    assert ju.lower() == json.dumps(u).lower()
    assert rapidjson.dumps(u, ensure_ascii=False) == json.dumps(u, ensure_ascii=False)


@pytest.mark.unit
def test_unicode_3():
    u = 'asdf \U0010ffff \U0001ffff qwert \uffff \u10ff \u00ff \u0080 \u7fff \b\n\r'
    s = u.encode('utf-8')
    ju = rapidjson.dumps(u)
    js = rapidjson.dumps(s)
    assert ju == js
    assert ju.lower() == json.dumps(u).lower()
    assert rapidjson.dumps(u, ensure_ascii=False) == json.dumps(u, ensure_ascii=False)
