.. -*- coding: utf-8 -*-
.. :Project:   python-rapidjson -- dumps function documentation
.. :Author:    Lele Gaifax <lele@metapensiero.it>
.. :License:   MIT License
.. :Copyright: © 2016, 2017, 2018, 2019, 2020, 2022 Lele Gaifax
..

==================
 dumps() function
==================

.. currentmodule:: rapidjson

.. testsetup::

   from rapidjson import (dumps, loads, BM_NONE, BM_UTF8, DM_NONE, DM_ISO8601,
                          DM_UNIX_TIME, DM_ONLY_SECONDS, DM_IGNORE_TZ, DM_NAIVE_IS_UTC,
                          DM_SHIFT_TO_UTC, IM_ANY_ITERABLE, IM_ONLY_LISTS, MM_ANY_MAPPING,
                          MM_ONLY_DICTS, MM_COERCE_KEYS_TO_STRINGS, MM_SORT_KEYS,
                          NM_NATIVE, NM_DECIMAL, NM_NAN, PM_NONE, PM_COMMENTS,
                          PM_TRAILING_COMMAS, UM_NONE, UM_CANONICAL, UM_HEX, WM_COMPACT,
                          WM_PRETTY, WM_SINGLE_LINE_ARRAY)

.. function:: dumps(obj, *, skipkeys=False, ensure_ascii=True, write_mode=WM_COMPACT, \
                    indent=4, default=None, sort_keys=False, number_mode=None, \
                    datetime_mode=None, uuid_mode=None, bytes_mode=BM_UTF8, \
                    iterable_mode=IM_ANY_ITERABLE, mapping_mode=MM_ANY_MAPPING, \
                    allow_nan=True)

   Encode given Python `obj` instance into a ``JSON`` string.

   :param obj: the value to be serialized
   :param bool skipkeys: whether invalid :class:`dict` keys will be skipped
   :param bool ensure_ascii: whether the output should contain only ASCII
                             characters
   :param int write_mode: enable particular pretty print behaviors
   :param indent: indentation width or string to produce pretty printed JSON
   :param callable default: a function that gets called for objects that can't
                            otherwise be serialized
   :param bool sort_keys: whether dictionary keys should be sorted
                          alphabetically
   :param int number_mode: enable particular behaviors in handling numbers
   :param int datetime_mode: how should :class:`datetime`, :class:`time` and
                             :class:`date` instances be handled
   :param int uuid_mode: how should :class:`UUID` instances be handled
   :param int bytes_mode: how should :class:`bytes` instances be handled
   :param int iterable_mode: how should `iterable` values be handled
   :param int mapping_mode: how should `mapping` values be handled
   :param bool allow_nan: *compatibility* flag equivalent to ``number_mode=NM_NAN``
   :returns: A Python :class:`str` instance.


   .. _skip-invalid-keys:
   .. rubric:: `skipkeys`

   If `skipkeys` is true (default: ``False``), then dict keys that are not of a basic type
   (:class:`str`, :class:`int`, :class:`float`, :class:`bool`, ``None``) will be skipped
   instead of raising a :exc:`TypeError`:

   .. doctest::

      >>> dumps({(0,): 'empty tuple', True: 'a true value'})
      Traceback (most recent call last):
        File "<stdin>", line 1, in <module>
      TypeError: keys must be strings
      >>> dumps({(0,): 'empty tuple', True: 'a true value'}, skipkeys=True)
      '{}'

   .. note:: `skipkeys` is a backward compatible alias of new
             ``MM_SKIP_NON_STRING_KEYS`` :ref:`mapping mode <mapping_mode>`.

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


   .. _write-mode:
   .. rubric:: `write_mode`

   The `write_mode` controls how ``python-rapidjson`` emits JSON: by default it is
   :data:`WM_COMPACT`, that produces the most compact JSON representation:

   .. code-block:: pycon

      >>> dumps([1, 2, {'three': 3, 'four': 4}])
      '[1,2,{"four":4,"three":3}]'

   With :data:`WM_PRETTY` it will use ``RapidJSON``\ 's ``PrettyWriter``, with a default
   `indent` (see below) of four spaces:

   .. code-block:: pycon

      >>> print(dumps([1, 2, {'three': 3, 'four': 4}], write_mode=WM_PRETTY))
      [
          1,
          2,
          {
              "four": 4,
              "three": 3
          }
      ]

   With :data:`WM_SINGLE_LINE_ARRAY` arrays will be kept on a single line:

   .. code-block:: pycon

      >>> print(dumps([1, 2, 'three', [4, 5]], write_mode=WM_SINGLE_LINE_ARRAY))
      [1, 2, 'three', [4, 5]]
      >>> print(dumps([1, 2, {'three': 3, 'four': 4}], write_mode=WM_SINGLE_LINE_ARRAY))
      [1, 2, {
              "three": 3,
              "four": 4
          }]


   .. rubric:: `indent`

   The `indent` parameter may be either a positive integer number or a string: in the
   former case it specifies a number of spaces, while in the latter the string may contain
   zero or more ASCII *whitespace* characters (space, tab ``\t``, newline ``\n`` and
   carriage-return ``\r``), all equals (that is, ``"\n\t"`` is not accepted).

   The integer number or the length of the string determine how many spaces (or the
   characters composing the string) will be used to indent nested structures, when the
   `write_mode` above is not :data:`WM_COMPACT`, and it defaults to 4. Specifying a value
   different from ``None`` automatically sets `write_mode` to :data:`WM_PRETTY`, if not
   explicited.

   By setting `indent` to 0 each array item (when `write_mode` is not
   :data:`WM_SINGLE_LINE_MODE`) and each dictionary value will be followed by a newline. A
   positive integer means that each *level* will be indented by that many spaces:

   .. code-block:: pycon

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
      >>> print(dumps([1, 2, {'three': 3, 'four': 4}], indent=""))
      [
      1,
      2,
      {
      "four": 4,
      "three": 3
      }
      ]
      >>> print(dumps([1, 2, {'three': 3, 'four': 4}], indent="  "))
      [
        1,
        2,
        {
          "four": 4,
          "three": 3
        }
      ]
      >>> print(dumps([1, 2, {'three': 3, 'four': 4}], indent="\t"))
      [
              1,
              2,
              {
                      "three": 3,
                      "four": 4
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
      >>> dumps(point, default=point_jsonifier)
      '{"x":1,"y":2}'


   .. _sort-keys:
   .. rubric:: `sort_keys`

   When `sort_keys` is true (default: ``False``), the JSON representation of Python
   dictionaries is sorted by key:

   .. doctest::

      >>> data = {'a': 'A', 'c': 'C', 'i': 'I', 'd': 'D'}
      >>> dumps(data, sort_keys=True)
      '{"a":"A","c":"C","d":"D","i":"I"}'

   .. note:: `sort_keys` is a backward compatible alias of new ``MM_SORT_KEYS``
             :ref:`mapping mode <mapping_mode>`.

   .. doctest::

      >>> dumps(data, mapping_mode=MM_SORT_KEYS)
      '{"a":"A","c":"C","d":"D","i":"I"}'

   The default setting, on modern snakes (that is, on `Python >= 3.7`__), preserves
   original dictionary insertion order:

   .. doctest::

      >>> dumps(data)
      '{"a":"A","c":"C","i":"I","d":"D"}'

   __ https://mail.python.org/pipermail/python-dev/2017-December/151283.html


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

      >>> mode = DM_UNIX_TIME | DM_NAIVE_IS_UTC
      >>> dumps([now, now.date(), now.time()], datetime_mode=mode)
      '[1472409071.084418,1472342400.0,73871.084418]'
      >>> unixtime = float(dumps(now, datetime_mode=mode))
      >>> datetime.fromtimestamp(unixtime, here) == now
      True

   Combining it with the :data:`DM_ONLY_SECONDS` will produce integer values instead,
   dropping *microseconds*:

   .. doctest::

      >>> mode = DM_UNIX_TIME | DM_NAIVE_IS_UTC | DM_ONLY_SECONDS
      >>> dumps([now, now.date(), now.time()], datetime_mode=mode)
      '[1472409071,1472342400,73871]'

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


   .. _dumps-bytes-mode:
   .. rubric:: `bytes_mode`

   By default all :class:`bytes` instances are assumed to be ``UTF-8`` encoded strings,
   and acted on accordingly:

   .. doctest::

      >>> ascii_string = 'ciao'
      >>> bytes_string = b'cio\xc3\xa8'
      >>> unicode_string = 'cioè'
      >>> dumps([ascii_string, bytes_string, unicode_string])
      '["ciao","cio\\u00E8","cio\\u00E8"]'

   Sometime you may prefer a different approach, explicitly disabling that behavior using
   the :data:`BM_NONE` mode:

   .. doctest::

      >>> dumps([ascii_string, bytes_string, unicode_string],
      ...       bytes_mode=BM_NONE)
      Traceback (most recent call last):
        File "<stdin>", line 1, in <module>
      TypeError: b'cio\xc3\xa8' is not JSON serializable
      >>> my_bytes_handler = lambda b: b.decode('UTF-8').upper()
      >>> dumps([ascii_string, bytes_string, unicode_string],
      ...       bytes_mode=BM_NONE, default=my_bytes_handler)
      '["ciao","CIO\\u00C8","cio\\u00E8"]'


   .. dumps-iterable-mode:
   .. rubric:: `iterable_mode`

   By default a value that implements the `iterable` protocol gets encoded as a ``JSON``
   array:

   .. doctest::

      >>> from time import localtime, struct_time
      >>> lt = localtime()
      >>> dumps(lt) # doctest: +SKIP
      '[2020,11,28,19,55,40,5,333,0]'
      >>> class MyList(list):
      ...   pass
      >>> ml = MyList((1,2,3))
      >>> dumps(ml)
      '[1,2,3]'

   When that's not appropriate, for example because you want to use a different way to
   encode them, you may specify `iterable_mode` to ``IM_ONLY_LISTS``:

   .. doctest::

      >>> dumps(lt, iterable_mode=IM_ONLY_LISTS)
      Traceback (most recent call last):
        File "<stdin>", line 1, in <module>
      TypeError: <time.struct_time …> is not JSON serializable
      >>> dumps(ml, iterable_mode=IM_ONLY_LISTS)
      Traceback (most recent call last):
        ...
      TypeError: [1, 2, 3] is not JSON serializable

   and thus you can use the `default` argument:

   .. doctest::

      >>> def ts_or_ml(obj):
      ...   if isinstance(obj, struct_time):
      ...     return {'__class__': 'time.struct_time', '__init__': list(obj)}
      ...   elif isinstance(obj, MyList):
      ...     return [i*2 for i in obj]
      ...   else:
      ...     raise ValueError('%r is not JSON serializable' % obj)
      >>> dumps(lt, iterable_mode=IM_ONLY_LISTS, default=ts_or_ml) # doctest: +SKIP
      '{"__class__":"time.struct_time","__init__":[2020,11,28,19,55,40,5,333,0]}'
      >>> dumps(ml, iterable_mode=IM_ONLY_LISTS, default=ts_or_ml)
      '[2,4,6]'

   Obviously, in such case the value returned by the `default` callable **must not**
   be or contain a ``tuple``:

      >>> def bad_timestruct(obj):
      ...   if isinstance(obj, struct_time):
      ...     return {'__class__': 'time.struct_time', '__init__': tuple(obj)}
      ...   else:
      ...     raise ValueError('%r is not JSON serializable' % (obj,))
      >>> dumps(lt, iterable_mode=IM_ONLY_LISTS, default=bad_timestruct)
      Traceback (most recent call last):
        File "<stdin>", line 1, in <module>
      ValueError: (…) is not JSON serializable


   .. dumps-mapping-mode:
   .. rubric:: `mapping_mode`

   By default a value that implements the `mapping` protocol gets encoded as a ``JSON``
   object:

   .. doctest::

      >>> from collections import Counter
      >>> d = {"a":1,"b":2,"c":3}
      >>> c = Counter(d)
      >>> dumps([c, d])
      '[{"a":1,"b":2,"c":3},{"a":1,"b":2,"c":3}]'

   When that's not appropriate, for example because you want to use a different way to
   encode them, you may specify `mapping_mode` to ``MM_ONLY_DICTS``:

   .. doctest::

      >>> dumps(d, mapping_mode=MM_ONLY_DICTS)
      '{"a":1,"b":2,"c":3}'
      >>> dumps(c, mapping_mode=MM_ONLY_DICTS)
      Traceback (most recent call last):
        File "<stdin>", line 1, in <module>
      TypeError: Counter(…) is not JSON serializable

   and thus you can use the `default` argument:

   .. doctest::

      >>> def counter(obj):
      ...   if isinstance(obj, Counter):
      ...     return {'__class__': 'collections.Counter', '__init__': dict(obj)}
      ...   else:
      ...     raise ValueError('%r is not JSON serializable' % obj)
      >>> dumps(c, mapping_mode=MM_ONLY_DICTS, default=counter)
      '{"__class__":"collections.Counter","__init__":{"a":1,"b":2,"c":3}}'

   Obviously, in such case the value returned by the `default` callable **must not**
   be or contain mappings other than plain ``dict``\ s:

      >>> from collections import OrderedDict
      >>> def bad_counter(obj):
      ...   if isinstance(obj, Counter):
      ...     return {'__class__': 'time.struct_time', '__init__': OrderedDict(obj)}
      ...   else:
      ...     raise ValueError('%r is not JSON serializable' % (obj,))
      >>> dumps(c, mapping_mode=MM_ONLY_DICTS, default=bad_counter)
      Traceback (most recent call last):
        File "<stdin>", line 1, in <module>
      ValueError: OrderedDict([('a', 1), ('b', 2), ('c', 3)]) is not JSON serializable

   Normally, dumping a dictionary containing *non-string* keys raises a ``TypeError``
   exception:

   .. doctest::

      >>> dumps({-1: 'minus-one'})
      Traceback (most recent call last):
        File "<stdin>", line 1, in <module>
      TypeError: keys must be strings

   Setting `mapping_mode` to ``MM_COERCE_KEYS_TO_STRINGS`` such keys will be converted to
   their string representation:

   .. doctest::

      >>> dumps({-1: 'minus-one', True: "good", False: "bad", None: "ugly"},
      ...       mapping_mode=MM_COERCE_KEYS_TO_STRINGS)
      '{"-1":"minus-one","True":"good","False":"bad","None":"ugly"}'

   Alternatively, by providing a `default` function you can have finer control on how they
   should be encoded. For example the following mimics the default behaviour of the
   standard library ``json`` module:

   .. doctest::

      >>> def mimic_stdlib_json(obj):
      ...   if isinstance(obj, dict):
      ...     result = {}
      ...     for key in obj:
      ...       if key is True:
      ...         result['true'] = obj[key]
      ...       elif key is False:
      ...         result['false'] = obj[key]
      ...       elif key is None:
      ...         result['null'] = obj[key]
      ...       elif isinstance(key, (int, float)):
      ...         result[str(key)] = obj[key]
      ...       else:
      ...         raise TypeError('keys must be str, int, float, bool or None')
      ...     return result
      ...   else:
      ...     raise ValueError('%r is not JSON serializable' % (obj,))
      >>> dumps({True: 'good', False: 'bad', None: 'ugly'},
      ...       default=mimic_stdlib_json)
      '{"true":"good","false":"bad","null":"ugly"}'

   .. warning:: This can lead to an infinite recursion error, if the `default` function
                returns a dictionary that still contains *non-string* keys:

                .. doctest::

                   >>> dumps({True: 'vero', False: 'falso'},
                   ...       default=lambda map: map)
                   Traceback (most recent call last):
                     File "<stdin>", line 1, in <module>
                   RecursionError: maximum recursion depth exceeded

.. _ISO 8601: https://en.wikipedia.org/wiki/ISO_8601
.. _RapidJSON: http://rapidjson.org/
.. _UTC: https://en.wikipedia.org/wiki/Coordinated_Universal_Time
.. _Unix time: https://en.wikipedia.org/wiki/Unix_time
