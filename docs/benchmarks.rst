.. -*- coding: utf-8 -*-
.. :Project:   python-rapidjson -- Benchmark tables
.. :Author:    Lele Gaifax <lele@metapensiero.it>
.. :License:   MIT License
.. :Copyright: © 2016, 2017, 2018, 2020 Lele Gaifax
..

=============
 Performance
=============

``python-rapidjson`` tries to be as performant as possible while staying compatible with
the ``json`` module.


Tables
------

The following tables show a comparison between this module and other libraries with
different data sets. Last row (“overall”) is the total time taken by all the benchmarks.

Each number shows the factor between the time taken by each contender and
``python-rapidjson`` (in other words, they are *normalized* against a value of 1.0 for
``python-rapidjson``): the lower the number, the speedier the contender.

In **bold** the winner.

.. include:: benchmarks-tables.rst


DIY
---

To run these tests yourself, clone the repo and run:

.. code-block:: bash

   $ make benchmarks

to focus only on ``RapidJSON`` or:

.. code-block:: bash

   $ make benchmarks-others

to get full comparison against other engines.

To reproduce the tables above, run:

.. code-block:: bash

   $ make benchmarks-tables


Compare different versions
~~~~~~~~~~~~~~~~~~~~~~~~~~

`pytest-benchmark`__ implements an handy feature that allows you to `weight the impact`__
of a particular change. For example, you may start from a released version and execute:

.. code-block:: bash

   $ make benchmarks PYTEST_OPTIONS=--benchmark-autosave

After applying whatever change to the code base, you can get a differential view by
executing:

.. code-block:: bash

   $ make benchmarks PYTEST_OPTIONS=--benchmark-compare=0001

__ https://pypi.org/project/pytest-benchmark/
__ https://pytest-benchmark.readthedocs.org/en/latest/comparing.html
