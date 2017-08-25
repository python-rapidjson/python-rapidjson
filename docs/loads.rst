.. -*- coding: utf-8 -*-
.. :Project:   python-rapidjson -- loads function documentation
.. :Author:    Lele Gaifax <lele@metapensiero.it>
.. :License:   MIT License
.. :Copyright: © 2016, 2017 Lele Gaifax
..

==================
 loads() function
==================

.. module:: rapidjson

.. testsetup::

   from rapidjson import (dumps, loads, DM_NONE, DM_ISO8601, DM_UNIX_TIME,
                          DM_ONLY_SECONDS, DM_IGNORE_TZ, DM_NAIVE_IS_UTC, DM_SHIFT_TO_UTC,
                          UM_NONE, UM_CANONICAL, UM_HEX, NM_NATIVE, NM_DECIMAL, NM_NAN,
                          PM_NONE, PM_COMMENTS, PM_TRAILING_COMMAS)

.. function:: loads(s, object_hook=None, number_mode=None, datetime_mode=None, \
                    uuid_mode=None, parse_mode=None, allow_nan=True)

   Decode the given Python string `s` containing a ``JSON`` formatted value
   into Python object.

   :param str s: The JSON string to parse
   :param callable object_hook: an optional function that will be called with the result
                                of any object literal decoded (a :class:`dict`) and should
                                return the value to use instead of the :class:`dict`
   :param int number_mode: enable particular behaviors in handling numbers
   :param int datetime_mode: how should :class:`datetime` and :class:`date` instances be
                             handled
   :param int uuid_mode: how should :class:`UUID` instances be handled
   :param int parse_mode: whether the parser should allow non-standard JSON extensions
   :param bool allow_nan: *compatibility* flag equivalent to ``number_mode=NM_NAN``
   :returns: An equivalent Python object.


   .. rubric:: `object_hook`

   `object_hook` may be used to inject a custom deserializer that can replace any
   :class:`dict` instance found in the JSON structure with a *derived* object instance:

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


   .. _loads-number-mode:
   .. rubric:: `number_mode`

   The `number_mode` argument selects different behaviors in handling numeric values.

   By default *non-numbers* (``nan``, ``inf``, ``-inf``) are recognized, because
   ``NM_NAN`` is *on* by default:

   .. doctest::

      >>> loads('[NaN, Infinity]')
      [nan, inf]
      >>> loads('[NaN, Infinity]', number_mode=NM_NAN)
      [nan, inf]

   Explicitly setting `number_mode` or using the compatibility option `allow_nan` you can
   avoid that and obtain a ``ValueError`` exception instead:

   .. doctest::

      >>> loads('[NaN, Infinity]', number_mode=NM_NATIVE)
      Traceback (most recent call last):
        File "<stdin>", line 1, in <module>
      ValueError: … Out of range float values are not JSON compliant
      >>> loads('[NaN, Infinity]', allow_nan=False)
      Traceback (most recent call last):
        File "<stdin>", line 1, in <module>
      ValueError: … Out of range float values are not JSON compliant

   Normally all floating point literals present in the JSON structure will be loaded as
   Python :class:`float` instances, with :data:`NM_DECIMAL` they will be returned as
   :class:`Decimal` instances instead:

   .. doctest::

      >>> loads('1.2345')
      1.2345
      >>> loads('1.2345', number_mode=NM_DECIMAL)
      Decimal('1.2345')

   When you can be sure that all the numeric values are constrained within the
   architecture's hardware limits you can get a sensible speed gain with the
   :data:`NM_NATIVE` flag. While this is quite faster, integer literals that do not fit
   into the underlying C library ``long long`` limits will be converted (*truncated*) to
   ``double`` numbers:

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

   with the exception of :data:`NM_NATIVE` and :data:`NM_DECIMAL`, that does not make
   sense since there's little point in creating :class:`Decimal` instances out of possibly
   truncated float literals:

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


   .. _loads-datetime-mode:
   .. rubric:: `datetime_mode`

   With `datetime_mode` you can enable recognition of string literals containing an `ISO
   8601`_ representation as either :class:`date`, :class:`datetime` or :class:`time`
   instances:

   .. doctest::

      >>> loads('"2016-01-02T01:02:03+01:00"')
      '2016-01-02T01:02:03+01:00'
      >>> loads('"2016-01-02T01:02:03+01:00"', datetime_mode=DM_ISO8601)
      datetime.datetime(2016, 1, 2, 1, 2, 3, tzinfo=...delta(0, 3600)))
      >>> loads('"2016-01-02"', datetime_mode=DM_ISO8601)
      datetime.date(2016, 1, 2)
      >>> loads('"01:02:03+01:00"', datetime_mode=DM_ISO8601)
      datetime.time(1, 2, 3, tzinfo=...delta(0, 3600)))

   It can be combined with :data:`DM_SHIFT_TO_UTC` to *always* obtain values in the UTC_
   timezone:

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

   If you combine it with :data:`DM_NAIVE_IS_UTC` then all values without a timezone will
   be assumed to be relative to UTC_:

   .. doctest::

      >>> mode = DM_ISO8601 | DM_NAIVE_IS_UTC
      >>> loads('"2016-01-02T01:02:03"', datetime_mode=mode)
      datetime.datetime(2016, 1, 2, 1, 2, 3, tzinfo=...utc)
      >>> loads('"2016-01-02T01:02:03+01:00"', datetime_mode=mode)
      datetime.datetime(2016, 1, 2, 1, 2, 3, tzinfo=...delta(0, 3600)))
      >>> loads('"01:02:03"', datetime_mode=mode)
      datetime.time(1, 2, 3, tzinfo=...utc)

   Yet another combination is with :data:`DM_IGNORE_TZ` to ignore the timezone and obtain
   naïve values:

   .. doctest::

      >>> mode = DM_ISO8601 | DM_IGNORE_TZ
      >>> loads('"2016-01-02T01:02:03+01:00"', datetime_mode=mode)
      datetime.datetime(2016, 1, 2, 1, 2, 3)
      >>> loads('"01:02:03+01:00"', datetime_mode=mode)
      datetime.time(1, 2, 3)

   .. _no-unix-time-loads:

   The :data:`DM_UNIX_TIME` cannot be used here, because there isn't a reasonable
   heuristic to disambiguate between plain numbers and timestamps:

   .. doctest::

      >>> loads('[1,2,3]', datetime_mode=DM_UNIX_TIME)
      Traceback (most recent call last):
        File "<stdin>", line 1, in <module>
      ValueError: Invalid datetime_mode, can deserialize only from ISO8601


   .. _loads-uuid-mode:
   .. rubric:: `uuid_mode`

   With `uuid_mode` you can enable recognition of string literals containing two different
   representations of :class:`UUID` values:

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


   .. _loads-parse-mode:
   .. rubric:: `parse_mode`

   With `parse_mode` you can tell the parser to be *relaxed*, allowing either
   ``C++``/``JavaScript`` like comments (:data:`PM_COMMENTS`):

   .. doctest::

      >>> loads('"foo" // one line of explanation')
      Traceback (most recent call last):
        File "<stdin>", line 1, in <module>
      ValueError: Parse error at offset 6: The document root must not be followed by other values.
      >>> loads('"bar" /* detailed explanation */')
      Traceback (most recent call last):
        File "<stdin>", line 1, in <module>
      ValueError: Parse error at offset 6: The document root must not be followed by other values.
      >>> loads('"foo" // one line of explanation', parse_mode=PM_COMMENTS)
      'foo'
      >>> loads('"bar" /* detailed explanation */', parse_mode=PM_COMMENTS)
      'bar'

   or *trailing commas* at the end of arrays and objects (:data:`PM_TRAILING_COMMAS`):

   .. doctest::

      >>> loads('[1,]')
      Traceback (most recent call last):
        File "<stdin>", line 1, in <module>
      ValueError: Parse error at offset 3: Invalid value.
      >>> loads('[1,]', parse_mode=PM_TRAILING_COMMAS)
      [1]
      >>> loads('{"one": 1,}', parse_mode=PM_TRAILING_COMMAS)
      {'one': 1}

   or both:

   .. doctest::

      >>> loads('[1, /* 2, */ 3,]')
      Traceback (most recent call last):
        ...
      ValueError: Parse error at offset 4: Invalid value.
      >>> loads('[1, /* 2, */ 3,]', parse_mode=PM_COMMENTS | PM_TRAILING_COMMAS)
      [1, 3]

.. _ISO 8601: https://en.wikipedia.org/wiki/ISO_8601
.. _RapidJSON: https://github.com/miloyip/rapidjson
.. _UTC: https://en.wikipedia.org/wiki/Coordinated_Universal_Time
.. _Unix time: https://en.wikipedia.org/wiki/Unix_time
