
Serialization
~~~~~~~~~~~~~

+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+
|       serialize       |  ``dumps()``\ [1]_   | ``Encoder()``\ [2]_  |  ``dumps(n)``\ [3]_  | ``Encoder(n)``\ [4]_ |    simdjson\ [5]_    |     orjson\ [6]_     |     ujson\ [7]_      |   simplejson\ [8]_   |     stdlib\ [9]_     |
+=======================+======================+======================+======================+======================+======================+======================+======================+======================+======================+
|    100 arrays dict    |         1.00         |         0.99         |         0.77         |         0.76         |         1.97         |       **0.26**       |         1.09         |         4.00         |         1.95         |
+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+
|    100 dicts array    |         1.00         |         0.99         |         0.81         |         0.76         |         2.06         |       **0.34**       |         1.06         |         4.96         |         2.04         |
+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+
|    256 Trues array    |         1.00         |         1.00         |         1.17         |         1.04         |         2.38         |       **0.38**       |         1.52         |         2.70         |         2.35         |
+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+
|    256 ascii array    |         1.00         |         1.00         |         1.02         |         1.00         |         0.90         |       **0.26**       |         0.50         |         1.06         |         0.89         |
+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+
|   256 doubles array   |         1.00         |         0.99         |         0.99         |         0.99         |         0.84         |       **0.06**       |         0.21         |         0.88         |         0.83         |
+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+
|   256 unicode array   |         1.00         |         0.81         |         0.81         |         0.82         |         0.70         |       **0.09**       |         0.51         |         0.65         |         0.69         |
+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+
|      apache.json      |         1.00         |         1.00         |         1.01         |         1.01         |         1.66         |       **0.33**       |         1.18         |         2.51         |         1.65         |
+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+
|      canada.json      |         1.00         |         1.00         |         0.99         |         1.00         |         1.06         |       **0.09**       |         0.34         |         1.72         |         1.06         |
+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+
|    complex object     |         1.00         |         1.00         |         0.93         |         0.92         |         1.58         |       **0.26**       |         0.83         |         2.41         |         1.57         |
+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+
|   composite object    |         1.00         |         0.99         |         0.75         |         0.72         |         1.91         |       **0.36**       |         1.12         |         2.44         |         1.87         |
+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+
|       ctm.json        |         1.00         |         1.00         |         0.76         |         0.76         |         2.11         |       **0.33**       |         1.43         |         4.81         |         2.13         |
+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+
|      github.json      |         1.00         |         1.00         |         0.98         |         0.97         |         1.34         |       **0.30**       |         0.98         |         1.72         |         1.32         |
+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+
|   instruments.json    |         1.00         |         1.01         |         0.80         |         0.80         |         1.75         |       **0.34**       |         1.15         |         2.28         |         1.75         |
+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+
|       mesh.json       |         1.00         |         0.99         |         0.90         |         0.90         |         0.94         |       **0.14**       |         0.35         |         1.09         |         0.93         |
+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+
|     truenull.json     |         1.00         |         1.02         |         1.03         |         1.01         |         1.87         |       **0.41**       |         1.69         |         1.89         |         1.86         |
+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+
|      tweet.json       |         1.00         |         0.99         |         0.99         |         0.94         |         1.66         |       **0.33**       |         1.02         |         2.33         |         1.65         |
+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+
|     twitter.json      |         1.00         |         1.00         |         0.96         |         0.95         |         1.39         |       **0.32**       |         1.05         |         1.65         |         1.36         |
+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+
|        overall        |         1.00         |         0.99         |         0.83         |         0.82         |         1.71         |       **0.22**       |         0.90         |         3.33         |         1.70         |
+-----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+----------------------+

Deserialization
~~~~~~~~~~~~~~~

