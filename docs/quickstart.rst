.. -*- coding: utf-8 -*-
.. :Project:   python-rapidjson -- Quickstart examples
.. :Author:    Lele Gaifax <lele@metapensiero.it>
.. :License:   MIT License
.. :Copyright: © 2016, 2017, 2018, 2020, 2021 Lele Gaifax
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

If possible this installs a *binary wheel*, containing the latest version of the package
already compiled for your system.  Otherwise it will download a *source distribution* and
will try to compile it: as the module is written in C++, in this case you most probably
will need to install a minimal C++ compiler toolchain on your system.

Alternatively it is also possible to install it using `Conda`__.

__ https://anaconda.org/conda-forge/python-rapidjson


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

Python's lists, tuples and iterators get serialized as JSON arrays:

.. doctest::

    >>> names_t = ('Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat')
    >>> names_l = list(names_t)
    >>> names_i = iter(names_l)
    >>> def names_g():
    ...     for name in names_t:
    ...         yield name
    >>> dumps(names_t) == dumps(names_l) == dumps(names_i) == dumps(names_g())
    True

Values can also be :class:`bytes` or :class:`bytearray` instances, which are assumed to
contain proper ``UTF-8``\ -encoded strings:

.. doctest::

    >>> clef = "\N{MUSICAL SYMBOL G CLEF}"
    >>> bytes_utf8 = clef.encode('utf-8')
    >>> bytearray = bytearray(bytes_utf8)
    >>> dumps(clef) == dumps(bytes_utf8) == dumps(bytearray) == '"\\uD834\\uDD1E"'
    True

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

The module exposes also a *stream* interface:

.. doctest::

    >>> from io import StringIO
    >>> from rapidjson import dump, load
    >>> stream = StringIO()
    >>> dump(data, stream)
    >>> stream.seek(0)
    0
    >>> load(stream) == data
    True


Incompatibilities
-----------------

Here are things in the standard ``json`` library that we have decided not to support:

``separators`` argument
  This is mostly used for pretty printing and not supported by RapidJSON_ so it isn't a
  high priority. We do support ``indent`` kwarg that would get you nice looking JSON
  anyways.

Coercing keys when dumping
  ``json`` will stringify a ``True`` dictionary key as ``"true"`` if you dump it out but
  when you load it back in it'll still be a string. We want the dump and load to return
  the exact same objects so we have decided not to do this coercion by default; you can
  however use ``MM_COERCE_KEYS_TO_STRINGS`` or a ``default`` function to mimic that.

Arbitrary encodings
  ``json.loads()`` accepts an ``encoding`` kwarg determining the encoding of its input,
  when that is a ``bytes`` or ``bytearray`` instance. Although ``RapidJSON`` is able to
  cope with several different encodings, we currently supports only the recommended one,
  ``UTF-8``.

``cls`` argument to ``loads()`` and ``dumps()``
  The ``json`` top level functions accept a ``cls`` parameter that allows to specify
  custom encoder/decoder class. If you must use that approach, that is you have to use the
  standard ``json`` top level functions but want to use ``RapidJSON`` functionalities, the
  following snippet shows a reasonably simple way to do that:

  .. doctest::

      >>> import datetime
      >>> import json
      >>> import rapidjson
      >>>
      >>> class Encoder:
      ...     def __init__(self, *args, **kwargs):
      ...         # Filter/adapt JSON arguments to RapidJSON ones
      ...         rjkwargs = {'datetime_mode': rapidjson.DM_ISO8601}
      ...         encoder = rapidjson.Encoder(**rjkwargs)
      ...         self.encode = encoder.__call__
      >>>
      >>> json.dumps([1,2,datetime.date(2020, 12, 8)], cls=Encoder)
      '[1,2,"2020-12-08"]'
      >>>
      >>> class Decoder:
      ...     def __init__(self, *args, **kwargs):
      ...         # Filter/adapt JSON arguments to RapidJSON ones
      ...         rjkwargs = {'datetime_mode': rapidjson.DM_ISO8601}
      ...         encoder = rapidjson.Decoder(**rjkwargs)
      ...         self.decode = encoder.__call__
      >>>
      >>> json.loads('[1,2,"2020-12-08"]', cls=Decoder)
      [1, 2, datetime.date(2020, 12, 8)]

``object_pairs_hook`` argument
  ``json`` decoding functions accept an ``object_pairs_hook`` kwarg, a variant of
  ``object_hook`` that selects a different way to translate JSON objects into Python
  dictionaries by first collecting their content into a sequence of *key-value pairs* and
  eventually passing that sequence to the hook function. That behaviour may be easily
  simulated:

  .. doctest::

     >>> def loads(s, object_pairs_hook=None):
     ...     if object_pairs_hook is None:
     ...         d = rapidjson.Decoder()
     ...     else:
     ...         class KWPairsDecoder(rapidjson.Decoder):
     ...             def start_object(self):
     ...                 return []
     ...             def end_object(self, pairs):
     ...                 return object_pairs_hook(pairs)
     ...         d = KWPairsDecoder()
     ...     return d(s)
     >>>
     >>> loads('{"foo": "bar"}', lambda pairs: ','.join(f'{k}={v}' for k, v in pairs))
     'foo=bar'

.. _JSON: https://www.json.org/
.. _RapidJSON: http://rapidjson.org/
