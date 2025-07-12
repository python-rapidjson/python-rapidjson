.. -*- coding: utf-8 -*-
.. :Project:   python-rapidjson -- Validator class documentation
.. :Author:    Lele Gaifax <lele@metapensiero.it>
.. :License:   MIT License
.. :Copyright: © 2017, 2018, 2019, 2020, 2024 Lele Gaifax
..

=================
 Validator class
=================

.. currentmodule:: rapidjson

.. testsetup::

   from rapidjson import ValidationError, Validator

.. class:: Validator(json_schema)

   :param json_schema: the `JSON schema`__, specified as a ``str`` instance or an *UTF-8*
                       :class:`bytes`/:class:`bytearray` instance
   :raises JSONDecodeError: if `json_schema` is not a valid ``JSON`` value

   __ http://json-schema.org/documentation.html

   .. method:: __call__(json)

      :param json: the ``JSON`` value, specified as a ``str`` instance or an *UTF-8*
                   :class:`bytes`/:class:`bytearray` instance, that will be validated
      :raises JSONDecodeError: if `json` is not a valid ``JSON`` value

      The given `json` value will be validated accordingly to the *schema*: a
      :exc:`ValidationError` will be raised if the validation fails, and the exception
      will contain three arguments, respectively the type of the error, the position in
      the schema and the position in the ``JSON`` document where the error occurred:

      .. doctest::

         >>> validate = Validator('{"required": ["a", "b"]}')
         >>> validate('{"a": null, "b": 1}')
         >>> try:
         ...   validate('{"a": null, "c": false}')
         ... except ValidationError as error:
         ...   print(error.args)
         ...
         ('required', '#', '#')

      .. doctest::

         >>> validate = Validator('{"type": "array",'
         ...                      ' "items": {"type": "string"},'
         ...                      ' "minItems": 1}')
         >>> validate('["foo", "bar"]')
         >>> try:
         ...   validate('[]')
         ... except ValidationError as error:
         ...   print(error.args)
         ...
         ('minItems', '#', '#')

      .. doctest::

         >>> try:
         ...   validate('[1]')
         ... except ValidationError as error:
         ...   print(error.args)
         ...
         ('type', '#/items', '#/0')

      When `json` is not a valid JSON document, a :exc:`JSONDecodeError` is raised instead:

      .. doctest::

         >>> validate('x')
         Traceback (most recent call last):
           File "<stdin>", line 1, in <module>
         rapidjson.JSONDecodeError: Invalid JSON

============
Regex Engine
============
By default RapidJson uses a simple NFA regular expression engine for it's schema
validation, see the section `RapidJson Regular Expression`__, it is possible
to use ``std::regex`` instead of the original implementation by setting the
environmental variable ``RAPIDJSON_SCHEMA_USE_STDREGEX=1``.

.. code-block:: bash

    $ export RAPIDJSON_SCHEMA_USE_STDREGEX=1
    $ pip install python-rapidjson

After installation, you can verify that std::regex is being used by checking the
constant in the module:

.. code-block:: python

    >>> import rapidjson
    >>> rapidjson.RAPIDJSON_SCHEMA_USE_STDREGEX
    1

__ http://rapidjson.org/md_doc_schema.html#Regex
