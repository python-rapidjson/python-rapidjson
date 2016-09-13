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


static PyObject* rapidjson_decimal_type = NULL;
static PyObject* rapidjson_timezone_type = NULL;
static PyObject* rapidjson_timezone_utc = NULL;
static PyObject* rapidjson_uuid_type = NULL;

struct HandlerContext {
    PyObject* object;
    const char* key;
    SizeType keyLength;
    bool isObject;
};

enum DatetimeMode {
    DATETIME_MODE_NONE = 0,
    DATETIME_MODE_ISO8601 = 1,
    DATETIME_MODE_ISO8601_IGNORE_TZ = 2,
    DATETIME_MODE_ISO8601_UTC = 3
};

static int
days_per_month(int year, int month) {
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
    UUID_MODE_NONE = 0,
    UUID_MODE_CANONICAL = 1, // only 4-dashed 32 hex chars: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
    UUID_MODE_HEX = 2        // canonical OR 32 hex chars
};

struct PyHandler {
    int useDecimal;
    int allowNan;
    PyObject* root;
    PyObject* objectHook;
    DatetimeMode datetimeMode;
    UuidMode uuidMode;
    std::vector<HandlerContext> stack;

    PyHandler(int ud, PyObject* hook, int an, DatetimeMode dm, UuidMode um)
    : useDecimal(ud), allowNan(an), root(NULL), objectHook(hook), datetimeMode(dm),
      uuidMode(um)
    {
        stack.reserve(128);
    }

