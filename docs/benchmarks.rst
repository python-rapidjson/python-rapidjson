=============
 Performance
=============

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
-------------

+-----------------------+-----------------+-----------------+-----------------+-----------------+-----------------+
|       serialize       |   native [1]_   |   ujson [2]_    | simplejson [3]_ |   stdlib [4]_   |    yajl [5]_    |
+=======================+=================+=================+=================+=================+=================+
|    100 arrays dict    |    **0.67**     |      1.31       |      6.28       |      2.88       |      1.74       |
+-----------------------+-----------------+-----------------+-----------------+-----------------+-----------------+
|    100 dicts array    |    **0.79**     |      1.19       |      7.16       |      2.92       |      1.69       |
+-----------------------+-----------------+-----------------+-----------------+-----------------+-----------------+
|  **256 Trues array**  |      1.19       |      1.41       |      3.02       |      2.19       |      1.20       |
+-----------------------+-----------------+-----------------+-----------------+-----------------+-----------------+
|    256 ascii array    |      1.02       |    **0.92**     |      1.90       |      1.77       |      2.05       |
+-----------------------+-----------------+-----------------+-----------------+-----------------+-----------------+
| **256 doubles array** |      1.06       |      7.55       |      8.30       |      7.65       |      4.39       |
+-----------------------+-----------------+-----------------+-----------------+-----------------+-----------------+
|   256 unicode array   |      0.87       |      0.72       |      0.82       |      0.88       |    **0.53**     |
+-----------------------+-----------------+-----------------+-----------------+-----------------+-----------------+
|    complex object     |    **0.82**     |      1.41       |      5.17       |      3.39       |      2.87       |
+-----------------------+-----------------+-----------------+-----------------+-----------------+-----------------+
|   composite object    |    **0.68**     |      0.93       |      3.01       |      1.92       |      1.85       |
+-----------------------+-----------------+-----------------+-----------------+-----------------+-----------------+
|        overall        |    **0.67**     |      1.30       |      6.27       |      2.88       |      1.74       |
+-----------------------+-----------------+-----------------+-----------------+-----------------+-----------------+

.. [1] rapidjson with ``number_mode=NM_NATIVE``
.. [2] `ujson 1.35 <https://pypi.python.org/pypi/ujson/1.35>`__
.. [3] `simplejson 3.11.1 <https://pypi.python.org/pypi/simplejson/3.11.1>`__
.. [4] Python 3.6 standard library
.. [5] `yajl 0.3.5 <https://pypi.python.org/pypi/yajl/0.3.5>`__


Deserialization
---------------

+-----------------------+------------+------------+------------+------------+------------+
|      deserialize      |   native   |   ujson    | simplejson |   stdlib   |    yajl    |
+=======================+============+============+============+============+============+
|    100 arrays dict    |  **0.90**  |    0.97    |    1.48    |    1.25    |    1.20    |
+-----------------------+------------+------------+------------+------------+------------+
|    100 dicts array    |  **0.88**  |    0.96    |    1.99    |    1.58    |    1.34    |
+-----------------------+------------+------------+------------+------------+------------+
|  **256 Trues array**  |    1.22    |    1.31    |    2.08    |    1.93    |    2.08    |
+-----------------------+------------+------------+------------+------------+------------+
|  **256 ascii array**  |    1.05    |    1.37    |    1.14    |    1.25    |    1.56    |
+-----------------------+------------+------------+------------+------------+------------+
|   256 doubles array   |  **0.16**  |    0.33    |    0.72    |    0.70    |    0.47    |
+-----------------------+------------+------------+------------+------------+------------+
|   256 unicode array   |    0.89    |  **0.79**  |    4.12    |    4.50    |    1.90    |
+-----------------------+------------+------------+------------+------------+------------+
|    complex object     |  **0.72**  |    0.88    |    1.36    |    1.28    |    1.24    |
+-----------------------+------------+------------+------------+------------+------------+
|   composite object    |  **0.83**  |    0.85    |    1.94    |    1.43    |    1.26    |
+-----------------------+------------+------------+------------+------------+------------+
|        overall        |  **0.90**  |    0.97    |    1.49    |    1.25    |    1.20    |
+-----------------------+------------+------------+------------+------------+------------+


DIY
---

To run these tests yourself, clone the repo and run:

.. code-block:: bash

   $ tox -e py36 -- -m benchmark --compare-other-engines

Without the option ``--compare-other-engines`` it will focus only on
``RapidJSON``.  This is particularly handy coupled with the `compare past
runs`__ functionality of ``pytest-benchmark``:

.. code-block:: bash

   $ tox -e py36 -- -m benchmark --benchmark-autosave
   # hack, hack, hack!
   $ tox -e py36 -- -m benchmark --benchmark-compare=0001

   ----------------------- benchmark 'deserialize': 18 tests ------------------------
   Name (time in us)                                                            Min…
   ----------------------------------------------------------------------------------
   test_loads[rapidjson-256 Trues array] (NOW)                         5.2320 (1.0)…
   test_loads[rapidjson-256 Trues array] (0001)                        5.4180 (1.04)…
   …

To reproduce the tables above, use the option ``--benchmark-json`` so that the
the results are written in the specified filename the run the
``benchmark-tables.py`` script giving that filename as the only argument:

.. code-block:: bash

   $ tox -e py36 -- -m benchmark --compare-other-engines --benchmark-json=comparison.json
   $ python3 benchmark-tables.py comparison.json


__ http://pytest-benchmark.readthedocs.org/en/latest/comparing.html
