import pytest

import rapidjson as rj


# from http://json.org/JSON_checker/test/pass3.json
JSON = r'''
{
    "JSON Test Pattern pass3": {
        "The outermost value": "must be an object or array.",
        "In this test": "It is an object."
    }
}
'''

@pytest.mark.unit
def test_parse():
    # test in/out equivalence and parsing
    res = rj.loads(JSON)
    out = rj.dumps(res)
    assert res == rj.loads(out)
