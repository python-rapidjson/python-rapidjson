// -*- coding: utf-8 -*-
// :Project:   python-rapidjson -- Python extension module
// :Author:    Ken Robbins <ken@kenrobbins.com>
// :License:   MIT License
// :Copyright: © 2015 Ken Robbins
// :Copyright: © 2015, 2016, 2017 Lele Gaifax
//

#include <Python.h>
#include <datetime.h>
#include <structmember.h>

#include <algorithm>
#include <string>
#include <vector>

#include "rapidjson/reader.h"
#include "rapidjson/schema.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/error/en.h"


using namespace rapidjson;


static PyObject* decimal_type = NULL;
static PyObject* timezone_type = NULL;
static PyObject* timezone_utc = NULL;
static PyObject* uuid_type = NULL;


/* These are the names of oftenly used methods or literal values, interned in the module
   initialization function, to avoid repeated creation/destruction of PyUnicode values
   from plain C strings.

   We cannot use _Py_IDENTIFIER() because that upsets the GNU C++ compiler in -pedantic
   mode. */

static PyObject* astimezone_name = NULL;
static PyObject* hex_name = NULL;
static PyObject* timestamp_name = NULL;
static PyObject* total_seconds_name = NULL;
static PyObject* utcoffset_name = NULL;
static PyObject* is_infinite_name = NULL;
static PyObject* is_nan_name = NULL;
static PyObject* start_object_name = NULL;
static PyObject* end_object_name = NULL;
static PyObject* default_name = NULL;
static PyObject* end_array_name = NULL;
static PyObject* string_name = NULL;

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
    DM_ISO8601 = 1<<0,      // Bidirectional ISO8601 for datetimes, dates and times
    DM_UNIX_TIME = 1<<1,    // Serialization only, "Unix epoch"-based number of seconds
    // Options
    DM_ONLY_SECONDS = 1<<4, // Truncate values to the whole second, ignoring micro seconds
    DM_IGNORE_TZ = 1<<5,    // Ignore timezones
    DM_NAIVE_IS_UTC = 1<<6, // Assume naive datetime are in UTC timezone
    DM_SHIFT_TO_UTC = 1<<7  // Shift to/from UTC
};


#define DATETIME_MODE_FORMATS_MASK 0x0f // 0b00001111 in C++14


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
    UM_CANONICAL = 1<<0, // 4-dashed 32 hex chars: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
    UM_HEX = 1<<1        // canonical OR 32 hex chars in a row
};


enum NumberMode {
    NM_NONE = 0,
    NM_NAN = 1<<0,     // allow "not-a-number" values
    NM_DECIMAL = 1<<1, // serialize Decimal instances, deserialize floats as Decimal
    NM_NATIVE = 1<<2   // use faster native C library number handling
};


enum ParseMode {
    PM_NONE = 0,
    PM_COMMENTS = 1<<0,       // Allow one-line // ... and multi-line /* ... */ comments
    PM_TRAILING_COMMAS = 1<<1 // allow trailing commas at the end of objects and arrays
};


//////////////////////////
// Forward declarations //
//////////////////////////


static PyObject* do_decode(PyObject* decoder, const char* jsonStr, Py_ssize_t jsonStrlen,
                           PyObject* objectHook, NumberMode numberMode,
                           DatetimeMode datetimeMode, UuidMode uuidMode,
                           ParseMode parseMode);
static PyObject* decoder_call(PyObject* self, PyObject* args, PyObject* kwargs);
static PyObject* decoder_new(PyTypeObject* type, PyObject* args, PyObject* kwargs);


static PyObject* do_encode(PyObject* value, bool skipInvalidKeys, PyObject* defaultFn,
                           bool sortKeys, unsigned maxRecursionDepth, bool ensureAscii,
                           bool prettyPrint, unsigned indent, NumberMode numberMode,
                           DatetimeMode datetimeMode, UuidMode uuidMode);
static PyObject* encoder_call(PyObject* self, PyObject* args, PyObject* kwargs);
static PyObject* encoder_new(PyTypeObject* type, PyObject* args, PyObject* kwargs);


static PyObject* validator_call(PyObject* self, PyObject* args, PyObject* kwargs);
static void validator_dealloc(PyObject* self);
static PyObject* validator_new(PyTypeObject* type, PyObject* args, PyObject* kwargs);


/////////////
// Decoder //
/////////////


struct PyHandler {
    PyObject* decoderStartObject;
    PyObject* decoderEndObject;
    PyObject* decoderEndArray;
    PyObject* decoderString;
    PyObject* root;
    PyObject* objectHook;
    DatetimeMode datetimeMode;
    UuidMode uuidMode;
    NumberMode numberMode;
    std::vector<HandlerContext> stack;

    PyHandler(PyObject* decoder,
              PyObject* hook,
              DatetimeMode dm,
              UuidMode um,
              NumberMode nm)
        : decoderStartObject(NULL),
          decoderEndObject(NULL),
          decoderEndArray(NULL),
          decoderString(NULL),
          root(NULL),
          objectHook(hook),
          datetimeMode(dm),
          uuidMode(um),
          numberMode(nm)
        {
            stack.reserve(128);
            if (decoder != NULL) {
                assert(!objectHook);
                if (PyObject_HasAttr(decoder, start_object_name)) {
                    decoderStartObject = PyObject_GetAttr(decoder, start_object_name);
                    Py_INCREF(decoderStartObject);
                }
                if (PyObject_HasAttr(decoder, end_object_name)) {
                    decoderEndObject = PyObject_GetAttr(decoder, end_object_name);
                    Py_INCREF(decoderEndObject);
                }
                if (PyObject_HasAttr(decoder, end_array_name)) {
                    decoderEndArray = PyObject_GetAttr(decoder, end_array_name);
                    Py_INCREF(decoderEndArray);
                }
                if (PyObject_HasAttr(decoder, string_name)) {
                    decoderString = PyObject_GetAttr(decoder, string_name);
                    Py_INCREF(decoderString);
                }
            }
        }

