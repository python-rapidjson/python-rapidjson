from decimal import Decimal
import math

import pytest

import rapidjson as rj


@pytest.mark.unit
def test_infinity():
    inf = float("inf")
    dumped = rj.dumps(inf)
    loaded = rj.loads(dumped)
    assert loaded == inf

    d = Decimal(inf)
    dumped = rj.dumps(d, use_decimal=True)
    loaded = rj.loads(dumped, use_decimal=True)
    assert loaded == inf


@pytest.mark.unit
def test_negative_infinity():
    inf = float("-infinity")
    dumped = rj.dumps(inf)
    loaded = rj.loads(dumped)
    assert loaded == inf

    d = Decimal(inf)
    dumped = rj.dumps(d, use_decimal=True)
    loaded = rj.loads(dumped, use_decimal=True)
    assert loaded == inf


@pytest.mark.unit
def test_nan():
    nan = float("nan")
    dumped = rj.dumps(nan)
    loaded = rj.loads(dumped)

    assert math.isnan(nan)
    assert math.isnan(loaded)

    d = Decimal(nan)
    dumped = rj.dumps(d, use_decimal=True)
    loaded = rj.loads(dumped, use_decimal=True)

    assert math.isnan(d)
    assert math.isnan(loaded)
