===============================
 Exposed functions and symbols
===============================

.. default-domain:: py

.. data:: __author__

   The author of the module.

.. data:: __version__

   The version of the module.

.. data:: __rapidjson_version__

   The version of the RapidJSON_ library this module is built with.

.. data:: DM_NONE

   This is the default ``datetime_mode``: *neither* :class:`date` *nor*
   :class:`datetime` *nor* :class:`time` instances are recognized by
   :func:`dumps` and :func:`loads`.

.. data:: DM_ISO8601

   In this ``datetime_mode`` mode :func:`dumps` and :func:`loads` handle
   :class:`date`, :class:`datetime` *and* :class:`date` instances representing
   those values using the `ISO 8601`_ format.

.. data:: DM_UNIX_TIME

   This mode tells RapidJSON to serialize :class:`date`, :class:`datetime` and
   :class:`time` as *numeric timestamps*: for the formers it is exactly the
   result of their :meth:`timestamp` method, that is as the number of seconds
   passed since ``EPOCH``; ``date`` instances are serialized as the
   corresponding ``datetime`` instance with all the `time` slots set to 0;
   ``time`` instances are serialized as the number of seconds since midnight.

   Since this is obviously *irreversible*, this flag is usable **only** for
   :func:`dumps`: an error is raised when passed to :func:`loads`.

.. data:: DM_ONLY_SECONDS

   This is usable in combination with :data:`DM_UNIX_TIME` so that an integer
   representation is used, ignoring *microseconds*.

.. data:: DM_IGNORE_TZ

   This can be used combined with :data:`DM_ISO8601` or :data:`DM_UNIX_TIME`,
   to ignore the value's timezones.

.. data:: DM_NAIVE_IS_UTC

   This can be used combined with :data:`DM_ISO8601` or :data:`DM_UNIX_TIME`,
   to tell RapidJSON that the naïve values (that is, without an explicit
   timezone) are already in UTC_ timezone.

.. data:: DM_SHIFT_TO_UTC

   This can be used combined with :data:`DM_ISO8601` or :data:`DM_UNIX_TIME`,
   to always *shift* values the UTC_ timezone.

.. data:: UM_NONE

   This is the default ``uuid_mode``: :class:`UUID` instances are *not*
   recognized by :func:`dumps` and :func:`loads`.

.. data:: UM_CANONICAL

   In this ``uuid_mode``, :func:`loads` recognizes string values containing
   the ``xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`` canonical representation as
   :class:`UUID` instances; :func:`dumps` emits same kind of representation
   for :class:`UUID` instances as a string value.

.. data:: UM_HEX

   In this ``uuid_mode`` :func:`loads` recognizes string values containing
   exactly 32 hex digits *or* the ``xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx``
   canonical representation as :class:`UUID` instances; :func:`dumps` emits
   the 32 hex digits of :class:`UUID` instances as a string value.

.. data:: NM_NONE

   This is the default ``number_mode``: numeric values can be as wide as the
   memory allows.

.. data:: NM_DECIMAL

   In this ``number_mode`` :func:`loads` will return floating point values as
   :class:`Decimal` instances instead of :class:`float`; :func:`dumps` will
   serialize :class:`Decimal` instances like any other :class:`float` number.

.. data:: NM_NAN

   This enables *non-numbers* handling in both directions.

.. data:: NM_NATIVE

   In this alternative ``number_mode`` numeric values must fit into the
   underlying C library limits, with a considerable speed benefit.

.. testsetup::

   from rapidjson import (dumps, loads, DM_NONE, DM_ISO8601, DM_UNIX_TIME,
                          DM_ONLY_SECONDS, DM_IGNORE_TZ, DM_NAIVE_IS_UTC, DM_SHIFT_TO_UTC,
                          UM_NONE, UM_CANONICAL, UM_HEX, NM_NATIVE, NM_DECIMAL, NM_NAN)

