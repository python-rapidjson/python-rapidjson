===============================
 Exposed functions and symbols
===============================

.. default-domain:: py

.. data:: __version__

   The version of the module.

.. data:: __author__

   The author of the module.

.. data:: DATETIME_MODE_NONE = 0

   This is the default ``datetime_mode``: *neither* :class:`datetime` *nor* :class:`date`
   instances are recognized by :func:`dumps` and :func:`loads`.

.. data:: DATETIME_MODE_ISO8601 = 1

   In this ``datetime_mode`` mode :func:`dumps` and :func:`loads` handle :class:`datetime`
   *and* :class:`date` instances representing those values using the `ISO 8601`_ format.

.. data:: DATETIME_MODE_ISO8601_IGNORE_TZ = 2

   This is like :data:`DATETIME_MODE_ISO8601` except that the value's timezone is ignored.

.. data:: DATETIME_MODE_ISO8601_UTC = 3

   This is like :data:`DATETIME_MODE_ISO8601` except that the times are always *shifted* to
   the UTC_ timezone.

.. data:: UUID_MODE_NONE = 0

   This is the default ``uuid_mode``: :class:`UUID` instances are *not* recognized by
   :func:`dumps` and :func:`loads`.

.. data:: UUID_MODE_CANONICAL = 1

   In this ``uuid_mode``, :func:`loads` recognizes string values containing the
   ``xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`` canonical representation as :class:`UUID`
   instances; :func:`dumps` emits same kind of representation for :class:`UUID` instances as a
   string value.

.. data:: UUID_MODE_HEX = 2

   In this ``uuid_mode`` :func:`loads` recognizes string values containing exactly 32 hex
   digits *or* the ``xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`` canonical representation as
   :class:`UUID` instances; :func:`dumps` emits the 32 hex digits of :class:`UUID` instances as
   a string value.

.. testsetup::

    from rapidjson import (dumps, loads, DATETIME_MODE_NONE, DATETIME_MODE_ISO8601,
                           DATETIME_MODE_ISO8601_IGNORE_TZ, DATETIME_MODE_ISO8601_UTC,
                           UUID_MODE_NONE, UUID_MODE_CANONICAL, UUID_MODE_HEX)

