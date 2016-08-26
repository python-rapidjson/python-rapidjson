import math
import pytest
import rapidjson
from decimal import Decimal


@pytest.mark.unit
def test_infinity():
    inf = float("inf")
    dumped = rapidjson.dumps(inf)
    loaded = rapidjson.loads(dumped)
    assert loaded == inf

    d = Decimal(inf)
    dumped = rapidjson.dumps(d, use_decimal=True)
    loaded = rapidjson.loads(dumped, use_decimal=True)
    assert loaded == inf


@pytest.mark.unit
def test_negative_infinity():
    inf = float("-infinity")
    dumped = rapidjson.dumps(inf)
    loaded = rapidjson.loads(dumped)
    assert loaded == inf

    d = Decimal(inf)
    dumped = rapidjson.dumps(d, use_decimal=True)
    loaded = rapidjson.loads(dumped, use_decimal=True)
    assert loaded == inf


@pytest.mark.unit
def test_nan():
    nan = float("nan")
    dumped = rapidjson.dumps(nan)
    loaded = rapidjson.loads(dumped)

    assert math.isnan(nan)
    assert math.isnan(loaded)

    d = Decimal(nan)
    dumped = rapidjson.dumps(d, use_decimal=True)
    loaded = rapidjson.loads(dumped, use_decimal=True)

    assert math.isnan(d)
    assert math.isnan(loaded)
