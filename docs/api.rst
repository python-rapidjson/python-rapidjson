.. -*- coding: utf-8 -*-
.. :Project:   python-rapidjson -- API documentation
.. :Author:    Lele Gaifax <lele@metapensiero.it>
.. :License:   MIT License
.. :Copyright: © 2016, 2017 Lele Gaifax
..

===============================
 Exposed functions and symbols
===============================

.. module:: rapidjson

.. toctree::
   :maxdepth: 2

   dumps
   loads
   encoder
   decoder
   validator

.. data:: __author__

   The author of the module.

.. data:: __version__

   The version of the module.

.. data:: __rapidjson_version__

   The version of the RapidJSON_ library this module is built with.


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


.. _ISO 8601: https://en.wikipedia.org/wiki/ISO_8601
.. _RapidJSON: https://github.com/miloyip/rapidjson
.. _UTC: https://en.wikipedia.org/wiki/Coordinated_Universal_Time
.. _Unix time: https://en.wikipedia.org/wiki/Unix_time
