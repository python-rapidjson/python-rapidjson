#include <Python.h>
#include <datetime.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#include "rapidjson/reader.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/error/en.h"

#include "docstrings.h"
#include "version.h"


using namespace rapidjson;


static PyObject* decimal_type = NULL;
static PyObject* timezone_type = NULL;
static PyObject* timezone_utc = NULL;
static PyObject* uuid_type = NULL;


/* These are the names of oftenly used methods or literal values, interned in the
   module initialization function, to avoid repeated creation/destruction of
   PyUnicode values from plain C strings.

   We cannot use _Py_IDENTIFIER() because that upsets the GNU C++ compiler in
   -pedantic mode. */

static PyObject* astimezone_name = NULL;
static PyObject* hex_name = NULL;
static PyObject* timestamp_name = NULL;
static PyObject* total_seconds_name = NULL;
static PyObject* utcoffset_name = NULL;
static PyObject* is_infinite_name = NULL;
static PyObject* is_nan_name = NULL;

static PyObject* minus_inf_string_value = NULL;
static PyObject* nan_string_value = NULL;
static PyObject* plus_inf_string_value = NULL;


struct HandlerContext {
    PyObject* object;
    const char* key;
    SizeType keyLength;
    bool isObject;
};


enum DatetimeMode {
    DM_NONE = 0,
    // Formats
    DM_ISO8601 = 1,        // Bidirectional ISO8601 for datetimes, dates and times
    DM_UNIX_TIME = 2,      // Serialization only, "Unix epoch"-based number of seconds
    // Options
    DM_ONLY_SECONDS = 16,  // Truncate values to the whole second, ignoring micro seconds
    DM_IGNORE_TZ = 32,     // Ignore timezones
    DM_NAIVE_IS_UTC = 64,  // Assume naive datetime are in UTC timezone
    DM_SHIFT_TO_UTC = 128, // Shift to/from UTC
};


#define DATETIME_MODE_FORMATS_MASK 0x0f


static inline int
datetime_mode_format(DatetimeMode mode) {
    return mode & DATETIME_MODE_FORMATS_MASK;
}


static inline bool
valid_datetime_mode(int mode) {
    return (mode >= 0
            && ((mode & DATETIME_MODE_FORMATS_MASK) <= DM_UNIX_TIME)
            && (mode == 0 || (mode & DATETIME_MODE_FORMATS_MASK) != 0));
}


static int
days_per_month(int year, int month) {
    assert(month >= 1);
    assert(month <= 12);
    if (month == 1 || month == 3 || month == 5 || month == 7
        || month == 8 || month == 10 || month == 12)
        return 31;
    else if (month == 4 || month == 6 || month == 9 || month == 11)
        return 30;
    else if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
        return 29;
    else
        return 28;
}


enum UuidMode {
    UM_NONE = 0,
    UM_CANONICAL = 1, // only 4-dashed 32 hex chars: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
    UM_HEX = 2        // canonical OR 32 hex chars
};


enum NumberMode {
    NM_NONE = 0,
    NM_NAN = 1,       // allow "not-a-number" values
    NM_DECIMAL = 2,   // serialize Decimal instances, deserialize floats as Decimal
    NM_NATIVE = 4     // use faster native C library number handling
};


struct PyHandler {
    PyObject* root;
    PyObject* objectHook;
    DatetimeMode datetimeMode;
    UuidMode uuidMode;
    NumberMode numberMode;
    std::vector<HandlerContext> stack;

    PyHandler(PyObject* hook, DatetimeMode dm, UuidMode um, NumberMode nm)
    : root(NULL), objectHook(hook), datetimeMode(dm), uuidMode(um), numberMode(nm)
    {
        stack.reserve(128);
    }

    bool Handle(PyObject* value) {
        if (root) {
            const HandlerContext& current = stack.back();

            if (current.isObject) {
                PyObject* key = PyUnicode_FromStringAndSize(current.key,
                                                            current.keyLength);
                if (key == NULL) {
                    Py_DECREF(value);
                    return false;
                }

                int rc = PyDict_SetItem(current.object, key, value);
                Py_DECREF(key);
                Py_DECREF(value);

                if (rc == -1) {
                    return false;
                }
            }
            else {
                PyList_Append(current.object, value);
                Py_DECREF(value);
            }
        }
        else {
            root = value;
        }
        return true;
    }

    bool HandleSimpleType(PyObject* value) {
        if (!Handle(value)) {
            return false;
        }

        return true;
    }

    bool Key(const char* str, SizeType length, bool copy) {
        HandlerContext& current = stack.back();
        current.key = str;
        current.keyLength = length;

        return true;
    }

    bool StartObject() {
        PyObject* dict = PyDict_New();
        if (dict == NULL) {
            return false;
        }

        if (!Handle(dict)) {
            return false;
        }

        HandlerContext ctx;
        ctx.isObject = true;
        ctx.object = dict;

        stack.push_back(ctx);

        return true;
    }

    bool EndObject(SizeType member_count) {
        if (!objectHook) {
            stack.pop_back();
            return true;
        }

        PyObject* dict = stack.back().object;
        stack.pop_back();

        PyObject* value = PyObject_CallFunctionObjArgs(objectHook, dict, NULL);

        if (value == NULL)
            return false;

        Py_INCREF(value);

        if (!stack.empty()) {
            const HandlerContext& current = stack.back();

            if (current.isObject) {
                PyObject* key = PyUnicode_FromStringAndSize(current.key,
                                                            current.keyLength);
                if (key == NULL) {
                    Py_DECREF(value);
                    return false;
                }

                int rc = PyDict_SetItem(current.object, key, value);
                Py_DECREF(key);

                if (rc == -1) {
                    Py_DECREF(value);
                    return false;
                }
            }
            else {
                Py_ssize_t listLen = PyList_GET_SIZE(current.object);
                int rc = PyList_SetItem(current.object, listLen - 1, value);

                if (rc == -1) {
                    Py_DECREF(value);
                    return false;
                }
            }
        }
        else {
            Py_DECREF(root);
            root = value;
        }

        Py_DECREF(value);
        return true;
    }

    bool StartArray() {
        PyObject* list = PyList_New(0);
        if (list == NULL) {
            return false;
        }

        if (!Handle(list)) {
            return false;
        }

        HandlerContext ctx;
        ctx.isObject = false;
        ctx.object = list;

        stack.push_back(ctx);

        return true;
    }

    bool EndArray(SizeType elementCount) {
        stack.pop_back();
        return true;
    }

    bool NaN() {
        if (!(numberMode & NM_NAN)) {
            PyErr_SetString(PyExc_ValueError,
                            "Out of range float values are not JSON compliant");
            return false;
        }

        PyObject* value;
        if (numberMode & NM_DECIMAL) {
            value = PyObject_CallFunctionObjArgs(decimal_type, nan_string_value, NULL);
        } else {
            value = PyFloat_FromString(nan_string_value);
        }

        if (value == NULL)
            return false;

        return HandleSimpleType(value);
    }

    bool Infinity(bool minus) {
        if (!(numberMode & NM_NAN)) {
            PyErr_SetString(PyExc_ValueError,
                            "Out of range float values are not JSON compliant");
            return false;
        }

        PyObject* value;
        if (numberMode & NM_DECIMAL) {
            value = PyObject_CallFunctionObjArgs(decimal_type,
                                                 minus
                                                 ? minus_inf_string_value
                                                 : plus_inf_string_value, NULL);
        } else {
            value = PyFloat_FromString(minus
                                       ? minus_inf_string_value
                                       : plus_inf_string_value);
        }

        if (value == NULL)
            return false;

        return HandleSimpleType(value);
    }

    bool Null() {
        PyObject* value = Py_None;
        Py_INCREF(value);

        return HandleSimpleType(value);
    }

    bool Bool(bool b) {
        PyObject* value = b ? Py_True : Py_False;
        Py_INCREF(value);

        return HandleSimpleType(value);
    }