    ~PyHandler() {
        Py_CLEAR(decoderStartObject);
        Py_CLEAR(decoderEndObject);
        Py_CLEAR(decoderEndArray);
        Py_CLEAR(decoderString);
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

                int rc;
                if (PyDict_Check(current.object))
                    // If it's a standard dictionary, this is +20% faster
                    rc = PyDict_SetItem(current.object, key, value);
                else
                    rc = PyObject_SetItem(current.object, key, value);
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

    bool Key(const char* str, SizeType length, bool copy) {
        HandlerContext& current = stack.back();
        current.key = str;
        current.keyLength = length;

        return true;
    }

    bool StartObject() {
        PyObject* mapping;

        if (decoderStartObject != NULL) {
            mapping = PyObject_CallFunctionObjArgs(decoderStartObject, NULL);
            if (mapping == NULL)
                return false;
            if (!PyMapping_Check(mapping)) {
                PyErr_SetString(PyExc_ValueError,
                                "start_object() must return a mapping instance");
                return false;
            }
            Py_INCREF(mapping);
        } else {
            mapping = PyDict_New();
            if (mapping == NULL) {
                return false;
            }
        }

        if (!Handle(mapping)) {
            return false;
        }

        HandlerContext ctx;
        ctx.isObject = true;
        ctx.object = mapping;
        Py_INCREF(mapping);

        stack.push_back(ctx);

        return true;
    }

    bool EndObject(SizeType member_count) {
        PyObject* mapping = stack.back().object;
        stack.pop_back();

        if (objectHook == NULL && decoderEndObject == NULL) {
            Py_DECREF(mapping);
            return true;
        }

        PyObject* replacement;
        if (decoderEndObject != NULL) {
            replacement = PyObject_CallFunctionObjArgs(decoderEndObject, mapping, NULL);
        } else /* if (objectHook != NULL) */ {
            replacement = PyObject_CallFunctionObjArgs(objectHook, mapping, NULL);
        }

        Py_DECREF(mapping);
        if (replacement == NULL)
            return false;

        Py_INCREF(replacement);

        if (!stack.empty()) {
            const HandlerContext& current = stack.back();

            if (current.isObject) {
                PyObject* key = PyUnicode_FromStringAndSize(current.key,
                                                            current.keyLength);
                if (key == NULL) {
                    Py_DECREF(replacement);
                    return false;
                }

                int rc;
                if (PyDict_Check(current.object))
                    // If it's a standard dictionary, this is +20% faster
                    rc = PyDict_SetItem(current.object, key, replacement);
                else
                    rc = PyObject_SetItem(current.object, key, replacement);
                Py_DECREF(key);

                if (rc == -1) {
                    Py_DECREF(replacement);
                    return false;
                }
            }
            else {
                // Change these to PySequence_Size() and PySequence_SetItem(),
                // should we implement Decoder.start_array()
                Py_ssize_t listLen = PyList_GET_SIZE(current.object);
                int rc = PyList_SetItem(current.object, listLen - 1, replacement);

                if (rc == -1) {
                    Py_DECREF(replacement);
                    return false;
                }
            }
        }
        else {
            Py_DECREF(root);
            root = replacement;
        }

        Py_DECREF(replacement);
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
        Py_INCREF(list);

        stack.push_back(ctx);

        return true;
    }

    bool EndArray(SizeType elementCount) {
        PyObject* sequence = stack.back().object;
        stack.pop_back();

        if (decoderEndArray == NULL) {
            Py_DECREF(sequence);
            return true;
        }

        PyObject* replacement = PyObject_CallFunctionObjArgs(decoderEndArray, sequence,
                                                             NULL);
        Py_DECREF(sequence);
        if (replacement == NULL)
            return false;

        Py_INCREF(replacement);

        if (!stack.empty()) {
            const HandlerContext& current = stack.back();

            if (current.isObject) {
                PyObject* key = PyUnicode_FromStringAndSize(current.key,
                                                            current.keyLength);
                if (key == NULL) {
                    Py_DECREF(replacement);
                    return false;
                }

                int rc;
                if (PyDict_Check(current.object))
                    // If it's a standard dictionary, this is +20% faster
                    rc = PyDict_SetItem(current.object, key, replacement);
                else
                    rc = PyObject_SetItem(current.object, key, replacement);
                Py_DECREF(key);

                if (rc == -1) {
                    Py_DECREF(replacement);
                    return false;
                }
            }
            else {
                // Change these to PySequence_Size() and PySequence_SetItem(),
                // should we implement Decoder.start_array()
                Py_ssize_t listLen = PyList_GET_SIZE(current.object);
                int rc = PyList_SetItem(current.object, listLen - 1, replacement);

                if (rc == -1) {
                    Py_DECREF(replacement);
                    return false;
                }
            }
        }
        else {
            Py_DECREF(root);
            root = replacement;
        }

        Py_DECREF(replacement);
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

        return Handle(value);
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

        return Handle(value);
    }

    bool Null() {
        PyObject* value = Py_None;
        Py_INCREF(value);

        return Handle(value);
    }

    bool Bool(bool b) {
        PyObject* value = b ? Py_True : Py_False;
        Py_INCREF(value);

        return Handle(value);
    }

    bool Int(int i) {
        PyObject* value = PyLong_FromLong(i);
        return Handle(value);
    }

    bool Uint(unsigned i) {
        PyObject* value = PyLong_FromUnsignedLong(i);
        return Handle(value);
    }

    bool Int64(int64_t i) {
        PyObject* value = PyLong_FromLongLong(i);
        return Handle(value);
    }

    bool Uint64(uint64_t i) {
        PyObject* value = PyLong_FromUnsignedLongLong(i);
        return Handle(value);
    }

    bool Double(double d) {
        PyObject* value = PyFloat_FromDouble(d);
        return Handle(value);
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
            return Handle(value);
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
        PyObject* value;
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
            return Handle(value);
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

        PyObject* value = PyObject_CallFunctionObjArgs(uuid_type, pystr, NULL);
        Py_DECREF(pystr);

        if (value == NULL)
            return false;
        else
            return Handle(value);
    }

    bool String(const char* str, SizeType length, bool copy) {
        PyObject* value;

        if (datetimeMode != DM_NONE && IsIso8601(str, length))
            return HandleIso8601(str, length);

        if (uuidMode != UM_NONE && IsUuid(str, length))
            return HandleUuid(str, length);

        value = PyUnicode_FromStringAndSize(str, length);
        if (value == NULL)
            return false;

        if (decoderString != NULL) {
            PyObject* replacement = PyObject_CallFunctionObjArgs(decoderString, value,
                                                                 NULL);
            Py_DECREF(value);
            if (replacement == NULL)
                return false;
            value = replacement;
        }

        return Handle(value);
    }
};


typedef struct {
    PyObject_HEAD
    DatetimeMode datetimeMode;
    UuidMode uuidMode;
    NumberMode numberMode;
    ParseMode parseMode;
} DecoderObject;


PyDoc_STRVAR(loads_docstring,
             "loads(s, object_hook=None, number_mode=None, datetime_mode=None,"
             " uuid_mode=None, parse_mode=None, allow_nan=True)\n"
             "\n"
             "Decode a JSON string into a Python object.");


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
    PyObject* parseModeObj = NULL;
    ParseMode parseMode = PM_NONE;
    int allowNan = -1;
    static char const * kwlist[] = {
        "s",
        "object_hook",
        "number_mode",
        "datetime_mode",
        "uuid_mode",
        "parse_mode",

        /* compatibility with stdlib json */
        "allow_nan",

        NULL
    };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OOOOOp:rapidjson.loads",
                                     (char **) kwlist,
                                     &jsonObject,
                                     &objectHook,
                                     &numberModeObj,
                                     &datetimeModeObj,
                                     &uuidModeObj,
                                     &parseModeObj,
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
                                "Invalid datetime_mode, can deserialize only from"
                                " ISO8601");
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
            int mode = PyLong_AsLong(uuidModeObj);
            if (mode < 0 || mode >= 1<<2) {
                PyErr_SetString(PyExc_ValueError, "Invalid uuid_mode");
                return NULL;
            }
            uuidMode = (UuidMode) mode;
        }
        else {
            PyErr_SetString(PyExc_TypeError,
                            "uuid_mode must be an integer value or None");
            return NULL;
        }
    }

    if (parseModeObj) {
        if (parseModeObj == Py_None)
            parseMode = PM_NONE;
        else if (PyLong_Check(parseModeObj)) {
            int mode = PyLong_AsLong(parseModeObj);
            if (mode < 0 || mode >= 1<<2) {
                PyErr_SetString(PyExc_ValueError, "Invalid parse_mode");
                return NULL;
            }
            parseMode = (ParseMode) mode;
        }
        else {
            PyErr_SetString(PyExc_TypeError,
                            "parse_mode must be an integer value or None");
            return NULL;
        }
    }

    return do_decode(NULL, jsonStr, jsonStrLen, objectHook,
                     numberMode, datetimeMode, uuidMode, parseMode);
}