    bool Handle(PyObject* value) {
        if (root) {
            const HandlerContext& current = stack.back();

            if (current.isObject) {
                PyObject* key = PyUnicode_FromStringAndSize(current.key, current.keyLength);
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
            Py_DECREF(value);
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
            Py_DECREF(dict);
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
        // In case the dict is returned from the function.
        if (value == dict)
            Py_INCREF(value);

        if (value == NULL)
            return false;

        if (!stack.empty()) {
            const HandlerContext& current = stack.back();

            if (current.isObject) {
                PyObject* key = PyUnicode_FromStringAndSize(current.key, current.keyLength);
                if (key == NULL) {
                    Py_DECREF(value);
                    return false;
                }

                int rc = PyDict_SetItem(current.object, key, value);
                Py_DECREF(key);
                Py_DECREF(value);

                if (rc == -1)
                    return false;
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

        Py_DECREF(dict);
        return true;
    }

    bool StartArray() {
        PyObject* list = PyList_New(0);
        if (list == NULL) {
            return false;
        }

        if (!Handle(list)) {
            Py_DECREF(list);
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
        if (!allowNan) {
            PyErr_SetString(PyExc_ValueError, "Out of range float values are not JSON compliant");
            return false;
        }

        PyObject* str = PyUnicode_FromStringAndSize("nan", 3);
        if (str == NULL)
            return false;

        PyObject* value;
        if (!useDecimal)
            value = PyFloat_FromString(str);
        else
            value = PyObject_CallFunctionObjArgs(rapidjson_decimal_type, str, NULL);

        Py_DECREF(str);

        if (value == NULL)
            return false;

        return HandleSimpleType(value);
    }

    bool Infinity(bool minus) {
        if (!allowNan) {
            PyErr_SetString(PyExc_ValueError, "Out of range float values are not JSON compliant");
            return false;
        }

        PyObject* str;

        if (minus)
            str = PyUnicode_FromStringAndSize("-Infinity", 9);
        else
            str = PyUnicode_FromStringAndSize("Infinity", 8);

        if (str == NULL)
            return false;

        PyObject* value;
        if (!useDecimal)
            value = PyFloat_FromString(str);
        else
            value = PyObject_CallFunctionObjArgs(rapidjson_decimal_type, str, NULL);

        Py_DECREF(str);

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

            if (!useDecimal) {
                value = PyFloat_FromString(pystr);
            } else {
                value = PyObject_CallFunctionObjArgs(rapidjson_decimal_type, pystr, NULL);
            }

            Py_DECREF(pystr);
        } else {
            char zstr[length + 1];

            strncpy(zstr, str, length);
            zstr[length] = '\0';

            value = PyLong_FromString(zstr, NULL, 10);
        }

        if (value == NULL) {
            PyErr_SetString(PyExc_ValueError,
                            isFloat ? "Invalid float value" : "Invalid integer value");
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
                    res = str[8] == '-';
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
                        if (res && (length == 20 || length == 24 || length == 27)) {
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
                    || day > days_per_month(year, month)
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
                usecs = digit(9)*100000
                    + digit(10) * 10000
                    + digit(11) * 1000;
                if (length == 15 || length == 16 || length == 21)
                    usecs += digit(12)*100
                        + digit(13) * 10
                        + digit(14);
            }
            if (datetimeMode == DATETIME_MODE_ISO8601_IGNORE_TZ
                || length == 8 || length == 12 || length == 15)
                value = PyTime_FromTime(hours, mins, secs, usecs);
            else if (length == 9 || length == 13 || length == 16)
                value = PyDateTimeAPI->Time_FromTime(
                    hours, mins, secs, usecs, rapidjson_timezone_utc,
                    PyDateTimeAPI->TimeType);
            else /* if (length == 14 || length == 18 || length == 21) */ {
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
                        rapidjson_timezone_type, offset, NULL);
                    Py_DECREF(offset);
                    if (tz == NULL)
                        value = NULL;
                    else {
                        value = PyDateTimeAPI->Time_FromTime(
                            hours, mins, secs, usecs, tz,
                            PyDateTimeAPI->TimeType);
                        Py_DECREF(tz);

                        if (value != NULL && datetimeMode == DATETIME_MODE_ISO8601_UTC) {
                            PyObject* asUTC = PyObject_CallMethod(value, "astimezone", "O",
                                                                  rapidjson_timezone_utc);

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
            if (datetimeMode == DATETIME_MODE_ISO8601_IGNORE_TZ
                || length == 19 || length == 23 || length == 26)
                value = PyDateTime_FromDateAndTime(
                    year, month, day, hours, mins, secs, usecs);
            else if (length == 20 || length == 24 || length == 27)
                value = PyDateTimeAPI->DateTime_FromDateAndTime(
                    year, month, day, hours, mins, secs, usecs,
                    rapidjson_timezone_utc, PyDateTimeAPI->DateTimeType);
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
                        rapidjson_timezone_type, offset, NULL);
                    Py_DECREF(offset);
                    if (tz == NULL)
                        value = NULL;
                    else {
                        value = PyDateTimeAPI->DateTime_FromDateAndTime(
                            year, month, day, hours, mins, secs, usecs,
                            tz, PyDateTimeAPI->DateTimeType);
                        Py_DECREF(tz);

                        if (value != NULL && datetimeMode == DATETIME_MODE_ISO8601_UTC) {
                            PyObject* asUTC = PyObject_CallMethod(value, "astimezone", "O",
                                                                  rapidjson_timezone_utc);

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
        if (uuidMode == UUID_MODE_HEX && length == 32) {
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

        PyObject* value = PyObject_CallFunctionObjArgs(rapidjson_uuid_type, pystr, NULL);
        Py_DECREF(pystr);

        if (value == NULL)
            return false;
        else
            return HandleSimpleType(value);
    }

    bool String(const char* str, SizeType length, bool copy) {
        PyObject* value;

        if (datetimeMode != DATETIME_MODE_NONE && IsIso8601(str, length))
            return HandleIso8601(str, length);

        if (uuidMode != UUID_MODE_NONE && IsUuid(str, length))
            return HandleUuid(str, length);

        value = PyUnicode_FromStringAndSize(str, length);
        return HandleSimpleType(value);
    }
};

static PyObject*
rapidjson_loads(PyObject* self, PyObject* args, PyObject* kwargs)
{
    /* Converts a JSON encoded string to a Python object. */

    PyObject* jsonObject;
    PyObject* objectHook = NULL;
    int useDecimal = 0;
    int allowNan = 1;
    int nativeNumbers = 0;
    PyObject* datetimeModeObj = NULL;
    DatetimeMode datetimeMode = DATETIME_MODE_NONE;
    PyObject* uuidModeObj = NULL;
    UuidMode uuidMode = UUID_MODE_NONE;

    static char const * kwlist[] = {
        "s",
        "object_hook",
        "use_decimal",
        "allow_nan",
        "native_numbers",
        "datetime_mode",
        "uuid_mode",
        NULL
    };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OpppOO:rapidjson.loads",
                                     (char **) kwlist,
                                     &jsonObject,
                                     &objectHook,
                                     &useDecimal,
                                     &allowNan,
                                     &nativeNumbers,
                                     &datetimeModeObj,
                                     &uuidModeObj))

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

    if (datetimeModeObj && PyLong_Check(datetimeModeObj)) {
        datetimeMode = (DatetimeMode) PyLong_AsLong(datetimeModeObj);
        if (datetimeMode < DATETIME_MODE_NONE || datetimeMode > DATETIME_MODE_ISO8601_UTC) {
            PyErr_SetString(PyExc_ValueError, "Invalid date_time");
            return NULL;
        }
    }

    if (uuidModeObj && PyLong_Check(uuidModeObj)) {
        uuidMode = (UuidMode) PyLong_AsLong(uuidModeObj);
        if (uuidMode < UUID_MODE_NONE || uuidMode > UUID_MODE_HEX) {
            PyErr_SetString(PyExc_ValueError, "Invalid uuid_time");
            return NULL;
        }
    }

    char* jsonStrCopy = (char*) malloc(sizeof(char) * (jsonStrLen+1));
    memcpy(jsonStrCopy, jsonStr, jsonStrLen+1);

    PyHandler handler(useDecimal, objectHook, allowNan, datetimeMode, uuidMode);
    Reader reader;
    InsituStringStream ss(jsonStrCopy);

    if (allowNan)
        if (nativeNumbers)
            reader.Parse<kParseInsituFlag |
                         kParseNanAndInfFlag>(ss, handler);
        else
            reader.Parse<kParseInsituFlag |
                         kParseNumbersAsStringsFlag |
                         kParseNanAndInfFlag>(ss, handler);
    else
        if (nativeNumbers)
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
    PyObject* object;
    PyObject* decref;
    unsigned level;
    bool isObject;

    WriterContext(const char* k, PyObject* o, bool isO, int l, PyObject* d=NULL)
    : key(k), object(o), decref(d), level(l), isObject(isO)
    {}
};

struct DictItem {
    char* key_str;
    PyObject* item;

    DictItem(char* k, PyObject* i)
    : key_str(k), item(i)
    {}

    bool operator<(const DictItem& other) const {
        return strcmp(other.key_str, this->key_str) < 0;
    }
};

static const int MAX_RECURSION_DEPTH = 2048;


template<typename WriterT, typename BufferT>
static PyObject*
rapidjson_dumps_internal(
    WriterT* writer,
    BufferT* buf,
    PyObject* value,
    int skipKeys,
    int allowNan,
    int nativeNumbers,
    PyObject* defaultFn,
    int sortKeys,
    int useDecimal,
    unsigned maxRecursionDepth,
    DatetimeMode datetimeMode,
    UuidMode uuidMode)
{
    int isDec;
    std::vector<WriterContext> stack;
    stack.reserve(128);

    stack.push_back(WriterContext(NULL, value, false, 0));

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
                writer->Key(current.key);
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
        else if (useDecimal && (isDec = PyObject_IsInstance(object, rapidjson_decimal_type))) {
            if (isDec == -1)
                return NULL;

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
            if (nativeNumbers) {
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
                if (allowNan)
                    writer->RawValue("NaN", 3, kNumberType);
                else {
                    PyErr_SetString(PyExc_ValueError, "Out of range float values are not JSON compliant");
                    return NULL;
                }
            } else if (Py_IS_INFINITY(d)) {
                if (!allowNan) {
                    PyErr_SetString(PyExc_ValueError, "Out of range float values are not JSON compliant");
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
            stack.push_back(WriterContext(NULL, NULL, false, currentLevel));

            Py_ssize_t size = PyList_GET_SIZE(object);

            for (Py_ssize_t i = size - 1; i >= 0; --i) {
                PyObject* item = PyList_GET_ITEM(object, i);
                stack.push_back(WriterContext(NULL, item, false, nextLevel));
            }
        }
        else if (PyTuple_Check(object)) {
            writer->StartArray();
            stack.push_back(WriterContext(NULL, NULL, false, currentLevel));

            Py_ssize_t size = PyTuple_GET_SIZE(object);

            for (Py_ssize_t i = size - 1; i >= 0; --i) {
                PyObject* item = PyTuple_GET_ITEM(object, i);
                stack.push_back(WriterContext(NULL, item, false, nextLevel));
            }
        }
        else if (PyDict_Check(object)) {
            writer->StartObject();
            stack.push_back(WriterContext(NULL, NULL, true, currentLevel));

            Py_ssize_t pos = 0;
            PyObject* key;
            PyObject* item;

            if (!sortKeys) {
                while (PyDict_Next(object, &pos, &key, &item)) {
                    if (PyUnicode_Check(key)) {
                        char* key_str = PyUnicode_AsUTF8(key);
                        stack.push_back(WriterContext(NULL, item, false, nextLevel));
                        stack.push_back(WriterContext(key_str, NULL, false, nextLevel));
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
                        char* key_str = PyUnicode_AsUTF8(key);
                        items.push_back(DictItem(key_str, item));
                    }
                    else if (!skipKeys) {
                        PyErr_SetString(PyExc_TypeError, "keys must be a string");
                        goto error;
                    }
                }

                std::sort(items.begin(), items.end());

                std::vector<DictItem>::const_iterator iter = items.begin();
                for (; iter != items.end(); ++iter) {
                    stack.push_back(WriterContext(NULL, iter->item, false, nextLevel));
                    stack.push_back(WriterContext(iter->key_str, NULL, false, nextLevel));
                }
            }
        }
        else if (datetimeMode != DATETIME_MODE_NONE
                 && (PyTime_Check(object) || PyDateTime_Check(object))) {
            int year, month, day, hour, min, sec, microsec;
            PyObject* dtObject = object;
            PyObject* asUTC = NULL;

            const int ISOFORMAT_LEN = 40;
            char isoformat[ISOFORMAT_LEN];
            memset(isoformat, 0, ISOFORMAT_LEN);

            const int TIMEZONE_LEN = 10;
            char timezone[TIMEZONE_LEN];
            memset(timezone, 0, TIMEZONE_LEN);

            if (datetimeMode != DATETIME_MODE_ISO8601_IGNORE_TZ
                && PyObject_HasAttrString(object, "utcoffset")) {
                PyObject* utcOffset = PyObject_CallMethod(object, "utcoffset", NULL);

                if (!utcOffset)
                    goto error;

                if (utcOffset != Py_None) {
                    if (datetimeMode == DATETIME_MODE_ISO8601_UTC && PyObject_IsTrue(utcOffset)) {
                        asUTC = PyObject_CallMethod(object, "astimezone", "O", rapidjson_timezone_utc);

                        if (asUTC == NULL) {
                            Py_DECREF(utcOffset);
                            goto error;
                        }

                        dtObject = asUTC;
                        strcpy(timezone, "+00:00");
                    } else {
                        PyObject* daysObj = PyObject_GetAttrString(utcOffset, "days");
                        PyObject* secondsObj = PyObject_GetAttrString(utcOffset, "seconds");

                        if (daysObj && secondsObj) {
                            int days = PyLong_AsLong(daysObj);
                            int seconds = PyLong_AsLong(secondsObj);

                            int total_seconds = days * 24 * 3600 + seconds;

                            char sign = '+';
                            if (total_seconds < 0) {
                                sign = '-';
                                total_seconds = -total_seconds;
                            }

                            int tz_hour = total_seconds / 3600;
                            int tz_min = (total_seconds % 3600) / 60;

                            snprintf(timezone, TIMEZONE_LEN-1, "%c%02d:%02d", sign, tz_hour, tz_min);
                        }

                        Py_XDECREF(daysObj);
                        Py_XDECREF(secondsObj);
                    }
                }
                Py_DECREF(utcOffset);
            }

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
                             timezone);
                } else {
                    snprintf(isoformat,
                             ISOFORMAT_LEN-1,
                             "%04d-%02d-%02dT%02d:%02d:%02d%s",
                             year, month, day,
                             hour, min, sec,
                             timezone);
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
                             timezone);
                } else {
                    snprintf(isoformat,
                             ISOFORMAT_LEN-1,
                             "%02d:%02d:%02d%s",
                             hour, min, sec,
                             timezone);
                }
            }
            Py_XDECREF(asUTC);
            writer->String(isoformat);
        }
        else if (datetimeMode != DATETIME_MODE_NONE && PyDate_Check(object)) {
            int year = PyDateTime_GET_YEAR(object);
            int month = PyDateTime_GET_MONTH(object);
            int day = PyDateTime_GET_DAY(object);

            const int ISOFORMAT_LEN = 12;
            char isoformat[ISOFORMAT_LEN];
            memset(isoformat, 0, ISOFORMAT_LEN);

            snprintf(isoformat, ISOFORMAT_LEN-1, "%04d-%02d-%02d", year, month, day);
            writer->String(isoformat);
        }
        else if (uuidMode != UUID_MODE_NONE
                 && PyObject_TypeCheck(object, (PyTypeObject *) rapidjson_uuid_type)) {
            PyObject* retval;
            if (uuidMode == UUID_MODE_CANONICAL)
                retval = PyObject_Str(object);
            else
                retval = PyObject_GetAttrString(object, "hex");
            if (retval == NULL)
                goto error;

            // Decref the return value once it's done being dumped to a string.
            stack.push_back(WriterContext(NULL, NULL, false, currentLevel, retval));
            stack.push_back(WriterContext(NULL, retval, false, currentLevel));
        }
        else if (defaultFn) {
            PyObject* retval = PyObject_CallFunctionObjArgs(defaultFn, object, NULL);
            if (retval == NULL)
                goto error;

            // Decref the return value once it's done being dumped to a string.
            stack.push_back(WriterContext(NULL, NULL, false, currentLevel, retval));
            stack.push_back(WriterContext(NULL, retval, false, currentLevel));
        }
        else {
            PyObject* repr = PyObject_Repr(object);
            PyErr_Format(PyExc_TypeError, "%s is not JSON serializable", PyUnicode_AsUTF8(repr));
            Py_XDECREF(repr);
            goto error;
        }
    }

    return PyUnicode_FromString(buf->GetString());

error:
    return NULL;
}


#define RAPIDJSON_DUMPS_INTERNAL_CALL \
    rapidjson_dumps_internal( \
        &writer, \
        &buf, \
        value, \
        skipKeys, \
        allowNan, \
        nativeNumbers, \
        defaultFn, \
        sortKeys, \
        useDecimal, \
        maxRecursionDepth, \
        datetimeMode, \
        uuidMode)


static PyObject*
rapidjson_dumps(PyObject* self, PyObject* args, PyObject* kwargs)
{
    /* Converts a Python object to a JSON-encoded string. */

    PyObject* value;
    int skipKeys = 0;
    int ensureAscii = 1;
    int allowNan = 1;
    int nativeNumbers = 0;
    PyObject* indent = NULL;
    PyObject* defaultFn = NULL;
    int sortKeys = 0;
    int useDecimal = 0;
    unsigned maxRecursionDepth = MAX_RECURSION_DEPTH;
    PyObject* datetimeModeObj = NULL;
    DatetimeMode datetimeMode = DATETIME_MODE_NONE;
    PyObject* uuidModeObj = NULL;
    UuidMode uuidMode = UUID_MODE_NONE;

    bool prettyPrint = false;
    const char indentChar = ' ';
    unsigned indentCharCount = 4;

    static char const * kwlist[] = {
        "obj",
        "skipkeys",
        "ensure_ascii",
        "allow_nan",
        "native_numbers",
        "indent",
        "default",
        "sort_keys",
        "use_decimal",
        "max_recursion_depth",
        "datetime_mode",
        "uuid_mode",
        NULL
    };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|ppppOOppIOO:rapidjson.dumps",
                                     (char **) kwlist,
                                     &value,
                                     &skipKeys,
                                     &ensureAscii,
                                     &allowNan,
                                     &nativeNumbers,
                                     &indent,
                                     &defaultFn,
                                     &sortKeys,
                                     &useDecimal,
                                     &maxRecursionDepth,
                                     &datetimeModeObj,
                                     &uuidModeObj))
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

    if (datetimeModeObj && PyLong_Check(datetimeModeObj)) {
        datetimeMode = (DatetimeMode) PyLong_AsLong(datetimeModeObj);
        if (datetimeMode < DATETIME_MODE_NONE || datetimeMode > DATETIME_MODE_ISO8601_UTC) {
            PyErr_SetString(PyExc_ValueError, "Invalid date_time");
            return NULL;
        }
    }

    if (uuidModeObj && PyLong_Check(uuidModeObj)) {
        uuidMode = (UuidMode) PyLong_AsLong(uuidModeObj);
        if (uuidMode < UUID_MODE_NONE || uuidMode > UUID_MODE_HEX) {
            PyErr_SetString(PyExc_ValueError, "Invalid uuid_time");
            return NULL;
        }
    }

    if (!prettyPrint) {
        if (ensureAscii) {
            GenericStringBuffer<ASCII<> > buf;
            Writer<GenericStringBuffer<ASCII<> >, UTF8<>, ASCII<> > writer(buf);
            return RAPIDJSON_DUMPS_INTERNAL_CALL;
        }
        else {
            StringBuffer buf;
            Writer<StringBuffer> writer(buf);
            return RAPIDJSON_DUMPS_INTERNAL_CALL;
        }
    }
    else if (ensureAscii) {
        GenericStringBuffer<ASCII<> > buf;
        PrettyWriter<GenericStringBuffer<ASCII<> >, UTF8<>, ASCII<> > writer(buf);
        writer.SetIndent(indentChar, indentCharCount);
        return RAPIDJSON_DUMPS_INTERNAL_CALL;
    }
    else {
        StringBuffer buf;
        PrettyWriter<StringBuffer> writer(buf);
        writer.SetIndent(indentChar, indentCharCount);
        return RAPIDJSON_DUMPS_INTERNAL_CALL;
    }
}



static PyMethodDef
rapidjson_functions[] = {
    {"loads", (PyCFunction) rapidjson_loads, METH_VARARGS | METH_KEYWORDS, rapidjson_loads_docstring},
    {"dumps", (PyCFunction) rapidjson_dumps, METH_VARARGS | METH_KEYWORDS, rapidjson_dumps_docstring},
    {NULL, NULL, 0, NULL} /* sentinel */
};

static PyModuleDef rapidjson_module = {
    PyModuleDef_HEAD_INIT,
    "rapidjson",
    rapidjson_module_docstring,
    -1,
    rapidjson_functions
};

PyMODINIT_FUNC
PyInit_rapidjson()
{
    PyDateTime_IMPORT;

    PyObject* datetimeModule = PyImport_ImportModule("datetime");
    if (datetimeModule == NULL)
        return NULL;

    rapidjson_timezone_type = PyObject_GetAttrString(datetimeModule, "timezone");
    Py_DECREF(datetimeModule);

    if (rapidjson_timezone_type == NULL)
        return NULL;

    rapidjson_timezone_utc = PyObject_GetAttrString(rapidjson_timezone_type, "utc");
    if (rapidjson_timezone_utc == NULL) {
        Py_DECREF(rapidjson_timezone_type);
        return NULL;
    }

    PyObject* uuidModule = PyImport_ImportModule("uuid");
    if (uuidModule == NULL) {
        Py_DECREF(rapidjson_timezone_type);
        Py_DECREF(rapidjson_timezone_utc);
        return NULL;
    }

    rapidjson_uuid_type = PyObject_GetAttrString(uuidModule, "UUID");
    Py_DECREF(uuidModule);

    if (rapidjson_uuid_type == NULL) {
        Py_DECREF(rapidjson_timezone_type);
        Py_DECREF(rapidjson_timezone_utc);
        return NULL;
    }

    PyObject* decimalModule = PyImport_ImportModule("decimal");
    if (decimalModule == NULL) {
        Py_DECREF(rapidjson_timezone_type);
        Py_DECREF(rapidjson_timezone_utc);
        Py_DECREF(rapidjson_uuid_type);
        return NULL;
    }

    rapidjson_decimal_type = PyObject_GetAttrString(decimalModule, "Decimal");
    Py_DECREF(decimalModule);

    if (rapidjson_decimal_type == NULL) {
        Py_DECREF(rapidjson_timezone_type);
        Py_DECREF(rapidjson_timezone_utc);
        Py_DECREF(rapidjson_uuid_type);
        return NULL;
    }

    PyObject* module;

    module = PyModule_Create(&rapidjson_module);
    if (module == NULL) {
        Py_DECREF(rapidjson_timezone_type);
        Py_DECREF(rapidjson_timezone_utc);
        Py_DECREF(rapidjson_decimal_type);
        Py_DECREF(rapidjson_uuid_type);
        return NULL;
    }

    PyModule_AddIntConstant(module, "DATETIME_MODE_NONE", DATETIME_MODE_NONE);
    PyModule_AddIntConstant(module, "DATETIME_MODE_ISO8601", DATETIME_MODE_ISO8601);
    PyModule_AddIntConstant(module, "DATETIME_MODE_ISO8601_IGNORE_TZ", DATETIME_MODE_ISO8601_IGNORE_TZ);
    PyModule_AddIntConstant(module, "DATETIME_MODE_ISO8601_UTC", DATETIME_MODE_ISO8601_UTC);

    PyModule_AddIntConstant(module, "UUID_MODE_NONE", UUID_MODE_NONE);
    PyModule_AddIntConstant(module, "UUID_MODE_HEX", UUID_MODE_HEX);
    PyModule_AddIntConstant(module, "UUID_MODE_CANONICAL", UUID_MODE_CANONICAL);

    PyModule_AddStringConstant(module, "__version__", PYTHON_RAPIDJSON_VERSION);
    PyModule_AddStringConstant(
        module,
        "__author__",
        PYTHON_RAPIDJSON_AUTHOR " <" PYTHON_RAPIDJSON_AUTHOR_EMAIL ">");

    return module;
}
