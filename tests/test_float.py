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

    dumped = rj.dumps(inf, allow_nan=True)
    loaded = rj.loads(dumped, allow_nan=True)
    assert loaded == inf

    with pytest.raises(ValueError):
        rj.dumps(inf, number_mode=None)

    with pytest.raises(ValueError):
        rj.dumps(inf, allow_nan=False)

    d = Decimal(inf)
    assert d.is_infinite()

    with pytest.raises(ValueError):
        rj.dumps(d, number_mode=rj.NM_DECIMAL)

    dumped = rj.dumps(d, number_mode=rj.NM_DECIMAL, allow_nan=True)
    loaded = rj.loads(dumped, number_mode=rj.NM_DECIMAL|rj.NM_NAN)
    assert loaded == inf
    assert loaded.is_infinite()


@pytest.mark.unit
def test_negative_infinity():
    inf = float("-infinity")
    dumped = rj.dumps(inf)
    loaded = rj.loads(dumped)
    assert loaded == inf

    dumped = rj.dumps(inf, allow_nan=True)
    loaded = rj.loads(dumped, allow_nan=True)
    assert loaded == inf

    with pytest.raises(ValueError):
        rj.dumps(inf, number_mode=None)

    with pytest.raises(ValueError):
        rj.dumps(inf, allow_nan=False)

    d = Decimal(inf)
    assert d.is_infinite()

    with pytest.raises(ValueError):
        rj.dumps(d, number_mode=rj.NM_DECIMAL)

    dumped = rj.dumps(d, number_mode=rj.NM_DECIMAL|rj.NM_NAN)
    loaded = rj.loads(dumped, number_mode=rj.NM_DECIMAL, allow_nan=True)
    assert loaded == inf
    assert loaded.is_infinite()


@pytest.mark.unit
def test_nan():
    nan = float("nan")
    dumped = rj.dumps(nan)
    loaded = rj.loads(dumped)

    assert math.isnan(nan)
    assert math.isnan(loaded)

    with pytest.raises(ValueError):
        rj.dumps(nan, number_mode=None)

    with pytest.raises(ValueError):
        rj.dumps(nan, allow_nan=False)

    d = Decimal(nan)
    assert d.is_nan()

    with pytest.raises(ValueError):
        rj.dumps(d, number_mode=rj.NM_DECIMAL)

    dumped = rj.dumps(d, number_mode=rj.NM_DECIMAL|rj.NM_NAN)
    loaded = rj.loads(dumped, number_mode=rj.NM_DECIMAL|rj.NM_NAN)
    assert loaded.is_nan()
