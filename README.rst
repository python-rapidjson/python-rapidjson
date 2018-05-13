.. -*- coding: utf-8 -*-
.. :Project:   python-rapidjson -- Introduction
.. :Author:    Ken Robbins <ken@kenrobbins.com>
.. :License:   MIT License
.. :Copyright: © 2015 Ken Robbins
.. :Copyright: © 2016, 2017, 2018 Lele Gaifax
..

==================
 python-rapidjson
==================

Python wrapper around RapidJSON
===============================

 :Authors: Ken Robbins <ken@kenrobbins.com>; Lele Gaifax <lele@metapensiero.it>
 :License: `MIT License`__
 :Status: |build| |doc|

__ https://raw.githubusercontent.com/python-rapidjson/python-rapidjson/master/LICENSE
.. |build| image:: https://travis-ci.org/python-rapidjson/python-rapidjson.svg?branch=master
   :target: https://travis-ci.org/python-rapidjson/python-rapidjson
   :alt: Build status
.. |doc| image:: https://readthedocs.org/projects/python-rapidjson/badge/?version=latest
   :target: https://readthedocs.org/projects/python-rapidjson/builds/
   :alt: Documentation status

RapidJSON_ is an extremely fast C++ JSON parser and serialization library: this module
wraps it into a Python 3 extension, exposing its serialization/deserialization (to/from
either ``bytes``, ``str`` or *file-like* instances) and `JSON Schema`__ validation
capabilities.

Latest version documentation is automatically rendered by `Read the Docs`__.

__ http://json-schema.org/documentation.html
__ http://python-rapidjson.readthedocs.io/en/latest/


Getting Started
---------------

First install ``python-rapidjson``:

.. code-block:: bash

    $ pip install python-rapidjson

or, if you prefer `Conda`__:

.. code-block:: bash

    $ conda install -c conda-forge python-rapidjson

__ https://conda.io/docs/

Basic usage looks like this:

.. code-block:: python

    >>> import rapidjson
    >>> data = {'foo': 100, 'bar': 'baz'}
    >>> rapidjson.dumps(data)
    '{"bar":"baz","foo":100}'
    >>> rapidjson.loads('{"bar":"baz","foo":100}')
    {'bar': 'baz', 'foo': 100}
    >>>
    >>> class Stream:
    ...   def write(self, data):
    ...      print("Chunk:", data)
    ...
    >>> rapidjson.dump(data, Stream(), chunk_size=5)
    Chunk: b'{"foo'
    Chunk: b'":100'
    Chunk: b',"bar'
    Chunk: b'":"ba'
    Chunk: b'z"}'


Development
-----------

If you want to install the development version (maybe to contribute fixes or
enhancements) you may clone the repository:

.. code-block:: bash

    $ git clone --recursive https://github.com/python-rapidjson/python-rapidjson.git

.. note:: The ``--recursive`` option is needed because we use a *submodule* to
          include RapidJSON_ sources. Alternatively you can do a plain
          ``clone`` immediately followed by a ``git submodule update --init``.

          Alternatively, if you already have (a *compatible* version of)
          RapidJSON includes around, you can compile the module specifying
          their location with the option ``--rj-include-dir``, for example:

          .. code-block:: shell

             $ python3 setup.py build --rj-include-dir=/usr/include/rapidjson

A set of makefiles implement most common operations, such as *build*, *check*
and *release*; see ``make help`` output for a list of available targets.


Performance
-----------

``python-rapidjson`` tries to be as performant as possible while staying
compatible with the ``json`` module.

See the `this section`__ in the documentation for a comparison with other JSON libraries.

__ http://python-rapidjson.readthedocs.io/en/latest/benchmarks.html


Incompatibility
---------------

Here are things in the standard ``json`` library supports that we have decided
not to support:

``separators`` argument
  This is mostly used for pretty printing and not supported by ``RapidJSON``
  so it isn't a high priority. We do support ``indent`` kwarg that would get
  you nice looking JSON anyways.

Coercing keys when dumping
  ``json`` will stringify a ``True`` dictionary key as ``"true"`` if you dump it out but
  when you load it back in it'll still be a string. We want the dump and load to return
  the exact same objects so we have decided not to do this coercion.

Arbitrary encodings
  ``json.loads()`` accepts an ``encoding`` kwarg determining the encoding of its input,
  when that is a ``bytes`` or ``bytearray`` instance. Although ``RapidJSON`` is able to
  cope with several different encodings, we currently support only the recommended one,
  ``UTF-8``.

.. _RapidJSON: http://rapidjson.org/
