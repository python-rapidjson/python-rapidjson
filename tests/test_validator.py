import pytest

import rapidjson as rj


@pytest.mark.parametrize('schema,json', (
    ('{ "type": ["number", "string"] }', '42'),
    ('{ "type": ["number", "string"] }', '"Life, the universe, and everything"'),
))
@pytest.mark.unit
def test_valid(schema, json):
    validate = rj.Validator(schema)
    validate(json)


@pytest.mark.parametrize('schema,json,details', (
    ('{ "type": ["number", "string"] }',
     '["Life", "the universe", "and everything"]',
     ('type', '#', '#'),
    ),
))
@pytest.mark.unit
def test_invalid(schema, json, details):
    validate = rj.Validator(schema)
    with pytest.raises(ValueError) as error:
        validate(json)
    assert error.value.args == details
