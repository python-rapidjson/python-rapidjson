import pytest

import rapidjson as rj


@pytest.mark.unit
@pytest.mark.parametrize('dumps', (rj.dumps, rj.Encoder()))
def test_circular_dict(dumps):
    dct = {}
    dct['a'] = dct

    with pytest.raises(OverflowError):
        dumps(dct)


@pytest.mark.unit
@pytest.mark.parametrize('dumps', (rj.dumps, rj.Encoder()))
def test_circular_list(dumps):
    lst = []
    lst.append(lst)

    with pytest.raises(OverflowError):
        dumps(lst)


@pytest.mark.unit
@pytest.mark.parametrize('dumps', (rj.dumps, rj.Encoder()))
def test_circular_composite(dumps):
    dct2 = {}
    dct2['a'] = []
    dct2['a'].append(dct2)

    with pytest.raises(OverflowError):
        dumps(dct2)
