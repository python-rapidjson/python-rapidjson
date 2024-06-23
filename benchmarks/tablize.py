# -*- coding: utf-8 -*-
# :Project:   python-rapidjson -- reST tables writer tool
# :Author:    Lele Gaifax <lele@metapensiero.it>
# :License:   MIT License
# :Copyright: © 2017, 2018, 2020, 2021, 2022, 2024 Lele Gaifax
#

# Assume we did::
#
#  pytest --benchmark-json=comparison.json --compare-other-engines benchmarks/
#
# Read back that JSON outcome and produce three reST tables summarizing the
# comparison.
#

from collections import defaultdict, namedtuple
import sys

import rapidjson


Benchmark = namedtuple('Benchmark', 'group, name, contender')


class Comparison:
    COMPARE_FIELD = 'min'  # one of min, max, mean, median...

    def __init__(self, contenders, benchmarks):
        self.contenders = contenders
        self.timings = {}
        self.load_data(benchmarks)

    def load_data(self, benchmarks):
        for bm in benchmarks:
            group = bm['group']
            desc = bm['name'].split('[', 1)[1][:-1]
            contender, name = desc.split('-')
            if contender in self.contenders:
                bmark = Benchmark(group, name, contender)
                self.timings[bmark] = bm['stats'][self.COMPARE_FIELD]

    def normalize_and_summarize(self, group):
        table = defaultdict(dict)
        for bmark, timing in self.timings.items():
            if bmark.group == group:
                table[bmark.name][bmark.contender] = timing

        normalized = {}
        sums = defaultdict(float)
        by = self.contenders[0]
        for name, values in table.items():
            for c in self.contenders:
                sums[c] += values[c]
            normalized[name] = [values[c] / values[by] for c in self.contenders]

        normalized_sum = [sums[c] / sums[by] for c in self.contenders]

        return normalized, normalized_sum

    def tabulate(self, group, headers):
        normalized, sum = self.normalize_and_summarize(group)

        nc = len(self.contenders)
        widths = [max((len(n)+4 for n in normalized))] + \
                 [max((len(h) for h in headers))] * nc

        cells = ['-' * (width+2) for width in widths]
        seprow = '+' + '+'.join(cells) + '+'

        cells = ['=' * (width+2) for width in widths]
        headseprow = '+' + '+'.join(cells) + '+'

        cells = [f' {{:^{width}}} ' for width in widths]
        rowfmt = '|' + '|'.join(cells) + '|'

        print(seprow)
        print(rowfmt.format(*([group] + [headers[i]
                                         for i, c in enumerate(self.contenders)])))
        print(headseprow)

        def printrow(name, row):
            best = min(row)
            fvalues = [('**%.2f**' if v==best else '%.2f') % v for v in row]
            print(rowfmt.format(*([name] + fvalues)))
            print(seprow)

        for n in sorted(normalized):
            printrow(n, normalized[n])

        printrow('overall', sum)


def dumps_and_loads(benchmarks):
    contenders = ('rapidjson_f',
                  'rapidjson_c',
                  'rapidjson_nn_f',
                  'rapidjson_nn_c',
                  'rapidjson_p',
                  'simdjson',
                  'orjson',
                  'ujson',
                  'simplejson',
                  'stdlib json')
    s_headers = (r'``dumps()``\ [1]_',
                 r'``Encoder()``\ [2]_',
                 r'``dumps(n)``\ [3]_',
                 r'``Encoder(n)``\ [4]_',
                 r'``PyEncoder()``\ [5]_',
                 r'simdjson\ [6]_',
                 r'orjson\ [7]_',
                 r'ujson\ [8]_',
                 r'simplejson\ [9]_',
                 r'stdlib\ [10]_')
    d_headers = (r'``loads()``\ [11]_',
                 r'``Decoder()``\ [12]_',
                 r'``loads(n)``\ [13]_',
                 r'``Decoder(n)``\ [14]_',
                 r'``PyDecoder()``\ [15]_',
                 r'simdjson',
                 r'orjson',
                 r'ujson',
                 r'simplejson',
                 r'stdlib')

    assert len(contenders) == len(s_headers) == len(d_headers)

    from csimdjson import VERSION as simdjv
    from orjson import __version__ as orjv
    from simplejson import __version__ as sjv
    from ujson import __version__ as ujv

    pyv = '%d.%d.%d' % sys.version_info[:3]

    footnotes = [
        '.. [1] ``rapidjson.dumps()``',
        '.. [2] ``rapidjson.Encoder()``',
        '.. [3] ``rapidjson.dumps(number_mode=NM_NATIVE)``',
        '.. [4] ``rapidjson.Encoder(number_mode=NM_NATIVE)``',
        '.. [5] ``rapidjson.Encoder()`` subclassed in Python with *no-op* hook methods',
        f'.. [6] `simdjson {simdjv} <https://pypi.org/project/pysimdjson/>`__',
        f'.. [7] `orjson {orjv} <https://pypi.org/project/orjson/{orjv}/>`__',
        f'.. [8] `ujson {ujv} <https://pypi.org/project/ujson/{ujv}/>`__',
        f'.. [9] `simplejson {sjv} <https://pypi.org/pypi/simplejson/{sjv}>`__',
        f'.. [10] Python {pyv} standard library ``json``',
        '.. [11] ``rapidjson.loads()``',
        '.. [12] ``rapidjson.Decoder()``',
        '.. [13] ``rapidjson.loads(number_mode=NM_NATIVE)``',
        '.. [14] ``rapidjson.Decoder(number_mode=NM_NATIVE)``',
        '.. [15] ``rapidjson.Decoder() subclassed in Python with *no-op* hook methods``']

    comparison = Comparison(contenders, benchmarks)

    print()
    print('Serialization')
    print('~~~~~~~~~~~~~')
    print()
    comparison.tabulate('serialize', s_headers)

    print()
    print('Deserialization')
    print('~~~~~~~~~~~~~~~')
    print()
    comparison.tabulate('deserialize', d_headers)
    return footnotes


def ascii_vs_utf8(benchmarks):
    engines = (('rapidjson', 'rj'),
               ('ujson', 'uj'),
               ('simplejson', 'sj'),
               ('stdlib json', 'json'))
    contenders = [f'{e[0]} {k}' for e in engines for k in ('ascii', 'utf8')]
    i = 15
    s_headers = []
    footnotes = []
    for e in engines:
        i += 1
        s_headers.append(rf'``{e[1]} ascii``\ [{i}]_')
        footnotes.append(f'.. [{i}] ``{e[0]}.dumps(ensure_ascii=True)``')
        i += 1
        s_headers.append(rf'``{e[1]} utf8``\ [{i}]_')
        footnotes.append(f'.. [{i}] ``{e[0]}.dumps(ensure_ascii=False)``')

    comparison = Comparison(contenders, benchmarks)

    print()
    print('ASCII vs UTF-8 Serialization')
    print('~~~~~~~~~~~~~~~~~~~~~~~~~~~~')
    print()
    comparison.tabulate('serialize', s_headers)
    return footnotes


def main():
    import sys

    if len(sys.argv) > 1:
        fname = sys.argv[1]
    else:
        fname = 'comparison.json'

    with open(fname) as f:
        benchmarks = rapidjson.loads(f.read())['benchmarks']

    footnotes = []

    footnotes.extend(dumps_and_loads(benchmarks))
    footnotes.extend(ascii_vs_utf8(benchmarks))

    print()
    for fn in footnotes:
        print(fn)


if __name__ == '__main__':
    main()
