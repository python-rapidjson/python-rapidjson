.. -*- coding: utf-8 -*-
.. :Project:   python-rapidjson -- API documentation
.. :Author:    Lele Gaifax <lele@metapensiero.it>
.. :License:   MIT License
.. :Copyright: © 2016, 2017, 2018, 2019, 2020 Lele Gaifax
..

===============================
 Exposed functions and symbols
===============================

.. currentmodule:: rapidjson

.. toctree::
   :maxdepth: 2

   dumps
   dump
   loads
   load
   encoder
   decoder
   validator
   rawjson

.. data:: __author__

   The authors of the module.

.. data:: __version__

   The version of the module.

.. data:: __rapidjson_version__

   The version of the RapidJSON_ library this module is built with.

.. data:: __rapidjson_exact_version__

   The *exact* version of the RapidJSON library, as determined by ``git describe``.


.. rubric:: `datetime_mode` related constants

.. data:: DM_NONE

   This is the default `datetime_mode`: *neither* :class:`date` *nor* :class:`datetime`
   *nor* :class:`time` instances are recognized by :func:`dumps` and :func:`loads`.

.. data:: DM_ISO8601

   In this `datetime_mode` mode :func:`dumps` and :func:`loads` handle :class:`date`,
   :class:`datetime` *and* :class:`date` instances representing those values using the
   `ISO 8601`_ format.

.. data:: DM_UNIX_TIME

   This mode tells RapidJSON to serialize :class:`date`, :class:`datetime` and
   :class:`time` as *numeric timestamps*: for the formers it is exactly the result of
   their :meth:`timestamp` method, that is as the number of seconds passed since
   ``EPOCH``; ``date`` instances are serialized as the corresponding ``datetime`` instance
   with all the `time` slots set to 0; ``time`` instances are serialized as the number of
   seconds since midnight.

   Since this is obviously *irreversible*, this flag is usable **only** for :func:`dumps`:
   an error is raised when passed to :func:`loads`.

.. data:: DM_ONLY_SECONDS

   This is usable in combination with :data:`DM_UNIX_TIME` so that an integer
   representation is used, ignoring *microseconds*.

.. data:: DM_IGNORE_TZ

   This can be used combined with :data:`DM_ISO8601` or :data:`DM_UNIX_TIME`, to ignore
   the value's timezones.

.. data:: DM_NAIVE_IS_UTC

   This can be used combined with :data:`DM_ISO8601` or :data:`DM_UNIX_TIME`, to tell
   RapidJSON that the naïve values (that is, without an explicit timezone) are already in
   UTC_ timezone.

.. data:: DM_SHIFT_TO_UTC

   This can be used combined with :data:`DM_ISO8601` or :data:`DM_UNIX_TIME`, to always
   *shift* values the UTC_ timezone.


.. rubric:: `uuid_mode` related constants

.. data:: UM_NONE

   This is the default `uuid_mode`: :class:`UUID` instances are *not* recognized by
   :func:`dumps` and :func:`loads`.

.. data:: UM_CANONICAL

   In this `uuid_mode`, :func:`loads` recognizes string values containing the
   ``xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`` canonical representation as :class:`UUID`
   instances; :func:`dumps` emits same kind of representation for :class:`UUID` instances
   as a string value.

.. data:: UM_HEX

   In this `uuid_mode` :func:`loads` recognizes string values containing exactly 32 hex
   digits *or* the ``xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`` canonical representation as
   :class:`UUID` instances; :func:`dumps` emits the 32 hex digits of :class:`UUID`
   instances as a string value.


.. rubric:: `number_mode` related constants

.. data:: NM_NONE

   This is the default `number_mode`: numeric values can be as wide as the memory allows.

.. data:: NM_DECIMAL

   In this `number_mode` :func:`loads` will return floating point values as
   :class:`Decimal` instances instead of :class:`float`; :func:`dumps` will serialize
   :class:`Decimal` instances like any other :class:`float` number.

.. data:: NM_NAN

   This enables *non-numbers* (i.e. ``nan``, ``+inf`` and ``-inf``) handling in both
   directions.

.. data:: NM_NATIVE

   In this alternative `number_mode` numeric values must fit into the underlying C library
   limits, with a considerable speed benefit.