.. function:: dumps(obj, skipkeys=False, ensure_ascii=True, allow_nan=True, \
                    native_numbers=False, indent=None, \
                    default=None, sort_keys=False, use_decimal=False, \
                    max_recursion_depth=2048, datetime_mode=None, uuid_mode=None)

   :param bool skipkeys: whether skip invalid :class:`dict` keys
   :param bool ensure_ascii: whether the output should contain only ASCII characters
   :param bool allow_nan: whether ``NaN`` values are handled or not
   :param bool native_numbers: whether use arch's native numbers or not
   :param int indent: indentation width to produce pretty printed JSON
   :param callable default: a function that gets called for objects that can't otherwise be
                            serialized
   :param bool sort_keys: whether dictionary keys should be sorted alphabetically
   :param bool use_decimal: whether :class:`Decimal` should be handled
   :param int max_recursion_depth: maximum depth for nested structures
   :param int datetime_mode: how should :class:`datetime` and :class:`date` instances be
                             handled
   :param int uuid_mode: how should :class:`UUID` instances be handled
   :returns: A Python :class:`str` instance.

   Encode given Python `obj` instance into a JSON string.

   If `skipkeys` is true (default: ``False``), then dict keys that are not of a basic type
   (:class:`str`, :class:`int`, :class:`float`, :class:`bool`, ``None``) will be skipped
   instead of raising a :exc:`TypeError`:

   .. doctest::

       >>> dumps({(0,): 'empty tuple'})
       Traceback (most recent call last):
         File "<stdin>", line 1, in <module>
       TypeError: keys must be a string
       >>> dumps({(0,): 'empty tuple'}, skipkeys=True)
       '{}'

   If `ensure_ascii` is true (the default), the output is guaranteed to have all incoming
   non-ASCII characters escaped.  If `ensure_ascii` is false, these characters will be output
   as-is:

   .. doctest::

       >>> dumps('The symbol for the Euro currency is €')
       '"The symbol for the Euro currency is \\u20AC"'
       >>> dumps('The symbol for the Euro currency is €',
       ...       ensure_ascii=False)
       '"The symbol for the Euro currency is €"'

   If `allow_nan` is false (default: ``True``), then it will be a :exc:`ValueError` to
   serialize out of range :class:`float` values (``nan``, ``inf``, ``-inf``) in strict
   compliance of the JSON specification.  If `allow_nan` is true, their JavaScript equivalents
   (``NaN``, ``Infinity``, ``-Infinity``) will be used:

   .. doctest::

       >>> nan = float('nan')
       >>> inf = float('inf')
       >>> dumps([nan, inf])
       '[NaN,Infinity]'
       >>> dumps([nan, inf], allow_nan=False)
       Traceback (most recent call last):
         File "<stdin>", line 1, in <module>
       ValueError: Out of range float values are not JSON compliant

   If `native_numbers` is true (default: ``False``), then the numeric values (i.e. *floats* and
   *integers*) will be handled using architecture *native* arithmetic: while this is somewhat
   faster, it is subject to the underlying C library ``long long`` and ``double`` limits:

   .. doctest::

      >>> dumps(123456789012345678901234567890)
      '123456789012345678901234567890'
      >>> dumps(123456789012345678901234567890, native_numbers=True)
      Traceback (most recent call last):
        File "<stdin>", line 1, in <module>
      OverflowError: int too big to convert

   When `indent` is ``None`` (the default), ``python-rapidjson`` produces the most compact JSON
   representation. By setting `indent` to 0 each array item and each dictionary value will be
   followed by a newline. A positive integer means that each *level* will be indented by that
   many spaces:

   .. code-block:: pycon

       >>> dumps([1, 2, {'three': 3, 'four': 4}])
       '[1,2,{"four":4,"three":3}]'
       >>> print(dumps([1, 2, {'three': 3, 'four': 4}], indent=0))
       [
       1,
       2,
       {
       "four": 4,
       "three": 3
       }
       ]
       >>> print(dumps([1, 2, {'three': 3, 'four': 4}], indent=2))
       [
         1,
         2,
         {
           "four": 4,
           "three": 3
         }
       ]

   The `default` argument may be used to specify a custom serializer for otherwise not handled
   objects. If specified, it should be a function that gets called for such objects and returns
   a JSON encodable version of the object itself or raise a :exc:`TypeError`:

   .. doctest::

       >>> class Point(object):
       ...   def __init__(self, x, y):
       ...     self.x = x
       ...     self.y = y
       ...
       >>> point = Point(1,2)
       >>> dumps(point)
       Traceback (most recent call last):
         File "<stdin>", line 1, in <module>
       TypeError: <__main__.Point object at …> is not JSON serializable
       >>> def point_jsonifier(obj):
       ...   if isinstance(obj, Point):
       ...     return {'x': obj.x, 'y': obj.y}
       ...   else:
       ...     raise ValueError('%r is not JSON serializable' % obj)
       ...
       >>> dumps(point, default=point_jsonifier) # doctest: +SKIP
       '{"y":2,"x":1}'

   When `sort_keys` is true (default: ``False``), the JSON representation of Python
   dictionaries is sorted by key:

   .. doctest::

       >>> dumps(point, default=point_jsonifier, sort_keys=True)
       '{"x":1,"y":2}'

   If `use_decimal` is true (default: ``False``), :class:`Decimal` instances will be
   serialized as their textual representation like any other float value, instead of raising
   an error:

   .. doctest::

       >>> from decimal import Decimal
       >>> pi = Decimal('3.1415926535897932384626433832795028841971')
       >>> dumps(pi)
       Traceback (most recent call last):
         File "<stdin>", line 1, in <module>
       TypeError: Decimal(…) is not JSON serializable
       >>> dumps(pi, use_decimal=True)
       '3.1415926535897932384626433832795028841971'

   With `max_recursion_depth` you can control the maximum depth that will be reached when
   serializing nested structures:

   .. doctest::

       >>> a = []
       >>> for i in range(10):
       ...  a = [a]
       ...
       >>> dumps(a)
       '[[[[[[[[[[[]]]]]]]]]]]'
       >>> dumps(a, max_recursion_depth=2)
       Traceback (most recent call last):
         File "<stdin>", line 1, in <module>
       OverflowError: Max recursion depth reached

   By default :class:`date` and :class:`datetime` instances are not serializable. When
   `datetime_mode` is set to :data:`DATETIME_MODE_ISO8601` those values are serialized using
   the common `ISO 8601`_ format:

   .. doctest::

       >>> from datetime import date, datetime
       >>> today = date.today()
       >>> right_now = datetime.now()
       >>> dumps({'date': today, 'timestamp': right_now})
       Traceback (most recent call last):
         File "<stdin>", line 1, in <module>
       TypeError: datetime(…) is not JSON serializable
       >>> dumps({'a date': today, 'a timestamp': right_now},
       ...       datetime_mode=DATETIME_MODE_ISO8601) # doctest: +SKIP
       '{"timestamp":"2016-08-28T13:14:52.277256","date":"2016-08-28"}'

   Another mode is :data:`DATETIME_MODE_ISO8601_UTC`, that *shifts* all timestamps to the UTC_
   timezone before serializing them:

   .. doctest::

       >>> from datetime import timedelta, timezone
       >>> here = timezone(timedelta(hours=2))
       >>> now = datetime.now(here)
       >>> dumps(now)
       Traceback (most recent call last):
         File "<stdin>", line 1, in <module>
       TypeError: datetime.datetime(…) is not JSON serializable
       >>> dumps(now, datetime_mode=DATETIME_MODE_ISO8601) # doctest: +SKIP
       '"2016-08-28T20:31:11.084418+02:00"'
       >>> dumps(now, datetime_mode=DATETIME_MODE_ISO8601_UTC) # doctest: +SKIP
       '"2016-08-28T18:31:11.084418+00:00"'

   With :data:`DATETIME_MODE_ISO8601_IGNORE_TZ` the timezone, if present, is simply omitted:

   .. doctest::

       >>> dumps(now, datetime_mode=DATETIME_MODE_ISO8601_IGNORE_TZ) # doctest: +SKIP
       '"2016-08-28T20:31:11.084418"'

   Likewise, to handle :class:`UUID` instances there are two modes that can be specified with
   the `uuid_mode` argument, that will use the string representation of their values:

   .. doctest::

       >>> from uuid import uuid4
       >>> random_uuid = uuid4()
       >>> dumps(random_uuid)
       Traceback (most recent call last):
         File "<stdin>", line 1, in <module>
       TypeError: UUID(…) is not JSON serializable
       >>> dumps(random_uuid, uuid_mode=UUID_MODE_CANONICAL) # doctest: +SKIP
       '"be576345-65b5-4fc2-92c5-94e2f82e38fd"'
       >>> dumps(random_uuid, uuid_mode=UUID_MODE_HEX) # doctest: +SKIP
       '"be57634565b54fc292c594e2f82e38fd"'

