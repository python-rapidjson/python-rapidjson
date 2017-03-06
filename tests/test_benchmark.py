import datetime
import pytest
import sys
import random


composite_object = {
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
numbers = []
unicode_strings = []
strings = []
booleans = []
datetimes = []
list_dicts = []
dict_lists = {}

complex_object = [
    [user, friends],  [user, friends],  [user, friends],
    [user, friends],  [user, friends],  [user, friends]
]

for x in range(256):
    doubles.append(sys.maxsize * random.random())
    numbers.append(sys.maxsize * random.random())
    numbers.append(random.randint(0, sys.maxsize))
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


datasets = [('composite object', composite_object),
            ('256 doubles array', doubles),
            ('256 unicode array', unicode_strings),
            ('256 ascii array', strings),
            ('256 Trues array', booleans),
            ('100 dicts array', list_dicts),
            ('100 arrays dict', dict_lists),
            ('complex object', complex_object),
]


@pytest.mark.benchmark(group='serialize')
@pytest.mark.parametrize('data', [d[1] for d in datasets], ids=[d[0] for d in datasets])
def test_dumps(contender, data, benchmark):
    benchmark(contender.dumps, data)


@pytest.mark.benchmark(group='deserialize')
@pytest.mark.parametrize('data', [d[1] for d in datasets], ids=[d[0] for d in datasets])
def test_loads(contender, data, benchmark):
    data = contender.dumps(data)
    benchmark(contender.loads, data)


# Special case 1: load datetimes as plain strings vs datetime.xxx instances

@pytest.mark.benchmark(group='deserialize')
@pytest.mark.parametrize('data', [datetimes], ids=['256x3 datetimes'])
def test_loads_datetimes(datetimes_loads_contender, data, benchmark):
    from rapidjson import dumps, DATETIME_MODE_ISO8601
    data = dumps(data, datetime_mode=DATETIME_MODE_ISO8601)
    benchmark(datetimes_loads_contender, data)


# Special case 2: native numbers

@pytest.mark.benchmark(group='serialize')
@pytest.mark.parametrize('data', [numbers], ids=['512 numbers array'])
def test_dumps_numbers(numbers_contender, data, benchmark):
    benchmark(numbers_contender.dumps, data)

@pytest.mark.benchmark(group='deserialize')
@pytest.mark.parametrize('data', [numbers], ids=['512 numbers array'])
def test_loads_numbers(numbers_contender, data, benchmark):
    data = numbers_contender.dumps(data)
    benchmark(numbers_contender.loads, data)
