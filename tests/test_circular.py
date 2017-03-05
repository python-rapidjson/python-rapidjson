import pytest

import rapidjson as rj


@pytest.mark.unit
def test_circular_dict():
    dct = {}
    dct['a'] = dct

    with pytest.raises(OverflowError):
        rj.dumps(dct)


@pytest.mark.unit
def test_circular_list():
    lst = []
    lst.append(lst)

    with pytest.raises(OverflowError):
        rj.dumps(lst)


@pytest.mark.unit
def test_circular_composite():
    dct2 = {}
    dct2['a'] = []
    dct2['a'].append(dct2)

    with pytest.raises(OverflowError):
        rj.dumps(dct2)
