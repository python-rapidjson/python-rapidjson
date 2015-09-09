import pytest
import time

try:
    import yajl
except ImportError:
    yajl = None

try:
    import simplejson
except ImportError:
    simplejson = None

try:
    import json
except ImportError:
    json = None

try:
    import rapidjson
except ImportError:
    rapidjson = None

try:
    import ujson
except ImportError:
    ujson = None

default_data = {
    'words': """
        Lorem ipsum dolor sit amet, consectetur adipiscing
        elit. Mauris adipiscing adipiscing placerat.
        Vestibulum augue augue,
        pellentesque quis sollicitudin id, adipiscing.
        """,
    'list': list(range(200)),
    'dict': dict((str(i),'a') for i in list(range(200))),
    'int': 100100100,
    'float': 100999.123456
}


def time_func(func, data, iterations):
    start = time.time()
    while iterations:
        iterations -= 1
        func(data)
    return time.time() - start


def run_client_test(
        name, serialize, deserialize, iterations=100*1000, data=default_data
):
    squashed_data = serialize(data)
    serialize_profile = time_func(serialize, data, iterations)
    deserialize_profile = time_func(deserialize, squashed_data, iterations)

    return serialize_profile, deserialize_profile

contenders = []

if yajl:
    contenders.append(('yajl', yajl.Encoder().encode, yajl.Decoder().decode))

if simplejson:
    contenders.append(('simplejson', simplejson.dumps, simplejson.loads))

if json:
    contenders.append(('stdlib json', json.dumps, json.loads))

if rapidjson:
    contenders.append(('rapidjson', rapidjson.dumps, rapidjson.loads))

if ujson:
    contenders.append(('ujson', ujson.dumps, ujson.loads))


@pytest.mark.benchmark
@pytest.mark.parametrize('name,serialize,deserialize', contenders)
def test_json_serialization(name, serialize, deserialize):
    ser_data, des_data = run_client_test(name, serialize, deserialize)
    msg = "\n%-11s serialize: %0.3f  deserialize: %0.3f  total: %0.3f" % (
        name, ser_data, des_data, ser_data + des_data
    )

    print(msg)