PyDoc_STRVAR(decoder_doc,
             "Decoder(number_mode=None, datetime_mode=None, uuid_mode=None,"
             " parse_mode=None)\n"
             "\n"
             "Create and return a new Decoder instance.");


static PyMemberDef decoder_members[] = {
    {"datetime_mode",
     T_UINT, offsetof(DecoderObject, datetimeMode), READONLY, "datetime_mode"},
    {"uuid_mode",
     T_UINT, offsetof(DecoderObject, uuidMode), READONLY, "uuid_mode"},
    {"number_mode",
     T_UINT, offsetof(DecoderObject, numberMode), READONLY, "number_mode"},
    {"parse_mode",
     T_UINT, offsetof(DecoderObject, parseMode), READONLY, "parse_mode"},
    {NULL}
};


static PyTypeObject Decoder_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "rapidjson.Decoder",                      /* tp_name */
    sizeof(DecoderObject),                    /* tp_basicsize */
    0,                                        /* tp_itemsize */
    0,                                        /* tp_dealloc */
    0,                                        /* tp_print */
    0,                                        /* tp_getattr */
    0,                                        /* tp_setattr */
    0,                                        /* tp_compare */
    0,                                        /* tp_repr */
    0,                                        /* tp_as_number */
    0,                                        /* tp_as_sequence */
    0,                                        /* tp_as_mapping */
    0,                                        /* tp_hash */
    (ternaryfunc) decoder_call,               /* tp_call */
    0,                                        /* tp_str */
    0,                                        /* tp_getattro */
    0,                                        /* tp_setattro */
    0,                                        /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    decoder_doc,                              /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    0,                                        /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    0,                                        /* tp_iter */
    0,                                        /* tp_iternext */
    0,                                        /* tp_methods */
    decoder_members,                          /* tp_members */
    0,                                        /* tp_getset */
    0,                                        /* tp_base */
    0,                                        /* tp_dict */
    0,                                        /* tp_descr_get */
    0,                                        /* tp_descr_set */
    0,                                        /* tp_dictoffset */
    0,                                        /* tp_init */
    0,                                        /* tp_alloc */
    decoder_new,                              /* tp_new */
    PyObject_Del,                             /* tp_free */
};


