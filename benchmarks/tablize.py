# -*- coding: utf-8 -*-
# :Project:   python-rapidjson -- reST tables writer tool
# :Author:    Lele Gaifax <lele@metapensiero.it>
# :License:   MIT License
# :Copyright: © 2017 Lele Gaifax
#

# Assume we did::
#
#  pytest --benchmark-json=comparison.json --compare-other-engines benchmarks/
#
# Read back that JSON outcome and produce two reST tables summarizing the
# comparison.
#

from collections import defaultdict, namedtuple
import sys

import rapidjson


CONTENDERS = 'rapidjson_f,rapidjson_c,rapidjson_nn_f,rapidjson_nn_c,ujson,simplejson,json,yajl'.split(',')
S_HEADERS = r'``dumps()``\ [1]_,``Encoder()``\ [2]_,``dumps(n)``\ [3]_,``Encoder(n)``\ [4]_,ujson\ [5]_,simplejson\ [6]_,stdlib\ [7]_,yajl\ [8]_'.split(',')
D_HEADERS = r'``loads()``\ [9]_,``Decoder()``\ [10]_,``loads(n)``\ [11]_,``Decoder(n)``\ [12]_,ujson,simplejson,stdlib,yajl'.split(',')

assert len(CONTENDERS) == len(S_HEADERS) == len(D_HEADERS)

FOOTNOTES = [
    '``rapidjson.dumps()``',
    '``rapidjson.Encoder()``',
    '``rapidjson.dumps(number_mode=NM_NATIVE)``',
    '``rapidjson.Encoder(number_mode=NM_NATIVE)``',
    '`ujson 1.35 <https://pypi.python.org/pypi/ujson/1.35>`__',
    '`simplejson 3.11.1 <https://pypi.python.org/pypi/simplejson/3.11.1>`__',
    'Python %d.%d.%d standard library ``json``' % sys.version_info[:3],
    '`yajl 0.3.5 <https://pypi.python.org/pypi/yajl/0.3.5>`__',
    '``rapidjson.loads()``',
    '``rapidjson.Decoder()``',
    '``rapidjson.loads(number_mode=NM_NATIVE)``',
    '``rapidjson.Decoder(number_mode=NM_NATIVE)``']

Timings = namedtuple('Timings', 'min, max, mean, rounds, median')
Benchmark = namedtuple('Benchmark', 'group, name, contender')
SingleValue = namedtuple('SingleValue', CONTENDERS)


class Comparison:

    def __init__(self, fname):
        self.timings = timings = {}

        with open(fname) as f:
            benchmark_data = rapidjson.loads(f.read())

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

    def normalize_and_summarize(self, group, fieldname, by='rapidjson_f'):
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

    def tabulate(self, group, fieldname, headers, normalize_by='rapidjson_f'):
        normalized, sum = self.normalize_and_summarize(group, fieldname, normalize_by)

        nc = len(CONTENDERS)
        widths = [max((len(n)+4 for n in normalized))] + \
                 [max((len(h) for h in headers))] * nc

        cells = ['-' * (width+2) for width in widths]
        seprow = '+' + '+'.join(cells) + '+'

        cells = ['=' * (width+2) for width in widths]
        headseprow = '+' + '+'.join(cells) + '+'

        cells = [' {:^%d} ' % width for width in widths]
        rowfmt = '|' + '|'.join(cells) + '|'

        print(seprow)
        print(rowfmt.format(*([group] + [headers[i] for i, c in enumerate(CONTENDERS)])))
        print(headseprow)

        def printrow(name, row):
            values = [getattr(row, c) for c in CONTENDERS]
            best = min(values)
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
    print()
    comparison.tabulate('deserialize', 'mean', D_HEADERS)

    print()
    for idx, fn in enumerate(FOOTNOTES, 1):
        print('.. [%d] %s' % (idx, fn))


if __name__ == '__main__':
    main()
