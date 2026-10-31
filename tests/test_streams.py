# -*- coding: utf-8 -*-
# :Project:   python-rapidjson -- Streaming API related tests
# :Author:    Lele Gaifax <lele@metapensiero.it>
# :License:   MIT License
# :Copyright: © 2017, 2018, 2020 Lele Gaifax
#

import io
import sys
import tempfile
from decimal import Decimal

import pytest

import rapidjson as rj


@pytest.mark.parametrize('cs', (-1, 0, sys.maxsize*10, 1.23, 'foo'))
def test_invalid_chunk_size(cs):
    s = io.StringIO('"foo"')
    with pytest.raises((ValueError, TypeError)):
        rj.load(s, chunk_size=cs)


@pytest.mark.parametrize('chunk_size', [4, 5, 65536])
@pytest.mark.parametrize('stream_type', [io.StringIO, io.BytesIO])
@pytest.mark.parametrize('use_decoder', [False, True])
def test_null_at_chunk_boundary(chunk_size, stream_type, use_decoder):
    document = '1234\0garbage'
    stream = stream_type(document.encode() if stream_type is io.BytesIO else document)
    with pytest.raises(rj.JSONDecodeError, match='Parse error at offset 4:'):
        if use_decoder:
            rj.Decoder()(stream, chunk_size=chunk_size)
        else:
            rj.load(stream, chunk_size=chunk_size)


class ChunkedStream(io.StringIO):
    def __init__(self):
        super().__init__()
        self.chunks = []

    def write(self, s):
        super().write(s)
        self.chunks.append(s)


def test_chunked_stream():
    stream = ChunkedStream()
    rj.dump('1234567890', stream)
    assert len(stream.chunks) == 1

    stream = ChunkedStream()
    rj.dump('1234567890', stream, chunk_size=4)
    assert len(stream.chunks) == 3
    assert stream.chunks == ['"123', '4567', '890"']

    stream = ChunkedStream()
    rj.dump('~𓆙~', stream, ensure_ascii=False, chunk_size=4)
    assert len(stream.chunks) == 3
    assert stream.chunks == ['"~', '𓆙', '~"']

    stream = ChunkedStream()
    rj.dump('~𓆙~', stream, chunk_size=4)
    assert len(stream.chunks) == 4
    assert stream.chunks == ['"~\\u', 'D80C', '\\uDD', '99~"']


class CattyError(RuntimeError):
    pass


class CattyStream(io.StringIO):
    def read(self, *args, **kwargs):
        raise CattyError('No real reason')

    def write(self, *args, **kwargs):
        raise CattyError('No real reason')


def test_underlying_stream_read_error():
    stream = CattyStream()
    with pytest.raises(CattyError):
        rj.load(stream)


def test_underlying_stream_write_error():
    stream = CattyStream()
    with pytest.raises(CattyError):
        rj.dump('1234567890', stream)


@pytest.mark.parametrize('value', [
    Decimal('NaN'), Decimal('sNaN'), Decimal('-NaN123'),
    Decimal('1.5'), float('nan'),
])
@pytest.mark.parametrize('use_encoder', [False, True])
@pytest.mark.parametrize('nested', [False, True])
def test_nonfinite_closed_stream_error(value, use_encoder, nested):
    stream = io.StringIO()
    stream.close()
    mode = rj.NM_DECIMAL | rj.NM_NAN
    datum = [value] if nested else value
    with pytest.raises(ValueError, match='I/O operation on closed file'):
        if use_encoder:
            rj.Encoder(number_mode=mode)(datum, stream=stream)
        else:
            rj.dump(datum, stream, number_mode=mode)


def test_file_object():
    for stream in tempfile.TemporaryFile(), tempfile.TemporaryFile('w+', encoding='utf-8'):
        with stream:
            datum = ['1234567890', 1234, 3.14, '~𓆙~']
            rj.dump(datum, stream)
            stream.seek(0)
            assert rj.load(stream) == datum