    bool Int(int i) {
        PyObject* value = PyLong_FromLong(i);
        return HandleSimpleType(value);
    }

    bool Uint(unsigned i) {
        PyObject* value = PyLong_FromUnsignedLong(i);
        return HandleSimpleType(value);
    }

    bool Int64(int64_t i) {
        PyObject* value = PyLong_FromLongLong(i);
        return HandleSimpleType(value);
    }

    bool Uint64(uint64_t i) {
        PyObject* value = PyLong_FromUnsignedLongLong(i);
        return HandleSimpleType(value);
    }

    bool Double(double d) {
        PyObject* value = PyFloat_FromDouble(d);
        return HandleSimpleType(value);
    }

    bool RawNumber(const char* str, SizeType length, bool copy) {
        PyObject* value;
        bool isFloat = false;

        for (int i = length - 1; i >= 0; --i) {
            // consider it a float if there is at least one non-digit character,
            // it may be either a decimal number or +-infinity or nan
            if (!isdigit(str[i]) && str[i] != '-') {
                isFloat = true;
                break;
            }
        }

        if (isFloat) {
            PyObject* pystr = PyUnicode_FromStringAndSize(str, length);

            if (pystr == NULL) {
                return false;
            }

            if (numberMode & NM_DECIMAL) {
                value = PyObject_CallFunctionObjArgs(decimal_type, pystr, NULL);
            } else {
                value = PyFloat_FromString(pystr);
            }

            Py_DECREF(pystr);
        } else {
            std::string zstr(str, length);

            value = PyLong_FromString(zstr.c_str(), NULL, 10);
        }

        if (value == NULL) {
            PyErr_SetString(PyExc_ValueError,
                            isFloat
                            ? "Invalid float value"
                            : "Invalid integer value");
            return false;
        } else {
            return HandleSimpleType(value);
        }
    }

#define digit(idx) (str[idx] - '0')

    bool IsIso8601(const char* str, SizeType length) {
        bool res;
        int hours = 0, mins = 0, secs = 0;
        int year = -1, month = 0, day = 0;
        int hofs = 0, mofs = 0;

        switch(length) {
        case 8:                     /* 20:02:20 */
        case 9:                     /* 20:02:20Z */
        case 12:                    /* 20:02:20.123 */
        case 13:                    /* 20:02:20.123Z */
        case 14:                    /* 20:02:20-05:00 */
        case 15:                    /* 20:02:20.123456 */
        case 16:                    /* 20:02:20.123456Z */
        case 18:                    /* 20:02:20.123-05:00 */
        case 21:                    /* 20:02:20.123456-05:00 */
            res = (str[2] == ':' && str[5] == ':' &&
                   isdigit(str[0]) && isdigit(str[1]) &&
                   isdigit(str[3]) && isdigit(str[4]) &&
                   isdigit(str[6]) && isdigit(str[7]));
            if (res) {
                hours = digit(0)*10 + digit(1);
                mins = digit(3)*10 + digit(4);
                secs = digit(6)*10 + digit(7);

                if (length == 9)
                    res = str[8] == 'Z';
                else if (length == 14)
                    res = str[8] == '-' || str[8] == '+';
                else if (length > 8)
                    res = str[8] == '.';
                if (res && (length == 13 || length == 16)) {
                    res = str[length-1] == 'Z';
                    length--;
                }
                if (res && (length == 14 || length == 18 || length == 21)) {
                    res = ((str[length-6] == '+' || str[length-6] == '-') &&
                           isdigit(str[length-5]) &&
                           isdigit(str[length-4]) &&
                           str[length-3] == ':' &&
                           isdigit(str[length-2]) &&
                           isdigit(str[length-1]));
                    if (res) {
                        hofs = digit(length-5)*10 + digit(length-4);
                        mofs = digit(length-2)*10 + digit(length-1);
                    }
                    length -= 6;
                }
                if (res && length > 9 && length != 14) {
                    res = isdigit(str[9]) && isdigit(str[10]) && isdigit(str[11]);
                    if (res && length > 12)
                        res = isdigit(str[12]) && isdigit(str[13]) && isdigit(str[14]);
                }
            }
            break;

        case 10:                    /* 1999-02-03 */
        case 19:                    /* 1999-02-03T10:20:30 */
        case 20:                    /* 1999-02-03T10:20:30Z */
        case 23:                    /* 1999-02-03T10:20:30.123 */
        case 24:                    /* 1999-02-03T10:20:30.123Z */
        case 25:                    /* 1999-02-03T10:20:30-05:00 */
        case 26:                    /* 1999-02-03T10:20:30.123456 */
        case 27:                    /* 1999-02-03T10:20:30.123456Z */
        case 29:                    /* 1999-02-03T10:20:30.123-05:00 */
        case 32:                    /* 1999-02-03T10:20:30.123456-05:00 */
            res = (str[4] == '-' && str[7] == '-' &&
                   isdigit(str[0]) && isdigit(str[1]) &&
                   isdigit(str[2]) && isdigit(str[3]) &&
                   isdigit(str[5]) && isdigit(str[6]) &&
                   isdigit(str[8]) && isdigit(str[9]));
            if (res) {
                year = digit(0)*1000
                    + digit(1)*100
                    + digit(2)*10
                    + digit(3);
                month = digit(5)*10 + digit(6);
                day = digit(8)*10 + digit(9);
            }
            if (res && length > 10) {
                if (str[10] == ' ' || str[10] == 'T') {
                    res = (str[13] == ':' && str[16] == ':' &&
                           isdigit(str[11]) && isdigit(str[12]) &&
                           isdigit(str[14]) && isdigit(str[15]) &&
                           isdigit(str[17]) && isdigit(str[18]));
                    if (res) {
                        hours = digit(11)*10 + digit(12);
                        mins = digit(14)*10 + digit(15);
                        secs = digit(17)*10 + digit(18);
                        if (length == 25 || length == 29 || length == 32) {
                            res = ((str[length-6] == '+'
                                    || str[length-6] == '-') &&
                                   isdigit(str[length-5]) &&
                                   isdigit(str[length-4]) &&
                                   str[length-3] == ':' &&
                                   isdigit(str[length-2]) &&
                                   isdigit(str[length-1]));
                            if (res) {
                                hofs = digit(length-5)*10 + digit(length-4);
                                mofs = digit(length-2)*10 + digit(length-1);
                            }
                            length -= 6;
                        }
                        if (res && (length == 20
                                    || length == 24
                                    || length == 27)) {
                            res = str[length-1] == 'Z';
                            length--;
                        }
                        if (res && length == 23)
                            res = (str[19] == '.' &&
                                   isdigit(str[20]) &&
                                   isdigit(str[21]) &&
                                   isdigit(str[22]));
                        else if (res && length == 26)
                            res = (str[19] == '.' &&
                                   isdigit(str[20]) &&
                                   isdigit(str[21]) &&
                                   isdigit(str[22]) &&
                                   isdigit(str[23]) &&
                                   isdigit(str[24]) &&
                                   isdigit(str[25]));
                    }
                } else
                    res = false;
            }
            break;

        default:
            res = false;
            break;
        }

        if (res && (hours > 23 || mins > 59 || secs > 59
                    || year == 0 || month > 12
                    || (month > 0 && day > days_per_month(year, month))
                    || hofs > 23 || mofs > 59))
            res = false;

        return res;
    }

