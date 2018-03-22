.. -*- coding: utf-8 -*-
.. :Project:   python-rapidjson -- RawJSON class documentation
.. :Author:    Silvio Tomatis <silviot@gmail.com>
.. :License:   MIT License
.. :Copyright: © 2018 Silvio Tomatis
.. :Copyright: © 2018 Lele Gaifax
..

===============
 RawJSON class
===============

A possible use case is mixing preserialized objects with regular ones.

For instance, you might want to store preserialized records in your database,
and then be able to use rapidjson to pack them in a serialized list.

The RawJSON class serves this purpose.

It must be instantiated with a bytestring:

      .. doctest::

        >>> import rapidjson
        >>> raw_list = rapidjson.RawJSON('[1, 2,3]')
        >>> rapidjson.dumps({'foo': raw_list})
        '{"foo":[1, 2,3]}'
