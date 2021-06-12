.. -*- coding: utf-8 -*-
.. :Project:   python-rapidjson -- Decoder class documentation
.. :Author:    Lele Gaifax <lele@metapensiero.it>
.. :License:   MIT License
.. :Copyright: © 2017, 2018, 2020, 2021 Lele Gaifax
..

===============
 Decoder class
===============

.. currentmodule:: rapidjson

.. testsetup::

   import io
   from rapidjson import Decoder, Encoder, DM_ISO8601

.. class:: Decoder(number_mode=None, datetime_mode=None, uuid_mode=None, parse_mode=None)

   Class-based :func:`loads`\ -like functionality.

   :param int number_mode: enable particular :ref:`behaviors in handling numbers
                           <loads-number-mode>`
   :param int datetime_mode: how should :ref:`datetime, time and date instances be handled
                             <loads-datetime-mode>`
   :param int uuid_mode: how should :ref:`UUID instances be handled <loads-uuid-mode>`
   :param int parse_mode: whether the parser should allow :ref:`non-standard JSON
                          extensions <loads-parse-mode>`

   .. rubric:: Attributes

   .. attribute:: datetime_mode

      :type: int

      The datetime mode, whether and how datetime literals will be recognized.

   .. attribute:: number_mode

      :type: int

      The number mode, whether numeric literals will be decoded.

   .. attribute:: parse_mode

      :type: int

      The parse mode, whether comments and trailing commas are allowed.

   .. attribute:: uuid_mode

      :type: int

      The UUID mode, whether and how UUID literals will be recognized.

   .. rubric:: Methods

   .. method:: __call__(json, *, chunk_size=65536)

      :param json: either a ``str`` instance, an *UTF-8* ``bytes`` instance or a
                   *file-like* stream, containing the ``JSON`` to be decoded
      :param int chunk_size: in case of a stream, it will be read in chunks of this size
      :returns: a Python value

      .. doctest::

         >>> decoder = Decoder()
         >>> decoder('"€ 0.50"')
         '€ 0.50'
         >>> decoder(io.StringIO('"€ 0.50"'))
         '€ 0.50'
         >>> decoder(io.BytesIO(b'"\xe2\x82\xac 0.50"'))
         '€ 0.50'
         >>> decoder(b'"\xe2\x82\xac 0.50"')
         '€ 0.50'

   .. method:: end_array(sequence)

      :param sequence: an instance implement the *mutable sequence* protocol
      :returns: a new value

      This is called, if implemented, when a *JSON array* has been completely parsed, and
      can be used replace it with an arbitrary different value:

      .. doctest::

         >>> class TupleDecoder(Decoder):
         ...   def end_array(self, a):
         ...     return tuple(a)
         ...
         >>> td = TupleDecoder()
         >>> res = td('[{"one": [1]}, {"two":[2,3]}]')
         >>> isinstance(res, tuple)
         True
         >>> res[0]
         {'one': (1,)}
         >>> res[1]
         {'two': (2, 3)}

   .. method:: end_object(mapping)

      :param mapping: an instance representing the JSON object, either one implementing
                      the *mapping protocol* or a list containing ``(key, value)`` tuples
      :returns: a new value

      This is called, if implemented, when a *JSON object* has been completely parsed, and
      can be used to replace it with an arbitrary different value, like what can be done
      with the ``object_hook`` argument of the :func:`loads` function:

      .. doctest::

         >>> class Point(object):
         ...   def __init__(self, x, y):
         ...     self.x = x
         ...     self.y = y
         ...   def __repr__(self):
         ...     return 'Point(%s, %s)' % (self.x, self.y)
         ...
         >>> class PointDecoder(Decoder):
         ...   def end_object(self, d):
         ...     if 'x' in d and 'y' in d:
         ...       return Point(d['x'], d['y'])
         ...     else:
         ...       return d
         ...
         >>> pd = PointDecoder()
         >>> pd('{"x":1,"y":2}')
         Point(1, 2)

      When :meth:`start_object` returns a ``list`` instance, then the `mapping` argument
      is actually a list of tuples.

   .. method:: start_object()

      :returns: either a list or mapping instance

      This method, when implemented, is called whenever a new *JSON object* is found: it
      must return either a list or an instance implementing the *mapping protocol*.

      It can be used to select a different implementation than the standard ``dict`` used
      by default:

      .. doctest::

         >>> from collections import OrderedDict
         >>> class OrderedDecoder(Decoder):
         ...   def start_object(self):
         ...     return OrderedDict()
         ...
         >>> od = OrderedDecoder()
         >>> type(od('{"foo": "bar"}'))
         <class 'collections.OrderedDict'>

      By returning a ``list`` you obtain a different handling of *JSON objects*: instead
      of translating them into Python maps as soon as they are found, their *key-value
      tuples* are accumulated in the given list; when the *JSON object* has been
      completely parsed, then the sequence is passed to the method :meth:`end_object`, if
      implemented, that will reasonably transmogrify it into some kind of dictionary. When
      that method is missing, then the list is kept as is, thus representing all objects
      in the JSON origin as lists of key-value tuples in Python:

      .. doctest::

         >>> class KVPairsDecoder(Decoder):
         ...   def start_object(self):
         ...     return []
         ...
         >>> kvpd = KVPairsDecoder()
         >>> kvpd('{"one": 1, "two": 2}')
         [('one', 1), ('two', 2)]

   .. method:: string(s)

      :param s: a ``str`` instance
      :returns: a new value

      This method, when implemented, is called whenever a *JSON string* has been
      completely parsed, and can be used to replace it with an arbitrary different value:

      .. doctest::

         >>> class SwapStringCase(Decoder):
         ...   def string(self, s):
         ...     return s.swapcase()
         ...
         >>> ssc = SwapStringCase()
         >>> ssc('"Hello World!"')
         'hELLO wORLD!'

      Note that it is called **after** the recognition of dates and UUIDs, when
      `datetime_mode` and/or `uuid_mode` are specified:

      .. doctest::

         >>> class DDMMYYYY(Decoder):
         ...   def string(self, s):
         ...     if len(s) == 8 and s.isdigit():
         ...       dd = int(s[:2])
         ...       mm = int(s[2:4])
         ...       yyyy = int(s[-4:])
         ...       return (yyyy, mm, dd)
         ...     return s
         ...
         >>> ddmmyyyy = DDMMYYYY(datetime_mode=DM_ISO8601)
         >>> ddmmyyyy('["2017-08-21", "21082017"]')
         [datetime.date(2017, 8, 21), (2017, 8, 21)]
