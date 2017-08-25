.. -*- coding: utf-8 -*-
.. :Project:   python-rapidjson -- Validator class documentation
.. :Author:    Lele Gaifax <lele@metapensiero.it>
.. :License:   MIT License
.. :Copyright: © 2017 Lele Gaifax
..

=================
 Validator class
=================

.. module:: rapidjson

.. testsetup::

   from rapidjson import Validator

.. class:: Validator(json_schema)

   :param json_schema: the `JSON schema`__, specified as a ``str`` instance or an *UTF-8*
                       ``bytes`` instance

   __ http://json-schema.org/documentation.html

   .. method:: __call__(json)

      :param json: the ``JSON`` value, specified as a ``str`` instance or an *UTF-8*
                   ``bytes`` instance, that will be validated

      The given `json` value will be validated accordingly to the *schema*: a
      ``ValueError`` will be raised if the validation fails, and the exception will
      contain three arguments, respectively the type of the error, the position in the
      schema and the position in the ``JSON`` document where the error occurred:

      .. doctest::

         >>> validate = Validator('{"required": ["a", "b"]}')
         >>> validate('{"a": null, "b": 1}')
         >>> try:
         ...   validate('{"a": null, "c": false}')
         ... except ValueError as error:
         ...   print(error.args)
         ...
         ('required', '#', '#')
