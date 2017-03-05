import json

import pytest

import rapidjson as rj


@pytest.mark.unit
@pytest.mark.parametrize('u', [
    '\N{GREEK SMALL LETTER ALPHA}\N{GREEK CAPITAL LETTER OMEGA}',
    '\U0010ffff',
    'asdf \U0010ffff \U0001ffff qwert \uffff \u10ff \u00ff \u0080 \u7fff \b\n\r',
])
def test_unicode(u):
    s = u.encode('utf-8')
    ju = rj.dumps(u)
    js = rj.dumps(s)
    assert ju == js
    assert ju.lower() == json.dumps(u).lower()
    assert rj.dumps(u, ensure_ascii=False) == json.dumps(u, ensure_ascii=False)
