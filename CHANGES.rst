Changes
-------

0.2.4 (2017-09-17)
~~~~~~~~~~~~~~~~~~

* Fix compatibility with MacOS/clang


0.2.3 (2017-08-24)
~~~~~~~~~~~~~~~~~~

* Limit the precision of DM_UNIX_TIME timestamps to six decimal digits


0.2.2 (2017-08-24)
~~~~~~~~~~~~~~~~~~

* Nothing new, attempt to fix production of Python 3.6 binary wheels


0.2.1 (2017-08-24)
~~~~~~~~~~~~~~~~~~

* Nothing new, attempt to fix production of Python 3.6 binary wheels


0.2.0 (2017-08-24)
~~~~~~~~~~~~~~~~~~

* New ``parse_mode`` option, implementing relaxed JSON syntax (`issue #73`__)

  __ https://github.com/python-rapidjson/python-rapidjson/issues/73

* New ``Encoder`` and ``Decoder``, implementing a class-based interface

* New ``Validator``, exposing the underlying *JSON schema* validation (`issue #71`__)

  __ https://github.com/python-rapidjson/python-rapidjson/issues/71


0.1.0 (2017-08-16)
~~~~~~~~~~~~~~~~~~

* Remove beta status


0.1.0b4 (2017-08-14)
~~~~~~~~~~~~~~~~~~~~

* Make execution of the test suite on Appveyor actually happen


0.1.0b3 (2017-08-12)
~~~~~~~~~~~~~~~~~~~~

* Exclude CI configurations from the source distribution


0.1.0b2 (2017-08-12)
~~~~~~~~~~~~~~~~~~~~

* Fix Powershell wheel upload script in appveyor configuration


0.1.0b1 (2017-08-12)
~~~~~~~~~~~~~~~~~~~~

* Compilable with somewhat old g++ (`issue #69`__)

  __ https://github.com/python-rapidjson/python-rapidjson/issues/69

* **Backward incompatibilities**:

  - all ``DATETIME_MODE_XXX`` constants have been shortened to ``DM_XXX``
    ``DATETIME_MODE_ISO8601_UTC`` has been renamed to ``DM_SHIFT_TO_UTC``

  - all ``UUID_MODE_XXX`` constants have been shortened to ``UM_XXX``

* New option ``DM_UNIX_TIME`` to serialize date, datetime and time values as
  `UNIX timestamps`__ targeting `issue #61`__

  __ https://en.wikipedia.org/wiki/Unix_time
  __ https://github.com/python-rapidjson/python-rapidjson/issues/61

* New option ``DM_NAIVE_IS_UTC`` to treat naïve datetime and time values as if
  they were in the UTC timezone (also for issue #61)

* New keyword argument ``number_mode`` to use underlying C library numbers

* Binary wheels for GNU/Linux and Windows on PyPI (one would hope: this is the
  reason for the beta1 release)


0.0.11 (2017-03-05)
~~~~~~~~~~~~~~~~~~~

* Fix a couple of refcount handling glitches, hopefully targeting `issue
  #48`__.

  __ https://github.com/python-rapidjson/python-rapidjson/issues/48


0.0.10 (2017-03-02)
~~~~~~~~~~~~~~~~~~~

* Fix source distribution to contain all required stuff (`PR #64`__)

  __ https://github.com/python-rapidjson/python-rapidjson/pull/64


0.0.9 (2017-03-02)
~~~~~~~~~~~~~~~~~~

* CI testing on GitHub

* Allow using locally installed RapidJSON library (`issue #60`__)

  __ https://github.com/python-rapidjson/python-rapidjson/issues/60

* Bug fixes (`issue #37`__, `issue #51`__, `issue #57`__)

  __ https://github.com/python-rapidjson/python-rapidjson/issues/37
  __ https://github.com/python-rapidjson/python-rapidjson/issues/51
  __ https://github.com/python-rapidjson/python-rapidjson/issues/57


0.0.8 (2016-12-09)
~~~~~~~~~~~~~~~~~~

* Use unpatched RapidJSON 1.1 (`PR #46`__)

  __ https://github.com/python-rapidjson/python-rapidjson/pull/46

* Handle serialization and deserialization of datetime, date and time
  instances (`PR #35`__) and of UUID instances (`PR #40`__)

  __ https://github.com/python-rapidjson/python-rapidjson/pull/35
  __ https://github.com/python-rapidjson/python-rapidjson/pull/40

* Sphinx based documentation (`PR #44`__)

  __ https://github.com/python-rapidjson/python-rapidjson/pull/44

* Refresh benchmarks (`PR #45`__)

  __ https://github.com/python-rapidjson/python-rapidjson/pull/45

* Bug fixes (`issue #25`__, `issue #38`__, `PR #43`__)

  __ https://github.com/python-rapidjson/python-rapidjson/issues/25
  __ https://github.com/python-rapidjson/python-rapidjson/issues/38
  __ https://github.com/python-rapidjson/python-rapidjson/pull/43
