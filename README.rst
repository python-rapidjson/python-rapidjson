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


Getting Started
---------------

First install ``python-rapidjson``:

.. code-block:: bash

    $ pip install python-rapidjson

RapidJSON tries to be compatible with the standard library ``json`` module so
it should be a drop in replacement. Basic usage looks like this:

.. code-block:: python

    >>> import rapidjson
    >>> data = {'foo': 100, 'bar': 'baz'}
    >>> rapidjson.dumps(data)
    '{"bar":"baz","foo":100}'
    >>> rapidjson.loads('{"bar":"baz","foo":100}')
    {'bar': 'baz', 'foo': 100}

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

+-----------------------+------------+------------+------------+------------+
|       serialize       |   ujson    | simplejson |    json    |    yajl    |
+=======================+============+============+============+============+
|  **100 arrays dict**  |    1.34    |    5.88    |    3.23    |    1.76    |
+-----------------------+------------+------------+------------+------------+
|  **100 dicts array**  |    1.50    |    6.87    |    2.85    |    1.70    |
+-----------------------+------------+------------+------------+------------+
|  **256 Trues array**  |    1.36    |    2.40    |    2.08    |    1.12    |
+-----------------------+------------+------------+------------+------------+
|    256 ascii array    |  **0.92**  |    1.96    |    1.75    |    1.97    |
+-----------------------+------------+------------+------------+------------+
| **256 doubles array** |    6.28    |    6.79    |    6.33    |    3.59    |
+-----------------------+------------+------------+------------+------------+
|   256 unicode array   |    0.65    |    0.82    |    0.87    |  **0.52**  |
+-----------------------+------------+------------+------------+------------+
|  **complex object**   |    1.34    |    4.78    |    3.04    |    2.50    |
+-----------------------+------------+------------+------------+------------+
|   composite object    |  **0.97**  |    3.13    |    1.96    |    1.93    |
+-----------------------+------------+------------+------------+------------+
|      **overall**      |    1.34    |    5.87    |    3.22    |    1.76    |
+-----------------------+------------+------------+------------+------------+

Deserialization
~~~~~~~~~~~~~~~

+-----------------------+------------+------------+------------+------------+
|      deserialize      |   ujson    | simplejson |    json    |    yajl    |
+=======================+============+============+============+============+
|    100 arrays dict    |  **0.96**  |    1.46    |    1.19    |    1.21    |
+-----------------------+------------+------------+------------+------------+
|    100 dicts array    |  **0.95**  |    1.98    |    1.56    |    1.30    |
+-----------------------+------------+------------+------------+------------+
|  **256 Trues array**  |    1.27    |    2.10    |    1.92    |    2.05    |
+-----------------------+------------+------------+------------+------------+
|  **256 ascii array**  |    1.38    |    1.13    |    1.23    |    1.56    |
+-----------------------+------------+------------+------------+------------+
|   256 doubles array   |  **0.45**  |    1.02    |    0.94    |    0.45    |
+-----------------------+------------+------------+------------+------------+
|   256 unicode array   |  **0.91**  |    4.60    |    5.09    |    2.18    |
+-----------------------+------------+------------+------------+------------+
|    complex object     |  **0.96**  |    1.48    |    1.27    |    1.26    |
+-----------------------+------------+------------+------------+------------+
|   composite object    |  **0.86**  |    2.03    |    1.45    |    1.26    |
+-----------------------+------------+------------+------------+------------+
|        overall        |  **0.96**  |    1.46    |    1.20    |    1.21    |
+-----------------------+------------+------------+------------+------------+

DIY
~~~

To run these tests yourself, clone the repo and run:

.. code-block::

   $ tox -e py34 -- -m benchmark --compare-other-engines

Without the option ``--compare-other-engines`` it will focus only on
``RapidJSON``.  This is particularly handy coupled with the `compare past
runs`__ functionality of ``pytest-benchmark``:

.. code-block::

   $ tox -e py34 -- -m benchmark --benchmark-autosave
   # hack, hack, hack!
   $ tox -e py34 -- -m benchmark --benchmark-compare=0001

   ----------------------- benchmark 'deserialize': 18 tests ------------------------
   Name (time in us)                                                            Min…
   ----------------------------------------------------------------------------------
   test_loads[rapidjson-256 Trues array] (NOW)                         5.2320 (1.0)…
   test_loads[rapidjson-256 Trues array] (0001)                        5.4180 (1.04)…
   …

To reproduce the tables above, use the option ``--benchmark-json`` so that the
the results are written in the specified filename the run the
``benchmark-tables.py`` script giving that filename as the only argument:

.. code-block::

   $ tox -e py36 -- -m benchmark --compare-other-engines --benchmark-json=comparison.json
   $ python3 benchmark-tables.py comparison.json


__ http://pytest-benchmark.readthedocs.org/en/latest/comparing.html


Incompatibility
---------------

Here are things in the standard ``json`` library supports that we have decided
not to support:

* ``separators`` argument. This is mostly used for pretty printing and not
  supported by ``RapidJSON`` so it isn't a high priority. We do support
  ``indent`` kwarg that would get you nice looking JSON anyways.

* Coercing keys when dumping. ``json`` will turn ``True`` into ``'True'`` if
  you dump it out but when you load it back in it'll still be a string. We
  want the dump and load to return the exact same objects so we have decided
  not to do this coercing.

.. _RapidJSON: https://github.com/miloyip/rapidjson
