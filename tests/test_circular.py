import pytest
import rapidjson


@pytest.mark.unit
def test_circular_dict():
    dct = {}
    dct['a'] = dct

    with pytest.raises(OverflowError):
        rapidjson.dumps(dct)


@pytest.mark.unit
def test_circular_list():
    lst = []
    lst.append(lst)
    with pytest.raises(OverflowError):
        rapidjson.dumps(lst)


@pytest.mark.unit
def test_circular_composite():
    dct2 = {}
    dct2['a'] = []
    dct2['a'].append(dct2)

    with pytest.raises(OverflowError):
        rapidjson.dumps(dct2)