#define Decoder_CheckExact(v) (Py_TYPE(v) == &Decoder_Type)
#define Decoder_Check(v) PyObject_TypeCheck(v, &Decoder_Type)


static PyObject*
do_decode(PyObject* decoder, const char* jsonStr, Py_ssize_t jsonStrLen,
          PyObject* objectHook, NumberMode numberMode, DatetimeMode datetimeMode,
          UuidMode uuidMode, ParseMode parseMode)
{
    char* jsonStrCopy = (char*) malloc(sizeof(char) * (jsonStrLen+1));
    memcpy(jsonStrCopy, jsonStr, jsonStrLen+1);

    PyHandler handler(decoder, objectHook, datetimeMode, uuidMode, numberMode);
    Reader reader;
    InsituStringStream ss(jsonStrCopy);

    if (numberMode & NM_NAN)
        if (numberMode & NM_NATIVE)
            if (parseMode & PM_TRAILING_COMMAS)
                if (parseMode & PM_COMMENTS)
                    reader.Parse<kParseInsituFlag |
                                 kParseNanAndInfFlag |
                                 kParseCommentsFlag |
                                 kParseTrailingCommasFlag>(ss, handler);
                else
                    reader.Parse<kParseInsituFlag |
                                 kParseNanAndInfFlag |
                                 kParseTrailingCommasFlag>(ss, handler);
            else if (parseMode & PM_COMMENTS)
                reader.Parse<kParseInsituFlag |
                             kParseNanAndInfFlag |
                             kParseCommentsFlag>(ss, handler);
            else
                reader.Parse<kParseInsituFlag |
                             kParseNanAndInfFlag>(ss, handler);
        else if (parseMode & PM_TRAILING_COMMAS)
            if (parseMode & PM_COMMENTS)
                reader.Parse<kParseInsituFlag |
                             kParseNumbersAsStringsFlag |
                             kParseNanAndInfFlag |
                             kParseCommentsFlag |
                             kParseTrailingCommasFlag>(ss, handler);
            else
                reader.Parse<kParseInsituFlag |
                             kParseNumbersAsStringsFlag |
                             kParseNanAndInfFlag |
                             kParseTrailingCommasFlag>(ss, handler);
        else if (parseMode & PM_COMMENTS)
            reader.Parse<kParseInsituFlag |
                         kParseNumbersAsStringsFlag |
                         kParseNanAndInfFlag |
                         kParseCommentsFlag>(ss, handler);
        else
            reader.Parse<kParseInsituFlag |
                         kParseNumbersAsStringsFlag |
                         kParseNanAndInfFlag>(ss, handler);
    else
        if (numberMode & NM_NATIVE)
            if (parseMode & PM_TRAILING_COMMAS)
                if (parseMode & PM_COMMENTS)
                    reader.Parse<kParseInsituFlag |
                                 kParseCommentsFlag |
                                 kParseTrailingCommasFlag>(ss, handler);
                else
                    reader.Parse<kParseInsituFlag |
                                 kParseTrailingCommasFlag>(ss, handler);
            else if (parseMode & PM_COMMENTS)
                reader.Parse<kParseInsituFlag |
                             kParseCommentsFlag>(ss, handler);
            else
                reader.Parse<kParseInsituFlag>(ss, handler);
        else if (parseMode & PM_TRAILING_COMMAS)
            if (parseMode & PM_COMMENTS)
                reader.Parse<kParseInsituFlag |
                             kParseNumbersAsStringsFlag |
                             kParseCommentsFlag |
                             kParseNumbersAsStringsFlag>(ss, handler);
            else
                reader.Parse<kParseInsituFlag |
                             kParseNumbersAsStringsFlag |
                             kParseTrailingCommasFlag>(ss, handler);
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


static PyObject*
decoder_call(PyObject* self, PyObject* args, PyObject* kwargs)
{
    PyObject* jsonObject;

    if (!PyArg_ParseTuple(args, "O", &jsonObject))
        return NULL;

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

    DecoderObject* d = (DecoderObject*) self;

    return do_decode(self, jsonStr, jsonStrLen, NULL,
                     d->numberMode, d->datetimeMode, d->uuidMode, d->parseMode);
}


static PyObject*
decoder_new(PyTypeObject* type, PyObject* args, PyObject* kwargs)
{
    DecoderObject* d;
    PyObject* datetimeModeObj = NULL;
    DatetimeMode datetimeMode = DM_NONE;
    PyObject* uuidModeObj = NULL;
    UuidMode uuidMode = UM_NONE;
    PyObject* numberModeObj = NULL;
    NumberMode numberMode = NM_NAN;
    PyObject* parseModeObj = NULL;
    ParseMode parseMode = PM_NONE;
    static char const * kwlist[] = {
        "number_mode",
        "datetime_mode",
        "uuid_mode",
        "parse_mode",
        NULL
    };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OOOO:Decoder",
                                     (char **) kwlist,
                                     &numberModeObj,
                                     &datetimeModeObj,
                                     &uuidModeObj,
                                     &parseModeObj))
        return NULL;

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
                                "Invalid datetime_mode, can deserialize only from"
                                " ISO8601");
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
            int mode = PyLong_AsLong(uuidModeObj);
            if (mode < 0 || mode >= 1<<2) {
                PyErr_SetString(PyExc_ValueError, "Invalid uuid_mode");
                return NULL;
            }
            uuidMode = (UuidMode) mode;
        }
        else {
            PyErr_SetString(PyExc_TypeError,
                            "uuid_mode must be an integer value or None");
            return NULL;
        }
    }

    if (parseModeObj) {
        if (parseModeObj == Py_None)
            parseMode = PM_NONE;
        else if (PyLong_Check(parseModeObj)) {
            int mode = PyLong_AsLong(parseModeObj);
            if (mode < 0 || mode >= 1<<2) {
                PyErr_SetString(PyExc_ValueError, "Invalid parse_mode");
                return NULL;
            }
            parseMode = (ParseMode) mode;
        }
        else {
            PyErr_SetString(PyExc_TypeError,
                            "parse_mode must be an integer value or None");
            return NULL;
        }
    }

    d = (DecoderObject*) type->tp_alloc(type, 0);
    if (d == NULL)
        return NULL;

    d->datetimeMode = datetimeMode;
    d->uuidMode = uuidMode;
    d->numberMode = numberMode;
    d->parseMode = parseMode;

    return (PyObject*) d;
}


