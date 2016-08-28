===============================
 Exposed functions and symbols
===============================

.. default-domain:: py

.. data:: __version__

   The version of the module.

.. data:: __author__

   The author of the module.

.. data:: DATETIME_MODE_NONE

   This is the default ``datetime_mode``: *neither* :class:`datetime` *nor* :class:`date`
   instances are recognized by :func:`loads` and :func:`dumps`.

.. data:: DATETIME_MODE_ISO8601

   In this ``datetime_mode`` mode :func:`loads` and :func:`dumps` handle :class:`datetime`
   *and* :class:`date` instances representing those values using the `ISO 8601`_ format.

.. data:: DATETIME_MODE_ISO8601_IGNORE_TZ

   This is like :data:`DATETIME_MODE_ISO8601` except that the value's timezone is ignored.

.. data:: DATETIME_MODE_ISO8601_UTC

   This is like :data:`DATETIME_MODE_ISO8601` except that the times are always *shifted* to
   the UTC_ timezone.

.. data:: UUID_MODE_NONE

   This is the default ``uuid_mode``: :class:`UUID` instances are *not* recognized by
   :func:`loads` and :func:`dumps`.

.. data:: UUID_MODE_HEX

   In this ``uuid_mode`` :func:`loads` recognizes string values containing exactly 32 hex
   digits *or* the ``xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`` canonical representation as
   :class:`UUID` instances; :func:`dumps` emits the 32 hex digits of :class:`UUID` instances as
   a string value.

.. data:: UUID_MODE_CANONICAL

   In this ``uuid_mode``, :func:`loads` recognizes string values the
   ``xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`` canonical representation as :class:`UUID`
   instances; :func:`dumps` emits same kind of representation for :class:`UUID` instances as a
   string value.

.. function:: loads(s, object_hook=None, use_decimal=False, precise_float=True, \
                    allow_nan=True, datetime_mode=None, uuid_mode=None)

   :param str s: The JSON string to parse
   :param callable object_hook: an optional function that will be called with the result of
                                any object literal decoded (a :class:`dict`) and should return
                                the value to use instead of the :class:`dict`
   :param bool use_decimal: whether :class:`Decimal` instances are handled
   :param bool precise_float: use slower-but-more-precise float parser
   :param bool allow_nan: whether ``NaN`` values are handled
   :param int datetime_mode: how should :class:`datetime` and :class:`date` instances be
                             handled
   :param int uuid_mode: how should :class:`UUID` instances be handled
   :returns: An equivalent Python object.

   Decodes a JSON string into Python object.

.. function:: dumps(obj, skipkeys=False, ensure_ascii=True, allow_nan=True, indent=None, \
                    default=None, sort_keys=False, use_decimal=False, \
                    max_recursion_depth=2048, \
                    datetime_mode=None, uuid_mode=None)

   Encodes Python object into a JSON string.

.. _ISO 8601: https://en.wikipedia.org/wiki/ISO_8601
.. _UTC: https://en.wikipedia.org/wiki/Coordinated_Universal_Time
