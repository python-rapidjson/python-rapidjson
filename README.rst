.. -*- coding: utf-8 -*-
.. :Project:   python-rapidjson -- Introduction
.. :Author:    Ken Robbins <ken@kenrobbins.com>
.. :License:   MIT License
.. :Copyright: © 2015 Ken Robbins
.. :Copyright: © 2016, 2017 Lele Gaifax
..

==================
 python-rapidjson
==================

Python wrapper around RapidJSON
===============================

RapidJSON_ is an extremely fast C++ JSON serialization library.

We do not support legacy Python versions, you will need to upgrade to Python 3
to use this library.

Latest version documentation is automatically rendered by `Read the Docs`__.

__ http://python-rapidjson.readthedocs.io/en/latest/

.. image:: https://travis-ci.org/python-rapidjson/python-rapidjson.svg?branch=master
   :target: https://travis-ci.org/python-rapidjson/python-rapidjson
   :alt: Build status

.. image:: https://readthedocs.org/projects/python-rapidjson/badge/?version=latest
   :target: http://python-rapidjson.readthedocs.io/en/latest/?badge=latest
   :alt: Documentation status


Getting Started
---------------

First install ``python-rapidjson``:

.. code-block:: bash

    $ pip install python-rapidjson

Basic usage looks like this:

.. code-block:: python

    >>> import rapidjson
    >>> data = {'foo': 100, 'bar': 'baz'}
    >>> rapidjson.dumps(data)
    '{"bar":"baz","foo":100}'
    >>> rapidjson.loads('{"bar":"baz","foo":100}')
    {'bar': 'baz', 'foo': 100}


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

The following tables show a comparison between this module and other libraries
with different data sets.  Last row (“overall”) is the total time taken by all
the benchmarks.

Each number show the factor between the time taken by each contender and
``python-rapidjson`` (in other words, they are *normalized* against a value of
1.0 for ``python-rapidjson``): the lower the number, the speedier the
contender.

In **bold** the winner.


Serialization
~~~~~~~~~~~~~

+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+
|       serialize       |  ``dumps()``\ [1]_   | ``Encoder()``\ [2]_  |  ``dumps(n)``\ [3]_  | ``Encoder(n)``\ [4]_ |     ujson\ [5]_      |   simplejson\ [6]_   |     stdlib\ [7]_     |      yajl\ [8]_      |
+=======================+======================+======================+======================+======================+======================+======================+======================+======================+
|    100 arrays dict    |         1.00         |         0.99         |       **0.73**       |         0.77         |         0.97         |         4.82         |         2.27         |         1.37         |
+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+
|    100 dicts array    |         1.00         |         1.02         |       **0.82**       |         0.85         |         0.90         |         5.68         |         2.19         |         1.36         |
+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+
|    256 Trues array    |       **1.00**       |         1.04         |         1.24         |         1.09         |         1.31         |         2.41         |         2.04         |         1.12         |
+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+
|    256 ascii array    |         1.00         |         1.01         |         1.04         |         1.09         |       **0.49**       |         1.01         |         0.95         |         1.08         |
+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+
|   256 doubles array   |       **1.00**       |         1.02         |         1.08         |         1.03         |         6.76         |         7.34         |         6.88         |         3.89         |
+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+
|   256 unicode array   |         1.00         |         0.89         |         0.89         |         0.91         |         0.56         |         0.73         |         0.83         |       **0.47**       |
+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+
|    complex object     |         1.00         |         1.01         |         0.85         |       **0.82**       |         1.04         |         4.11         |         2.62         |         2.16         |
+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+
|   composite object    |         1.00         |         1.02         |         0.72         |       **0.68**       |         0.88         |         2.79         |         1.82         |         1.71         |
+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+
|        overall        |         1.00         |         0.99         |       **0.73**       |         0.78         |         0.97         |         4.81         |         2.27         |         1.36         |
+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+


Deserialization
~~~~~~~~~~~~~~~