/////////////
// Encoder //
/////////////


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
        Py_ssize_t tks = this->key_size;
        Py_ssize_t oks = other.key_size;
        int cmp = strncmp(other.key_str, this->key_str, oks < tks ? oks : tks);
        return (cmp == 0) ? oks < tks : cmp < 0;
    }
};


static const int MAX_RECURSION_DEPTH = 2048;


template<typename WriterT, typename BufferT>
static PyObject*
dumps_internal(
    WriterT* writer,
    BufferT* buf,
    PyObject* value,
    bool skipKeys,
    PyObject* defaultFn,
    bool sortKeys,
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
                PyObject* is_inf = PyObject_CallMethodObjArgs(object, is_infinite_name,
                                                              NULL);

                if (is_inf == NULL) {
                    return NULL;
                }
                is_inf_or_nan = is_inf == Py_True;
                Py_DECREF(is_inf);

                if (!is_inf_or_nan) {
                    PyObject* is_nan = PyObject_CallMethodObjArgs(object, is_nan_name,
                                                                  NULL);

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
                        PyErr_SetString(PyExc_TypeError, "keys must be strings");
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
                        PyErr_SetString(PyExc_TypeError, "keys must be strings");
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
            unsigned int year, month, day, hour, min, sec, microsec;
            PyObject* dtObject = object;
            PyObject* asUTC = NULL;

            const int ISOFORMAT_LEN = 40;
            char isoformat[ISOFORMAT_LEN];
            memset(isoformat, 0, ISOFORMAT_LEN);

            const int TIMEZONE_LEN = 16;
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

                        unsigned int tz_hour = seconds_from_utc / 3600;
                        unsigned int tz_min = (seconds_from_utc % 3600) / 60;

                        snprintf(timeZone, TIMEZONE_LEN-1, "%c%02u:%02u",
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
                                 "%04u-%02u-%02uT%02u:%02u:%02u.%06u%s",
                                 year, month, day,
                                 hour, min, sec, microsec,
                                 timeZone);
                    } else {
                        snprintf(isoformat,
                                 ISOFORMAT_LEN-1,
                                 "%04u-%02u-%02uT%02u:%02u:%02u%s",
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
                                 "%02u:%02u:%02u.%06u%s",
                                 hour, min, sec, microsec,
                                 timeZone);
                    } else {
                        snprintf(isoformat,
                                 ISOFORMAT_LEN-1,
                                 "%02u:%02u:%02u%s",
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
                    else {
                        int precision = writer->GetMaxDecimalPlaces();
                        writer->SetMaxDecimalPlaces(6);
                        writer->Double(timestamp);
                        writer->SetMaxDecimalPlaces(precision);
                    }
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
            unsigned int year = PyDateTime_GET_YEAR(object);
            unsigned int month = PyDateTime_GET_MONTH(object);
            unsigned int day = PyDateTime_GET_DAY(object);

            if (datetime_mode_format(datetimeMode) == DM_ISO8601) {
                const int ISOFORMAT_LEN = 16;
                char isoformat[ISOFORMAT_LEN];
                memset(isoformat, 0, ISOFORMAT_LEN);

                snprintf(isoformat, ISOFORMAT_LEN-1, "%04u-%02u-%02u", year, month, day);
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
                else {
                    int precision = writer->GetMaxDecimalPlaces();
                    writer->SetMaxDecimalPlaces(6);
                    writer->Double(timestamp);
                    writer->SetMaxDecimalPlaces(precision);
                }
            }
        }
        else if (uuidMode != UM_NONE
                 && PyObject_TypeCheck(object, (PyTypeObject*) uuid_type)) {
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
        skipInvalidKeys, \
        defaultFn, \
        sortKeys, \
        maxRecursionDepth, \
        numberMode, \
        datetimeMode, \
        uuidMode)


typedef struct {
    PyObject_HEAD
    bool skipInvalidKeys;
    bool ensureAscii;
    bool prettyPrint;
    unsigned indent;
    bool sortKeys;
    unsigned maxRecursionDepth;
    DatetimeMode datetimeMode;
    UuidMode uuidMode;
    NumberMode numberMode;
} EncoderObject;


PyDoc_STRVAR(dumps_docstring,
             "dumps(obj, skipkeys=False, ensure_ascii=True, indent=None, default=None,"
             " sort_keys=False, max_recursion_depth=2048,"
             " number_mode=None, datetime_mode=None, uuid_mode=None,"
             " allow_nan=True)\n"
             "\n"
             "Encode a Python object into a JSON string.");


static PyObject*
dumps(PyObject* self, PyObject* args, PyObject* kwargs)
{
    /* Converts a Python object to a JSON-encoded string. */

    PyObject* value;
    int skipKeys = false;
    int ensureAscii = true;
    PyObject* indent = NULL;
    PyObject* defaultFn = NULL;
    int sortKeys = false;
    unsigned maxRecursionDepth = MAX_RECURSION_DEPTH;
    PyObject* numberModeObj = NULL;
    NumberMode numberMode = NM_NAN;
    PyObject* datetimeModeObj = NULL;
    DatetimeMode datetimeMode = DM_NONE;
    PyObject* uuidModeObj = NULL;
    UuidMode uuidMode = UM_NONE;
    bool prettyPrint = false;
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

    return do_encode(value, skipKeys ? true : false, defaultFn, sortKeys ? true : false,
                     maxRecursionDepth, ensureAscii ? true : false,
                     prettyPrint ? true : false, indentCharCount, numberMode,
                     datetimeMode, uuidMode);
}


PyDoc_STRVAR(encoder_doc,
             "Encoder(skip_invalid_keys=False, ensure_ascii=True, indent=None,"
             " sort_keys=False, max_recursion_depth=2048, number_mode=None,"
             " datetime_mode=None, uuid_mode=None)\n"
             "\n"
             "Create and return a new Encoder instance.");


static PyMemberDef encoder_members[] = {
    {"skip_invalid_keys",
     T_BOOL, offsetof(EncoderObject, skipInvalidKeys), READONLY, "skip_invalid_keys"},
    {"ensure_ascii",
     T_BOOL, offsetof(EncoderObject, ensureAscii), READONLY, "ensure_ascii"},
    {"indent",
     T_UINT, offsetof(EncoderObject, indent), READONLY, "indent"},
    {"sort_keys",
     T_BOOL, offsetof(EncoderObject, ensureAscii), READONLY, "sort_keys"},
    {"max_recursion_depth",
     T_UINT, offsetof(EncoderObject, maxRecursionDepth), READONLY, "max_recursion_depth"},
    {"datetime_mode",
     T_UINT, offsetof(EncoderObject, datetimeMode), READONLY, "datetime_mode"},
    {"uuid_mode",
     T_UINT, offsetof(EncoderObject, uuidMode), READONLY, "uuid_mode"},
    {"number_mode",
     T_UINT, offsetof(EncoderObject, numberMode), READONLY, "number_mode"},
    {NULL}
};


static PyTypeObject Encoder_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "rapidjson.Encoder",                      /* tp_name */
    sizeof(EncoderObject),                    /* tp_basicsize */
    0,                                        /* tp_itemsize */
    0,                                        /* tp_dealloc */
    0,                                        /* tp_print */
    0,                                        /* tp_getattr */
    0,                                        /* tp_setattr */
    0,                                        /* tp_compare */
    0,                                        /* tp_repr */
    0,                                        /* tp_as_number */
    0,                                        /* tp_as_sequence */
    0,                                        /* tp_as_mapping */
    0,                                        /* tp_hash */
    (ternaryfunc) encoder_call,               /* tp_call */
    0,                                        /* tp_str */
    0,                                        /* tp_getattro */
    0,                                        /* tp_setattro */
    0,                                        /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    encoder_doc,                              /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    0,                                        /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    0,                                        /* tp_iter */
    0,                                        /* tp_iternext */
    0,                                        /* tp_methods */
    encoder_members,                          /* tp_members */
    0,                                        /* tp_getset */
    0,                                        /* tp_base */
    0,                                        /* tp_dict */
    0,                                        /* tp_descr_get */
    0,                                        /* tp_descr_set */
    0,                                        /* tp_dictoffset */
    0,                                        /* tp_init */
    0,                                        /* tp_alloc */
    encoder_new,                              /* tp_new */
    PyObject_Del,                             /* tp_free */
};


#define Encoder_CheckExact(v) (Py_TYPE(v) == &Encoder_Type)
#define Encoder_Check(v) PyObject_TypeCheck(v, &Encoder_Type)


static PyObject*
do_encode(PyObject* value, bool skipInvalidKeys, PyObject* defaultFn, bool sortKeys,
          unsigned maxRecursionDepth, bool ensureAscii, bool prettyPrint, unsigned indent,
          NumberMode numberMode, DatetimeMode datetimeMode, UuidMode uuidMode)
{
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
        writer.SetIndent(' ', indent);
        return DUMPS_INTERNAL_CALL;
    }
    else {
        StringBuffer buf;
        PrettyWriter<StringBuffer> writer(buf);
        writer.SetIndent(' ', indent);
        return DUMPS_INTERNAL_CALL;
    }
}


