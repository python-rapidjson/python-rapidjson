import pytest
import rapidjson

# from http://json.org/JSON_checker/test/pass2.json
JSON = r'''
[[[[[[[[[[[[[[[[[[["Not too deep"]]]]]]]]]]]]]]]]]]]
'''

@pytest.mark.unit
def test_parse():
    # test in/out equivalence and parsing
    res = rapidjson.loads(JSON)
    out = rapidjson.dumps(res)
    assert res == rapidjson.loads(out)
