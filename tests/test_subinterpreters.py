# -*- coding: utf-8 -*-
# :Project:   python-rapidjson -- Subinterpreters tests
# :Author:    Lele Gaifax <lele@metapensiero.it>
# :License:   MIT License
# :Copyright: © 2026 Lele Gaifax
#

import contextlib
import json
import os

import pytest

import rapidjson as rj

interpreters = pytest.importorskip('_interpreters',
                                   reason='this Python has no subinterpreters support')

# Ensure that the directory where the binary module lives is present on the sys.path:
# apparently a newly spawned interpreter may not have it while developing, when usually
# the module is built in-place.
MODULE_DIR = os.path.dirname(os.path.abspath(rj.__file__))

PRELUDE = f"""import sys
sys.path.insert(0, {MODULE_DIR!r})
import rapidjson as rj
"""


@contextlib.contextmanager
def interpreter():
    sub = interpreters.create()
    try:
        interpreters.run_string(sub, PRELUDE)
        yield sub
    finally:
        interpreters.destroy(sub)


def test_full_api():
    with interpreter() as sub:
        interpreters.run_string(sub, """
import datetime
assert rj.dumps({'a': [1, 2.5, None, True, 'x']}) == '{"a":[1,2.5,null,true,"x"]}'
assert rj.loads('{"a": [1, 2]}') == {'a': [1, 2]}
assert rj.Encoder(sort_keys=True)({'b': 1, 'a': 2}) == '{"a":2,"b":1}'
assert rj.Decoder()('[1, 2]') == [1, 2]
assert rj.Validator('{"type": "integer"}')('42') is None
assert rj.dumps({'a': rj.RawJSON('[1,2]')}) == '{"a":[1,2]}'
when = datetime.datetime(2020, 1, 2, 3, 4, 5)
assert rj.loads(rj.dumps(when, datetime_mode=rj.DM_ISO8601),
                datetime_mode=rj.DM_ISO8601) == when
""")


def test_independent_state(tmp_path):
    seen = tmp_path / 'ids.json'
    with interpreter() as first, interpreter() as second:
        for sub in (first, second):
            interpreters.run_string(sub, f"""
import json
with open({str(seen)!r}, 'a') as f:
    print(json.dumps([id(rj.Decoder), id(rj.ValidationError), id(rj.RawJSON)]), file=f)
""")
        # The second interpreter executed the module while the first was alive: the
        # first must still be using the types cached in its own state.
        interpreters.run_string(first, """
assert rj.dumps({'a': rj.RawJSON('[1,2]')}) == '{"a":[1,2]}'
""")

    ids = [json.loads(line) for line in seen.read_text().splitlines()]
    assert len(ids) == 2
    assert set(ids[0]).isdisjoint(ids[1])


def test_repeated_teardown():
    # Exercises module_exec/module_clear/module_free several times in the same process
    for _ in range(20):
        with interpreter() as sub:
            interpreters.run_string(sub, """
assert rj.loads(rj.dumps([1, 'x'])) == [1, 'x']
""")
