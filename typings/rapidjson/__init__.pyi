# -*- coding: utf-8 -*-
# :Project:   python-rapidjson -- PEP-484 typing stubs
# :Author:    Rodion Kosianenko <GoodWasHere@gmail.com>
# :License:   MIT License
# :Copyright: © 2024 Rodion Kosianenko
# :Copyright: © 2024 Lele Gaifax
#

import typing as t


__rapidjson_exact_version__: str
__rapidjson_version__: str


_JSONType = t.Union[
    str,
    int,
    float,
    bool,
    None,
    t.Dict[str, "_JSONType"],
    t.List["_JSONType"],
]


# Const types
_BM_NONE_TYPE = t.Literal[0]
_BM_UTF8_TYPE = t.Literal[1]
_DM_IGNORE_TZ_TYPE = t.Literal[32]
_DM_ISO8601_TYPE = t.Literal[1]
_DM_NAIVE_IS_UTC_TYPE = t.Literal[64]
_DM_NONE_TYPE = t.Literal[0]
_DM_ONLY_SECONDS_TYPE = t.Literal[16]
_DM_SHIFT_TO_UTC_TYPE = t.Literal[128]
_DM_UNIX_TIME_TYPE = t.Literal[2]
_IM_ANY_ITERABLE_TYPE = t.Literal[0]
_IM_ONLY_LISTS_TYPE = t.Literal[1]
_MM_ANY_MAPPING_TYPE = t.Literal[0]
_MM_COERCE_KEYS_TO_STRINGS_TYPE = t.Literal[2]
_MM_ONLY_DICTS_TYPE = t.Literal[1]
_MM_SKIP_NON_STRING_KEYS_TYPE = t.Literal[4]
_MM_SORT_KEYS_TYPE = t.Literal[8]
_NM_DECIMAL_TYPE = t.Literal[2]
_NM_NAN_TYPE = t.Literal[1]
_NM_NATIVE_TYPE = t.Literal[4]
_NM_NONE_TYPE = t.Literal[0]
_PM_COMMENTS_TYPE = t.Literal[1]
_PM_NONE_TYPE = t.Literal[0]
_PM_TRAILING_COMMAS_TYPE = t.Literal[2]
_UM_CANONICAL_TYPE = t.Literal[1]
_UM_HEX_TYPE = t.Literal[2]
_UM_NONE_TYPE = t.Literal[0]
_WM_COMPACT_TYPE = t.Literal[0]
_WM_PRETTY_TYPE = t.Literal[1]
_WM_SINGLE_LINE_ARRAY_TYPE = t.Literal[2]


# Const values
BM_NONE: _BM_NONE_TYPE = 0
BM_UTF8: _BM_UTF8_TYPE = 1
DM_IGNORE_TZ: _DM_IGNORE_TZ_TYPE = 32
DM_ISO8601: _DM_ISO8601_TYPE = 1
DM_NAIVE_IS_UTC: _DM_NAIVE_IS_UTC_TYPE = 64
DM_NONE: _DM_NONE_TYPE = 0
DM_ONLY_SECONDS: _DM_ONLY_SECONDS_TYPE = 16
DM_SHIFT_TO_UTC: _DM_SHIFT_TO_UTC_TYPE = 128
DM_UNIX_TIME: _DM_UNIX_TIME_TYPE = 2
IM_ANY_ITERABLE: _IM_ANY_ITERABLE_TYPE = 0
IM_ONLY_LISTS: _IM_ONLY_LISTS_TYPE = 1
MM_ANY_MAPPING: _MM_ANY_MAPPING_TYPE = 0
MM_COERCE_KEYS_TO_STRINGS: _MM_COERCE_KEYS_TO_STRINGS_TYPE = 2
MM_ONLY_DICTS: _MM_ONLY_DICTS_TYPE = 1
MM_SKIP_NON_STRING_KEYS: _MM_SKIP_NON_STRING_KEYS_TYPE = 4
MM_SORT_KEYS: _MM_SORT_KEYS_TYPE = 8
NM_DECIMAL: _NM_DECIMAL_TYPE = 2
NM_NAN: _NM_NAN_TYPE = 1
NM_NATIVE: _NM_NATIVE_TYPE = 4
NM_NONE: _NM_NONE_TYPE = 0
PM_COMMENTS: _PM_COMMENTS_TYPE = 1
PM_NONE: _PM_NONE_TYPE = 0
PM_TRAILING_COMMAS: _PM_TRAILING_COMMAS_TYPE = 2
UM_CANONICAL: _UM_CANONICAL_TYPE = 1
UM_HEX: _UM_HEX_TYPE = 2
UM_NONE: _UM_NONE_TYPE = 0
WM_COMPACT: _WM_COMPACT_TYPE = 0
WM_PRETTY: _WM_PRETTY_TYPE = 1
WM_SINGLE_LINE_ARRAY: _WM_SINGLE_LINE_ARRAY_TYPE = 2


# Mode types
_NumberMode = int
_DatetimeMode = int
_UUIDMode = t.Literal[_UM_CANONICAL_TYPE, _UM_HEX_TYPE, _UM_NONE_TYPE]
_ParseMode = int
_WriteMode = t.Literal[_WM_COMPACT_TYPE, _WM_PRETTY_TYPE, _WM_SINGLE_LINE_ARRAY_TYPE]
_BytesMode = t.Literal[_BM_NONE_TYPE, _BM_UTF8_TYPE]
_IterableMode = t.Literal[_IM_ANY_ITERABLE_TYPE, _IM_ONLY_LISTS_TYPE]
_MappingMode = int


