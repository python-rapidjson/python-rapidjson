import pytest

import rapidjson as rj


# from http://json.org/JSON_checker/test/pass2.json
JSON = r'''
[[[[[[[[[[[[[[[[[[["Not too deep"]]]]]]]]]]]]]]]]]]]
'''

@pytest.mark.unit
def test_parse():
    # test in/out equivalence and parsing
    res = rj.loads(JSON)
    out = rj.dumps(res)
    assert res == rj.loads(out)
