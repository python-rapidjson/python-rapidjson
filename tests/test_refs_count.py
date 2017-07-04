# NB: this is a simplicistic test that uses sys.gettotalrefcount(), available
# when the interpreter is built --with-pydebug, that tries to assert that
# repeated calls to dumps() and loads() does not leak object references.
# Since it's not an exact science, it should be taken with a grain of salt.

import datetime
import sys
import uuid

import pytest

import rapidjson as rj


class Foo:
    def __init__(self, foo):
        self.foo = foo


def hook(d):
    if 'foo' in d:
        return Foo(d['foo'])
    return d


def default(obj):
    return {'foo': obj.foo}


# Empirical tollerance used to test the refcount growing
THRESHOLD = 7


plain_string = 'plain-string'
bigint = 123456789
pi = 3.1415926
right_now = datetime.datetime.now()
date = right_now.date()
time = right_now.time()
nil = uuid.UUID(int=0)
foo = Foo('foo')


NO_OPTION = {}
DATETIMES = {'datetime_mode': rj.DM_ISO8601}
UUIDS = {'uuid_mode': rj.UM_CANONICAL}
FOOS_DUMP = {'default': default}
FOOS_LOAD = {'object_hook': hook}


@pytest.mark.skipif(not hasattr(sys, 'gettotalrefcount'), reason='Non-debug Python')
@pytest.mark.parametrize('value,dumps_options,loads_options', [
    ( plain_string, NO_OPTION, NO_OPTION ),
    ( bigint, NO_OPTION, NO_OPTION ),
    ( pi, NO_OPTION, NO_OPTION ),
    ( right_now, DATETIMES, DATETIMES ),
    ( date, DATETIMES, DATETIMES ),
    ( time, DATETIMES, DATETIMES ),
    ( nil, UUIDS, UUIDS ),
    ( foo, FOOS_DUMP, FOOS_LOAD ),
])
def test_leaks(value, dumps_options, loads_options):
    rc0 = sys.gettotalrefcount()
    for i in range(1000):
        asjson = rj.dumps(value, **dumps_options)
        aspython = rj.loads(asjson, **loads_options)
        del asjson
        del aspython
        rc1 = sys.gettotalrefcount()
        assert (rc1 - rc0) < THRESHOLD
