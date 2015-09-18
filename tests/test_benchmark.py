import pytest
import time
import sys
import random

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

user = {
    "userId": 3381293,
    "age": 213,
    "username": "johndoe",
    "fullname": u"John Doe the Second",
    "isAuthorized": True,
    "liked": 31231.31231202,
    "approval": 31.1471,
    "jobs": [ 1, 2 ],
    "currJob": None
}

friends = [ user, user, user, user, user, user, user, user ]


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


doubles = []
unicode_strings = []
strings = []
booleans = []
list_dicts = []
dict_lists = {}

medium_complex = [
    [user, friends],  [user, friends],  [user, friends],
    [user, friends],  [user, friends],  [user, friends]
]

for x in range(256):
    doubles.append(sys.maxsize * random.random())
    unicode_strings.append("نظام الحكم سلطاني وراثي في الذكور من ذرية السيد تركي بن سعيد بن سلطان ويشترط فيمن يختار لولاية الحكم من بينهم ان يكون مسلما رشيدا عاقلا ًوابنا شرعيا لابوين عمانيين ")
    strings.append("A pretty long string which is in a list")
    booleans.append(True)

for y in range(100):
    arrays = []
    list_dicts.append({str(random.random()*20): int(random.random()*1000000)})

    for x in range(100):
        arrays.append({str(random.random() * 20): int(random.random()*1000000)})
        dict_lists[str(random.random() * 20)] = arrays


@pytest.mark.benchmark
@pytest.mark.parametrize('name,serialize,deserialize', contenders)
def test_json_serialization(name, serialize, deserialize):
    ser_data, des_data = run_client_test(name, serialize, deserialize)
    msg = "\n%-11s serialize: %0.3f  deserialize: %0.3f  total: %0.3f" % (
        name, ser_data, des_data, ser_data + des_data
    )

    print(msg)


@pytest.mark.benchmark
@pytest.mark.parametrize('name,serialize,deserialize', contenders)
def test_json_doubles(name, serialize, deserialize):
    print("\nArray with 256 doubles:")
    ser_data, des_data = run_client_test(
        name, serialize, deserialize,
        data=doubles,
        iterations=50000,
    )
    msg = "%-11s serialize: %0.3f  deserialize: %0.3f  total: %0.3f" % (
        name, ser_data, des_data, ser_data + des_data
    )

    print(msg)


@pytest.mark.benchmark
@pytest.mark.parametrize('name,serialize,deserialize', contenders)
def test_json_unicode_strings(name, serialize, deserialize):
    print("\nArray with 256 unicode strings:")
    ser_data, des_data = run_client_test(
        name, serialize, deserialize,
        data=unicode_strings,
        iterations=5000,
    )
    msg = "%-11s serialize: %0.3f  deserialize: %0.3f  total: %0.3f" % (
        name, ser_data, des_data, ser_data + des_data
    )

    print(msg)


@pytest.mark.benchmark
@pytest.mark.parametrize('name,serialize,deserialize', contenders)
def test_json_scii_strings(name, serialize, deserialize):
    print("\nArray with 256 ascii strings:")
    ser_data, des_data = run_client_test(
        name, serialize, deserialize,
        data=strings,
    )
    msg = "%-11s serialize: %0.3f  deserialize: %0.3f  total: %0.3f" % (
        name, ser_data, des_data, ser_data + des_data
    )

    print(msg)


@pytest.mark.benchmark
@pytest.mark.parametrize('name,serialize,deserialize', contenders)
def test_json_booleans(name, serialize, deserialize):
    print("\nArray with 256 True's:")
    ser_data, des_data = run_client_test(
        name, serialize, deserialize,
        data=booleans,
    )
    msg = "%-11s serialize: %0.3f  deserialize: %0.3f  total: %0.3f" % (
        name, ser_data, des_data, ser_data + des_data
    )

    print(msg)


@pytest.mark.benchmark
@pytest.mark.parametrize('name,serialize,deserialize', contenders)
def test_json_list_of_dictionaries(name, serialize, deserialize):
    print("\nArray of 100 dictionaries:")
    ser_data, des_data = run_client_test(
        name, serialize, deserialize,
        data=list_dicts,
        iterations=5,
    )
    msg = "%-11s serialize: %0.3f  deserialize: %0.3f  total: %0.3f" % (
        name, ser_data, des_data, ser_data + des_data
    )

    print(msg)


@pytest.mark.benchmark
@pytest.mark.parametrize('name,serialize,deserialize', contenders)
def test_json_dictionary_of_lists(name, serialize, deserialize):
    print("\nDictionary of 100 Arrays:")
    ser_data, des_data = run_client_test(
        name, serialize, deserialize,
        data=dict_lists,
        iterations=5,
    )
    msg = "%-11s serialize: %0.3f  deserialize: %0.3f  total: %0.3f" % (
        name, ser_data, des_data, ser_data + des_data
    )

    print(msg)


@pytest.mark.benchmark
@pytest.mark.parametrize('name,serialize,deserialize', contenders)
def test_json_medium_complex_objects(name, serialize, deserialize):
    print("\n256 Medium Complex objects:")
    ser_data, des_data = run_client_test(
        name, serialize, deserialize,
        data=medium_complex,
        iterations=50000,
    )
    msg = "%-11s serialize: %0.3f  deserialize: %0.3f  total: %0.3f" % (
        name, ser_data, des_data, ser_data + des_data
    )

    print(msg)


@pytest.mark.benchmark
def test_double_performance_float_precision():
    from functools import partial

    print("\nArray with 256 doubles:")
    name = 'rapidjson (precise)'
    serialize = rapidjson.dumps
    deserialize = rapidjson.loads

    ser_data, des_data = run_client_test(
        name, serialize, deserialize,
        data=doubles,
        iterations=50000,
    )
    msg = "%-11s serialize: %0.3f  deserialize: %0.3f  total: %0.3f" % (
        name, ser_data, des_data, ser_data + des_data
    )

    print(msg)

    name = 'rapidjson (not precise)'
    serialize = rapidjson.dumps
    deserialize = partial(rapidjson.loads, precise_float=False)

    ser_data, des_data = run_client_test(
        name, serialize, deserialize,
        data=doubles,
        iterations=50000,
    )
    msg = "%-11s serialize: %0.3f  deserialize: %0.3f  total: %0.3f" % (
        name, ser_data, des_data, ser_data + des_data
    )
    print(msg)
