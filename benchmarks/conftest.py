# -*- coding: utf-8 -*-
# :Project:   python-rapidjson -- Benchmarks specific pytest configuration
# :Author:    Lele Gaifax <lele@metapensiero.it>
# :License:   MIT License
# :Copyright: © 2016, 2017 Lele Gaifax
#

from collections import namedtuple
from functools import partial
from operator import attrgetter

Contender = namedtuple('Contender', 'name,dumps,loads')


# See https://github.com/ionelmc/pytest-benchmark/issues/48

def pytest_benchmark_group_stats(config, benchmarks, group_by):
    result = {}
    for bench in benchmarks:
        engine, data_kind = bench['param'].split('-')
        group = result.setdefault("%s: %s" % (data_kind, bench['group']), [])
        group.append(bench)
    return sorted(result.items())


def pytest_addoption(parser):
    parser.addoption('--compare-other-engines', action='store_true',
                     help='compare against other JSON engines')


contenders = []

import rapidjson as rj

contenders.append(Contender('rapidjson_f',
                            rj.dumps,
                            rj.loads))
contenders.append(Contender('rapidjson_c',
                            rj.Encoder(),
                            rj.Decoder()))
contenders.append(Contender('rapidjson_nn_f',
                            partial(rj.dumps, number_mode=rj.NM_NATIVE),
                            partial(rj.loads, number_mode=rj.NM_NATIVE)))
contenders.append(Contender('rapidjson_nn_c',
                            rj.Encoder(number_mode=rj.NM_NATIVE),
                            rj.Decoder(number_mode=rj.NM_NATIVE)))

numbers_contenders = [
    Contender('Wide numbers', rj.dumps, rj.loads),
    Contender('Native numbers',
              partial(rj.dumps, number_mode=rj.NM_NATIVE),
              partial(rj.loads, number_mode=rj.NM_NATIVE)),
]

try:
    import yajl
except ImportError:
    pass
else:
    contenders.append(Contender('yajl',
                                yajl.Encoder().encode,
                                yajl.Decoder().decode))

try:
    import simplejson
except ImportError:
    pass
else:
    contenders.append(Contender('simplejson',
                                simplejson.dumps,
                                simplejson.loads))

try:
    import json
except ImportError:
    pass
else:
    contenders.append(Contender('stdlib json',
                                json.dumps,
                                json.loads))

try:
    import ujson
except ImportError:
    pass
else:
    contenders.append(Contender('ujson',
                                ujson.dumps,
                                partial(ujson.loads, precise_float=True)))


def pytest_generate_tests(metafunc):
    if 'contender' in metafunc.fixturenames:
        if metafunc.config.option.compare_other_engines:
            metafunc.parametrize('contender', contenders, ids=attrgetter('name'))
        else:
            metafunc.parametrize('contender', contenders[:1], ids=attrgetter('name'))

    if 'datetimes_loads_contender' in metafunc.fixturenames:
        metafunc.parametrize('datetimes_loads_contender',
                             [rj.loads,
                              partial(rj.loads, datetime_mode=rj.DM_ISO8601)],
                             ids=['Ignore datetimes', 'Parse datetimes'])

    if 'numbers_contender' in metafunc.fixturenames:
        metafunc.parametrize('numbers_contender', numbers_contenders, ids=attrgetter('name'))
