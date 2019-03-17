# -*- coding: utf-8 -*-
# :Project:   python-rapidjson -- Test dict subclasses
# :Created:   dom 17 mar 2019 12:38:24 CET
# :Author:    Lele Gaifax <lele@metapensiero.it>
# :License:   MIT License
# :Copyright: © 2019 Lele Gaifax
#

from collections import OrderedDict
from rapidjson import Decoder


class OrderedDecoder(Decoder):
    def start_object(self):
        return OrderedDict()


def test_ordered_dict():
    od = OrderedDecoder()
    result = od('{"foo": "bar", "bar": "foo"}')
    assert isinstance(result, OrderedDict)
    assert list(result.keys()) == ["foo", "bar"]