+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|      deserialize      |   ``loads()``\ [9]_   | ``Decoder()``\ [10]_  |  ``loads(n)``\ [11]_  | ``Decoder(n)``\ [12]_ |         ujson         |      simplejson       |        stdlib         |         yajl          |
+=======================+=======================+=======================+=======================+=======================+=======================+=======================+=======================+=======================+
|    100 arrays dict    |         1.00          |         1.00          |         0.90          |       **0.90**        |         0.95          |         1.60          |         1.11          |         1.18          |
+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|    100 dicts array    |         1.00          |         1.04          |         0.86          |       **0.86**        |         0.94          |         2.00          |         1.43          |         1.27          |
+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|    256 Trues array    |       **1.00**        |         1.16          |         1.10          |         1.12          |         1.20          |         1.95          |         1.90          |         1.89          |
+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|    256 ascii array    |       **1.00**        |         1.02          |         1.02          |         1.02          |         1.39          |         1.14          |         1.25          |         1.62          |
+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|   256 doubles array   |         1.00          |         0.90          |       **0.16**        |         0.16          |         0.39          |         0.86          |         0.83          |         0.42          |
+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|   256 unicode array   |       **1.00**        |         1.01          |         1.01          |         1.00          |         1.02          |         5.14          |         5.34          |         2.40          |
+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|    complex object     |         1.00          |         1.01          |       **0.73**        |         0.73          |         0.88          |         1.60          |         1.14          |         1.20          |
+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|   composite object    |         1.00          |         1.02          |       **0.81**        |         0.81          |         0.85          |         1.97          |         1.42          |         1.26          |
+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|        overall        |         1.00          |         1.00          |         0.90          |       **0.90**        |         0.95          |         1.61          |         1.12          |         1.18          |
+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+

.. [1] ``rapidjson.dumps()``
.. [2] ``rapidjson.Encoder()``
.. [3] ``rapidjson.dumps(number_mode=NM_NATIVE)``
.. [4] ``rapidjson.Encoder(number_mode=NM_NATIVE)``
.. [5] `ujson 1.35 <https://pypi.python.org/pypi/ujson/1.35>`__
.. [6] `simplejson 3.11.1 <https://pypi.python.org/pypi/simplejson/3.11.1>`__
.. [7] Python 3.6.2 standard library ``json``
.. [8] `yajl 0.3.5 <https://pypi.python.org/pypi/yajl/0.3.5>`__
.. [9] ``rapidjson.loads()``
.. [10] ``rapidjson.Decoder()``
.. [11] ``rapidjson.loads(number_mode=NM_NATIVE)``
.. [12] ``rapidjson.Decoder(number_mode=NM_NATIVE)``


DIY
~~~

To run these tests yourself, clone the repo and run:

.. code-block:: bash

   $ make benchmarks

or

.. code-block:: bash

   $ make benchmarks-other

The former will focus only on ``RapidJSON`` and is particularly handy coupled
with the `compare past runs`__ functionality of ``pytest-benchmark``:

.. code-block:: bash

   $ make benchmarks PYTEST_OPTIONS=--benchmark-autosave
   # hack, hack, hack!
   $ make benchmarks PYTEST_OPTIONS=--benchmark-compare=0001

   ----------------------- benchmark 'deserialize': 18 tests ------------------------
   Name (time in us)                                                            Min…
   ----------------------------------------------------------------------------------
   test_loads[rapidjson-256 Trues array] (NOW)                         5.2320 (1.0)…
   test_loads[rapidjson-256 Trues array] (0001)                        5.4180 (1.04)…
   …

To reproduce the tables above run ``make benchmarks-tables``

__ http://pytest-benchmark.readthedocs.org/en/latest/comparing.html


Incompatibility
---------------

Here are things in the standard ``json`` library supports that we have decided
not to support:

``separators`` argument
  This is mostly used for pretty printing and not supported by ``RapidJSON``
  so it isn't a high priority. We do support ``indent`` kwarg that would get
  you nice looking JSON anyways.

Coercing keys when dumping
  ``json`` will turn ``True`` into ``'True'`` if you dump it out but when you
  load it back in it'll still be a string. We want the dump and load to return
  the exact same objects so we have decided not to do this coercion.

.. _RapidJSON: https://github.com/miloyip/rapidjson