# Functions
def dumps(
    obj: t.Any,
    *,
    skipkeys: t.Optional[bool] = False,
    ensure_ascii: t.Optional[bool] = True,
    write_mode: t.Optional[_WriteMode] = WM_COMPACT,
    indent: t.Optional[t.Union[int, str]] = 4,
    default: t.Optional[t.Callable[[t.Any], _JSONType]] = None,
    sort_keys: t.Optional[bool] = False,
    number_mode: t.Optional[_NumberMode] = NM_NAN,
    datetime_mode: t.Optional[_DatetimeMode] = DM_NONE,
    uuid_mode: t.Optional[_UUIDMode] = UM_NONE,
    bytes_mode: t.Optional[_BytesMode] = BM_UTF8,
    iterable_mode: t.Optional[_IterableMode] = IM_ANY_ITERABLE,
    mapping_mode: t.Optional[_MappingMode] = MM_ANY_MAPPING,
    allow_nan: t.Optional[bool] = True,
) -> str: ...
def dump(
    obj: t.Any,
    stream: t.IO,
    *,
    skipkeys: t.Optional[bool] = False,
    ensure_ascii: t.Optional[bool] = True,
    write_mode: t.Optional[_WriteMode] = WM_COMPACT,
    indent: t.Optional[t.Union[int, str]] = 4,
    default: t.Optional[t.Callable[[t.Any], _JSONType]] = None,
    sort_keys: t.Optional[bool] = False,
    number_mode: t.Optional[_NumberMode] = NM_NAN,
    datetime_mode: t.Optional[_DatetimeMode] = DM_NONE,
    uuid_mode: t.Optional[_UUIDMode] = UM_NONE,
    bytes_mode: t.Optional[_BytesMode] = BM_UTF8,
    iterable_mode: t.Optional[_IterableMode] = IM_ANY_ITERABLE,
    mapping_mode: t.Optional[_MappingMode] = MM_ANY_MAPPING,
    chunk_size: t.Optional[int] = 65536,
    allow_nan: t.Optional[bool] = True,
) -> None: ...
def load(
    stream: t.IO,
    *,
    object_hook: t.Optional[t.Callable[[t.Dict[str, t.Any]], t.Any]] = None,
    number_mode: t.Optional[_NumberMode] = NM_NAN,
    datetime_mode: t.Optional[_DatetimeMode] = DM_NONE,
    uuid_mode: t.Optional[_UUIDMode] = UM_NONE,
    parse_mode: t.Optional[_ParseMode] = PM_NONE,
    chunk_size: t.Optional[int] = 65536,
    allow_nan: t.Optional[bool] = True,
) -> t.Any: ...
def loads(
    string: t.Union[str, bytes, bytearray],
    *,
    object_hook: t.Optional[t.Callable[[t.Dict[str, t.Any]], t.Any]] = None,
    number_mode: t.Optional[_NumberMode] = NM_NAN,
    datetime_mode: t.Optional[_DatetimeMode] = DM_NONE,
    uuid_mode: t.Optional[_UUIDMode] = UM_NONE,
    parse_mode: t.Optional[_ParseMode] = PM_NONE,
    allow_nan: t.Optional[bool] = True,
) -> t.Any: ...


# Classes
class JSONDecodeError(Exception): ...
class ValidationError(Exception): ...


class Decoder:
    datetime_mode: _DatetimeMode
    number_mode: _NumberMode
    parse_mode: _ParseMode
    uuid_mode: _UUIDMode

    def __init__(
        self,
        datetime_mode: t.Optional[_DatetimeMode] = DM_NONE,
        number_mode: t.Optional[_NumberMode] = NM_NAN,
        parse_mode: t.Optional[_ParseMode] = PM_NONE,
        uuid_mode: t.Optional[_UUIDMode] = UM_NONE,
    ) -> None: ...
    def __call__(
        self,
        json: t.Union[str, bytes, bytearray, t.IO],
        chunk_size: t.Optional[int] = 65536,
    ) -> t.Any: ...


class Encoder:
    bytes_mode: _BytesMode
    datetime_mode: _DatetimeMode
    ensure_ascii: bool
    indent_char: str
    indent_count: int
    iterable_mode: _IterableMode
    mapping_mode: _MappingMode
    number_mode: _NumberMode
    skip_invalid_keys: bool
    sort_keys: bool
    uuid_mode: _UUIDMode
    write_mode: _WriteMode

    def __init__(
        self,
        skip_invalid_keys: t.Optional[bool] = False,
        ensure_ascii: t.Optional[bool] = True,
        write_mode: t.Optional[_WriteMode] = WM_COMPACT,
        indent: t.Optional[t.Union[int, str]] = 4,
        sort_keys: t.Optional[bool] = False,
        number_mode: t.Optional[_NumberMode] = NM_NAN,
        datetime_mode: t.Optional[_DatetimeMode] = DM_NONE,
        uuid_mode: t.Optional[_UUIDMode] = UM_NONE,
        bytes_mode: t.Optional[_BytesMode] = BM_UTF8,
        iterable_mode: t.Optional[_IterableMode] = IM_ANY_ITERABLE,
        mapping_mode: t.Optional[_MappingMode] = MM_ANY_MAPPING,
    ) -> None: ...
    def __call__(
        self,
        obj: t.Any,
        stream: t.Optional[t.IO] = None,
        chunk_size: t.Optional[int] = 65536,
    ) -> t.Optional[str]: ...


@t.final
class RawJSON:
    value: RawJSON
    def __init__(self, value: str) -> None: ...


@t.final
class Validator:
    def __init__(self, json_schema: t.Union[str, bytes, bytearray]) -> None: ...
    def __call__(self, json: t.Union[str, bytes, bytearray]) -> None: ...
