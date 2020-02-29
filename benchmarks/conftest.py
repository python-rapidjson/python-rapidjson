# -*- coding: utf-8 -*-
# :Project:   python-rapidjson -- Benchmarks specific pytest configuration
# :Author:    Lele Gaifax <lele@metapensiero.it>
# :License:   MIT License
# :Copyright: © 2016, 2017, 2018, 2020 Lele Gaifax
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


# Remove parametrization data from JSON output to keep it to a reasonable size
# See https://github.com/ionelmc/pytest-benchmark/issues/96

def pytest_benchmark_update_json(config, benchmarks, output_json):
    for benchmark in output_json['benchmarks']:
        if 'data' in benchmark['params']:
            benchmark['params'].pop('data')
        if 'data' in benchmark['stats']:
            benchmark['stats'].pop('data')


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

string_contenders = [
    Contender('rapidjson utf8',
              partial(rj.dumps, ensure_ascii=False),
              rj.loads),
    Contender('rapidjson ascii',
              partial(rj.dumps, ensure_ascii=True),
              rj.loads),
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
    string_contenders.extend([
        Contender('simplejson utf8',
                  partial(simplejson.dumps, ensure_ascii=False),
                  simplejson.loads),
        Contender('simplejson ascii',
                  partial(simplejson.dumps, ensure_ascii=True),
                  simplejson.loads),
    ])

try:
    import json
except ImportError:
    pass
else:
    contenders.append(Contender('stdlib json',
                                json.dumps,
                                json.loads))
    string_contenders.extend([
        Contender('stdlib json utf8',
                  partial(json.dumps, ensure_ascii=False),
                  json.loads),
        Contender('stdlib json ascii',
                  partial(json.dumps, ensure_ascii=True),
                  json.loads),
    ])

try:
    import ujson
except ImportError:
    pass
else:
    contenders.append(Contender('ujson',
                                ujson.dumps,
                                partial(ujson.loads, precise_float=True)))
    string_contenders.extend([
        Contender('ujson utf8',
                  partial(ujson.dumps, ensure_ascii=False),
                  ujson.loads),
        Contender('ujson ascii',
                  partial(ujson.dumps, ensure_ascii=True),
                  ujson.loads),
    ])

try:
    import hyperjson
except ImportError:
    pass
else:
    contenders.append(Contender('hyperjson',
                                hyperjson.dumps,
                                hyperjson.loads))

try:
    import orjson
except ImportError:
    pass
else:
    contenders.append(Contender('orjson',
                                orjson.dumps,
                                orjson.loads))


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

    if 'string_contender' in metafunc.fixturenames:
        if metafunc.config.option.compare_other_engines:
            metafunc.parametrize('string_contender', string_contenders, ids=attrgetter('name'))
        else:
            metafunc.parametrize('string_contender',
                                 [c for c in string_contenders if 'rapidjson' in c[0]],
                                 ids=attrgetter('name'))