static PyObject*
encoder_call(PyObject* self, PyObject* args, PyObject* kwargs)
{
    PyObject* value;
    PyObject* defaultFn = NULL;

    if (!PyArg_ParseTuple(args, "O", &value))
        return NULL;

    EncoderObject* e = (EncoderObject*) self;

    if (PyObject_HasAttr(self, default_name)) {
        defaultFn = PyObject_GetAttr(self, default_name);
    }

    return do_encode(value, e->skipInvalidKeys, defaultFn, e->sortKeys,
                     e->maxRecursionDepth, e->ensureAscii, e->prettyPrint,
                     e->indent, e->numberMode, e->datetimeMode, e->uuidMode);
}


static PyObject*
encoder_new(PyTypeObject* type, PyObject* args, PyObject* kwargs)
{
    EncoderObject* e;
    int skipInvalidKeys = false;
    int ensureAscii = true;
    PyObject* indent = NULL;
    int sortKeys = false;
    unsigned maxRecursionDepth = MAX_RECURSION_DEPTH;
    PyObject* numberModeObj = NULL;
    NumberMode numberMode = NM_NAN;
    PyObject* datetimeModeObj = NULL;
    DatetimeMode datetimeMode = DM_NONE;
    PyObject* uuidModeObj = NULL;
    UuidMode uuidMode = UM_NONE;
    unsigned indentCharCount = 4;
    bool prettyPrint = false;

    static char const * kwlist[] = {
        "skip_invalid_keys",
        "ensure_ascii",
        "indent",
        "sort_keys",
        "max_recursion_depth",
        "number_mode",
        "datetime_mode",
        "uuid_mode",
        NULL
    };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|ppOpIOOO:Encoder",
                                     (char **) kwlist,
                                     &skipInvalidKeys,
                                     &ensureAscii,
                                     &indent,
                                     &sortKeys,
                                     &maxRecursionDepth,
                                     &numberModeObj,
                                     &datetimeModeObj,
                                     &uuidModeObj))
        return NULL;

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

    e = (EncoderObject*) type->tp_alloc(type, 0);
    if (e == NULL)
        return NULL;

    e->skipInvalidKeys = skipInvalidKeys ? true : false;
    e->ensureAscii = ensureAscii ? true : false;
    e->prettyPrint = prettyPrint;
    e->indent = indentCharCount;
    e->sortKeys = sortKeys ? true : false;
    e->maxRecursionDepth = maxRecursionDepth;
    e->datetimeMode = datetimeMode;
    e->uuidMode = uuidMode;
    e->numberMode = numberMode;

    return (PyObject*) e;
}


