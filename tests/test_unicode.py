# -*- coding: utf-8 -*-
# :Project:   python-rapidjson -- Unicode tests
# :Author:    John Anderson <sontek@gmail.com>
# :License:   MIT License
# :Copyright: © 2015 John Anderson
# :Copyright: © 2016, 2017 Lele Gaifax
#

import io
import json

import pytest

import rapidjson


@pytest.mark.unit
@pytest.mark.parametrize('u', [
    '\N{GREEK SMALL LETTER ALPHA}\N{GREEK CAPITAL LETTER OMEGA}',
    '\U0010ffff',
    'asdf \U0010ffff \U0001ffff qwert \uffff \u10ff \u00ff \u0080 \u7fff \b\n\r',
])
def test_unicode(u, dumps, loads):
    s = u.encode('utf-8')
    ju = dumps(u)
    js = dumps(s)
    assert ju == js
    assert ju.lower() == json.dumps(u).lower()
    assert dumps(u, ensure_ascii=False) == json.dumps(u, ensure_ascii=False)


@pytest.mark.unit
@pytest.mark.parametrize('o', [
    "\ud80d",
    {"foo": "\ud80d"},
    {"\ud80d": "foo"},
])
def test_dump_surrogate(o, dumps):
    with pytest.raises(UnicodeEncodeError, match="surrogates not allowed"):
        dumps(o)


@pytest.mark.unit
@pytest.mark.parametrize('j', [
    '"\\ud80d"',
    '{"foo": "\\ud80d"}',
    '{"\\ud80d": "foo"}',
])
def test_load_surrogate(j, loads):
    with pytest.raises(ValueError, match="surrogate pair in string is invalid"):
        loads(j)


@pytest.mark.unit
@pytest.mark.parametrize('j', [
    '"\\udc00"',
    '"\\udfff"',
])
def test_unicode_decode_error(j, loads):
    with pytest.raises(UnicodeDecodeError, match="'utf-8' codec can't decode byte"):
        loads(j)


class ChunkedStream(io.StringIO):
    def __init__(self):
        super().__init__()
        self.chunks = []

    def write(self, s):
        super().write(s)
        self.chunks.append(s)


@pytest.mark.unit
def test_chunked_stream():
    stream = ChunkedStream()
    rapidjson.dump('1234567890', stream, ensure_ascii=False)
    assert len(stream.chunks) == 1

    stream = ChunkedStream()
    rapidjson.dump('1234567890', stream, ensure_ascii=False, chunk_size=4)
    assert len(stream.chunks) == 3
    assert stream.chunks == ['"123', '4567', '890"']

    stream = ChunkedStream()
    rapidjson.dump('~𓆙~', stream, ensure_ascii=False, chunk_size=4)
    assert len(stream.chunks) == 3
    assert stream.chunks == ['"~', '𓆙', '~"']
