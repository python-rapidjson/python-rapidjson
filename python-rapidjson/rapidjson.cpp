#include <Python.h>
#include <datetime.h>

#include <string>
#include <vector>
#include <iostream>

#include "rapidjson/reader.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/error/en.h"

using namespace rapidjson;


struct HandlerContext {
    PyObject* object;
    const char* key;
    SizeType keyLength;
    bool isObject;
};

struct PyHandler {
    PyObject* root;
    PyObject* objectHook;
    std::vector<HandlerContext> stack;

    PyHandler(PyObject* hook)
    : root(NULL), objectHook(hook)
    {
        stack.reserve(64);
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

    static char* kwlist[] = {"value", "object_hook", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:rapidjson.loads",
                                     kwlist,
                                     &jsonObject,
                                     &objectHook))

    if (objectHook && !PyCallable_Check(objectHook))
        objectHook = NULL;

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

    PyHandler handler(objectHook);
    Reader reader;
    InsituStringStream ss(jsonStrCopy);
    reader.Parse<kParseInsituFlag>(ss, handler);

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
    bool isObject;

    WriterContext(const char* k, PyObject* o, bool isO, PyObject* d=NULL)
    : key(k), object(o), decref(d), isObject(isO)
    {}
};

static const int DATETIME_MODE_NONE = 0;
static const int DATETIME_MODE_ISO8601 = 1;

static PyObject*
rapidjson_dumps(PyObject* self, PyObject* args, PyObject* kwargs)
{
    /* Converts a Python object to a JSON-encoded string. */

    PyObject* value;
    PyObject* default_fn = NULL;
    int datetime_mode = DATETIME_MODE_ISO8601;

    static char* kwlist[] = {"value", "default", "datetime_mode", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|Oi:rapidjson.dumps",
                                     kwlist,
                                     &value,
                                     &default_fn,
                                     &datetime_mode))
        return NULL;

    if (default_fn && !PyCallable_Check(default_fn))
        default_fn = NULL;

    StringBuffer buf;
    Writer<StringBuffer> writer(buf);
    std::vector<WriterContext> stack;

    stack.push_back(WriterContext(NULL, value, false));

    while (!stack.empty()) {
        const WriterContext current = stack.back();
        stack.pop_back();

        PyObject* const object = current.object;

        if (object == NULL) {
            if (current.key != NULL) {
                writer.Key(current.key);
            }
            else if (current.decref != NULL) {
                Py_DECREF(current.decref);
            }
            else if (current.isObject) {
                writer.EndObject();
            }
            else {
                writer.EndArray();
            }
        }
        else if (object == Py_None) {
            writer.Null();
        }
        else if (PyBool_Check(object)) {
            writer.Bool(object == Py_True);
        }
        else if (PyLong_Check(object)) {
            // TODO overflow
            long long i = PyLong_AsLongLong(object);
            writer.Int64(i);
        }
        else if (PyFloat_Check(object)) {
            // TODO overflow
            double d = PyFloat_AsDouble(object);
            writer.Double(d);
        }
        else if (PyBytes_Check(object)) {
            char* s = PyBytes_AsString(object);
            if (s == NULL)
                return NULL;

            writer.String(s);
        }
        else if (PyUnicode_Check(object)) {
            char* s = PyUnicode_AsUTF8(object);
            writer.String(s);
        }
        else if (PyList_Check(object)) {
            writer.StartArray();
            stack.push_back(WriterContext(NULL, NULL, false));

            Py_ssize_t size = PyList_GET_SIZE(object);

            for (Py_ssize_t i = size - 1; i >= 0; --i) {
                PyObject* item = PyList_GET_ITEM(object, i);
                stack.push_back(WriterContext(NULL, item, false));
            }
        }
        else if (PyTuple_Check(object)) {
            writer.StartArray();
            stack.push_back(WriterContext(NULL, NULL, false));

            Py_ssize_t size = PyTuple_GET_SIZE(object);

            for (Py_ssize_t i = size - 1; i >= 0; --i) {
                PyObject* item = PyTuple_GET_ITEM(object, i);
                stack.push_back(WriterContext(NULL, item, false));
            }
        }
        else if (PyDict_Check(object)) {
            writer.StartObject();
            stack.push_back(WriterContext(NULL, NULL, true));

            Py_ssize_t pos = 0;
            PyObject* key;
            PyObject* item;

            while (PyDict_Next(object, &pos, &key, &item)) {
                if (PyUnicode_Check(key)) {
                    char* key_str = PyUnicode_AsUTF8(key);
                    stack.push_back(WriterContext(NULL, item, false));
                    stack.push_back(WriterContext(key_str, NULL, false));
                }
                else {
                    PyErr_Format(PyExc_TypeError, "keys must be a string");
                    return NULL;
                }
            }
        }
        else if (PyDateTime_Check(object) && datetime_mode == DATETIME_MODE_ISO8601) {
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

            writer.String(isoformat);
        }
        else if (default_fn) {
            PyObject* retval = PyObject_CallFunctionObjArgs(default_fn, object, NULL);
            if (retval == NULL)
                return NULL;

            // Decref the return value once it's done being dumped to a string.
            stack.push_back(WriterContext(NULL, NULL, false, retval));
            stack.push_back(WriterContext(NULL, retval, false));
        }
        else {
            PyObject* repr = PyObject_Repr(object);
            PyErr_Format(PyExc_TypeError, "%s is not JSON serializable", PyUnicode_AsUTF8(repr));
            Py_XDECREF(repr);
            return NULL;
        }
    }

    return PyUnicode_FromString(buf.GetString());
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

    PyObject* module;

    module = PyModule_Create(&rapidjson_module);
    if (module == NULL)
        return NULL;

    return module;
}
