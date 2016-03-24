import datetime
import pytest
import sys
import random
from functools import partial

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

contenders = []
unprecise_contenders = []

if yajl:
    contenders.append(('yajl', yajl.Encoder().encode, yajl.Decoder().decode))

if simplejson:
    contenders.append(('simplejson', simplejson.dumps, simplejson.loads))

if json:
    contenders.append(('stdlib json', json.dumps, json.loads))

if rapidjson:
    contenders.append(
        ('rapidjson', rapidjson.dumps,
         partial(rapidjson.loads, precise_float=True))
    )
    unprecise_contenders.append(
        ('(rapidjson not precise)', rapidjson.dumps,
         partial(rapidjson.loads, precise_float=False))
    )

if ujson:
    contenders.append(
        ('ujson', ujson.dumps, partial(ujson.loads, precise_float=True))
    )
    unprecise_contenders.append(
        ('(ujson not precise)', ujson.dumps,
         partial(ujson.loads, precise_float=False))
    )


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

doubles = []
unicode_strings = []
strings = []
booleans = []
datetimes = []
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
    datetimes.append([datetime.date(random.randint(1900, 2000),
                                    random.randint(1, 12),
                                    random.randint(1, 28)),
                      datetime.time(random.randint(0,23),
                                    random.randint(0, 59),
                                    random.randint(0, 59)),
                      datetime.datetime.now()])

for y in range(100):
    arrays = []
    list_dicts.append({str(random.random()*20): int(random.random()*1000000)})

    for x in range(100):
        arrays.append({str(random.random() * 20): int(random.random()*1000000)})
        dict_lists[str(random.random() * 20)] = arrays

datasets = [('composite object', default_data),
            ('256 doubles array', doubles),
            ('256 unicode array', unicode_strings),
            ('256 ascii array', strings),
            ('256 Trues array', booleans),
            ('100 dicts array', list_dicts),
            ('100 arrays dict', dict_lists),
            ('medium complex objects', medium_complex),
]


@pytest.mark.benchmark(group='serialize')
@pytest.mark.parametrize('serializer',
                         [c[1] for c in contenders],
                         ids=[c[0] for c in contenders])
@pytest.mark.parametrize('data', [d[1] for d in datasets], ids=[d[0] for d in datasets])
def test_dumps(serializer, data, benchmark):
    benchmark(serializer, data)


@pytest.mark.benchmark(group='deserialize')
@pytest.mark.parametrize('serializer,deserializer',
                         [(c[1], c[2]) for c in contenders],
                         ids=[c[0] for c in contenders])
@pytest.mark.parametrize('data', [d[1] for d in datasets], ids=[d[0] for d in datasets])
def test_loads(serializer, deserializer, data, benchmark):
    data = serializer(data)
    benchmark(deserializer, data)


# Special case 1: precise vs unprecise

@pytest.mark.benchmark(group='deserialize')
@pytest.mark.parametrize('serializer,deserializer',
                         [(c[1], c[2]) for c in unprecise_contenders],
                         ids=['256 doubles array ' + c[0] for c in unprecise_contenders])
def test_loads_float(serializer, deserializer, benchmark):
    data = serializer(doubles)
    benchmark(deserializer, data)


# Special case 2: load datetimes as plain strings vs datetime.xxx instances

@pytest.mark.benchmark(group='deserialize')
@pytest.mark.parametrize('deserializer',
                         [rapidjson.loads,
                          partial(rapidjson.loads,
                                  datetime_mode=rapidjson.DATETIME_MODE_ISO8601)],
                         ids=['Ignore datetimes', 'Parse datetimes'])
def test_loads_datetimes(deserializer, benchmark):
    data = rapidjson.dumps(datetimes, datetime_mode=rapidjson.DATETIME_MODE_ISO8601)
    benchmark(deserializer, data)
