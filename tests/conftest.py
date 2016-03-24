# See https://github.com/ionelmc/pytest-benchmark/issues/48

def pytest_benchmark_group_stats(config, benchmarks, group_by):
    result = {}
    for bench in benchmarks:
        data_kind = bench.param.split('-')[0]
        result.setdefault("%s: %s" % (data_kind, bench.group), []).append(bench)
    return sorted(result.items())
