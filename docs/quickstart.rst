.. -*- coding: utf-8 -*-
.. :Project:   python-rapidjson -- Quickstart examples
.. :Author:    Lele Gaifax <lele@metapensiero.it>
.. :License:   MIT License
.. :Copyright: © 2016, 2017 Lele Gaifax
..

=============
 Quick start
=============

This a quick overview of the module.


Installation
------------

First install ``python-rapidjson``:

.. code-block:: bash

    $ pip install python-rapidjson

As the module is written in C++, you most probably will need to install a minimal C++
compiler toolchain on your system.


Basic examples
--------------

``python-rapidjson`` tries to be compatible with the standard library ``json.dumps()`` and
``json.loads()`` functions (but see the incompatibilities_).

Basic usage looks like this:

.. doctest::

    >>> from pprint import pprint
    >>> from rapidjson import dumps, loads
    >>> data = {'foo': 100, 'bar': 'baz'}
    >>> dumps(data, sort_keys=True) # for doctest
    '{"bar":"baz","foo":100}'
    >>> pprint(loads('{"bar":"baz","foo":100}'))
    {'bar': 'baz', 'foo': 100}

All JSON_ data types are supported using their native Python counterparts:

.. doctest::

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

.. doctest::

    >>> import datetime, decimal, uuid
    >>> from rapidjson import DM_ISO8601, UM_CANONICAL, NM_DECIMAL
    >>> some_day = datetime.date(2016, 8, 28)
    >>> some_timestamp = datetime.datetime(2016, 8, 28, 13, 14, 15)
    >>> dumps({'a date': some_day, 'a timestamp': some_timestamp})
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    TypeError: datetime.datetime(…) is not JSON serializable
    >>> dumps({'a date': some_day, 'a timestamp': some_timestamp},
    ...       datetime_mode=DM_ISO8601,
    ...       sort_keys=True) # for doctests
    '{"a date":"2016-08-28","a timestamp":"2016-08-28T13:14:15"}'
    >>> as_json = _
    >>> pprint(loads(as_json))
    {'a date': '2016-08-28', 'a timestamp': '2016-08-28T13:14:15'}
    >>> pprint(loads(as_json, datetime_mode=DM_ISO8601))
    {'a date': datetime.date(2016, 8, 28),
     'a timestamp': datetime.datetime(2016, 8, 28, 13, 14, 15)}
    >>> some_uuid = uuid.uuid5(uuid.NAMESPACE_DNS, 'python.org')
    >>> dumps(some_uuid)
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    TypeError: UUID(…) is not JSON serializable
    >>> dumps(some_uuid, uuid_mode=UM_CANONICAL)
    '"886313e1-3b8a-5372-9b90-0c9aee199e5d"'
    >>> as_json = _
    >>> loads(as_json)
    '886313e1-3b8a-5372-9b90-0c9aee199e5d'
    >>> loads(as_json, uuid_mode=UM_CANONICAL)
    UUID('886313e1-3b8a-5372-9b90-0c9aee199e5d')
    >>> pi = decimal.Decimal('3.1415926535897932384626433832795028841971')
    >>> dumps(pi)
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    TypeError: Decimal(…) is not JSON serializable
    >>> dumps(pi, number_mode=NM_DECIMAL)
    '3.1415926535897932384626433832795028841971'
    >>> as_json = _
    >>> loads(as_json)
    3.141592653589793
    >>> type(loads(as_json))
    <class 'float'>
    >>> loads(as_json, number_mode=NM_DECIMAL)
    Decimal('3.1415926535897932384626433832795028841971')


Incompatibilities
-----------------

Here are things in the standard ``json`` library that we have decided not to support:

``separators`` argument
  This is mostly used for pretty printing and not supported by RapidJSON_ so it isn't a
  high priority. We do support ``indent`` kwarg that would get you nice looking JSON
  anyways.

Coercing keys when dumping
  ``json`` will turn ``True`` into ``'True'`` if you dump it out but when you load it back
  in it'll still be a string. We want the dump and load to return the exact same objects
  so we have decided not to do this coercion.


.. _JSON: http://json.org/
.. _RapidJSON: https://github.com/miloyip/rapidjson