    bool HandleIso8601(const char* str, SizeType length) {
        PyObject *value;
        int hours, mins, secs, usecs;
        int year, month, day;

        switch(length) {
        case 8:                     /* 20:02:20 */
        case 9:                     /* 20:02:20Z */
        case 12:                    /* 20:02:20.123 */
        case 13:                    /* 20:02:20.123Z */
        case 14:                    /* 20:02:20-05:00 */
        case 15:                    /* 20:02:20.123456 */
        case 16:                    /* 20:02:20.123456Z */
        case 18:                    /* 20:02:20.123-05:00 */
        case 21:                    /* 20:02:20.123456-05:00 */
            hours = digit(0)*10 + digit(1);
            mins = digit(3)*10 + digit(4);
            secs = digit(6)*10 + digit(7);
            if (length == 8 || length == 9 || length == 14)
                usecs = 0;
            else {
                usecs = digit(9) * 100000
                    + digit(10) * 10000
                    + digit(11) * 1000;
                if (length == 15 || length == 16 || length == 21)
                    usecs += digit(12) * 100
                        + digit(13) * 10
                        + digit(14);
            }
            if ((length == 8 && datetimeMode & DM_NAIVE_IS_UTC)
                || length == 9 || length == 13 || length == 16)
                value = PyDateTimeAPI->Time_FromTime(
                    hours, mins, secs, usecs, timezone_utc,
                    PyDateTimeAPI->TimeType);
            else if (datetimeMode & DM_IGNORE_TZ
                     || length == 8 || length == 12 || length == 15)
                value = PyTime_FromTime(hours, mins, secs, usecs);
            else /* if (length == 14 || length == 18 || length == 21) */ {
                int secsoffset = ((digit(length-5)*10 + digit(length-4)) * 3600
                                  + (digit(length-2)*10 + digit(length-1)) * 60);
                if (str[length-6] == '-')
                    secsoffset = -secsoffset;
                if (datetimeMode & DM_SHIFT_TO_UTC && secsoffset) {
                    PyErr_Format(PyExc_ValueError,
                                 "Time literal cannot be shifted to UTC: %s", str);
                    value = NULL;
                } else {
                    if (datetimeMode & DM_SHIFT_TO_UTC) {
                        value = PyDateTimeAPI->Time_FromTime(
                            hours, mins, secs, usecs, timezone_utc,
                            PyDateTimeAPI->TimeType);
                    } else {
                        PyObject* offset = PyDateTimeAPI->Delta_FromDelta(
                            0, secsoffset, 0, 1, PyDateTimeAPI->DeltaType);
                        if (offset == NULL)
                            value = NULL;
                        else {
                            PyObject* tz = PyObject_CallFunctionObjArgs(
                                timezone_type, offset, NULL);
                            Py_DECREF(offset);
                            if (tz == NULL)
                                value = NULL;
                            else {
                                value = PyDateTimeAPI->Time_FromTime(
                                    hours, mins, secs, usecs, tz,
                                    PyDateTimeAPI->TimeType);
                                Py_DECREF(tz);
                            }
                        }
                    }
                }
            }
            break;

        case 10:                    /* 1999-02-03 */
            year = digit(0)*1000
                + digit(1)*100
                + digit(2)*10
                + digit(3);
            month = digit(5)*10 + digit(6);
            day = digit(8)*10 + digit(9);
            value = PyDate_FromDate(year, month, day);
            break;

        case 19:                    /* 1999-02-03T10:20:30 */
        case 20:                    /* 1999-02-03T10:20:30Z */
        case 23:                    /* 1999-02-03T10:20:30.123 */
        case 24:                    /* 1999-02-03T10:20:30.123Z */
        case 25:                    /* 1999-02-03T10:20:30-05:00 */
        case 26:                    /* 1999-02-03T10:20:30.123456 */
        case 27:                    /* 1999-02-03T10:20:30.123456Z */
        case 29:                    /* 1999-02-03T10:20:30.123-05:00 */
        case 32:                    /* 1999-02-03T10:20:30.123456-05:00 */
            year = digit(0)*1000
                + digit(1)*100
                + digit(2)*10
                + digit(3);
            month = digit(5)*10 + digit(6);
            day = digit(8)*10 + digit(9);
            hours = digit(11)*10 + digit(12);
            mins = digit(14)*10 + digit(15);
            secs = digit(17)*10 + digit(18);
            if (length == 19 || length == 20 || length == 25)
                usecs = 0;
            else {
                usecs = digit(20)*100000
                    + digit(21)*10000
                    + digit(22)*1000;
                if (length == 26 || length == 27 || length == 32)
                    usecs += digit(23)*100
                        + digit(24)*10
                        + digit(25);
            }
            if ((length == 19 && datetimeMode & DM_NAIVE_IS_UTC)
                || length == 20 || length == 24 || length == 27)
                value = PyDateTimeAPI->DateTime_FromDateAndTime(
                    year, month, day, hours, mins, secs, usecs,
                    timezone_utc, PyDateTimeAPI->DateTimeType);
            else if (datetimeMode & DM_IGNORE_TZ
                     || length == 19 || length == 23 || length == 26)
                value = PyDateTime_FromDateAndTime(
                    year, month, day, hours, mins, secs, usecs);
            else /* if (length == 25 || length == 29 || length == 32) */ {
                int secsoffset = ((digit(length-5)*10 + digit(length-4)) * 3600
                                  + (digit(length-2)*10 + digit(length-1)) * 60);
                if (str[length-6] == '-')
                    secsoffset = -secsoffset;
                PyObject* offset = PyDateTimeAPI->Delta_FromDelta(
                    0, secsoffset, 0, 1, PyDateTimeAPI->DeltaType);
                if (offset == NULL)
                    value = NULL;
                else {
                    PyObject* tz = PyObject_CallFunctionObjArgs(
                        timezone_type, offset, NULL);
                    Py_DECREF(offset);
                    if (tz == NULL)
                        value = NULL;
                    else {
                        value = PyDateTimeAPI->DateTime_FromDateAndTime(
                            year, month, day, hours, mins, secs, usecs,
                            tz, PyDateTimeAPI->DateTimeType);
                        Py_DECREF(tz);

                        if (value != NULL && datetimeMode & DM_SHIFT_TO_UTC) {
                            PyObject* asUTC = PyObject_CallMethodObjArgs(
                                value, astimezone_name, timezone_utc, NULL);

                            Py_DECREF(value);

                            if (asUTC == NULL)
                                value = NULL;
                            else
                                value = asUTC;
                        }
                    }
                }
            }
            break;

        default:
            PyErr_SetString(PyExc_ValueError,
                            "not a datetime, nor a date, nor a time");
            value = NULL;
            break;
        }

        if (value == NULL)
            return false;
        else
            return HandleSimpleType(value);
    }

#undef digit

    bool IsUuid(const char* str, SizeType length) {
        if (uuidMode == UM_HEX && length == 32) {
            for (int i = length - 1; i >= 0; --i)
                if (!isxdigit(str[i]))
                    return false;
            return true;
        } else if (length == 36
                   && str[8] == '-' && str[13] == '-'
                   && str[18] == '-' && str[23] == '-') {
            for (int i = length - 1; i >= 0; --i)
                if (i != 8 && i != 13 && i != 18 && i != 23 && !isxdigit(str[i]))
                    return false;
            return true;
        }
        return false;
    }

    bool HandleUuid(const char* str, SizeType length) {
        PyObject* pystr = PyUnicode_FromStringAndSize(str, length);
        if (pystr == NULL)
            return false;

        PyObject* value = PyObject_CallFunctionObjArgs(uuid_type,
                                                       pystr, NULL);
        Py_DECREF(pystr);

        if (value == NULL)
            return false;
        else
            return HandleSimpleType(value);
    }

    bool String(const char* str, SizeType length, bool copy) {
        PyObject* value;

        if (datetimeMode != DM_NONE && IsIso8601(str, length))
            return HandleIso8601(str, length);

        if (uuidMode != UM_NONE && IsUuid(str, length))
            return HandleUuid(str, length);

        value = PyUnicode_FromStringAndSize(str, length);
        return HandleSimpleType(value);
    }
};