.. rubric:: `parse_mode` related constants

.. data:: PM_NONE

   This is the default `parse_mode`: with the exception of the *NaN and Infinite*
   recognition active by default, the parser is in *strict mode*.

.. data:: PM_COMMENTS

   In this `parse_mode`, the parser allows and ignores one-line ``// ...`` and multi-line
   ``/* ... */`` comments

.. data:: PM_TRAILING_COMMAS

   In this `parse_mode`, the parser allows and ignores trailing commas at the end of
   *arrays* and *objects*.


.. rubric:: `bytes_mode` related constants

.. data:: BM_NONE

   This disables the default handling mode (:data:`BM_UTF8`) of :class:`bytes` instances:
   they won't be treated in any special way and will raise a ``TypeError`` exception when
   encountered. On the other hand, in this mode they can be managed by a `default`
   handler.

.. data:: BM_UTF8

   This is the default setting for `bytes_mode`: any :class:`bytes` instance will be
   assumed to be an ``UTF-8`` encoded string, and decoded accordingly.


.. rubric:: `iterable_mode` related constants

.. data:: IM_ANY_ITERABLE

   This is the default setting for `iterable_mode`: any iterable will be dumped as a
   ``JSON`` array.

.. data:: IM_ONLY_LISTS

   This disables the default handling mode (:data:`IM_ANY_ITERABLE`) of `iterable` values
   (with the exception of ``str``\ ings and ``list``\ s): they won't be treated in any
   special way and will raise a ``TypeError`` exception when encountered. On the other
   hand, in this mode they can be managed by a `default` handler.

.. _mapping_mode:
.. rubric:: `mapping_mode` related constants

.. data:: MM_ANY_MAPPING

   This is the default setting for `mapping_mode`: any mapping will be dumped as a
   ``JSON`` object.

.. data:: MM_ONLY_DICTS

   This disables the default handling mode (:data:`MM_ANY_MAPPING`) of generic `mapping`
   values, they won't be treated like ``dict``\ s and will raise a ``TypeError`` exception
   when encountered. On the other hand, in this mode they can be managed by a `default`
   handler.

.. data:: MM_COERCE_KEYS_TO_STRINGS

   Since ``JSON`` objects require that all keys must be of type string, when a mapping
   contains *non-string* keys, it will be processed by the `default` handler if available,
   otherwise a ``TypeError`` exception is raised.

   Alternatively, you can use the this option to automatically render such keys as their
   ``str()`` representation.

.. data:: MM_SKIP_NON_STRING_KEYS

   In this mode, dict keys that are not of a basic type (:class:`str`, :class:`int`,
   :class:`float`, :class:`bool`, ``None``) will be skipped instead of raising a
   :exc:`TypeError`.

.. data:: MM_SORT_KEYS

   Alphabetically order dictionary keys.

.. rubric:: Exceptions

.. exception:: JSONDecodeError

   A subclass of :exc:`ValueError`, raised when trying to parse an invalid ``JSON``,
   either by :class:`Validator` objects or by :func:`loads` function.

.. exception:: ValidationError

   Exception raised by :class:`Validator` objects, a subclass of :exc:`ValueError`.

   Its `args` attribute is a tuple with three string values, respectively the *schema
   keyword* that generated the failure, its *JSON pointer* and a *JSON pointer* to the
   error location in the (invalid) document.


.. rubric:: `write_mode` related constants

.. data:: WM_COMPACT

   The default dump mode, without any extra whitespace.

.. data:: WM_PRETTY

   This selects the RapidJSON ``PrettyWriter``, to produce more readable ``JSON``: each
   array's item and object's key will be preceded by a newline, and nested structures will
   be indented.

.. data:: WM_SINGLE_LINE_ARRAY

   This tells the ``PrettyWriter`` to emit arrays on a single line, instead of separating
   items with a newline.


.. _ISO 8601: https://en.wikipedia.org/wiki/ISO_8601
.. _RapidJSON: http://rapidjson.org/
.. _UTC: https://en.wikipedia.org/wiki/Coordinated_Universal_Time
.. _Unix time: https://en.wikipedia.org/wiki/Unix_time
