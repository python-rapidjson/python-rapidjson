import typing as t

JSONType = t.Union[
    str,
    int,
    float,
    bool,
    None,
    t.Dict[str, "JSONType"],
    t.List["JSONType"],
]

# Const types
BM_NONE_TYPE = t.Literal[0]
BM_UTF8_TYPE = t.Literal[1]
DM_IGNORE_TZ_TYPE = t.Literal[32]
DM_ISO8601_TYPE = t.Literal[1]
DM_NAIVE_IS_UTC_TYPE = t.Literal[64]
DM_NONE_TYPE = t.Literal[0]
DM_ONLY_SECONDS_TYPE = t.Literal[16]
DM_SHIFT_TO_UTC_TYPE = t.Literal[128]
DM_UNIX_TIME_TYPE = t.Literal[2]
IM_ANY_ITERABLE_TYPE = t.Literal[0]
IM_ONLY_LISTS_TYPE = t.Literal[1]
MM_ANY_MAPPING_TYPE = t.Literal[0]
MM_COERCE_KEYS_TO_STRINGS_TYPE = t.Literal[2]
MM_ONLY_DICTS_TYPE = t.Literal[1]
MM_SKIP_NON_STRING_KEYS_TYPE = t.Literal[4]
MM_SORT_KEYS_TYPE = t.Literal[8]
NM_DECIMAL_TYPE = t.Literal[2]
NM_NAN_TYPE = t.Literal[1]
NM_NATIVE_TYPE = t.Literal[4]
NM_NONE_TYPE = t.Literal[0]
PM_COMMENTS_TYPE = t.Literal[1]
PM_NONE_TYPE = t.Literal[0]
PM_TRAILING_COMMAS_TYPE = t.Literal[2]
UM_CANONICAL_TYPE = t.Literal[1]
UM_HEX_TYPE = t.Literal[2]
UM_NONE_TYPE = t.Literal[0]
WM_COMPACT_TYPE = t.Literal[0]
WM_PRETTY_TYPE = t.Literal[1]
WM_SINGLE_LINE_ARRAY_TYPE = t.Literal[2]

# Const values
BM_NONE: BM_NONE_TYPE = 0
BM_UTF8: BM_UTF8_TYPE = 1
DM_IGNORE_TZ: DM_IGNORE_TZ_TYPE = 32
DM_ISO8601: DM_ISO8601_TYPE = 1
DM_NAIVE_IS_UTC: DM_NAIVE_IS_UTC_TYPE = 64
DM_NONE: DM_NONE_TYPE = 0
DM_ONLY_SECONDS: DM_ONLY_SECONDS_TYPE = 16
DM_SHIFT_TO_UTC: DM_SHIFT_TO_UTC_TYPE = 128
DM_UNIX_TIME: DM_UNIX_TIME_TYPE = 2
IM_ANY_ITERABLE: IM_ANY_ITERABLE_TYPE = 0
IM_ONLY_LISTS: IM_ONLY_LISTS_TYPE = 1
MM_ANY_MAPPING: MM_ANY_MAPPING_TYPE = 0
MM_COERCE_KEYS_TO_STRINGS: MM_COERCE_KEYS_TO_STRINGS_TYPE = 2
MM_ONLY_DICTS: MM_ONLY_DICTS_TYPE = 1
MM_SKIP_NON_STRING_KEYS: MM_SKIP_NON_STRING_KEYS_TYPE = 4
MM_SORT_KEYS: MM_SORT_KEYS_TYPE = 8
NM_DECIMAL: NM_DECIMAL_TYPE = 2
NM_NAN: NM_NAN_TYPE = 1
NM_NATIVE: NM_NATIVE_TYPE = 4
NM_NONE: NM_NONE_TYPE = 0
PM_COMMENTS: PM_COMMENTS_TYPE = 1
PM_NONE: PM_NONE_TYPE = 0
PM_TRAILING_COMMAS: PM_TRAILING_COMMAS_TYPE = 2
UM_CANONICAL: UM_CANONICAL_TYPE = 1
UM_HEX: UM_HEX_TYPE = 2
UM_NONE: UM_NONE_TYPE = 0
WM_COMPACT: WM_COMPACT_TYPE = 0
WM_PRETTY: WM_PRETTY_TYPE = 1
WM_SINGLE_LINE_ARRAY: WM_SINGLE_LINE_ARRAY_TYPE = 2

