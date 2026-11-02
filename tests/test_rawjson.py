# -*- coding: utf-8 -*-
# :Project:   python-rapidjson -- Tests on RawJSON
# :Author:    Silvio Tomatis <silviot@gmail.com>
# :License:   MIT License
# :Copyright: © 2018 Silvio Tomatis
# :Copyright: © 2018, 2020 Lele Gaifax
#

import pytest

import rapidjson as rj


def test_instantiation_positional():
    value = 'a string'
    raw = rj.RawJSON(value)
    assert raw.value == value


def test_instantiation_named():
    value = 'a string'
    raw = rj.RawJSON(value=value)
    assert raw.value == value


def test_only_bytes_allowed():
    with pytest.raises(TypeError):
        rj.RawJSON(b'A bytes string')
    with pytest.raises(TypeError):
        rj.RawJSON({})
    with pytest.raises(TypeError):
        rj.RawJSON(None)


def test_mix_preserialized():
    assert rj.dumps({'a': rj.RawJSON('{1 : 2}')}) == '{"a":{1 : 2}}'


@pytest.mark.parametrize('raw', ['"☃"', '{"key": "é"}', '"😀"'])
@pytest.mark.parametrize('write_mode', [rj.WM_COMPACT, rj.WM_PRETTY])
@pytest.mark.parametrize('ensure_ascii', [True, False])
@pytest.mark.parametrize('api', ['dumps', 'encoder', 'text', 'bytes'])
def test_unicode_rawjson_preserves_value(raw, write_mode, ensure_ascii, api):
    import io
    import json
    value = {'raw': rj.RawJSON(raw), 'normal': '☃'}
    options = dict(write_mode=write_mode, ensure_ascii=ensure_ascii)
    if api == 'dumps':
        encoded = rj.dumps(value, **options)
    elif api == 'encoder':
        encoded = rj.Encoder(**options)(value)
    else:
        stream = io.StringIO() if api == 'text' else io.BytesIO()
        rj.dump(value, stream, **options)
        encoded = stream.getvalue()
        if isinstance(encoded, bytes):
            encoded = encoded.decode('utf-8')
    if ensure_ascii:
        assert encoded.isascii()
    assert json.loads(encoded) == {'raw': json.loads(raw), 'normal': '☃'}


def test_ascii_rawjson_keeps_original_whitespace():
    raw = ' { "x" : [1,  2] } '
    assert rj.dumps(rj.RawJSON(raw)) == raw


def test_unicode_rawjson_stream_error_propagates():
    class BrokenStream:
        def write(self, data):
            raise RuntimeError('controlled write failure')
    with pytest.raises(RuntimeError, match='controlled write failure'):
        rj.dump(rj.RawJSON('"☃"'), BrokenStream())
