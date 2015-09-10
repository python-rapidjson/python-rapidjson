import pytest
import rapidjson


@pytest.mark.unit
def test_unicode_1():
    u = '\N{GREEK SMALL LETTER ALPHA}\N{GREEK CAPITAL LETTER OMEGA}'
    s = u.encode('utf-8')
    ju = rapidjson.dumps(u)
    js = rapidjson.dumps(s)
    assert ju == js
