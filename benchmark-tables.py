# Assume we did::
#
#  tox -e py36 -- -m benchmark --benchmark-json=comparison.json --compare-other-engines
#
# Read back that JSON outcome and produce two reST tables summarizing the
# comparison.
#

import json
from collections import defaultdict, namedtuple


CONTENDERS = 'rapidjson,rapidjson_nativenumbers,ujson,simplejson,json,yajl'.split(',')
S_HEADERS = 'rapidjson,native [1]_,ujson [2]_,simplejson [3]_,stdlib [4]_,yajl [5]_'.split(',')
D_HEADERS = 'rapidjson,native,ujson,simplejson,stdlib,yajl'.split(',')

assert len(CONTENDERS) == len(S_HEADERS) == len(D_HEADERS)

FOOTNOTES = ['rapidjson with ``native_numbers=True``',
             '`ujson 1.35 <https://pypi.python.org/pypi/ujson/1.35>`__',
             '`simplejson 3.10.0 <https://pypi.python.org/pypi/simplejson/3.10.0>`__',
             'Python 3.6 standard library',
             '`yajl 0.3.5 <https://pypi.python.org/pypi/yajl/0.3.5>`__']

Timings = namedtuple('Timings', 'min, max, mean, rounds, median')
Benchmark = namedtuple('Benchmark', 'group, name, contender')
SingleValue = namedtuple('SingleValue', CONTENDERS)


class Comparison:

    def __init__(self, fname):
        self.timings = timings = {}

        with open(fname) as f:
            benchmark_data = json.load(f)

        for bm in benchmark_data['benchmarks']:
            group = bm['group']
            desc = bm['name'].split('[', 1)[1][:-1]
            contender, name = desc.split('-')
            if contender == 'stdlib json':
                contender = 'json'
            if contender in CONTENDERS:
                bmark = Benchmark(group, name, contender)
                stats = bm['stats']
                timing = Timings(stats['min'], stats['max'], stats['mean'],
                                 stats['rounds'], stats['median'])
                timings[bmark] = timing

    def focalize(self, group, fieldname):
        values = defaultdict(dict)
        for bmark, timing in self.timings.items():
            if bmark.group == group:
                values[bmark.name][bmark.contender] = getattr(timing, fieldname)
        return {n: SingleValue(**values[n]) for n in values}

    def normalize_and_summarize(self, group, fieldname, by='rapidjson'):
        result = {}
        values = self.focalize(group, fieldname)
        sums = defaultdict(float)
        for name, value in values.items():
            for c in CONTENDERS:
                sums[c] += getattr(value, c)
            result[name] = SingleValue(*[(getattr(value, c) / getattr(value, by))
                                         for c in CONTENDERS])
        sum = SingleValue(**sums)

        return result, SingleValue(*[(getattr(sum, c) / getattr(sum, by))
                                     for c in CONTENDERS])

    def tabulate(self, group, fieldname, headers, normalize_by='rapidjson', omit='rapidjson'):
        normalized, sum = self.normalize_and_summarize(group, fieldname, normalize_by)

        nc = len(CONTENDERS) - 1
        widths = [max((len(n)+4 for n in normalized))] + \
                 [max((len(h) for h in headers))] * nc

        cells = ['-' * (width+2) for width in widths]
        seprow = '+' + '+'.join(cells) + '+'

        cells = ['=' * (width+2) for width in widths]
        headseprow = '+' + '+'.join(cells) + '+'

        cells = [' {:^%d} ' % width for width in widths]
        rowfmt = '|' + '|'.join(cells) + '|'

        print(seprow)
        print(rowfmt.format(*([group] + [headers[i] for i, c in enumerate(CONTENDERS)
                                         if c != omit])))
        print(headseprow)

        def printrow(name, row):
            values = [getattr(row, c) for c in CONTENDERS if c != omit]
            best = min(values)
            if best > getattr(row, omit):
                name = '**' + name + '**'
                best = 0
            fvalues = [('**%.2f**' if v==best else '%.2f') % v for v in values]
            print(rowfmt.format(*([name] + fvalues)))
            print(seprow)

        for n in sorted(normalized):
            printrow(n, normalized[n])

        printrow('overall', sum)


def main():
    import sys

    if len(sys.argv) > 1:
        fname = sys.argv[1]
    else:
        fname = 'comparison.json'

    comparison = Comparison(fname)
    comparison.tabulate('serialize', 'mean', S_HEADERS)
    comparison.tabulate('deserialize', 'mean', D_HEADERS)

    idx = 1
    print()
    for fn in FOOTNOTES:
        print('.. [%d] %s' % (idx, fn))
        idx += 1


if __name__ == '__main__':
    main()
