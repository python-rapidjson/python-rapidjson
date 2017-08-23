# -*- coding: utf-8 -*-
# :Project:   python-rapidjson -- Unicode tests
# :Author:    John Anderson <sontek@gmail.com>
# :License:   MIT License
# :Copyright: © 2015 John Anderson
# :Copyright: © 2016, 2017 Lele Gaifax
#

import json

import pytest


@pytest.mark.unit
@pytest.mark.parametrize('u', [
    '\N{GREEK SMALL LETTER ALPHA}\N{GREEK CAPITAL LETTER OMEGA}',
    '\U0010ffff',
    'asdf \U0010ffff \U0001ffff qwert \uffff \u10ff \u00ff \u0080 \u7fff \b\n\r',
])
def test_unicode(u, dumps, loads):
    s = u.encode('utf-8')
    ju = dumps(u)
    js = dumps(s)
    assert ju == js
    assert ju.lower() == json.dumps(u).lower()
    assert dumps(u, ensure_ascii=False) == json.dumps(u, ensure_ascii=False)
