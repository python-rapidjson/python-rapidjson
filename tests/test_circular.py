# -*- coding: utf-8 -*-
# :Project:   python-rapidjson -- Circular references tests
# :Author:    John Anderson <sontek@gmail.com>
# :License:   MIT License
# :Copyright: © 2015 John Anderson
# :Copyright: © 2017, 2018, 2020 Lele Gaifax
#

import sys

import pytest


if sys.version_info >= (3, 5):
    expected_exception = RecursionError
else:
    expected_exception = RuntimeError


def test_circular_dict(dumps):
    dct = {}
    dct['a'] = dct

    with pytest.raises(expected_exception):
        dumps(dct)


def test_circular_list(dumps):
    lst = []
    lst.append(lst)

    with pytest.raises(expected_exception):
        dumps(lst)


def test_circular_composite(dumps):
    dct2 = {}
    dct2['a'] = []
    dct2['a'].append(dct2)

    with pytest.raises(expected_exception):
        dumps(dct2)


def test_max_recursion_depth(dumps):
    import sys

    root = child = {}
    for i in range(500):
        nephew = {'value': i}
        child['child'] = nephew
        child = nephew

    dumps(root)

    rl = sys.getrecursionlimit()

    if hasattr(sys, "pyston_version_info"):
        # In Pyston setrecursionlimit() sets a lower bound on the number of frames,
        # not an exact count. So set the limit lower:
        sys.setrecursionlimit(100)
    else:
        # Otherwise set it to the exact value:
        sys.setrecursionlimit(500)

    try:
        with pytest.raises(expected_exception):
            dumps(root)
    finally:
        sys.setrecursionlimit(rl)