+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|      deserialize      |  ``loads()``\ [10]_   | ``Decoder()``\ [11]_  |  ``loads(n)``\ [12]_  | ``Decoder(n)``\ [13]_ |       simdjson        |        orjson         |         ujson         |      simplejson       |        stdlib         |
+=======================+=======================+=======================+=======================+=======================+=======================+=======================+=======================+=======================+=======================+
|    100 arrays dict    |         1.00          |         1.05          |         0.90          |         0.95          |         0.86          |       **0.56**        |         0.86          |         1.07          |         0.97          |
+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|    100 dicts array    |         1.00          |         0.99          |         0.87          |         0.85          |         0.63          |       **0.40**        |         0.68          |         1.27          |         1.02          |
+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|    256 Trues array    |         1.00          |         1.02          |         1.11          |         0.99          |         0.92          |       **0.48**        |         0.83          |         1.51          |         1.36          |
+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|    256 ascii array    |         1.00          |         0.99          |         1.02          |         1.01          |         0.46          |       **0.45**        |         0.82          |         0.84          |         0.81          |
+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|   256 doubles array   |         1.00          |         1.00          |         0.27          |         0.26          |         0.21          |       **0.16**        |         0.54          |         1.09          |         1.07          |
+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|   256 unicode array   |         1.00          |         1.00          |         1.00          |         1.00          |         1.10          |       **0.39**        |         0.80          |         4.49          |         1.29          |
+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|      apache.json      |         1.00          |         1.00          |         1.01          |         1.01          |         0.65          |       **0.48**        |         0.85          |         0.81          |         0.83          |
+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|      canada.json      |         1.00          |         1.00          |         0.34          |         0.34          |         0.29          |       **0.25**        |         0.49          |         1.04          |         0.96          |
+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|    complex object     |         1.00          |         1.00          |         0.84          |         0.84          |         0.65          |       **0.37**        |         0.80          |         1.15          |         1.02          |
+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|   composite object    |         1.00          |         0.99          |         0.83          |         0.81          |         0.61          |       **0.45**        |         0.59          |         1.37          |         1.07          |
+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|       ctm.json        |         1.00          |         1.01          |         0.92          |         0.92          |         0.74          |       **0.55**        |         1.08          |         1.20          |         1.14          |
+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|      github.json      |         1.00          |         1.00          |         0.98          |         0.98          |         0.62          |       **0.41**        |         0.76          |         0.86          |         0.79          |
+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|   instruments.json    |         1.00          |         1.00          |         0.91          |         0.90          |         0.63          |       **0.37**        |         0.72          |         1.12          |         0.92          |
+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|       mesh.json       |         1.00          |         1.01          |         0.51          |         0.51          |         0.44          |       **0.36**        |         0.63          |         1.44          |         1.03          |
+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|     truenull.json     |         1.00          |         0.99          |         1.00          |         1.00          |         0.58          |       **0.39**        |         0.84          |         0.98          |         0.87          |
+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|      tweet.json       |         1.00          |         1.02          |         0.99          |         0.98          |         0.66          |       **0.44**        |         0.84          |         1.14          |         1.03          |
+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|     twitter.json      |         1.00          |         1.00          |         0.99          |         0.99          |         0.65          |       **0.42**        |         0.86          |         1.01          |         1.00          |
+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|        overall        |         1.00          |         1.05          |         0.85          |         0.90          |         0.80          |       **0.53**        |         0.83          |         1.08          |         0.97          |
+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+

ASCII vs UTF-8 Serialization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

+-------------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|        serialize        |  ``rj ascii``\ [15]_  |  ``rj utf8``\ [16]_   |  ``uj ascii``\ [17]_  |  ``uj utf8``\ [18]_   |  ``sj ascii``\ [19]_  |  ``sj utf8``\ [20]_   | ``json ascii``\ [21]_ | ``json utf8``\ [22]_  |
+=========================+=======================+=======================+=======================+=======================+=======================+=======================+=======================+=======================+
|    Long ASCII string    |         1.00          |         0.46          |       **0.25**        |         0.41          |         0.63          |         0.93          |         0.57          |         1.49          |
+-------------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|   Long Unicode string   |         1.00          |         0.62          |         0.63          |       **0.58**        |         0.71          |         0.70          |         0.80          |         0.66          |
+-------------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+
|         overall         |         1.00          |         0.57          |       **0.51**        |         0.53          |         0.69          |         0.78          |         0.73          |         0.92          |
+-------------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+-----------------------+

.. [1] ``rapidjson.dumps()``
.. [2] ``rapidjson.Encoder()``
.. [3] ``rapidjson.dumps(number_mode=NM_NATIVE)``
.. [4] ``rapidjson.Encoder(number_mode=NM_NATIVE)``
.. [5] `simdjson 5.0.1 <https://pypi.org/project/pysimdjson/5.0.1/>`__
.. [6] `orjson 3.7.6 <https://pypi.org/project/orjson/3.7.6/>`__
.. [7] `ujson 5.4.0 <https://pypi.org/project/ujson/5.4.0/>`__
.. [8] `simplejson 3.17.6 <https://pypi.org/pypi/simplejson/3.17.6>`__
.. [9] Python 3.10.5 standard library ``json``
.. [10] ``rapidjson.loads()``
.. [11] ``rapidjson.Decoder()``
.. [12] ``rapidjson.loads(number_mode=NM_NATIVE)``
.. [13] ``rapidjson.Decoder(number_mode=NM_NATIVE)``
.. [15] ``rapidjson.dumps(ensure_ascii=True)``
.. [16] ``rapidjson.dumps(ensure_ascii=False)``
.. [17] ``ujson.dumps(ensure_ascii=True)``
.. [18] ``ujson.dumps(ensure_ascii=False)``
.. [19] ``simplejson.dumps(ensure_ascii=True)``
.. [20] ``simplejson.dumps(ensure_ascii=False)``
.. [21] ``stdlib json.dumps(ensure_ascii=True)``
.. [22] ``stdlib json.dumps(ensure_ascii=False)``
