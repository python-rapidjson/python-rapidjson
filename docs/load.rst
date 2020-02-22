.. -*- coding: utf-8 -*-
.. :Project:   python-rapidjson -- load function documentation
.. :Author:    Lele Gaifax <lele@metapensiero.it>
.. :License:   MIT License
.. :Copyright: © 2017, 2018, 2019, 2020 Lele Gaifax
..

=================
 load() function
=================

.. currentmodule:: rapidjson

.. testsetup::

   import io
   from rapidjson import load

.. function:: load(stream, *, object_hook=None, number_mode=None, datetime_mode=None, \
                   uuid_mode=None, parse_mode=None, chunk_size=65536, allow_nan=True)

   Decode the given Python file-like `stream` containing a ``JSON`` formatted value
   into Python object.

   :param stream: a file-like object
   :param callable object_hook: an optional function that will be called with the result
                                of any object literal decoded (a :class:`dict`) and should
                                return the value to use instead of the :class:`dict`
   :param int number_mode: enable particular behaviors in handling numbers
   :param int datetime_mode: how should :class:`datetime` and :class:`date` instances be
                             handled
   :param int uuid_mode: how should :class:`UUID` instances be handled
   :param int parse_mode: whether the parser should allow non-standard JSON extensions
   :param int chunk_size: read the stream in chunks of this size at a time
   :param bool allow_nan: *compatibility* flag equivalent to ``number_mode=NM_NAN``
   :returns: An equivalent Python object.
   :raises ValueError: if an invalid argument is given
   :raises JSONDecodeError: if `string` is not a valid ``JSON`` value

   The function has the same behaviour as :func:`loads()`, except for the kind of the
   first argument that is expected to be file-like object instead of a string:

   .. doctest::

      >>> load(io.StringIO('"Naïve"'))
      'Naïve'
      >>> load(io.BytesIO(b'["string", {"kind": "object"}, 3.14159]'))
      ['string', {'kind': 'object'}, 3.14159]

   .. rubric:: `chunk_size`

   The `chunk_size` argument determines the size of the *buffer* used to load the
   *stream*: the greater the value, the fewer calls will be made to its ``read()``
   method.

   Consult the :func:`loads()` documentation for details on all other arguments.