NumberMode = t.Literal[NM_DECIMAL_TYPE, NM_NAN_TYPE, NM_NATIVE_TYPE, NM_NONE_TYPE]
DatetimeMode = t.Literal[
    DM_IGNORE_TZ_TYPE,
    DM_ISO8601_TYPE,
    DM_NAIVE_IS_UTC_TYPE,
    DM_NONE_TYPE,
    DM_ONLY_SECONDS_TYPE,
    DM_SHIFT_TO_UTC_TYPE,
    DM_UNIX_TIME_TYPE,
]
UUIDMode = t.Literal[UM_CANONICAL_TYPE, UM_HEX_TYPE, UM_NONE_TYPE]
ParseMode = t.Literal[PM_COMMENTS_TYPE, PM_NONE_TYPE, PM_TRAILING_COMMAS_TYPE]
WriteMode = t.Literal[WM_COMPACT_TYPE, WM_PRETTY_TYPE, WM_SINGLE_LINE_ARRAY_TYPE]
BytesMode = t.Literal[BM_NONE_TYPE, BM_UTF8_TYPE]
IterableMode = t.Literal[IM_ANY_ITERABLE_TYPE, IM_ONLY_LISTS_TYPE]
MappingMode = t.Literal[
    MM_ANY_MAPPING_TYPE,
    MM_COERCE_KEYS_TO_STRINGS_TYPE,
    MM_ONLY_DICTS_TYPE,
    MM_SKIP_NON_STRING_KEYS_TYPE,
    MM_SORT_KEYS_TYPE,
]

# Functions
def dumps(
    obj: t.Any,
    *,
    skipkeys: t.Optional[bool] = False,
    ensure_ascii: t.Optional[bool] = True,
    write_mode: t.Optional[WriteMode] = WM_COMPACT,
    indent: t.Optional[t.Union[int, str]] = 4,
    default: t.Optional[t.Callable[[t.Any], JSONType]] = None,
    sort_keys: t.Optional[bool] = False,
    number_mode: t.Optional[NumberMode] = NM_NAN,
    datetime_mode: t.Optional[DatetimeMode] = DM_NONE,
    uuid_mode: t.Optional[UUIDMode] = UM_NONE,
    bytes_mode: t.Optional[BytesMode] = BM_UTF8,
    iterable_mode: t.Optional[IterableMode] = IM_ANY_ITERABLE,
    mapping_mode: t.Optional[MappingMode] = MM_ANY_MAPPING,
    allow_nan: t.Optional[bool] = True,
) -> str: ...
def dump(
    obj: t.Any,
    stream: t.IO,
    *,
    skipkeys: t.Optional[bool] = False,
    ensure_ascii: t.Optional[bool] = True,
    write_mode: t.Optional[WriteMode] = WM_COMPACT,
    indent: t.Optional[t.Union[int, str]] = 4,
    default: t.Optional[t.Callable[[t.Any], JSONType]] = None,
    sort_keys: t.Optional[bool] = False,
    number_mode: t.Optional[NumberMode] = NM_NAN,
    datetime_mode: t.Optional[DatetimeMode] = DM_NONE,
    uuid_mode: t.Optional[UUIDMode] = UM_NONE,
    bytes_mode: t.Optional[BytesMode] = BM_UTF8,
    iterable_mode: t.Optional[IterableMode] = IM_ANY_ITERABLE,
    mapping_mode: t.Optional[MappingMode] = MM_ANY_MAPPING,
    chunk_size: t.Optional[int] = 65536,
    allow_nan: t.Optional[bool] = True,
) -> None: ...
def load(
    stream: t.IO,
    *,
    object_hook: t.Optional[t.Callable[[t.Dict[str, t.Any]], t.Any]] = None,
    number_mode: t.Optional[NumberMode] = NM_NAN,
    datetime_mode: t.Optional[DatetimeMode] = DM_NONE,
    uuid_mode: t.Optional[UUIDMode] = UM_NONE,
    parse_mode: t.Optional[ParseMode] = PM_NONE,
    chunk_size: t.Optional[int] = 65536,
    allow_nan: t.Optional[bool] = True,
) -> t.Any: ...
def loads(
    string: str,
    *,
    object_hook: t.Optional[t.Callable[[t.Dict[str, t.Any]], t.Any]] = None,
    number_mode: t.Optional[NumberMode] = NM_NAN,
    datetime_mode: t.Optional[DatetimeMode] = DM_NONE,
    uuid_mode: t.Optional[UUIDMode] = UM_NONE,
    parse_mode: t.Optional[ParseMode] = PM_NONE,
    allow_nan: t.Optional[bool] = True,
) -> t.Any: ...