.. function:: loads(s, object_hook=None, use_decimal=False, allow_nan=True, \
                    native_numbers=False, datetime_mode=None, uuid_mode=None)

   :param str s: The JSON string to parse
   :param callable object_hook: an optional function that will be called with the result of
                                any object literal decoded (a :class:`dict`) and should return
                                the value to use instead of the :class:`dict`
   :param bool use_decimal: whether :class:`Decimal` should be used for float values
   :param bool allow_nan: whether ``NaN`` values are recognized
   :param bool native_numbers: whether use arch's native numbers or not
   :param int datetime_mode: how should :class:`datetime` and :class:`date` instances be
                             handled
   :param int uuid_mode: how should :class:`UUID` instances be handled
   :returns: An equivalent Python object.

   Decode the given Python string `s` containing a JSON formatted value into Python object.

   `object_hook` may be used to inject a custom deserializer that can replace any :class:`dict`
   instance found in the JSON structure with a *derived* object instance:

   .. doctest::

       >>> class Point(object):
       ...   def __init__(self, x, y):
       ...     self.x = x
       ...     self.y = y
       ...   def __repr__(self):
       ...     return 'Point(%s, %s)' % (self.x, self.y)
       ...
       >>> def point_dejsonifier(d):
       ...   if 'x' in d and 'y' in d:
       ...     return Point(d['x'], d['y'])
       ...   else:
       ...     return d
       ...
       >>> loads('{"x":1,"y":2}', object_hook=point_dejsonifier)
       Point(1, 2)

   If `use_decimal` is true (default: ``False``) then all floating point literals present in
   the JSON structure will be returned as :class:`Decimal` instances instead of plain
   :class:`float`:

   .. doctest::

       >>> loads('1.2345', use_decimal=True)
       Decimal('1.2345')

   If `allow_nan` is false (default: ``True``), then the values ``NaN`` and ``Infinity`` won't
   be recognized:

   .. doctest::

       >>> loads('[NaN, Infinity]')
       [nan, inf]
       >>> loads('[NaN, Infinity]', allow_nan=False)
       Traceback (most recent call last):
         File "<stdin>", line 1, in <module>
       ValueError: … Out of range float values are not JSON compliant

   If `native_numbers` is true (default: ``False``), then the numeric values (i.e. *floats* and
   *integers*) will be handled using architecture *native* arithmetic: while this is quite
   faster, integers that do not fit into the underlying C library ``long long`` limits will be
   converted (*truncated*) to ``double`` numbers:

   .. doctest::

      >>> loads('123456789012345678901234567890')
      123456789012345678901234567890
      >>> loads('123456789012345678901234567890', native_numbers=True)
      1.2345678901234566e+29

   With `datetime_mode` you can enable recognition of string literals containing an `ISO 8601`_
   representation as either :class:`date` or :class:`datetime` instances:

   .. doctest::

       >>> loads('"2016-01-02T01:02:03+01:00"')
       '2016-01-02T01:02:03+01:00'
       >>> loads('"2016-01-02T01:02:03+01:00"',
       ...       datetime_mode=DATETIME_MODE_ISO8601)
       datetime.datetime(2016, 1, 2, 1, 2, 3, tzinfo=...delta(0, 3600)))
       >>> loads('"2016-01-02T01:02:03+01:00"',
       ...       datetime_mode=DATETIME_MODE_ISO8601_UTC)
       datetime.datetime(2016, 1, 2, 0, 2, 3, tzinfo=...utc)
       >>> loads('"2016-01-02T01:02:03+01:00"',
       ...       datetime_mode=DATETIME_MODE_ISO8601_IGNORE_TZ)
       datetime.datetime(2016, 1, 2, 1, 2, 3)
       >>> loads('"2016-01-02"', datetime_mode=DATETIME_MODE_ISO8601)
       datetime.date(2016, 1, 2)

   With `uuid_mode` you can enable recognition of string literals containing two different
   representations of :class:`UUID` values:

   .. doctest::

       >>> loads('"aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"')
       'aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa'
       >>> loads('"aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"',
       ...       uuid_mode=UUID_MODE_CANONICAL)
       UUID('aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa')
       >>> loads('"aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"',
       ...       uuid_mode=UUID_MODE_HEX)
       UUID('aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa')
       >>> loads('"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"',
       ...       uuid_mode=UUID_MODE_CANONICAL)
       'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'
       >>> loads('"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"',
       ...       uuid_mode=UUID_MODE_HEX)
       UUID('aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa')


.. _ISO 8601: https://en.wikipedia.org/wiki/ISO_8601
.. _UTC: https://en.wikipedia.org/wiki/Coordinated_Universal_Time
