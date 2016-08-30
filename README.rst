python-rapidjson
================
Python wrapper around RapidJSON. RapidJSON_ is an extremely fast C++ json
serialization library.

We do not support legacy python versions, you will need to upgrade to Python 3
to use this library.


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


Performance
-----------
``python-rapidjson`` tries to be as performant as possible while staying
compatible with the ``json`` module.  Here are our current benchmarks:

+-----------------------------------------+--------+------------+------------+-----------+
|                                         | ujson  | simplejson | rapidjson  | yajl      |
+=========================================+========+============+============+===========+
|Array with 256 doubles                   |        |            |            |           |
+-----------------------------------------+--------+------------+------------+-----------+
| Encode                                  | 13.81s | 12.58s     | 1.82s      | 8.36s     |
+-----------------------------------------+--------+------------+------------+-----------+
| Decode                                  | 2.00s  | 4.65s      | 2.19s      | 2.09s     |
+-----------------------------------------+--------+------------+------------+-----------+
|                                         |        |            |            |           |
+-----------------------------------------+--------+------------+------------+-----------+
| Array with 256 utf-8 strings            |        |            |            |           |
+-----------------------------------------+--------+------------+------------+-----------+
| Encode                                  | 1.98s  | 2.36s      | 5.09s      | 1.45s     |
+-----------------------------------------+--------+------------+------------+-----------+
| Decode                                  | 2.77s  | 13.58s     | 2.42s      | 7.19s     |
+-----------------------------------------+--------+------------+------------+-----------+
|                                         |        |            |            |           |
+-----------------------------------------+--------+------------+------------+-----------+
|100 dictionaries of 100 arrays           |        |            |            |           |
+-----------------------------------------+--------+------------+------------+-----------+
| Encode                                  | 1.65s  | 5.20s      | 0.77s      | 2.09s     |
+-----------------------------------------+--------+------------+------------+-----------+
| Decode                                  | 2.76s  | 3.86s      | 2.79s      | 3.47      |
+-----------------------------------------+--------+------------+------------+-----------+
|                                         |        |            |            |           |
+-----------------------------------------+--------+------------+------------+-----------+

To run these tests yourself, clone the repo and run:

.. code-block::

   $ tox -e py34 -- -m benchmark --compare-other-engines

Without the option ``--compare-other-engines`` it will focus only on ``RapidJSON``.
This is particularly handy coupled with the `compare past runs`__ functionality of
``pytest-benchmark``:

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

__ http://pytest-benchmark.readthedocs.org/en/latest/comparing.html

Incompatibility
---------------
Here are things in the standard ``json`` library supports that we have decided
not to support:

* ``separators`` argument. This is mostly used for pretty printing and not
  supported by ``RapidJSON`` so it isn't a high priority. We do support
  ``indent`` kwarg that would get you nice looking JSON anyways.

* Coercing keys when dumping. ``json`` will turn ``True`` into ``'True'`` if you
  dump it out but when you load it back in it'll still be a string. We want the
  dump and load to return the exact same objects so we have decided not to do
  this coercing.

.. _RapidJSON: https://github.com/miloyip/rapidjson