static PyObject*
loads(PyObject* self, PyObject* args, PyObject* kwargs)
{
    /* Converts a JSON encoded string to a Python object. */

    PyObject* jsonObject;
    PyObject* objectHook = NULL;
    PyObject* datetimeModeObj = NULL;
    DatetimeMode datetimeMode = DM_NONE;
    PyObject* uuidModeObj = NULL;
    UuidMode uuidMode = UM_NONE;
    PyObject* numberModeObj = NULL;
    NumberMode numberMode = NM_NAN;

    int allowNan = -1;

    static char const * kwlist[] = {
        "s",
        "object_hook",
        "number_mode",
        "datetime_mode",
        "uuid_mode",

        /* compatibility with stdlib json */
        "allow_nan",

        NULL
    };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OOOOp:rapidjson.loads",
                                     (char **) kwlist,
                                     &jsonObject,
                                     &objectHook,
                                     &numberModeObj,
                                     &datetimeModeObj,
                                     &uuidModeObj,
                                     &allowNan))
        return NULL;

    if (objectHook && !PyCallable_Check(objectHook)) {
        PyErr_SetString(PyExc_TypeError, "object_hook is not callable");
        return NULL;
    }

    Py_ssize_t jsonStrLen;
    char* jsonStr;

    if (PyBytes_Check(jsonObject)) {
        int rc = PyBytes_AsStringAndSize(jsonObject, &jsonStr, &jsonStrLen);
        if (rc == -1)
            return NULL;
    }
    else if (PyUnicode_Check(jsonObject)) {
        jsonStr = PyUnicode_AsUTF8AndSize(jsonObject, &jsonStrLen);
        if (jsonStr == NULL)
            return NULL;
    }
    else {
        PyErr_SetString(PyExc_TypeError, "Expected string or utf-8 encoded bytes");
        return NULL;
    }

    if (numberModeObj) {
        if (numberModeObj == Py_None)
            numberMode = NM_NONE;
        else if (PyLong_Check(numberModeObj)) {
            int mode = PyLong_AsLong(numberModeObj);
            if (mode < 0 || mode >= 1<<3) {
                PyErr_SetString(PyExc_ValueError, "Invalid number_mode");
                return NULL;
            }
            numberMode = (NumberMode) mode;
            if (numberMode & NM_DECIMAL && numberMode & NM_NATIVE) {
                PyErr_SetString(PyExc_ValueError,
                                "Combining NM_NATIVE with NM_DECIMAL is not supported");
                return NULL;
            }
        }
    }
    if (allowNan != -1) {
        if (allowNan)
            numberMode = (NumberMode) (numberMode | NM_NAN);
        else
            numberMode = (NumberMode) (numberMode & ~NM_NAN);
    }

    if (datetimeModeObj) {
        if (datetimeModeObj == Py_None)
            datetimeMode = DM_NONE;
        else if (PyLong_Check(datetimeModeObj)) {
            int mode = PyLong_AsLong(datetimeModeObj);
            if (!valid_datetime_mode(mode)) {
                PyErr_SetString(PyExc_ValueError, "Invalid datetime_mode");
                return NULL;
            }
            datetimeMode = (DatetimeMode) mode;
            if (datetimeMode && datetime_mode_format(datetimeMode) != DM_ISO8601) {
                PyErr_SetString(PyExc_ValueError,
                                "Invalid datetime_mode, can deserialize only from ISO8601");
                return NULL;
            }
        }
        else {
            PyErr_SetString(PyExc_TypeError,
                            "datetime_mode must be a non-negative integer value or None");
            return NULL;
        }
    }

    if (uuidModeObj) {
        if (uuidModeObj == Py_None)
            uuidMode = UM_NONE;
        else if (PyLong_Check(uuidModeObj)) {
            uuidMode = (UuidMode) PyLong_AsLong(uuidModeObj);
            if (uuidMode < UM_NONE || uuidMode > UM_HEX) {
                PyErr_SetString(PyExc_ValueError, "Invalid uuid_mode");
                return NULL;
            }
        }
        else {
            PyErr_SetString(PyExc_TypeError,
                            "uuid_mode must be an integer value or None");
            return NULL;
        }
    }

    char* jsonStrCopy = (char*) malloc(sizeof(char) * (jsonStrLen+1));
    memcpy(jsonStrCopy, jsonStr, jsonStrLen+1);

    PyHandler handler(objectHook, datetimeMode, uuidMode, numberMode);
    Reader reader;
    InsituStringStream ss(jsonStrCopy);

    if (numberMode & NM_NAN)
        if (numberMode & NM_NATIVE)
            reader.Parse<kParseInsituFlag |
                         kParseNanAndInfFlag>(ss, handler);
        else
            reader.Parse<kParseInsituFlag |
                         kParseNumbersAsStringsFlag |
                         kParseNanAndInfFlag>(ss, handler);
    else
        if (numberMode & NM_NATIVE)
            reader.Parse<kParseInsituFlag>(ss, handler);
        else
            reader.Parse<kParseInsituFlag | kParseNumbersAsStringsFlag>(ss, handler);

    if (reader.HasParseError()) {
        SizeType offset = reader.GetErrorOffset();
        ParseErrorCode code = reader.GetParseErrorCode();
        const char* msg = GetParseError_En(code);
        const char* fmt = "Parse error at offset %d: %s";

        if (PyErr_Occurred()) {
            PyObject* etype;
            PyObject* evalue;
            PyObject* etraceback;
            PyErr_Fetch(&etype, &evalue, &etraceback);

            const char* emsg = msg;
            if (PyUnicode_Check(evalue))
                emsg = PyUnicode_AsUTF8(evalue);

            PyErr_Format(etype, fmt, offset, emsg);
        }
        else
            PyErr_Format(PyExc_ValueError, fmt, offset, msg);

        Py_XDECREF(handler.root);
        free(jsonStrCopy);
        return NULL;
    }

    free(jsonStrCopy);
    return handler.root;
}


struct WriterContext {
    const char* key;
    Py_ssize_t size;
    PyObject* object;
    PyObject* decref;
    unsigned level;
    bool isObject;

    WriterContext(const char* k,
                  Py_ssize_t s,
                  PyObject* o,
                  bool isO,
                  int l,
                  PyObject* d=NULL)
    : key(k),
      size(s),
      object(o),
      decref(d),
      level(l),
      isObject(isO)
    {}
};


struct DictItem {
    char* key_str;
    Py_ssize_t key_size;
    PyObject* item;

    DictItem(char* k,
             Py_ssize_t s,
             PyObject* i)
    : key_str(k),
      key_size(s),
      item(i)
    {}

    bool operator<(const DictItem& other) const {
        return strcmp(other.key_str, this->key_str) < 0;
    }
};


static const int MAX_RECURSION_DEPTH = 2048;


