.. -*- coding: utf-8 -*-
.. :Project:   python-rapidjson -- dumps function documentation
.. :Author:    Lele Gaifax <lele@metapensiero.it>
.. :License:   MIT License
.. :Copyright: © 2016, 2017 Lele Gaifax
..

==================
 dumps() function
==================

.. module:: rapidjson

.. testsetup::

   from rapidjson import (dumps, loads, DM_NONE, DM_ISO8601, DM_UNIX_TIME,
                          DM_ONLY_SECONDS, DM_IGNORE_TZ, DM_NAIVE_IS_UTC, DM_SHIFT_TO_UTC,
                          UM_NONE, UM_CANONICAL, UM_HEX, NM_NATIVE, NM_DECIMAL, NM_NAN,
                          PM_NONE, PM_COMMENTS, PM_TRAILING_COMMAS)

.. function:: dumps(obj, skipkeys=False, ensure_ascii=True, indent=None, \
                    default=None, sort_keys=False, max_recursion_depth=2048, \
                    number_mode=None, datetime_mode=None, uuid_mode=None, \
                    allow_nan=True)

   Encode given Python `obj` instance into a ``JSON`` string.

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
   :param bool allow_nan: *compatibility* flag equivalent to ``number_mode=NM_NAN``
   :returns: A Python :class:`str` instance.


   .. _skip-invalid-keys:
   .. rubric:: `skipkeys`

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


   .. _ensure-ascii:
   .. rubric:: `ensure_ascii`

   If `ensure_ascii` is true (the default), the output is guaranteed to have all incoming
   non-ASCII characters escaped.  If `ensure_ascii` is false, these characters will be
   output as-is:

   .. doctest::

      >>> dumps('The symbol for the Euro currency is €')
      '"The symbol for the Euro currency is \\u20AC"'
      >>> dumps('The symbol for the Euro currency is €',
      ...       ensure_ascii=False)
      '"The symbol for the Euro currency is €"'


   .. _pretty-print:
   .. rubric:: `indent`

   When `indent` is ``None`` (the default), ``python-rapidjson`` produces the most compact
   JSON representation. By setting `indent` to 0 each array item and each dictionary value
   will be followed by a newline. A positive integer means that each *level* will be
   indented by that many spaces:

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


   .. rubric:: `default`

   The `default` argument may be used to specify a custom serializer for otherwise not
   handled objects. If specified, it should be a function that gets called for such
   objects and returns a JSON encodable version of the object itself or raise a
   :exc:`TypeError`:

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


   .. _sort-keys:
   .. rubric:: `sort_keys`

   When `sort_keys` is true (default: ``False``), the JSON representation of Python
   dictionaries is sorted by key:

   .. doctest::

      >>> dumps(point, default=point_jsonifier, sort_keys=True)
      '{"x":1,"y":2}'


   .. _max-depth:
   .. rubric:: `max_recursion_depth`

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


   .. _dumps-number-mode:
   .. rubric:: `number_mode`

   The `number_mode` argument selects different behaviors in handling numeric values.

   By default *non-numbers* (``nan``, ``inf``, ``-inf``) will be serialized as their
   JavaScript equivalents (``NaN``, ``Infinity``, ``-Infinity``), because ``NM_NAN`` is
   *on* by default (**NB**: this is *not* compliant with the ``JSON`` standard):

   .. doctest::

      >>> nan = float('nan')
      >>> inf = float('inf')
      >>> dumps([nan, inf])
      '[NaN,Infinity]'
      >>> dumps([nan, inf], number_mode=NM_NAN)
      '[NaN,Infinity]'

   Explicitly setting `number_mode` or using the compatibility option `allow_nan` you can
   avoid that and obtain a ``ValueError`` exception instead:

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

   while using :data:`NM_DECIMAL` they will be serialized as their textual representation
   like any other float value:

   .. doctest::

      >>> dumps(pi, number_mode=NM_DECIMAL)
      '3.1415926535897932384626433832795028841971'

   Yet another possible flag affects how numeric values are passed to the underlying
   RapidJSON_ library: by default they are serialized to their string representation by
   the module itself, so they are virtually of unlimited precision:

   .. doctest::

      >>> dumps(123456789012345678901234567890)
      '123456789012345678901234567890'

   With :data:`NM_NATIVE` their binary values will be passed directly instead: this is
   somewhat faster, it is subject to the underlying C library ``long long`` and ``double``
   limits:

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


   .. _dumps-datetime-mode:
   .. rubric:: `datetime_mode`

   By default :class:`date`, :class:`datetime` and :class:`time` instances are not
   serializable:

   .. doctest::

      >>> from datetime import datetime
      >>> right_now = datetime(2016, 8, 28, 13, 14, 52, 277256)
      >>> date = right_now.date()
      >>> time = right_now.time()
      >>> dumps({'date': date, 'time': time, 'timestamp': right_now})
      Traceback (most recent call last):
        File "<stdin>", line 1, in <module>
      TypeError: datetime(…) is not JSON serializable

   When `datetime_mode` is set to :data:`DM_ISO8601` those values are serialized using the
   common `ISO 8601`_ format:

   .. doctest::

      >>> dumps(['date', date, 'time', time, 'timestamp', right_now],
      ...       datetime_mode=DM_ISO8601)
      '["date","2016-08-28","time","13:14:52.277256","timestamp","2016-08-28T13:14:52.277256"]'

   The `right_now` value is a naïve datetime (because it does not carry the timezone
   information) and is normally assumed to be in the local timezone, whatever your system
   thinks it is. When you instead *know* that your value, even being naïve are actually in
   the UTC_ timezone, you can use the :data:`DM_NAIVE_IS_UTC` flag to inform RapidJSON
   about that:

   .. doctest::

      >>> mode = DM_ISO8601 | DM_NAIVE_IS_UTC
      >>> dumps(['time', time, 'timestamp', right_now], datetime_mode=mode)
      '["time","13:14:52.277256+00:00","timestamp","2016-08-28T13:14:52.277256+00:00"]'

   A variant is :data:`DM_SHIFT_TO_UTC`, that *shifts* all datetime values to the UTC_
   timezone before serializing them:

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

   Another :ref:`one-way only <no-unix-time-loads>` alternative format is `Unix time`_:
   with :data:`DM_UNIX_TIME` :class:`date`, :class:`datetime` and :class:`time` instances
   are serialized as a number of seconds, respectively since the ``EPOCH`` for the first
   two kinds and since midnight for the latter:

   .. doctest::

      >>> mode = DM_UNIX_TIME
      >>> dumps([now, now.date(), now.time()], datetime_mode=mode)
      '[1472409071.084418,1472335200.0,73871.084418]'
      >>> unixtime = float(dumps(now, datetime_mode=mode))
      >>> datetime.fromtimestamp(unixtime, here) == now
      True

   Combining it with the :data:`DM_ONLY_SECONDS` will produce integer values instead,
   dropping *microseconds*:

   .. doctest::

      >>> mode = DM_UNIX_TIME | DM_ONLY_SECONDS
      >>> dumps([now, now.date(), now.time()], datetime_mode=mode)
      '[1472409071,1472335200,73871]'

   It can be used combined with :data:`DM_SHIFT_TO_UTC` to obtain the timestamp of the
   corresponding UTC_ time:

      >>> mode = DM_UNIX_TIME | DM_SHIFT_TO_UTC
      >>> dumps(now, datetime_mode=mode)
      '1472409071.084418'

   As above, when you know that your values are in the UTC_ timezone, you can use the
   :data:`DM_NAIVE_IS_UTC` flag to get the right result:

   .. doctest::

      >>> a_long_time_ago = datetime(1968, 3, 18, 9, 10, 0, 0)
      >>> mode = DM_UNIX_TIME | DM_NAIVE_IS_UTC
      >>> dumps([a_long_time_ago, a_long_time_ago.date(), a_long_time_ago.time()],
      ...       datetime_mode=mode)
      '[-56472600.0,-56505600.0,33000.0]'


   .. _dumps-uuid-mode:
   .. rubric:: `uuid_mode`

   Likewise, to handle :class:`UUID` instances there are two modes that can be specified
   with the `uuid_mode` argument, that will use the string representation of their values:

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


.. _ISO 8601: https://en.wikipedia.org/wiki/ISO_8601
.. _RapidJSON: https://github.com/miloyip/rapidjson
.. _UTC: https://en.wikipedia.org/wiki/Coordinated_Universal_Time
.. _Unix time: https://en.wikipedia.org/wiki/Unix_time
