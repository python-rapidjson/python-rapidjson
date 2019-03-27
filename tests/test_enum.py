# -*- coding: utf-8 -*-
# :Project:   python-rapidjson -- Test on stdlib enums
# :Created:   mer 27 mar 2019 08:13:23 CET
# :Author:    Lele Gaifax <lele@metapensiero.it>
# :License:   MIT License
# :Copyright: © 2019 Lele Gaifax
#

import enum

import pytest

import rapidjson as rj


@pytest.mark.unit
def test_intenums_as_ints():
    class IE(enum.IntEnum):
        val = 123
        bigval = 123123123123123123123123

    assert rj.dumps([IE.val, IE.bigval]) == "[123,123123123123123123123123]"