///////////////
// Validator //
///////////////


typedef struct {
    PyObject_HEAD
    SchemaDocument *schema;
} ValidatorObject;


PyDoc_STRVAR(validator_doc,
             "Validator(json_schema)\n"
             "\n"
             "Create and return a new Validator instance from the given `json_schema`"
             " string.");


static PyTypeObject Validator_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "rapidjson.Validator",          /* tp_name */
    sizeof(ValidatorObject),        /* tp_basicsize */
    0,                              /* tp_itemsize */
    (destructor) validator_dealloc, /* tp_dealloc */
    0,                              /* tp_print */
    0,                              /* tp_getattr */
    0,                              /* tp_setattr */
    0,                              /* tp_compare */
    0,                              /* tp_repr */
    0,                              /* tp_as_number */
    0,                              /* tp_as_sequence */
    0,                              /* tp_as_mapping */
    0,                              /* tp_hash */
    (ternaryfunc) validator_call,   /* tp_call */
    0,                              /* tp_str */
    0,                              /* tp_getattro */
    0,                              /* tp_setattro */
    0,                              /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,             /* tp_flags */
    validator_doc,                  /* tp_doc */
    0,                              /* tp_traverse */
    0,                              /* tp_clear */
    0,                              /* tp_richcompare */
    0,                              /* tp_weaklistoffset */
    0,                              /* tp_iter */
    0,                              /* tp_iternext */
    0,                              /* tp_methods */
    0,                              /* tp_members */
    0,                              /* tp_getset */
    0,                              /* tp_base */
    0,                              /* tp_dict */
    0,                              /* tp_descr_get */
    0,                              /* tp_descr_set */
    0,                              /* tp_dictoffset */
    0,                              /* tp_init */
    0,                              /* tp_alloc */
    validator_new,                  /* tp_new */
    PyObject_Del,                   /* tp_free */
};


static PyObject* validator_call(PyObject* self, PyObject* args, PyObject* kwargs)
{
    PyObject* jsonObject;

    if (!PyArg_ParseTuple(args, "O", &jsonObject))
        return NULL;

    const char* jsonStr;

    if (PyBytes_Check(jsonObject)) {
        jsonStr = PyBytes_AsString(jsonObject);
        if (jsonStr == NULL)
            return NULL;
    }
    else if (PyUnicode_Check(jsonObject)) {
        jsonStr = PyUnicode_AsUTF8(jsonObject);
        if (jsonStr == NULL)
            return NULL;
    }
    else {
        PyErr_SetString(PyExc_TypeError, "Expected string or utf-8 encoded bytes");
        return NULL;
    }

    Document d;
    bool error;

    Py_BEGIN_ALLOW_THREADS
    error = d.Parse(jsonStr).HasParseError();
    Py_END_ALLOW_THREADS

    if (error) {
        PyErr_SetString(PyExc_ValueError, "Invalid JSON");
        return NULL;
    }

    SchemaValidator validator(*((ValidatorObject*) self)->schema);
    bool accept;

    Py_BEGIN_ALLOW_THREADS
    accept = d.Accept(validator);
    Py_END_ALLOW_THREADS

    if (!accept) {
        StringBuffer sptr;
        StringBuffer dptr;

        Py_BEGIN_ALLOW_THREADS
        validator.GetInvalidSchemaPointer().StringifyUriFragment(sptr);
        validator.GetInvalidDocumentPointer().StringifyUriFragment(dptr);
        Py_END_ALLOW_THREADS

        PyErr_SetObject(PyExc_ValueError,
                        Py_BuildValue("sss", validator.GetInvalidSchemaKeyword(),
                                      sptr.GetString(), dptr.GetString()));
        sptr.Clear();
        dptr.Clear();

        return NULL;
    }

    Py_RETURN_NONE;
}


static void validator_dealloc(PyObject* self)
{
    ValidatorObject* s = (ValidatorObject*) self;
    delete s->schema;
    Py_TYPE(self)->tp_free(self);
}


static PyObject* validator_new(PyTypeObject* type, PyObject* args, PyObject* kwargs)
{
    PyObject* jsonObject;

    if (!PyArg_ParseTuple(args, "O", &jsonObject))
        return NULL;

    const char* jsonStr;

    if (PyBytes_Check(jsonObject)) {
        jsonStr = PyBytes_AsString(jsonObject);
        if (jsonStr == NULL)
            return NULL;
    }
    else if (PyUnicode_Check(jsonObject)) {
        jsonStr = PyUnicode_AsUTF8(jsonObject);
        if (jsonStr == NULL)
            return NULL;
    }
    else {
        PyErr_SetString(PyExc_TypeError, "Expected string or utf-8 encoded bytes");
        return NULL;
    }

    Document d;
    bool error;

    Py_BEGIN_ALLOW_THREADS
    error = d.Parse(jsonStr).HasParseError();
    Py_END_ALLOW_THREADS

    if (error) {
        PyErr_SetString(PyExc_ValueError, "Invalid JSON");
        return NULL;
    }

    ValidatorObject* v = (ValidatorObject*) type->tp_alloc(type, 0);
    if (v == NULL)
        return NULL;

    v->schema = new SchemaDocument(d);

    return (PyObject*) v;
}


