# -*- coding: utf-8 -*-
# :Project:   python-rapidjson -- Tests on RawJSON
# :Author:    Silvio Tomatis <silviot@gmail.com>
# :License:   MIT License
# :Copyright: © 2018 Silvio Tomatis
# :Copyright: © 2018 Lele Gaifax
#

import pytest

import rapidjson as rj


@pytest.mark.unit
def test_instantiation_positional():
    value = b'a bytes string'
    raw = rj.RawJSON(value)
    assert raw.value == b'a bytes string'


@pytest.mark.unit
def test_instantiation_named():
    value = b'a bytes string'
    raw = rj.RawJSON(value=value)
    assert raw.value == b'a bytes string'


@pytest.mark.unit
def test_only_bytes_allowed():
    with pytest.raises(TypeError):
        rj.RawJSON('not a bytes string')
    with pytest.raises(TypeError):
        rj.RawJSON({})
    with pytest.raises(TypeError):
        rj.RawJSON(None)
