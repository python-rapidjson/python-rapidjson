.. -*- coding: utf-8 -*-
.. :Project:   python-rapidjson -- RawJSON class documentation
.. :Author:    Silvio Tomatis <silviot@gmail.com>
.. :License:   MIT License
.. :Copyright: © 2018 Silvio Tomatis
.. :Copyright: © 2018, 2020 Lele Gaifax
..

===============
 RawJSON class
===============

.. currentmodule:: rapidjson

.. testsetup::

   from rapidjson import RawJSON, dumps

.. class:: RawJSON(value)

   Preserialized JSON string.

   :param str value: the string that rapidjson should use verbatim when serializing this
                     object

Some applications might decide to store JSON-serialized objects in their database, but
might need to assemble them in a bigger JSON.

The ``RawJSON`` class serves this purpose. When serialized, the string value provided will
be used verbatim, whitespace included:

.. doctest::

    >>> raw_list = RawJSON('[1, 2,3]')
    >>> dumps({'foo': raw_list})
    '{"foo":[1, 2,3]}'

.. warning:: ``python-rapidjson`` runs no checks on the preserialized data.
   This means that it can potentially output invalid JSON, if you provide it:

   .. doctest::

       >>> raw_list = RawJSON('[1, ')
       >>> dumps({'foo': raw_list})
       '{"foo":[1, }'
