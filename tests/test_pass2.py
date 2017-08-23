# -*- coding: utf-8 -*-
# :Project:   python-rapidjson -- Compliance tests
# :Author:    John Anderson <sontek@gmail.com>
# :License:   MIT License
# :Copyright: © 2015 John Anderson
# :Copyright: © 2017 Lele Gaifax
#

import pytest


# from http://json.org/JSON_checker/test/pass2.json
JSON = r'''
[[[[[[[[[[[[[[[[[[["Not too deep"]]]]]]]]]]]]]]]]]]]
'''

@pytest.mark.unit
def test_parse(dumps, loads):
    # test in/out equivalence and parsing
    res = loads(JSON)
    out = dumps(res)
    assert res == loads(out)
