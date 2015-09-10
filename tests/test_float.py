import pytest
import rapidjson


@pytest.mark.unit
def test_infinity():
    inf = float("inf")
    dumped = rapidjson.dumps(inf)
    loaded = rapidjson.loads(dumped)
    assert loaded == inf


@pytest.mark.unit
def test_negative_infinity():
    inf = float("-infinity")
    dumped = rapidjson.dumps(inf)
    loaded = rapidjson.loads(dumped)
    assert loaded == inf


@pytest.mark.unit
def test_nan():
    nan = float("nan")
    dumped = rapidjson.dumps(nan)
    loaded = rapidjson.loads(dumped)

    assert loaded == nan