template<typename WriterT, typename BufferT>
static PyObject*
dumps_internal(
    WriterT* writer,
    BufferT* buf,
    PyObject* value,
    int skipKeys,
    PyObject* defaultFn,
    int sortKeys,
    unsigned maxRecursionDepth,
    NumberMode numberMode,
    DatetimeMode datetimeMode,
    UuidMode uuidMode)
{
    int isDec;
    std::vector<WriterContext> stack;
    stack.reserve(128);

    stack.push_back(WriterContext(NULL, 0, value, false, 0));

    while (!stack.empty()) {
        const WriterContext& current = stack.back();
        stack.pop_back();

        const unsigned currentLevel = current.level;
        const unsigned nextLevel = current.level + 1;
        PyObject* const object = current.object;

        if (currentLevel > maxRecursionDepth) {
            PyErr_SetString(PyExc_OverflowError, "Max recursion depth reached");
            return NULL;
        }

        if (object == NULL) {
            if (current.key != NULL) {
                writer->Key(current.key, current.size);
            }
            else if (current.decref != NULL) {
                Py_DECREF(current.decref);
            }
            else if (current.isObject) {
                writer->EndObject();
            }
            else {
                writer->EndArray();
            }
        }
        else if (object == Py_None) {
            writer->Null();
        }
        else if (PyBool_Check(object)) {
            writer->Bool(object == Py_True);
        }
        else if (numberMode & NM_DECIMAL
                 && (isDec = PyObject_IsInstance(object, decimal_type))) {
            if (isDec == -1)
                return NULL;

            if (!(numberMode & NM_NAN)) {
                bool is_inf_or_nan;
                PyObject* is_inf = PyObject_CallMethodObjArgs(object, is_infinite_name, NULL);

                if (is_inf == NULL) {
                    return NULL;
                }
                is_inf_or_nan = is_inf == Py_True;
                Py_DECREF(is_inf);

                if (!is_inf_or_nan) {
                    PyObject* is_nan = PyObject_CallMethodObjArgs(object, is_nan_name, NULL);

                    if (is_nan == NULL) {
                        return NULL;
                    }
                    is_inf_or_nan = is_nan == Py_True;
                    Py_DECREF(is_nan);
                }

                if (is_inf_or_nan) {
                    PyErr_SetString(PyExc_ValueError,
                                    "Out of range decimal values are not JSON compliant");
                    return NULL;
                }
            }

            PyObject* decStrObj = PyObject_Str(object);
            if (decStrObj == NULL)
                return NULL;

            Py_ssize_t size;
            char* decStr = PyUnicode_AsUTF8AndSize(decStrObj, &size);
            if (decStr == NULL) {
                Py_DECREF(decStrObj);
                return NULL;
            }

            writer->RawValue(decStr, size, kNumberType);
            Py_DECREF(decStrObj);
        }
        else if (PyLong_Check(object)) {
            if (numberMode & NM_NATIVE) {
                int overflow;
                long long i = PyLong_AsLongLongAndOverflow(object, &overflow);
                if (i == -1 && PyErr_Occurred())
                    return NULL;

                if (overflow == 0) {
                    writer->Int64(i);
                } else {
                    unsigned long long ui = PyLong_AsUnsignedLongLong(object);
                    if (PyErr_Occurred())
                        return NULL;

                    writer->Uint64(ui);
                }
            } else {
                PyObject* intStrObj = PyObject_Str(object);
                if (intStrObj == NULL)
                    return NULL;

                Py_ssize_t size;
                char* intStr = PyUnicode_AsUTF8AndSize(intStrObj, &size);
                if (intStr == NULL) {
                    Py_DECREF(intStrObj);
                    return NULL;
                }

                writer->RawValue(intStr, size, kNumberType);
                Py_DECREF(intStrObj);
            }
        }
        else if (PyFloat_Check(object)) {
            double d = PyFloat_AsDouble(object);
            if (d == -1.0 && PyErr_Occurred())
                return NULL;

            if (Py_IS_NAN(d)) {
                if (numberMode & NM_NAN)
                    writer->RawValue("NaN", 3, kNumberType);
                else {
                    PyErr_SetString(PyExc_ValueError,
                                    "Out of range float values are not JSON compliant");
                    return NULL;
                }
            } else if (Py_IS_INFINITY(d)) {
                if (!(numberMode & NM_NAN)) {
                    PyErr_SetString(PyExc_ValueError,
                                    "Out of range float values are not JSON compliant");
                    return NULL;
                }
                else if (d < 0)
                    writer->RawValue("-Infinity", 9, kNumberType);
                else
                    writer->RawValue("Infinity", 8, kNumberType);
            }
            else
                writer->Double(d);
        }
        else if (PyBytes_Check(object)) {
            Py_ssize_t l;
            char* s;

            if (PyBytes_AsStringAndSize(object, &s, &l) == -1)
                goto error;

            writer->String(s, l);
        }
        else if (PyUnicode_Check(object)) {
            Py_ssize_t l;
            char* s = PyUnicode_AsUTF8AndSize(object, &l);
            writer->String(s, l);
        }
        else if (PyList_Check(object)) {
            writer->StartArray();
            stack.push_back(WriterContext(NULL, 0, NULL, false, currentLevel));

            Py_ssize_t size = PyList_GET_SIZE(object);

            for (Py_ssize_t i = size - 1; i >= 0; --i) {
                PyObject* item = PyList_GET_ITEM(object, i);
                stack.push_back(WriterContext(NULL, 0, item, false, nextLevel));
            }
        }
        else if (PyTuple_Check(object)) {
            writer->StartArray();
            stack.push_back(WriterContext(NULL, 0, NULL, false, currentLevel));

            Py_ssize_t size = PyTuple_GET_SIZE(object);

            for (Py_ssize_t i = size - 1; i >= 0; --i) {
                PyObject* item = PyTuple_GET_ITEM(object, i);
                stack.push_back(WriterContext(NULL, 0, item, false, nextLevel));
            }
        }
        else if (PyDict_Check(object)) {
            writer->StartObject();
            stack.push_back(WriterContext(NULL, 0, NULL, true, currentLevel));

            Py_ssize_t pos = 0;
            PyObject* key;
            PyObject* item;

            if (!sortKeys) {
                while (PyDict_Next(object, &pos, &key, &item)) {
                    if (PyUnicode_Check(key)) {
                        Py_ssize_t l;
                        char* key_str = PyUnicode_AsUTF8AndSize(key, &l);
                        stack.push_back(WriterContext(NULL, 0, item, false, nextLevel));
                        stack.push_back(WriterContext(key_str, l, NULL, false, nextLevel));
                    }
                    else if (!skipKeys) {
                        PyErr_SetString(PyExc_TypeError, "keys must be a string");
                        goto error;
                    }
                }
            }
            else {
                std::vector<DictItem> items;

                while (PyDict_Next(object, &pos, &key, &item)) {
                    if (PyUnicode_Check(key)) {
                        Py_ssize_t l;
                        char* key_str = PyUnicode_AsUTF8AndSize(key, &l);
                        items.push_back(DictItem(key_str, l, item));
                    }
                    else if (!skipKeys) {
                        PyErr_SetString(PyExc_TypeError, "keys must be a string");
                        goto error;
                    }
                }

                std::sort(items.begin(), items.end());

                std::vector<DictItem>::const_iterator iter = items.begin();
                for (; iter != items.end(); ++iter) {
                    stack.push_back(WriterContext(NULL, 0, iter->item, false, nextLevel));
                    stack.push_back(WriterContext(iter->key_str, iter->key_size,
                                                  NULL, false, nextLevel));
                }
            }
        }
        else if (datetimeMode != DM_NONE
                 && (PyTime_Check(object) || PyDateTime_Check(object))) {
            int year, month, day, hour, min, sec, microsec;
            PyObject* dtObject = object;
            PyObject* asUTC = NULL;

            const int ISOFORMAT_LEN = 40;
            char isoformat[ISOFORMAT_LEN];
            memset(isoformat, 0, ISOFORMAT_LEN);

            const int TIMEZONE_LEN = 10;
            char timeZone[TIMEZONE_LEN] = { 0 };

            if (!(datetimeMode & DM_IGNORE_TZ)
                && PyObject_HasAttr(object, utcoffset_name)) {
                PyObject* utcOffset = PyObject_CallMethodObjArgs(object,
                                                                 utcoffset_name,
                                                                 NULL);

                if (!utcOffset)
                    goto error;

                if (utcOffset == Py_None) {
                    // Naive value: maybe assume it's in UTC instead of local time
                    if (datetimeMode & DM_NAIVE_IS_UTC) {
                        if (PyDateTime_Check(object)) {
                            hour = PyDateTime_DATE_GET_HOUR(dtObject);
                            min = PyDateTime_DATE_GET_MINUTE(dtObject);
                            sec = PyDateTime_DATE_GET_SECOND(dtObject);
                            microsec = PyDateTime_DATE_GET_MICROSECOND(dtObject);
                            year = PyDateTime_GET_YEAR(dtObject);
                            month = PyDateTime_GET_MONTH(dtObject);
                            day = PyDateTime_GET_DAY(dtObject);

                            asUTC = PyDateTimeAPI->DateTime_FromDateAndTime(
                                year, month, day, hour, min, sec, microsec,
                                timezone_utc, PyDateTimeAPI->DateTimeType);
                        } else {
                            hour = PyDateTime_TIME_GET_HOUR(dtObject);
                            min = PyDateTime_TIME_GET_MINUTE(dtObject);
                            sec = PyDateTime_TIME_GET_SECOND(dtObject);
                            microsec = PyDateTime_TIME_GET_MICROSECOND(dtObject);
                            asUTC = PyDateTimeAPI->Time_FromTime(
                                hour, min, sec, microsec,
                                timezone_utc, PyDateTimeAPI->TimeType);
                        }

                        if (asUTC == NULL) {
                            Py_DECREF(utcOffset);
                            goto error;
                        }

                        dtObject = asUTC;

                        if (datetime_mode_format(datetimeMode) == DM_ISO8601)
                            strcpy(timeZone, "+00:00");
                    }
                } else {
                    // Timezone-aware value
                    if (datetimeMode & DM_SHIFT_TO_UTC) {
                        // If it's not already in UTC, shift the value
                        if (PyObject_IsTrue(utcOffset)) {
                            asUTC = PyObject_CallMethodObjArgs(object, astimezone_name,
                                                               timezone_utc, NULL);

                            if (asUTC == NULL) {
                                Py_DECREF(utcOffset);
                                goto error;
                            }

                            dtObject = asUTC;
                        }

                        if (datetime_mode_format(datetimeMode) == DM_ISO8601)
                            strcpy(timeZone, "+00:00");
                    } else if (datetime_mode_format(datetimeMode) == DM_ISO8601) {
                        int seconds_from_utc = 0;

                        if (PyObject_IsTrue(utcOffset)) {
                            PyObject* tsObj = PyObject_CallMethodObjArgs(utcOffset,
                                                                         total_seconds_name,
                                                                         NULL);

                            if (tsObj == NULL) {
                                Py_DECREF(utcOffset);
                                goto error;
                            }

                            seconds_from_utc = PyFloat_AsDouble(tsObj);

                            Py_DECREF(tsObj);
                        }

                        char sign = '+';

                        if (seconds_from_utc < 0) {
                            sign = '-';
                            seconds_from_utc = -seconds_from_utc;
                        }

                        int tz_hour = seconds_from_utc / 3600;
                        int tz_min = (seconds_from_utc % 3600) / 60;

                        snprintf(timeZone, TIMEZONE_LEN-1, "%c%02d:%02d",
                                 sign, tz_hour, tz_min);
                    }
                }
                Py_DECREF(utcOffset);
            }

            if (datetime_mode_format(datetimeMode) == DM_ISO8601) {
                if (PyDateTime_Check(dtObject)) {
                    year = PyDateTime_GET_YEAR(dtObject);
                    month = PyDateTime_GET_MONTH(dtObject);
                    day = PyDateTime_GET_DAY(dtObject);
                    hour = PyDateTime_DATE_GET_HOUR(dtObject);
                    min = PyDateTime_DATE_GET_MINUTE(dtObject);
                    sec = PyDateTime_DATE_GET_SECOND(dtObject);
                    microsec = PyDateTime_DATE_GET_MICROSECOND(dtObject);

                    if (microsec > 0) {
                        snprintf(isoformat,
                                 ISOFORMAT_LEN-1,
                                 "%04d-%02d-%02dT%02d:%02d:%02d.%06d%s",
                                 year, month, day,
                                 hour, min, sec, microsec,
                                 timeZone);
                    } else {
                        snprintf(isoformat,
                                 ISOFORMAT_LEN-1,
                                 "%04d-%02d-%02dT%02d:%02d:%02d%s",
                                 year, month, day,
                                 hour, min, sec,
                                 timeZone);
                    }
                } else {
                    hour = PyDateTime_TIME_GET_HOUR(dtObject);
                    min = PyDateTime_TIME_GET_MINUTE(dtObject);
                    sec = PyDateTime_TIME_GET_SECOND(dtObject);
                    microsec = PyDateTime_TIME_GET_MICROSECOND(dtObject);

                    if (microsec > 0) {
                        snprintf(isoformat,
                                 ISOFORMAT_LEN-1,
                                 "%02d:%02d:%02d.%06d%s",
                                 hour, min, sec, microsec,
                                 timeZone);
                    } else {
                        snprintf(isoformat,
                                 ISOFORMAT_LEN-1,
                                 "%02d:%02d:%02d%s",
                                 hour, min, sec,
                                 timeZone);
                    }
                }
                writer->String(isoformat);
            } else /* if (datetimeMode & DM_UNIX_TIME) */ {
                if (PyDateTime_Check(dtObject)) {
                    PyObject* timestampObj;

                    timestampObj = PyObject_CallMethodObjArgs(dtObject, timestamp_name, NULL);

                    if (!timestampObj) {
                        Py_XDECREF(asUTC);
                        goto error;
                    }

                    double timestamp = PyFloat_AsDouble(timestampObj);

                    Py_DECREF(timestampObj);

                    if (datetimeMode & DM_ONLY_SECONDS)
                        writer->Int64(timestamp);
                    else
                        writer->Double(timestamp);
                } else {
                    hour = PyDateTime_TIME_GET_HOUR(dtObject);
                    min = PyDateTime_TIME_GET_MINUTE(dtObject);
                    sec = PyDateTime_TIME_GET_SECOND(dtObject);
                    microsec = PyDateTime_TIME_GET_MICROSECOND(dtObject);

                    long timestamp = hour * 3600 + min * 60 + sec;

                    if (datetimeMode & DM_ONLY_SECONDS)
                        writer->Int64(timestamp);
                    else
                        writer->Double(timestamp + (microsec / 1000000.0));
                }
            }
            Py_XDECREF(asUTC);
        }
        else if (datetimeMode != DM_NONE && PyDate_Check(object)) {
            int year = PyDateTime_GET_YEAR(object);
            int month = PyDateTime_GET_MONTH(object);
            int day = PyDateTime_GET_DAY(object);

            if (datetime_mode_format(datetimeMode) == DM_ISO8601) {
                const int ISOFORMAT_LEN = 12;
                char isoformat[ISOFORMAT_LEN];
                memset(isoformat, 0, ISOFORMAT_LEN);

                snprintf(isoformat, ISOFORMAT_LEN-1, "%04d-%02d-%02d", year, month, day);
                writer->String(isoformat);
            } else /* datetime_mode_format(datetimeMode) == DM_UNIX_TIME */ {
                // A date object, take its midnight timestamp
                PyObject* midnightObj;
                PyObject* timestampObj;

                if (datetimeMode & (DM_SHIFT_TO_UTC | DM_NAIVE_IS_UTC))
                    midnightObj = PyDateTimeAPI->DateTime_FromDateAndTime(
                        year, month, day, 0, 0, 0, 0,
                        timezone_utc, PyDateTimeAPI->DateTimeType);
                else
                    midnightObj = PyDateTime_FromDateAndTime(year, month, day,
                                                             0, 0, 0, 0);

                if (!midnightObj) {
                    goto error;
                }

                timestampObj = PyObject_CallMethodObjArgs(midnightObj, timestamp_name,
                                                          NULL);

                Py_DECREF(midnightObj);

                if (!timestampObj) {
                    goto error;
                }

                double timestamp = PyFloat_AsDouble(timestampObj);

                Py_DECREF(timestampObj);

                if (datetimeMode & DM_ONLY_SECONDS)
                    writer->Int64(timestamp);
                else
                    writer->Double(timestamp);
            }
        }
        else if (uuidMode != UM_NONE
                 && PyObject_TypeCheck(object, (PyTypeObject *) uuid_type)) {
            PyObject* retval;
            if (uuidMode == UM_CANONICAL)
                retval = PyObject_Str(object);
            else
                retval = PyObject_GetAttr(object, hex_name);
            if (retval == NULL)
                goto error;

            // Decref the return value once it's done being dumped to a string.
            stack.push_back(WriterContext(NULL, 0, NULL, false, currentLevel, retval));
            stack.push_back(WriterContext(NULL, 0, retval, false, currentLevel));
        }
        else if (defaultFn) {
            PyObject* retval = PyObject_CallFunctionObjArgs(defaultFn, object, NULL);
            if (retval == NULL)
                goto error;

            // Decref the return value once it's done being dumped to a string.
            stack.push_back(WriterContext(NULL, 0, NULL, false, currentLevel, retval));
            stack.push_back(WriterContext(NULL, 0, retval, false, currentLevel));
        }
        else {
            PyObject* repr = PyObject_Repr(object);
            PyErr_Format(PyExc_TypeError, "%s is not JSON serializable",
                         PyUnicode_AsUTF8(repr));
            Py_XDECREF(repr);
            goto error;
        }
    }

    return PyUnicode_FromString(buf->GetString());

error:
    return NULL;
}