////////////
// Module //
////////////


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
    PyDoc_STR("Fast, simple JSON encoder and decoder. Based on RapidJSON C++ library."),
    -1,
    functions
};


PyMODINIT_FUNC
PyInit_rapidjson()
{
    PyObject* datetimeModule;
    PyObject* decimalModule;
    PyObject* uuidModule;

    if (PyType_Ready(&Decoder_Type) < 0)
        goto error;

    if (PyType_Ready(&Encoder_Type) < 0)
        goto error;

    if (PyType_Ready(&Validator_Type) < 0)
        goto error;

    PyDateTime_IMPORT;

    datetimeModule = PyImport_ImportModule("datetime");
    if (datetimeModule == NULL)
        goto error;

    decimalModule = PyImport_ImportModule("decimal");
    if (decimalModule == NULL)
        goto error;

    decimal_type = PyObject_GetAttrString(decimalModule, "Decimal");
    Py_DECREF(decimalModule);

    if (decimal_type == NULL)
        goto error;

    timezone_type = PyObject_GetAttrString(datetimeModule, "timezone");
    Py_DECREF(datetimeModule);

    if (timezone_type == NULL)
        goto error;

    timezone_utc = PyObject_GetAttrString(timezone_type, "utc");
    if (timezone_utc == NULL)
        goto error;

    uuidModule = PyImport_ImportModule("uuid");
    if (uuidModule == NULL)
        goto error;

    uuid_type = PyObject_GetAttrString(uuidModule, "UUID");
    Py_DECREF(uuidModule);

    if (uuid_type == NULL)
        goto error;

    astimezone_name = PyUnicode_InternFromString("astimezone");
    if (astimezone_name == NULL)
        goto error;

    hex_name = PyUnicode_InternFromString("hex");
    if (hex_name == NULL)
        goto error;

    timestamp_name = PyUnicode_InternFromString("timestamp");
    if (timestamp_name == NULL)
        goto error;

    total_seconds_name = PyUnicode_InternFromString("total_seconds");
    if (total_seconds_name == NULL)
        goto error;

    utcoffset_name = PyUnicode_InternFromString("utcoffset");
    if (utcoffset_name == NULL)
        goto error;

    is_infinite_name = PyUnicode_InternFromString("is_infinite");
    if (is_infinite_name == NULL)
        goto error;

    is_nan_name = PyUnicode_InternFromString("is_nan");
    if (is_infinite_name == NULL)
        goto error;

    minus_inf_string_value = PyUnicode_InternFromString("-Infinity");
    if (minus_inf_string_value == NULL)
        goto error;

    nan_string_value = PyUnicode_InternFromString("nan");
    if (nan_string_value == NULL)
        goto error;

    plus_inf_string_value = PyUnicode_InternFromString("+Infinity");
    if (plus_inf_string_value == NULL)
        goto error;

    start_object_name = PyUnicode_InternFromString("start_object");
    if (start_object_name == NULL)
        goto error;

    end_object_name = PyUnicode_InternFromString("end_object");
    if (end_object_name == NULL)
        goto error;

    default_name = PyUnicode_InternFromString("default");
    if (default_name == NULL)
        goto error;

    end_array_name = PyUnicode_InternFromString("end_array");
    if (end_array_name == NULL)
        goto error;

    string_name = PyUnicode_InternFromString("string");
    if (string_name == NULL)
        goto error;

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

    PyModule_AddIntConstant(m, "PM_NONE", PM_NONE);
    PyModule_AddIntConstant(m, "PM_COMMENTS", PM_COMMENTS);
    PyModule_AddIntConstant(m, "PM_TRAILING_COMMAS", PM_TRAILING_COMMAS);

#define STRINGIFY(x) XSTRINGIFY(x)
#define XSTRINGIFY(x) #x

    PyModule_AddStringConstant(m, "__version__", STRINGIFY(PYTHON_RAPIDJSON_VERSION));
    PyModule_AddStringConstant(m, "__author__", "Ken Robbins <ken@kenrobbins.com>");
    PyModule_AddStringConstant(m, "__rapidjson_version__", RAPIDJSON_VERSION_STRING);

    Py_INCREF(&Decoder_Type);
    PyModule_AddObject(m, "Decoder", (PyObject*) &Decoder_Type);

    Py_INCREF(&Encoder_Type);
    PyModule_AddObject(m, "Encoder", (PyObject*) &Encoder_Type);

    Py_INCREF(&Validator_Type);
    PyModule_AddObject(m, "Validator", (PyObject*) &Validator_Type);

    return m;

error:
    Py_CLEAR(astimezone_name);
    Py_CLEAR(hex_name);
    Py_CLEAR(timestamp_name);
    Py_CLEAR(total_seconds_name);
    Py_CLEAR(utcoffset_name);
    Py_CLEAR(is_infinite_name);
    Py_CLEAR(is_nan_name);
    Py_CLEAR(minus_inf_string_value);
    Py_CLEAR(nan_string_value);
    Py_CLEAR(plus_inf_string_value);
    Py_CLEAR(decimal_type);
    Py_CLEAR(timezone_type);
    Py_CLEAR(timezone_utc);
    Py_CLEAR(uuid_type);
    Py_CLEAR(start_object_name);
    Py_CLEAR(end_object_name);
    Py_CLEAR(default_name);
    Py_CLEAR(end_array_name);
    Py_CLEAR(string_name);

    return NULL;
}
