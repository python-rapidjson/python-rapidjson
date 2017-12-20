# -*- coding: utf-8 -*-
# :Project:   python-rapidjson -- Circular references tests
# :Author:    John Anderson <sontek@gmail.com>
# :License:   MIT License
# :Copyright: © 2015 John Anderson
# :Copyright: © 2017 Lele Gaifax
#


import pytest


@pytest.mark.unit
def test_circular_dict(dumps):
    dct = {}
    dct['a'] = dct

    with pytest.raises(RecursionError):
        dumps(dct)


@pytest.mark.unit
def test_circular_list(dumps):
    lst = []
    lst.append(lst)

    with pytest.raises(RecursionError):
        dumps(lst)


@pytest.mark.unit
def test_circular_composite(dumps):
    dct2 = {}
    dct2['a'] = []
    dct2['a'].append(dct2)

    with pytest.raises(RecursionError):
        dumps(dct2)

@pytest.mark.unit
def test_max_recursion_depth(dumps):
    import sys

    root = child = {}
    for i in range(500):
        nephew = {'value': i}
        child['child'] = nephew
        child = nephew

    dumps(root)

    rl = sys.getrecursionlimit()
    sys.setrecursionlimit(500)
    try:
        with pytest.raises(RecursionError):
            dumps(root)
    finally:
        sys.setrecursionlimit(rl)
