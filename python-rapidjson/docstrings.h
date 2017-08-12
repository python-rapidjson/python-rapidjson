#ifndef DOCSTRINGS_H_
#define DOCSTRINGS_H_

PyDoc_STRVAR(module_docstring,
             "Fast, simple JSON encoder and decoder. Based on RapidJSON C++ library.");

PyDoc_STRVAR(loads_docstring,
             "loads(s, object_hook=None, number_mode=None, datetime_mode=None,"
             " uuid_mode=None)\n"
             "\n"
             "Decodes a JSON string into Python object.");

PyDoc_STRVAR(dumps_docstring,
             "dumps(obj, skipkeys=False, ensure_ascii=True, indent=None, default=None,"
             " sort_keys=False, max_recursion_depth=2048,"
             " number_mode=None, datetime_mode=None, uuid_mode=None)\n"
             "\n"
             "Encodes Python object into a JSON string.");

#endif
