# See https://github.com/ionelmc/pytest-benchmark/issues/48

def pytest_benchmark_group_stats(config, benchmarks, group_by):
    result = {}
    for bench in benchmarks:
        data_kind = bench.param.split('-')[0]
        if 'not precise)' in data_kind:
            group = result.setdefault("not precise floats: %s" % bench.group, [])
        else:
            group = result.setdefault("%s: %s" % (data_kind, bench.group), [])
        group.append(bench)
    return sorted(result.items())