.. function:: dumps(obj, skipkeys=False, ensure_ascii=True, indent=None, \
                    default=None, sort_keys=False, max_recursion_depth=2048, \
                    number_mode=None, datetime_mode=None, uuid_mode=None)

   :param bool skipkeys: whether skip invalid :class:`dict` keys
   :param bool ensure_ascii: whether the output should contain only ASCII
                             characters
   :param int indent: indentation width to produce pretty printed JSON
   :param callable default: a function that gets called for objects that can't
                            otherwise be serialized
   :param bool sort_keys: whether dictionary keys should be sorted
                          alphabetically
   :param int max_recursion_depth: maximum depth for nested structures
   :param int number_mode: enable particular behaviors in handling numbers
   :param int datetime_mode: how should :class:`datetime`, :class:`time` and
                             :class:`date` instances be handled
   :param int uuid_mode: how should :class:`UUID` instances be handled
   :returns: A Python :class:`str` instance.

   Encode given Python `obj` instance into a JSON string.

   If `skipkeys` is true (default: ``False``), then dict keys that are not of
   a basic type (:class:`str`, :class:`int`, :class:`float`, :class:`bool`,
   ``None``) will be skipped instead of raising a :exc:`TypeError`:

   .. doctest::

      >>> dumps({(0,): 'empty tuple'})
      Traceback (most recent call last):
        File "<stdin>", line 1, in <module>
      TypeError: keys must be a string
      >>> dumps({(0,): 'empty tuple'}, skipkeys=True)
      '{}'

   If `ensure_ascii` is true (the default), the output is guaranteed to have
   all incoming non-ASCII characters escaped.  If `ensure_ascii` is false,
   these characters will be output as-is:

   .. doctest::

      >>> dumps('The symbol for the Euro currency is €')
      '"The symbol for the Euro currency is \\u20AC"'
      >>> dumps('The symbol for the Euro currency is €',
      ...       ensure_ascii=False)
      '"The symbol for the Euro currency is €"'

   When `indent` is ``None`` (the default), ``python-rapidjson`` produces the
   most compact JSON representation. By setting `indent` to 0 each array item
   and each dictionary value will be followed by a newline. A positive integer
   means that each *level* will be indented by that many spaces:

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

   The `default` argument may be used to specify a custom serializer for
   otherwise not handled objects. If specified, it should be a function that
   gets called for such objects and returns a JSON encodable version of the
   object itself or raise a :exc:`TypeError`:

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

   When `sort_keys` is true (default: ``False``), the JSON representation of
   Python dictionaries is sorted by key:

   .. doctest::

      >>> dumps(point, default=point_jsonifier, sort_keys=True)
      '{"x":1,"y":2}'

   With `max_recursion_depth` you can control the maximum depth that will be
   reached when serializing nested structures:

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

   The `number_mode` argument selects different behaviors in handling numeric
   values.

   By default *non-numbers* (``nan``, ``inf``, ``-inf``) will be serialized as
   their JavaScript equivalents (``NaN``, ``Infinity``, ``-Infinity``),
   because ``NM_NAN`` is *on* by default (**NB**: this is *not* compliant with
   the ``JSON`` standard):

   .. doctest::

      >>> nan = float('nan')
      >>> inf = float('inf')
      >>> dumps([nan, inf])
      '[NaN,Infinity]'
      >>> dumps([nan, inf], number_mode=NM_NAN)
      '[NaN,Infinity]'

   Explicitly setting `number_mode` or using the compatibility option
   `allow_nan` you can avoid that and obtain a ``ValueError`` exception
   instead:

   .. doctest::

      >>> dumps([nan, inf], number_mode=NM_NATIVE)
      Traceback (most recent call last):
        File "<stdin>", line 1, in <module>
      ValueError: Out of range float values are not JSON compliant
      >>> dumps([nan, inf], allow_nan=False)
      Traceback (most recent call last):
        File "<stdin>", line 1, in <module>
      ValueError: Out of range float values are not JSON compliant

   Likewise :class:`Decimal` instances cause a ``TypeError`` exception:

   .. doctest::

      >>> from decimal import Decimal
      >>> pi = Decimal('3.1415926535897932384626433832795028841971')
      >>> dumps(pi)
      Traceback (most recent call last):
        File "<stdin>", line 1, in <module>
      TypeError: Decimal(…) is not JSON serializable

   while using :data:`NM_DECIMAL` they will be serialized as their textual
   representation like any other float value:

   .. doctest::

      >>> dumps(pi, number_mode=NM_DECIMAL)
      '3.1415926535897932384626433832795028841971'

   Yet another possible flag affects how numeric values are passed to the
   underlying RapidJSON_ library: by default they are serialized to their
   string representation by the module itself, so they are virtually of
   unlimited precision:

   .. doctest::

      >>> dumps(123456789012345678901234567890)
      '123456789012345678901234567890'

   With :data:`NM_NATIVE` their binary values will be passed directly instead:
   this is somewhat faster, it is subject to the underlying C library ``long
   long`` and ``double`` limits:

   .. doctest::

      >>> dumps(123456789012345678901234567890, number_mode=NM_NATIVE)
      Traceback (most recent call last):
        File "<stdin>", line 1, in <module>
      OverflowError: int too big to convert

   These flags can be combined together:

   .. doctest::

      >>> fast_and_precise = NM_NATIVE | NM_DECIMAL | NM_NAN
      >>> dumps([-1, nan, pi], number_mode=fast_and_precise)
      '[-1,NaN,3.1415926535897932384626433832795028841971]'

   By default :class:`date`, :class:`datetime` and :class:`time` instances are
   not serializable:

   .. doctest::

      >>> from datetime import datetime
      >>> right_now = datetime(2016, 8, 28, 13, 14, 52, 277256)
      >>> date = right_now.date()
      >>> time = right_now.time()
      >>> dumps({'date': date, 'time': time, 'timestamp': right_now})
      Traceback (most recent call last):
        File "<stdin>", line 1, in <module>
      TypeError: datetime(…) is not JSON serializable

   When `datetime_mode` is set to :data:`DM_ISO8601` those values are
   serialized using the common `ISO 8601`_ format:

   .. doctest::

      >>> dumps(['date', date, 'time', time, 'timestamp', right_now],
      ...       datetime_mode=DM_ISO8601)
      '["date","2016-08-28","time","13:14:52.277256","timestamp","2016-08-28T13:14:52.277256"]'

   The `right_now` value is a naïve datetime (because it does not carry the
   timezone information) and is normally assumed to be in the local timezone,
   whatever your system thinks it is. When you instead *know* that your value,
   even being naïve are actually in the UTC_ timezone, you can use the
   :data:`DM_NAIVE_IS_UTC` flag to inform RapidJSON about that:

   .. doctest::

      >>> mode = DM_ISO8601 | DM_NAIVE_IS_UTC
      >>> dumps(['time', time, 'timestamp', right_now], datetime_mode=mode)
      '["time","13:14:52.277256+00:00","timestamp","2016-08-28T13:14:52.277256+00:00"]'

   A variant is :data:`DM_SHIFT_TO_UTC`, that *shifts* all datetime values to
   the UTC_ timezone before serializing them:

   .. doctest::

      >>> from datetime import timedelta, timezone
      >>> here = timezone(timedelta(hours=2))
      >>> now = datetime(2016, 8, 28, 20, 31, 11, 84418, here)
      >>> dumps(now, datetime_mode=DM_ISO8601)
      '"2016-08-28T20:31:11.084418+02:00"'
      >>> mode = DM_ISO8601 | DM_SHIFT_TO_UTC
      >>> dumps(now, datetime_mode=mode)
      '"2016-08-28T18:31:11.084418+00:00"'

   With :data:`DM_IGNORE_TZ` the timezone, if present, is simply omitted:

   .. doctest::

      >>> mode = DM_ISO8601 | DM_IGNORE_TZ
      >>> dumps(now, datetime_mode=mode)
      '"2016-08-28T20:31:11.084418"'

   Another :ref:`one-way only <no-unix-time-loads>` alternative format is
   `Unix time`_: with :data:`DM_UNIX_TIME` :class:`date`, :class:`datetime`
   and :class:`time` instances are serialized as a number of seconds,
   respectively since the ``EPOCH`` for the first two kinds and since midnight
   for the latter:

   .. doctest::

      >>> mode = DM_UNIX_TIME
      >>> dumps([now, now.date(), now.time()], datetime_mode=mode)
      '[1472409071.084418,1472335200.0,73871.084418]'
      >>> unixtime = float(dumps(now, datetime_mode=mode))
      >>> datetime.fromtimestamp(unixtime, here) == now
      True

   Combining it with the :data:`DM_ONLY_SECONDS` will produce integer values
   instead, dropping *microseconds*:

   .. doctest::

      >>> mode = DM_UNIX_TIME | DM_ONLY_SECONDS
      >>> dumps([now, now.date(), now.time()], datetime_mode=mode)
      '[1472409071,1472335200,73871]'

   It can be used combined with :data:`DM_SHIFT_TO_UTC` to obtain the
   timestamp of the corresponding UTC_ time:

      >>> mode = DM_UNIX_TIME | DM_SHIFT_TO_UTC
      >>> dumps(now, datetime_mode=mode)
      '1472409071.084418'

   As above, when you know that your values are in the UTC_ timezone, you can
   use the :data:`DM_NAIVE_IS_UTC` flag to get the right result:

   .. doctest::

      >>> a_long_time_ago = datetime(1968, 3, 18, 9, 10, 0, 0)
      >>> mode = DM_UNIX_TIME | DM_NAIVE_IS_UTC
      >>> dumps([a_long_time_ago, a_long_time_ago.date(), a_long_time_ago.time()],
      ...       datetime_mode=mode)
      '[-56472600.0,-56505600.0,33000.0]'

   Likewise, to handle :class:`UUID` instances there are two modes that can be
   specified with the `uuid_mode` argument, that will use the string
   representation of their values:

   .. doctest::

      >>> from uuid import uuid4
      >>> random_uuid = uuid4()
      >>> dumps(random_uuid)
      Traceback (most recent call last):
        File "<stdin>", line 1, in <module>
      TypeError: UUID(…) is not JSON serializable
      >>> dumps(random_uuid, uuid_mode=UM_CANONICAL) # doctest: +SKIP
      '"be576345-65b5-4fc2-92c5-94e2f82e38fd"'
      >>> dumps(random_uuid, uuid_mode=UM_HEX) # doctest: +SKIP
      '"be57634565b54fc292c594e2f82e38fd"'