# Classes
class JSONDecodeError(Exception): ...
class ValidationError(Exception): ...

class Decoder:
    datetime_mode: DatetimeMode
    number_mode: NumberMode
    parse_mode: ParseMode
    uuid_mode: UUIDMode

    def __init__(
        self,
        datetime_mode: t.Optional[DatetimeMode] = DM_NONE,
        number_mode: t.Optional[NumberMode] = NM_NAN,
        parse_mode: t.Optional[ParseMode] = PM_NONE,
        uuid_mode: t.Optional[UUIDMode] = UM_NONE,
    ) -> None: ...
    def __call__(
        self,
        json: t.Union[str, bytes, t.IO],
        chunk_size: t.Optional[int] = 65536,
    ) -> t.Any: ...
    def end_array(self, sequence: t.Sequence) -> t.Sequence: ...
    def end_object(self, mapping: t.Mapping) -> t.Mapping: ...
    def start_object(self) -> t.Union[t.Sequence, t.Mapping]: ...
    def string(self, s: str) -> str: ...

class Encoder:
    bytes_mode: BytesMode
    datetime_mode: DatetimeMode
    ensure_ascii: bool
    ident_char: str
    ident_count: int
    iterable_mode: IterableMode
    mapping_mode: MappingMode
    number_mode: NumberMode
    skip_invalid_keys: bool
    sort_keys: bool
    uuid_mode: UUIDMode
    write_mode: WriteMode

    def __init__(
        self,
        skip_invalid_keys: t.Optional[bool] = False,
        ensure_ascii: t.Optional[bool] = True,
        write_mode: t.Optional[WriteMode] = WM_COMPACT,
        indent: t.Optional[t.Union[int, str]] = 4,
        sort_keys: t.Optional[bool] = False,
        number_mode: t.Optional[NumberMode] = NM_NAN,
        datetime_mode: t.Optional[DatetimeMode] = DM_NONE,
        uuid_mode: t.Optional[UUIDMode] = UM_NONE,
        bytes_mode: t.Optional[BytesMode] = BM_UTF8,
        iterable_mode: t.Optional[IterableMode] = IM_ANY_ITERABLE,
        mapping_mode: t.Optional[MappingMode] = MM_ANY_MAPPING,
    ) -> None: ...
    def __call__(
        self,
        obj: t.Any,
        stream: t.Optional[t.IO],
        chunk_size: t.Optional[int] = 65536,
    ) -> t.Optional[str]: ...
    def default(self, obj: t.Any) -> JSONType: ...

class RawJSON:
    def __init__(self, value: str) -> None: ...

class Validator:
    def __init__(self, json_schema: t.Union[str, bytes]) -> None: ...
    def __call__(self, json: t.Union[str, bytes]) -> None: ...
