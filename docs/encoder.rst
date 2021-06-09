.. -*- coding: utf-8 -*-
.. :Project:   python-rapidjson -- Encoder class documentation
.. :Author:    Lele Gaifax <lele@metapensiero.it>
.. :License:   MIT License
.. :Copyright: © 2017, 2018, 2019, 2020 Lele Gaifax
..

===============
 Encoder class
===============

.. currentmodule:: rapidjson

.. testsetup::

   from rapidjson import Decoder, Encoder, BM_NONE, IM_ONLY_LISTS, MM_ONLY_DICTS

.. class:: Encoder(skip_invalid_keys=False, ensure_ascii=True, write_mode=WM_COMPACT, \
                   indent=4, sort_keys=False, number_mode=None, datetime_mode=None, \
                   uuid_mode=None, bytes_mode=BM_UTF8, iterable_mode=IM_ANY_ITERABLE, \
                   mapping_mode=MM_ANY_MAPPING)

   Class-based :func:`dumps`\ -like functionality.

   :param bool skip_invalid_keys: whether invalid :class:`dict` keys :ref:`will be skipped
                                  <skip-invalid-keys>`
   :param bool ensure_ascii: whether the output should contain :ref:`only ASCII
                             characters <ensure-ascii>`
   :param int write_mode: enable particular :ref:`pretty print <write-mode>` behaviors
   :param indent: either an integer, the indentation width, or a string, that will be used
                  as line separator, when `write_mode` is not :data:`WM_COMPACT`
   :param bool sort_keys: whether dictionary keys should be :ref:`sorted
                          alphabetically <sort-keys>`
   :param int number_mode: enable particular :ref:`behaviors in handling numbers
                           <dumps-number-mode>`
   :param int datetime_mode: how should :ref:`datetime, time and date instances be handled
                             <dumps-datetime-mode>`
   :param int uuid_mode: how should :ref:`UUID instances be handled <dumps-uuid-mode>`
   :param int bytes_mode: how should :ref:`bytes instances be handled <dumps-bytes-mode>`
   :param int iterable_mode: how should `iterable` values be handled
   :param int mapping_mode: how should `mapping` values be handled

   .. rubric:: Attributes

   .. attribute:: bytes_mode

      :type: int

      How bytes values should be treated.

   .. attribute:: datetime_mode

      :type: int

      Whether and how datetime values should be encoded.

   .. attribute:: ensure_ascii

      :type: bool

      Whether the output should contain only ASCII characters.

   .. attribute:: indent_char

      :type: str

      What will be used as end-of-line character.

   .. attribute:: indent_count

      :type: int

      The indentation width.

   .. attribute:: iterable_mode

      :type: int

      Whether `iterables` will be generically encoded as ``JSON`` arrays or not.

   .. attribute:: mapping_mode

      :type: int

      Whether `mappings` will be generically encoded as ``JSON`` objects or not.

   .. attribute:: number_mode

      :type: int

      The encoding behavior with regards to numeric values.

   .. attribute:: skip_invalid_keys

      :type: bool

      Whether invalid keys shall be skipped.

      .. note:: `skip_invalid_keys` is a backward compatible alias of new
                ``MM_SKIP_NON_STRING_KEYS`` :ref:`mapping mode <mapping_mode>`.

   .. attribute:: sort_keys

      :type: bool

      Whether dictionary keys shall be sorted alphabetically.

      .. note:: `sort_keys` is a backward compatible alias of new ``MM_SORT_KEYS``
                :ref:`mapping mode <mapping_mode>`.

   .. attribute:: uuid_mode

      :type: int

      Whether and how UUID values should be encoded

   .. attribute:: write_mode

      :type: int

      Whether the output should be pretty printed or not.

   .. rubric:: Methods

   .. method:: __call__(obj, stream=None, *, chunk_size=65536)

      :param obj: the value to be encoded
      :param stream: a *file-like* instance
      :param int chunk_size: write the stream in chunks of this size at a time
      :returns: a string with the ``JSON`` encoded `value`, when `stream` is ``None``

      When `stream` is specified, the encoded result will be written there, possibly in
      chunks of `chunk_size` bytes at a time, and the return value will be ``None``.

   .. method:: default(value)

      :param value: the Python value to be encoded
      :returns: a *JSON-serializable* value

      If implemented, this method is called whenever the serialization machinery finds a
      Python object that it does not recognize: if possible, the method should returns a
      *JSON encodable* version of the `value`, otherwise raise a :exc:`TypeError`:

      .. doctest::

         >>> class Point:
         ...   def __init__(self, x, y):
         ...     self.x = x
         ...     self.y = y
         ...
         >>> point = Point(1,2)
         >>> Encoder()(point)
         Traceback (most recent call last):
           File "<stdin>", line 1, in <module>
         TypeError: <__main__.Point object at …> is not JSON serializable
         >>> class PointEncoder(Encoder):
         ...   def default(self, obj):
         ...     if isinstance(obj, Point):
         ...       return {'x': obj.x, 'y': obj.y}
         ...     else:
         ...       raise TypeError('%r is not JSON serializable' % obj)
         ...
         >>> pe = PointEncoder(sort_keys=True)
         >>> pe(point)
         '{"x":1,"y":2}'

      When you want to treat your :class:`bytes` instances in a special way, you can use
      :data:`BM_NONE` together with this method:

      .. doctest::

         >>> class HexifyBytes(Encoder):
         ...   def default(self, obj):
         ...     if isinstance(obj, bytes):
         ...       return obj.hex()
         ...     else:
         ...       return obj
         ...
         >>> small_numbers = bytes([1, 2, 3])
         >>> hb = HexifyBytes(bytes_mode=BM_NONE)
         >>> hb(small_numbers)
         '"010203"'

      Likewise, when you want full control on how an `iterable` such as a ``tuple`` should
      be encoded, you can use :data:`IM_ONLY_LISTS` and implement a suitable ``default()``
      method:

      .. doctest::

         >>> from time import localtime, struct_time
         >>> class ObjectifyStructTime(Encoder):
         ...   def default(self, obj):
         ...     if isinstance(obj, struct_time):
         ...       return {'__class__': 'time.struct_time', '__init__': list(obj)}
         ...     else:
         ...       raise ValueError('%r is not JSON serializable' % obj)
         ...
         >>> obst = ObjectifyStructTime(iterable_mode=IM_ONLY_LISTS)
         >>> obst(localtime()) # doctest: +SKIP
         '[2020,11,28,19,55,40,5,333,0]'

      Similarly, when you want full control on how a `mapping` other than plain ``dict``
      should be encoded, you can use :data:`MM_ONLY_DICTS` and implement a ``default()``
      method:

      .. doctest::

         >>> from collections import OrderedDict
         >>> class ObjectifyOrderedDict(Encoder):
         ...   def default(self, obj):
         ...     if isinstance(obj, OrderedDict):
         ...       return {'__class__': 'collections.OrderedDict',
         ...               '__init__': list(obj.items())}
         ...     else:
         ...       raise ValueError('%r is not JSON serializable' % obj)
         ...
         >>> ood = ObjectifyOrderedDict(mapping_mode=MM_ONLY_DICTS)
         >>> ood(OrderedDict((('a', 1), ('b', 2))))
         '{"__class__":"collections.OrderedDict","__init__":[["a",1],["b",2]]}'