.. function:: loads(s, object_hook=None, number_mode=None, datetime_mode=None, uuid_mode=None)

   :param str s: The JSON string to parse
   :param callable object_hook: an optional function that will be called with
                                the result of any object literal decoded (a
                                :class:`dict`) and should return the value to
                                use instead of the :class:`dict`
   :param int number_mode: enable particular behaviors in handling numbers
   :param int datetime_mode: how should :class:`datetime` and :class:`date`
                             instances be handled
   :param int uuid_mode: how should :class:`UUID` instances be handled
   :returns: An equivalent Python object.

   Decode the given Python string `s` containing a JSON formatted value into
   Python object.

   `object_hook` may be used to inject a custom deserializer that can replace
   any :class:`dict` instance found in the JSON structure with a *derived*
   object instance:

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

   The `number_mode` argument selects different behaviors in handling numeric
   values.

   By default *non-numbers* (``nan``, ``inf``, ``-inf``) are recognized,
   because ``NM_NAN`` is *on* by default:

   .. doctest::

      >>> loads('[NaN, Infinity]')
      [nan, inf]
      >>> loads('[NaN, Infinity]', number_mode=NM_NAN)
      [nan, inf]

   Explicitly setting `number_mode` or using the compatibility option
   `allow_nan` you can avoid that and obtain a ``ValueError`` exception
   instead:

   .. doctest::

      >>> loads('[NaN, Infinity]', number_mode=NM_NATIVE)
      Traceback (most recent call last):
        File "<stdin>", line 1, in <module>
      ValueError: … Out of range float values are not JSON compliant
      >>> loads('[NaN, Infinity]', allow_nan=False)
      Traceback (most recent call last):
        File "<stdin>", line 1, in <module>
      ValueError: … Out of range float values are not JSON compliant

   Normally all floating point literals present in the JSON structure will be
   loaded as Python :class:`float` instances, with :data:`NM_DECIMAL` they
   will be returned as :class:`Decimal` instances instead:

   .. doctest::

      >>> loads('1.2345')
      1.2345
      >>> loads('1.2345', number_mode=NM_DECIMAL)
      Decimal('1.2345')

   When you can be sure that all the numeric values are constrained within the
   architecture's hardware limits you can get a sensible speed gain with the
   :data:`NM_NATIVE` flag. While this is quite faster, integer literals that
   do not fit into the underlying C library ``long long`` limits will be
   converted (*truncated*) to ``double`` numbers:

   .. doctest::

      >>> loads('123456789012345678901234567890')
      123456789012345678901234567890
      >>> loads('123456789012345678901234567890', number_mode=NM_NATIVE)
      1.2345678901234566e+29

   These flags can be combined together:

   .. doctest::

      >>> loads('[-1, NaN, 3.1415926535897932384626433832795028841971]',
      ...       number_mode=NM_DECIMAL | NM_NAN)
      [-1, Decimal('NaN'), Decimal('3.1415926535897932384626433832795028841971')]

   with the exception of :data:`NM_NATIVE` and :data:`NM_DECIMAL`, that does
   not make sense since there's little point in creating :class:`Decimal`
   instances out of possibly truncated float literals:

   .. doctest:

      >>> loads('3.1415926535897932384626433832795028841971')
      3.141592653589793
      >>> loads('3.1415926535897932384626433832795028841971',
      ...       number_mode=NM_NATIVE)
      3.141592653589793
      >>> loads('3.1415926535897932384626433832795028841971',
      ...       number_mode=NM_NATIVE | NM_DECIMAL)
      Traceback (most recent call last):
        ...
      ValueError: ... Combining NM_NATIVE with NM_DECIMAL is not supported

   With `datetime_mode` you can enable recognition of string literals
   containing an `ISO 8601`_ representation as either :class:`date`,
   :class:`datetime` or :class:`time` instances:

   .. doctest::

      >>> loads('"2016-01-02T01:02:03+01:00"')
      '2016-01-02T01:02:03+01:00'
      >>> loads('"2016-01-02T01:02:03+01:00"', datetime_mode=DM_ISO8601)
      datetime.datetime(2016, 1, 2, 1, 2, 3, tzinfo=...delta(0, 3600)))
      >>> loads('"2016-01-02"', datetime_mode=DM_ISO8601)
      datetime.date(2016, 1, 2)
      >>> loads('"01:02:03+01:00"', datetime_mode=DM_ISO8601)
      datetime.time(1, 2, 3, tzinfo=...delta(0, 3600)))

   It can be combined with :data:`DM_SHIFT_TO_UTC` to *always* obtain values
   in the UTC_ timezone:

   .. doctest::

      >>> mode = DM_ISO8601 | DM_SHIFT_TO_UTC
      >>> loads('"2016-01-02T01:02:03+01:00"', datetime_mode=mode)
      datetime.datetime(2016, 1, 2, 0, 2, 3, tzinfo=...utc)

   .. note::

      This option is somewhat limited when the value is a non-naïve time literal
      because negative values cannot be represented by the underlying Python
      type, so it cannot adapt such values reliably:

      .. doctest::

         >>> mode = DM_ISO8601 | DM_SHIFT_TO_UTC
         >>> loads('"00:01:02+00:00"', datetime_mode=mode)
         datetime.time(0, 1, 2, tzinfo=...utc)
         >>> loads('"00:01:02+01:00"', datetime_mode=mode)
         Traceback (most recent call last):
           ...
         ValueError: ... Time literal cannot be shifted to UTC: 00:01:02+01:00

   If you combine it with :data:`DM_NAIVE_IS_UTC` then all values without a
   timezone will be assumed to be relative to UTC_:

   .. doctest::

      >>> mode = DM_ISO8601 | DM_NAIVE_IS_UTC
      >>> loads('"2016-01-02T01:02:03"', datetime_mode=mode)
      datetime.datetime(2016, 1, 2, 1, 2, 3, tzinfo=...utc)
      >>> loads('"2016-01-02T01:02:03+01:00"', datetime_mode=mode)
      datetime.datetime(2016, 1, 2, 1, 2, 3, tzinfo=...delta(0, 3600)))
      >>> loads('"01:02:03"', datetime_mode=mode)
      datetime.time(1, 2, 3, tzinfo=...utc)

   Yet another combination is with :data:`DM_IGNORE_TZ` to ignore the timezone
   and obtain naïve values:

   .. doctest::

      >>> mode = DM_ISO8601 | DM_IGNORE_TZ
      >>> loads('"2016-01-02T01:02:03+01:00"', datetime_mode=mode)
      datetime.datetime(2016, 1, 2, 1, 2, 3)
      >>> loads('"01:02:03+01:00"', datetime_mode=mode)
      datetime.time(1, 2, 3)

   .. _no-unix-time-loads:

   The :data:`DM_UNIX_TIME` cannot be used here, because there isn't a
   reasonable heuristic to disambiguate between plain numbers and timestamps:

   .. doctest::

      >>> loads('[1,2,3]', datetime_mode=DM_UNIX_TIME)
      Traceback (most recent call last):
        File "<stdin>", line 1, in <module>
      ValueError: Invalid datetime_mode, can deserialize only from ISO8601

   With `uuid_mode` you can enable recognition of string literals containing
   two different representations of :class:`UUID` values:

   .. doctest::

      >>> loads('"aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"')
      'aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa'
      >>> loads('"aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"',
      ...       uuid_mode=UM_CANONICAL)
      UUID('aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa')
      >>> loads('"aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"',
      ...       uuid_mode=UM_HEX)
      UUID('aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa')
      >>> loads('"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"',
      ...       uuid_mode=UM_CANONICAL)
      'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'
      >>> loads('"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"',
      ...       uuid_mode=UM_HEX)
      UUID('aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa')


.. _ISO 8601: https://en.wikipedia.org/wiki/ISO_8601
.. _RapidJSON: https://github.com/miloyip/rapidjson
.. _UTC: https://en.wikipedia.org/wiki/Coordinated_Universal_Time
.. _Unix time: https://en.wikipedia.org/wiki/Unix_time
