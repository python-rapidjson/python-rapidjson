#include <Python.h>
#include <datetime.h>

#include <algorithm>
#include <string>
#include <vector>

#include "rapidjson/reader.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/error/en.h"

using namespace rapidjson;


static PyObject* rapidjson_decimal_type = NULL;

struct HandlerContext {
    PyObject* object;
    const char* key;
    SizeType keyLength;
    bool isObject;
};

struct PyHandler {
    int useDecimal;
    PyObject* root;
    PyObject* objectHook;
    std::vector<HandlerContext> stack;

    PyHandler(int ud, PyObject* hook)
    : useDecimal(ud), root(NULL), objectHook(hook)
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
        PyObject* str = PyUnicode_FromStringAndSize("nan", 3);
        if (str == NULL)
            return NULL;

        PyObject* value;
        if (!useDecimal)
            value = PyFloat_FromString(str);
        else
            value = PyObject_CallFunctionObjArgs(rapidjson_decimal_type, str, NULL);

        Py_DECREF(str);

        if (value == NULL)
            return NULL;

        return HandleSimpleType(value);
    }

    bool Infinity(bool minus) {
        PyObject* str;

        if (minus)
            str = PyUnicode_FromStringAndSize("-Infinity", 9);
        else
            str = PyUnicode_FromStringAndSize("Infinity", 8);

        if (str == NULL)
            return NULL;

        PyObject* value;
        if (!useDecimal)
            value = PyFloat_FromString(str);
        else
            value = PyObject_CallFunctionObjArgs(rapidjson_decimal_type, str, NULL);

        Py_DECREF(str);

        if (value == NULL)
            return NULL;

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

    bool Double(double d, const char* decimal, SizeType length, bool minus, size_t decimalPosition, int exp) {
        PyObject* value;
        if (!useDecimal)
            value = PyFloat_FromDouble(d);
        else {
            const int MAX_FINAL_SIZE = 512;
            const int MAX_EXP_SIZE = 11; // 32-bit
            const int MAX_DECIMAL_SIZE = MAX_FINAL_SIZE - MAX_EXP_SIZE - 3; // e, +/-, \0

            exp += decimalPosition - length;

            if (length > MAX_DECIMAL_SIZE)
                length = MAX_DECIMAL_SIZE;

            char finalStr[MAX_DECIMAL_SIZE];
            finalStr[0] = minus ? '-' : '+';
            memcpy(finalStr+1, decimal, length);

            if (exp == 0)
                finalStr[length+1] = 0;
            else {
                char expStr[MAX_EXP_SIZE];
                char* end = internal::i32toa(exp, expStr);
                size_t len = end - expStr;

                finalStr[length+1] = 'e';
                memcpy(finalStr+length+2, expStr, len);
                finalStr[length+2+len] = 0;
            }

            PyObject* raw = PyUnicode_FromString(finalStr);
            if (raw == NULL) {
                PyErr_Format(PyExc_ValueError, "Error generating decimal representation");
                return NULL;
            }

            value = PyObject_CallFunctionObjArgs(rapidjson_decimal_type, raw, NULL);
            Py_DECREF(raw);
        }

        return HandleSimpleType(value);
    }

    bool String(const char* str, SizeType length, bool copy) {
        PyObject* value = PyUnicode_FromStringAndSize(str, length);
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
    int preciseFloat = 1;

    static char* kwlist[] = {
        "obj",
        "object_hook",
        "use_decimal",
        "precise_float",
        NULL
    };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|Opp:rapidjson.loads",
                                     kwlist,
                                     &jsonObject,
                                     &objectHook,
                                     &useDecimal,
                                     &preciseFloat))

    if (objectHook && !PyCallable_Check(objectHook)) {
        PyErr_Format(PyExc_TypeError, "object_hook is not callable");
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
        PyErr_Format(PyExc_TypeError, "Expected string or utf-8 encoded bytes");
        return NULL;
    }

    char* jsonStrCopy = (char*) malloc(sizeof(char) * (jsonStrLen+1));
    memcpy(jsonStrCopy, jsonStr, jsonStrLen+1);

    PyHandler handler(useDecimal, objectHook);
    Reader reader;
    InsituStringStream ss(jsonStrCopy);

    if (preciseFloat) {
        reader.Parse<kParseInsituFlag | kParseFullPrecisionFlag>(ss, handler);
    }
    else {
        reader.Parse<kParseInsituFlag>(ss, handler);
    }

    if (reader.HasParseError()) {
        SizeType offset = reader.GetErrorOffset();
        ParseErrorCode code = reader.GetParseErrorCode();
        const char* msg = GetParseError_En(code);
        const char* fmt = "Parse error at offset %d: %s";

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

enum DatetimeMode {
    DATETIME_MODE_NONE = 0,
    DATETIME_MODE_ISO8601 = 1
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
    PyObject* defaultFn,
    int sortKeys,
    int useDecimal,
    unsigned maxRecursionDepth,
    DatetimeMode datetimeMode)
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
            PyErr_Format(PyExc_OverflowError, "Max recursion depth reached");
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

            writer->RawNumber(decStr, size);
            Py_DECREF(decStrObj);
        }
        else if (PyLong_Check(object)) {
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
        }
        else if (PyFloat_Check(object)) {
            double d = PyFloat_AsDouble(object);
            if (d == -1.0 && PyErr_Occurred())
                return NULL;

            if (Py_IS_NAN(d)) {
                if (allowNan)
                    writer->RawNumber("NaN", 3);
                else {
                    PyErr_Format(PyExc_ValueError, "Out of range float values are not JSON compliant");
                    return NULL;
                }
            } else if (Py_IS_INFINITY(d)) {
                if (!allowNan) {
                    PyErr_Format(PyExc_ValueError, "Out of range float values are not JSON compliant");
                    return NULL;
                }
                else if (d < 0)
                    writer->RawNumber("-Infinity", 9);
                else
                    writer->RawNumber("Infinity", 8);
            }
            else
                writer->Double(d);
        }
        else if (PyBytes_Check(object)) {
            char* s = PyBytes_AsString(object);
            if (s == NULL)
                goto error;

            writer->String(s);
        }
        else if (PyUnicode_Check(object)) {
            char* s = PyUnicode_AsUTF8(object);
            writer->String(s);
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
                        PyErr_Format(PyExc_TypeError, "keys must be a string");
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
                        PyErr_Format(PyExc_TypeError, "keys must be a string");
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
        else if (PyDateTime_Check(object) && datetimeMode == DATETIME_MODE_ISO8601) {
            int year = PyDateTime_GET_YEAR(object);
            int month = PyDateTime_GET_MONTH(object);
            int day = PyDateTime_GET_DAY(object);
            int hour = PyDateTime_DATE_GET_HOUR(object);
            int min = PyDateTime_DATE_GET_MINUTE(object);
            int sec = PyDateTime_DATE_GET_SECOND(object);
            int microsec = PyDateTime_DATE_GET_MICROSECOND(object);

            const int ISOFORMAT_LEN = 32;
            char isoformat[ISOFORMAT_LEN];
            memset(isoformat, 0, ISOFORMAT_LEN);

            if (microsec > 0) {
                snprintf(isoformat,
                         ISOFORMAT_LEN,
                         "%04d-%02d-%02dT%02d:%02d:%02d.%06d",
                         year, month, day,
                         hour, min, sec, microsec);
            } else {
                snprintf(isoformat,
                         ISOFORMAT_LEN,
                         "%04d-%02d-%02dT%02d:%02d:%02d",
                         year, month, day,
                         hour, min, sec);
            }

            writer->String(isoformat);
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
        defaultFn, \
        sortKeys, \
        useDecimal, \
        maxRecursionDepth, \
        datetimeMode)


static PyObject*
rapidjson_dumps(PyObject* self, PyObject* args, PyObject* kwargs)
{
    /* Converts a Python object to a JSON-encoded string. */

    PyObject* value;
    int skipKeys = 0;
    int ensureAscii = 1;
    int allowNan = 1;
    PyObject* indent = NULL;
    PyObject* defaultFn = NULL;
    int sortKeys = 0;
    int useDecimal = 0;
    unsigned maxRecursionDepth = MAX_RECURSION_DEPTH;
    DatetimeMode datetimeMode = DATETIME_MODE_NONE;

    bool prettyPrint = false;
    char indentChar = ' ';
    unsigned indentCharCount = 4;

    static char* kwlist[] = {
        "s",
        "skipkeys",
        "ensure_ascii",
        "allow_nan",
        "indent",
        "default",
        "sort_keys",
        "use_decimal",
        "max_recursion_depth",
        "datetime_mode",
        NULL
    };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|pppOOppIi:rapidjson.dumps",
                                     kwlist,
                                     &value,
                                     &skipKeys,
                                     &ensureAscii,
                                     &allowNan,
                                     &indent,
                                     &defaultFn,
                                     &sortKeys,
                                     &useDecimal,
                                     &maxRecursionDepth,
                                     &datetimeMode))
        return NULL;

    if (defaultFn && !PyCallable_Check(defaultFn)) {
        PyErr_Format(PyExc_TypeError, "default must be a callable");
        return NULL;
    }

    if (indent && indent != Py_None) {
        prettyPrint = true;

        if (PyLong_Check(indent)) {
            indentCharCount = PyLong_AsLong(indent);
        }
        else if (PyUnicode_Check(indent)) {
            char* s = PyUnicode_AsUTF8(indent);

            if (s != NULL) {
                indentCharCount = strlen(s);
                indentChar = *s;
            }
        }
        else if (PyBytes_Check(indent)) {
            char* s = PyBytes_AsString(indent);

            if (s != NULL) {
                indentCharCount = strlen(s);
                indentChar = *s;
            }
        }
        else {
            PyErr_Format(PyExc_TypeError, "indent must be an int or a string");
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
    {"loads", (PyCFunction) rapidjson_loads, METH_VARARGS | METH_KEYWORDS, "load object from json string"},
    {"dumps", (PyCFunction) rapidjson_dumps, METH_VARARGS | METH_KEYWORDS, "dump object as json string"},
    {NULL, NULL, 0, NULL} /* sentinel */
};

static PyModuleDef rapidjson_module = {
    PyModuleDef_HEAD_INIT,
    "rapidjson",
    "Python wrapper around rapidjson",
    -1,
    rapidjson_functions
};

PyMODINIT_FUNC
PyInit_rapidjson()
{
    PyDateTime_IMPORT;

    PyObject* decimalModule = PyImport_ImportModule("decimal");
    if (decimalModule == NULL)
        return NULL;

    rapidjson_decimal_type = PyObject_GetAttrString(decimalModule, "Decimal");
    Py_INCREF(rapidjson_decimal_type);
    Py_DECREF(decimalModule);

    PyObject* module;

    module = PyModule_Create(&rapidjson_module);
    if (module == NULL)
        return NULL;

    return module;
}