#define DUMPS_INTERNAL_CALL \
    dumps_internal( \
        &writer, \
        &buf, \
        value, \
        skipKeys, \
        defaultFn, \
        sortKeys, \
        maxRecursionDepth, \
        numberMode, \
        datetimeMode, \
        uuidMode)


static PyObject*
dumps(PyObject* self, PyObject* args, PyObject* kwargs)
{
    /* Converts a Python object to a JSON-encoded string. */

    PyObject* value;
    int skipKeys = 0;
    int ensureAscii = 1;
    PyObject* indent = NULL;
    PyObject* defaultFn = NULL;
    int sortKeys = 0;
    unsigned maxRecursionDepth = MAX_RECURSION_DEPTH;
    PyObject* numberModeObj = NULL;
    NumberMode numberMode = NM_NAN;
    PyObject* datetimeModeObj = NULL;
    DatetimeMode datetimeMode = DM_NONE;
    PyObject* uuidModeObj = NULL;
    UuidMode uuidMode = UM_NONE;

    bool prettyPrint = false;
    const char indentChar = ' ';
    unsigned indentCharCount = 4;

    int allowNan = -1;

    static char const * kwlist[] = {
        "obj",
        "skipkeys",
        "ensure_ascii",
        "indent",
        "default",
        "sort_keys",
        "max_recursion_depth",
        "number_mode",
        "datetime_mode",
        "uuid_mode",

        /* compatibility with stdlib json */
        "allow_nan",

        NULL
    };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|ppOOpIOOOp:rapidjson.dumps",
                                     (char **) kwlist,
                                     &value,
                                     &skipKeys,
                                     &ensureAscii,
                                     &indent,
                                     &defaultFn,
                                     &sortKeys,
                                     &maxRecursionDepth,
                                     &numberModeObj,
                                     &datetimeModeObj,
                                     &uuidModeObj,
                                     &allowNan))
        return NULL;

    if (defaultFn && !PyCallable_Check(defaultFn)) {
        PyErr_SetString(PyExc_TypeError, "default must be a callable");
        return NULL;
    }

    if (indent && indent != Py_None) {
        prettyPrint = true;

        if (PyLong_Check(indent) && PyLong_AsLong(indent) >= 0) {
            indentCharCount = PyLong_AsUnsignedLong(indent);
        }
        else {
            PyErr_SetString(PyExc_TypeError, "indent must be a non-negative int");
            return NULL;
        }
    }

    if (numberModeObj) {
        if (numberModeObj == Py_None)
            numberMode = NM_NONE;
        else if (PyLong_Check(numberModeObj)) {
            int mode = PyLong_AsLong(numberModeObj);
            if (mode < 0 || mode >= 1<<3) {
                PyErr_SetString(PyExc_ValueError, "Invalid number_mode");
                return NULL;
            }
            numberMode = (NumberMode) mode;
        }
    }
    if (allowNan != -1) {
        if (allowNan)
            numberMode = (NumberMode) (numberMode | NM_NAN);
        else
            numberMode = (NumberMode) (numberMode & ~NM_NAN);
    }

   if (datetimeModeObj) {
        if (datetimeModeObj == Py_None)
            datetimeMode = DM_NONE;
        else if (PyLong_Check(datetimeModeObj)) {
            int mode = PyLong_AsLong(datetimeModeObj);
            if (!valid_datetime_mode(mode)) {
                PyErr_SetString(PyExc_ValueError, "Invalid datetime_mode");
                return NULL;
            }
            datetimeMode = (DatetimeMode) mode;
        }
        else {
            PyErr_SetString(PyExc_TypeError,
                            "datetime_mode must be a non-negative integer value or None");
            return NULL;
        }
    }

    if (uuidModeObj) {
        if (uuidModeObj == Py_None)
            uuidMode = UM_NONE;
        else if (PyLong_Check(uuidModeObj)) {
            uuidMode = (UuidMode) PyLong_AsLong(uuidModeObj);
            if (uuidMode < UM_NONE || uuidMode > UM_HEX) {
                PyErr_SetString(PyExc_ValueError, "Invalid uuid_mode");
                return NULL;
            }
        }
        else {
            PyErr_SetString(PyExc_TypeError, "uuid_mode must be an integer value");
            return NULL;
        }
    }

    if (!prettyPrint) {
        if (ensureAscii) {
            GenericStringBuffer<ASCII<> > buf;
            Writer<GenericStringBuffer<ASCII<> >, UTF8<>, ASCII<> > writer(buf);
            return DUMPS_INTERNAL_CALL;
        }
        else {
            StringBuffer buf;
            Writer<StringBuffer> writer(buf);
            return DUMPS_INTERNAL_CALL;
        }
    }
    else if (ensureAscii) {
        GenericStringBuffer<ASCII<> > buf;
        PrettyWriter<GenericStringBuffer<ASCII<> >, UTF8<>, ASCII<> > writer(buf);
        writer.SetIndent(indentChar, indentCharCount);
        return DUMPS_INTERNAL_CALL;
    }
    else {
        StringBuffer buf;
        PrettyWriter<StringBuffer> writer(buf);
        writer.SetIndent(indentChar, indentCharCount);
        return DUMPS_INTERNAL_CALL;
    }
}


