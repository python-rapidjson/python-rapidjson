.. -*- coding: utf-8 -*-
.. :Project:   python-rapidjson -- dump function documentation
.. :Author:    Lele Gaifax <lele@metapensiero.it>
.. :License:   MIT License
.. :Copyright: © 2017, 2018, 2019, 2020 Lele Gaifax
..

=================
 dump() function
=================

.. currentmodule:: rapidjson

.. testsetup::

   import io
   from rapidjson import dump, dumps

.. function:: dump(obj, stream, *, skipkeys=False, ensure_ascii=True, \
                   write_mode=WM_COMPACT, indent=4, default=None, sort_keys=False, \
                   number_mode=None, datetime_mode=None, uuid_mode=None, \
                   bytes_mode=BM_UTF8, iterable_mode=IM_ANY_ITERABLE, \
                   mapping_mode=MM_ANY_MAPPING, chunk_size=65536, allow_nan=True)

   Encode given Python `obj` instance into a ``JSON`` stream.

   :param obj: the value to be serialized
   :param stream: a *file-like* instance
   :param bool skipkeys: whether skip invalid :class:`dict` keys
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
   :param int chunk_size: write the stream in chunks of this size at a time
   :param bool allow_nan: *compatibility* flag equivalent to ``number_mode=NM_NAN``

   The function has the same behaviour as :func:`dumps()`, except that it requires
   an additional mandatory parameter, a *file-like* writable `stream` instance:

   .. doctest::

      >>> stream = io.BytesIO()
      >>> dump('bar', stream)
      >>> stream.getvalue()
      b'"bar"'

   The target may also be a text stream\ [#]_:

   .. doctest::

      >>> stream = io.StringIO()
      >>> dump(r'¯\_(ツ)_/¯', stream)
      >>> stream.getvalue() == dumps(r'¯\_(ツ)_/¯')
      True

   .. rubric:: `chunk_size`

   The `chunk_size` argument determines the size of the *buffer* used to feed the
   *stream*: the greater the value, the fewer calls will be made to its ``write()``
   method.

   Consult the :func:`dumps()` documentation for details on all other arguments.

.. [#] A *text stream* is recognized by checking the presence of an ``encoding`` member
       attribute on the instance.
