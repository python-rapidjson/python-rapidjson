=============
 Quick start
=============

This a quick overview of the module.


Installation
------------

First install ``python-rapidjson``:

.. code-block:: bash

    $ pip install python-rapidjson

As the module is written in C++, you most probably will need to install a minimal C++ compiler
toolchain on your system.


Basic examples
--------------

``python-rapidjson`` tries to be compatible with the standard library ``json.dumps()`` and
``json.loads()`` functions (but see the incompatibilities_).

Basic usage looks like this:

.. code-block:: pycon

    >>> from rapidjson import dumps, loads
    >>> data = {'foo': 100, 'bar': 'baz'}
    >>> dumps(data)
    '{"bar":"baz","foo":100}'
    >>> loads('{"bar":"baz","foo":100}')
    {'bar': 'baz', 'foo': 100}

All JSON_ data types are supported using their native Python counterparts:

.. code-block:: pycon

    >>> from pprint import pprint
    >>> from rapidjson import dumps, loads
    >>> int_number = 42
    >>> float_number = 1.4142
    >>> string = "√2 ≅ 1.4142"
    >>> false = False
    >>> true = True
    >>> null = None
    >>> array = [int_number, float_number, string, false, true, null]
    >>> an_object = {'int': int_number, 'float': float_number,
    ...              'string': string,
    ...              'true': true, 'false': false,
    ...              'array': array }
    >>> pprint(loads(dumps({'object': an_object})))
    {'object': {'array': [42, 1.4142, '√2 ≅ 1.4142', False, True, None],
                'false': False,
                'float': 1.4142,
                'int': 42,
                'string': '√2 ≅ 1.4142',
                'true': True}}

``python-rapidjson`` can optionally handle also a few other commonly used data types:

.. code-block:: pycon

    >>> import datetime
    >>> from pprint import pprint
    >>> from rapidjson import dumps, loads
    >>> from rapidjson import DATETIME_MODE_ISO8601, UUID_MODE_CANONICAL
    >>> today = datetime.date.today()
    >>> right_now = datetime.datetime.now()
    >>> dumps({'a date': today, 'a timestamp': right_now})
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    TypeError: datetime.datetime(…) is not JSON serializable
    >>> dumps({'a date': today, 'a timestamp': right_now},
    ...       datetime_mode=DATETIME_MODE_ISO8601)
    '{"a timestamp":"2016-08-28T13:14:52.277256","a date":"2016-08-28"}'
    >>> as_json = _
    >>> loads(as_json)
    {'a date': '2016-08-28', 'a timestamp': '2016-08-28T13:14:52.277256'}
    >>> pprint(loads(as_json, datetime_mode=DATETIME_MODE_ISO8601))
    {'a date': datetime.date(2016, 8, 28),
     'a timestamp': datetime.datetime(2016, 8, 28, 13, 14, 52, 277256)}
    >>> random_uuid = uuid.uuid4()
    >>> rapidjson.dumps(random_uuid)
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    TypeError: UUID(…) is not JSON serializable
    >>> dumps(random_uuid, uuid_mode=UUID_MODE_CANONICAL)
    '"be576345-65b5-4fc2-92c5-94e2f82e38fd"'
    >>> as_json = _
    >>> rapidjson.loads(as_json)
    'be576345-65b5-4fc2-92c5-94e2f82e38fd'
    >>> rapidjson.loads(as_json, uuid_mode=UUID_MODE_CANONICAL)
    UUID('be576345-65b5-4fc2-92c5-94e2f82e38fd')


Incompatibilities
-----------------

Here are things in the standard ``json`` library supports that we have decided
not to support:

* ``separators`` argument. This is mostly used for pretty printing and not
  supported by RapidJSON_ so it isn't a high priority. We do support
  ``indent`` kwarg that would get you nice looking JSON anyways.

* Coercing keys when dumping. ``json`` will turn ``True`` into ``'True'`` if you
  dump it out but when you load it back in it'll still be a string. We want the
  dump and load to return the exact same objects so we have decided not to do
  this coercing.


.. _JSON: http://json.org/
.. _RapidJSON: https://github.com/miloyip/rapidjson