static PyMethodDef
functions[] = {
    {"loads", (PyCFunction) loads, METH_VARARGS | METH_KEYWORDS,
     loads_docstring},
    {"dumps", (PyCFunction) dumps, METH_VARARGS | METH_KEYWORDS,
     dumps_docstring},
    {NULL, NULL, 0, NULL} /* sentinel */
};


static PyModuleDef module = {
    PyModuleDef_HEAD_INIT,
    "rapidjson",
    module_docstring,
    -1,
    functions
};


PyMODINIT_FUNC
PyInit_rapidjson()
{
    astimezone_name = PyUnicode_InternFromString("astimezone");
    if (astimezone_name == NULL)
        return NULL;

    hex_name = PyUnicode_InternFromString("hex");
    if (hex_name == NULL) {
        Py_DECREF(astimezone_name);
        return NULL;
    }

    timestamp_name = PyUnicode_InternFromString("timestamp");
    if (timestamp_name == NULL) {
        Py_DECREF(astimezone_name);
        Py_DECREF(hex_name);
        return NULL;
    }

    total_seconds_name = PyUnicode_InternFromString("total_seconds");
    if (total_seconds_name == NULL) {
        Py_DECREF(astimezone_name);
        Py_DECREF(hex_name);
        Py_DECREF(timestamp_name);
        return NULL;
    }

    utcoffset_name = PyUnicode_InternFromString("utcoffset");
    if (utcoffset_name == NULL) {
        Py_DECREF(astimezone_name);
        Py_DECREF(hex_name);
        Py_DECREF(timestamp_name);
        Py_DECREF(total_seconds_name);
        return NULL;
    }

    is_infinite_name = PyUnicode_InternFromString("is_infinite");
    if (is_infinite_name == NULL) {
        Py_DECREF(astimezone_name);
        Py_DECREF(hex_name);
        Py_DECREF(timestamp_name);
        Py_DECREF(total_seconds_name);
        Py_DECREF(utcoffset_name);
        return NULL;
    }

    is_nan_name = PyUnicode_InternFromString("is_nan");
    if (is_infinite_name == NULL) {
        Py_DECREF(astimezone_name);
        Py_DECREF(hex_name);
        Py_DECREF(timestamp_name);
        Py_DECREF(total_seconds_name);
        Py_DECREF(utcoffset_name);
        Py_DECREF(is_infinite_name);
        return NULL;
    }

    minus_inf_string_value = PyUnicode_InternFromString("-Infinity");
    if (minus_inf_string_value == NULL) {
        Py_DECREF(astimezone_name);
        Py_DECREF(hex_name);
        Py_DECREF(timestamp_name);
        Py_DECREF(total_seconds_name);
        Py_DECREF(utcoffset_name);
        Py_DECREF(is_infinite_name);
        Py_DECREF(is_nan_name);
        return NULL;
    }

    nan_string_value = PyUnicode_InternFromString("nan");
    if (nan_string_value == NULL) {
        Py_DECREF(astimezone_name);
        Py_DECREF(hex_name);
        Py_DECREF(timestamp_name);
        Py_DECREF(total_seconds_name);
        Py_DECREF(utcoffset_name);
        Py_DECREF(is_infinite_name);
        Py_DECREF(is_nan_name);
        Py_DECREF(minus_inf_string_value);
        return NULL;
    }

    plus_inf_string_value = PyUnicode_InternFromString("+Infinity");
    if (plus_inf_string_value == NULL) {
        Py_DECREF(astimezone_name);
        Py_DECREF(hex_name);
        Py_DECREF(timestamp_name);
        Py_DECREF(total_seconds_name);
        Py_DECREF(utcoffset_name);
        Py_DECREF(is_infinite_name);
        Py_DECREF(is_nan_name);
        Py_DECREF(minus_inf_string_value);
        Py_DECREF(nan_string_value);
        return NULL;
    }

    PyDateTime_IMPORT;

    PyObject* datetimeModule = PyImport_ImportModule("datetime");
    if (datetimeModule == NULL) {
        Py_DECREF(astimezone_name);
        Py_DECREF(hex_name);
        Py_DECREF(timestamp_name);
        Py_DECREF(total_seconds_name);
        Py_DECREF(utcoffset_name);
        Py_DECREF(is_infinite_name);
        Py_DECREF(is_nan_name);
        Py_DECREF(minus_inf_string_value);
        Py_DECREF(nan_string_value);
        Py_DECREF(plus_inf_string_value);
        return NULL;
    }

    PyObject* decimalModule = PyImport_ImportModule("decimal");
    if (decimalModule == NULL) {
        return NULL;
    }

    decimal_type = PyObject_GetAttrString(decimalModule, "Decimal");
    Py_DECREF(decimalModule);

    if (decimal_type == NULL) {
        Py_DECREF(astimezone_name);
        Py_DECREF(hex_name);
        Py_DECREF(timestamp_name);
        Py_DECREF(total_seconds_name);
        Py_DECREF(utcoffset_name);
        Py_DECREF(is_infinite_name);
        Py_DECREF(is_nan_name);
        Py_DECREF(minus_inf_string_value);
        Py_DECREF(nan_string_value);
        Py_DECREF(plus_inf_string_value);
        Py_DECREF(datetimeModule);
        return NULL;
    }

    timezone_type = PyObject_GetAttrString(datetimeModule, "timezone");
    Py_DECREF(datetimeModule);

    if (timezone_type == NULL) {
        Py_DECREF(astimezone_name);
        Py_DECREF(hex_name);
        Py_DECREF(timestamp_name);
        Py_DECREF(total_seconds_name);
        Py_DECREF(utcoffset_name);
        Py_DECREF(is_infinite_name);
        Py_DECREF(is_nan_name);
        Py_DECREF(minus_inf_string_value);
        Py_DECREF(nan_string_value);
        Py_DECREF(plus_inf_string_value);
        Py_DECREF(decimal_type);
        return NULL;
    }

    timezone_utc = PyObject_GetAttrString(timezone_type, "utc");
    if (timezone_utc == NULL) {
        Py_DECREF(astimezone_name);
        Py_DECREF(hex_name);
        Py_DECREF(timestamp_name);
        Py_DECREF(total_seconds_name);
        Py_DECREF(utcoffset_name);
        Py_DECREF(is_infinite_name);
        Py_DECREF(is_nan_name);
        Py_DECREF(minus_inf_string_value);
        Py_DECREF(nan_string_value);
        Py_DECREF(plus_inf_string_value);
        Py_DECREF(decimal_type);
        Py_DECREF(timezone_type);
        return NULL;
    }

    PyObject* uuidModule = PyImport_ImportModule("uuid");
    if (uuidModule == NULL) {
        Py_DECREF(astimezone_name);
        Py_DECREF(hex_name);
        Py_DECREF(timestamp_name);
        Py_DECREF(total_seconds_name);
        Py_DECREF(utcoffset_name);
        Py_DECREF(is_infinite_name);
        Py_DECREF(is_nan_name);
        Py_DECREF(minus_inf_string_value);
        Py_DECREF(nan_string_value);
        Py_DECREF(plus_inf_string_value);
        Py_DECREF(decimal_type);
        Py_DECREF(timezone_type);
        Py_DECREF(timezone_utc);
        return NULL;
    }

    uuid_type = PyObject_GetAttrString(uuidModule, "UUID");
    Py_DECREF(uuidModule);

    if (uuid_type == NULL) {
        Py_DECREF(astimezone_name);
        Py_DECREF(hex_name);
        Py_DECREF(timestamp_name);
        Py_DECREF(total_seconds_name);
        Py_DECREF(utcoffset_name);
        Py_DECREF(is_infinite_name);
        Py_DECREF(is_nan_name);
        Py_DECREF(minus_inf_string_value);
        Py_DECREF(nan_string_value);
        Py_DECREF(plus_inf_string_value);
        Py_DECREF(decimal_type);
        Py_DECREF(timezone_type);
        Py_DECREF(timezone_utc);
        Py_DECREF(uuid_type);
        return NULL;
    }

    PyObject* m;

    m = PyModule_Create(&module);
    if (m == NULL) {
        return NULL;
    }

    PyModule_AddIntConstant(m, "DM_NONE", DM_NONE);
    PyModule_AddIntConstant(m, "DM_ISO8601", DM_ISO8601);
    PyModule_AddIntConstant(m, "DM_UNIX_TIME", DM_UNIX_TIME);
    PyModule_AddIntConstant(m, "DM_ONLY_SECONDS", DM_ONLY_SECONDS);
    PyModule_AddIntConstant(m, "DM_IGNORE_TZ", DM_IGNORE_TZ);
    PyModule_AddIntConstant(m, "DM_NAIVE_IS_UTC", DM_NAIVE_IS_UTC);
    PyModule_AddIntConstant(m, "DM_SHIFT_TO_UTC", DM_SHIFT_TO_UTC);

    PyModule_AddIntConstant(m, "UM_NONE", UM_NONE);
    PyModule_AddIntConstant(m, "UM_HEX", UM_HEX);
    PyModule_AddIntConstant(m, "UM_CANONICAL", UM_CANONICAL);

    PyModule_AddIntConstant(m, "NM_NONE", NM_NONE);
    PyModule_AddIntConstant(m, "NM_NAN", NM_NAN);
    PyModule_AddIntConstant(m, "NM_DECIMAL", NM_DECIMAL);
    PyModule_AddIntConstant(m, "NM_NATIVE", NM_NATIVE);

    PyModule_AddStringConstant(m, "__version__",
                               PYTHON_RAPIDJSON_VERSION);
    PyModule_AddStringConstant(m, "__author__",
                               PYTHON_RAPIDJSON_AUTHOR
                               " <" PYTHON_RAPIDJSON_AUTHOR_EMAIL ">");

    PyModule_AddStringConstant(m, "__rapidjson_version__",
                               RAPIDJSON_VERSION_STRING);

    return m;
}
