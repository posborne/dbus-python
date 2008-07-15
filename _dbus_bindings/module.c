/* Main module source for the _dbus_bindings extension.
 *
 * Copyright (C) 2006 Collabora Ltd. <http://www.collabora.co.uk/>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include "config.h"

#include <Python.h>
#include <structmember.h>

#include "dbus_bindings-internal.h"

PyDoc_STRVAR(module_doc,
"Low-level Python bindings for libdbus. Don't use this module directly -\n"
"the public API is provided by the `dbus`, `dbus.service`, `dbus.mainloop`\n"
"and `dbus.mainloop.glib` modules, with a lower-level API provided by the\n"
"`dbus.lowlevel` module.\n"
);

/* Global functions - validation wrappers ===========================*/

PyDoc_STRVAR(validate_bus_name__doc__,
"validate_bus_name(name, allow_unique=True, allow_well_known=True)\n"
"\n"
"Raise ValueError if the argument is not a valid bus name.\n"
"\n"
"By default both unique and well-known names are accepted.\n"
"\n"
":Parameters:\n"
"   `name` : str\n"
"       The name to be validated\n"
"   `allow_unique` : bool\n"
"       If False, unique names of the form :1.123 will be rejected\n"
"   `allow_well_known` : bool\n"
"       If False, well-known names of the form com.example.Foo\n"
"       will be rejected\n"
":Since: 0.80\n"
);

static PyObject *
validate_bus_name(PyObject *unused UNUSED, PyObject *args, PyObject *kwargs)
{
    const char *name;
    int allow_unique = 1;
    int allow_well_known = 1;
    static char *argnames[] = { "name", "allow_unique", "allow_well_known",
                                NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "s|ii:validate_bus_name", argnames,
                                     &name, &allow_unique,
                                     &allow_well_known)) {
        return NULL;
    }
    if (!dbus_py_validate_bus_name(name, !!allow_unique, !!allow_well_known)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(validate_member_name__doc__,
"validate_member_name(name)\n"
"\n"
"Raise ValueError if the argument is not a valid member (signal or method) "
"name.\n"
"\n"
":Since: 0.80\n"
);

static PyObject *
validate_member_name(PyObject *unused UNUSED, PyObject *args)
{
    const char *name;

    if (!PyArg_ParseTuple(args, "s:validate_member_name", &name)) {
        return NULL;
    }
    if (!dbus_py_validate_member_name(name)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(validate_interface_name__doc__,
"validate_interface_name(name)\n\n"
"Raise ValueError if the given string is not a valid interface name.\n"
"\n"
":Since: 0.80\n"
);

PyDoc_STRVAR(validate_error_name__doc__,
"validate_error_name(name)\n\n"
"Raise ValueError if the given string is not a valid error name.\n"
"\n"
":Since: 0.80\n"
);

static PyObject *
validate_interface_name(PyObject *unused UNUSED, PyObject *args)
{
    const char *name;

    if (!PyArg_ParseTuple(args, "s:validate_interface_name", &name)) {
        return NULL;
    }
    if (!dbus_py_validate_interface_name(name)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(validate_object_path__doc__,
"validate_object_path(name)\n\n"
"Raise ValueError if the given string is not a valid object path.\n"
"\n"
":Since: 0.80\n"
);

static PyObject *
validate_object_path(PyObject *unused UNUSED, PyObject *args)
{
    const char *name;

    if (!PyArg_ParseTuple(args, "s:validate_object_path", &name)) {
        return NULL;
    }
    if (!dbus_py_validate_object_path(name)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

/* Global functions - main loop =====================================*/

/* The main loop if none is passed to the constructor */
static PyObject *default_main_loop = NULL;

/* Return a new reference to the default main loop */
PyObject *
dbus_py_get_default_main_loop(void)
{
    if (!default_main_loop) {
        Py_RETURN_NONE;
    }
    Py_INCREF(default_main_loop);
    return default_main_loop;
}

PyDoc_STRVAR(get_default_main_loop__doc__,
"get_default_main_loop() -> object\n\n"
"Return the global default dbus-python main loop wrapper, which is used\n"
"when no main loop wrapper is passed to the Connection constructor.\n"
"\n"
"If None, there is no default and you should always pass the mainloop\n"
"parameter to the constructor - if you don't, then asynchronous calls,\n"
"connecting to signals and exporting objects will raise an exception.\n"
"There is no default until set_default_main_loop is called.\n");
static PyObject *
get_default_main_loop(PyObject *always_null UNUSED,
                      PyObject *no_args UNUSED)
{
    return dbus_py_get_default_main_loop();
}

PyDoc_STRVAR(set_default_main_loop__doc__,
"set_default_main_loop(object)\n\n"
"Change the global default dbus-python main loop wrapper, which is used\n"
"when no main loop wrapper is passed to the Connection constructor.\n"
"\n"
"If None, return to the initial situation: there is no default, and you\n"
"must always pass the mainloop parameter to the constructor.\n"
"\n"
"Two types of main loop wrapper are planned in dbus-python.\n"
"Native main-loop wrappers are instances of `dbus.mainloop.NativeMainLoop`\n"
"supplied by extension modules like `dbus.mainloop.glib`: they have no\n"
"Python API, but connect themselves to ``libdbus`` using native code.\n"

"Python main-loop wrappers are not yet implemented. They will be objects\n"
"supporting the interface defined by `dbus.mainloop.MainLoop`, with an\n"
"API entirely based on Python methods.\n"
"\n"
);
static PyObject *
set_default_main_loop(PyObject *always_null UNUSED,
                      PyObject *args)
{
    PyObject *new_loop, *old_loop;

    if (!PyArg_ParseTuple(args, "O", &new_loop)) {
        return NULL;
    }
    if (!dbus_py_check_mainloop_sanity(new_loop)) {
        return NULL;
    }
    old_loop = default_main_loop;
    Py_INCREF(new_loop);
    default_main_loop = new_loop;
    Py_XDECREF(old_loop);
    Py_RETURN_NONE;
}

static PyMethodDef module_functions[] = {
#define ENTRY(name,flags) {#name, (PyCFunction)name, flags, name##__doc__}
    ENTRY(validate_interface_name, METH_VARARGS),
    ENTRY(validate_member_name, METH_VARARGS),
    ENTRY(validate_bus_name, METH_VARARGS|METH_KEYWORDS),
    ENTRY(validate_object_path, METH_VARARGS),
    ENTRY(set_default_main_loop, METH_VARARGS),
    ENTRY(get_default_main_loop, METH_NOARGS),
    /* validate_error_name is just implemented as validate_interface_name */
    {"validate_error_name", validate_interface_name,
     METH_VARARGS, validate_error_name__doc__},
#undef ENTRY
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
init_dbus_bindings(void)
{
    PyObject *this_module, *c_api;
    static const int API_count = DBUS_BINDINGS_API_COUNT;
    static _dbus_py_func_ptr dbus_bindings_API[DBUS_BINDINGS_API_COUNT];

    dbus_bindings_API[0] = (_dbus_py_func_ptr)&API_count;
    dbus_bindings_API[1] = (_dbus_py_func_ptr)DBusPyConnection_BorrowDBusConnection;
    dbus_bindings_API[2] = (_dbus_py_func_ptr)DBusPyNativeMainLoop_New4;

    default_main_loop = NULL;

    /* I'd rather not initialize threads if we can help it - dbus-python and
    pygobject both release and re-obtain the GIL on a regular basis, which is
    much simpler (basically free) before threads are initialized.

    However, on Python < 2.4.2c1 you aren't allowed to call
    PyGILState_Release without initializing threads first. */
    if (strcmp(Py_GetVersion(), "2.4.2c1") < 0) {
        PyEval_InitThreads();
    }

    if (!dbus_py_init_generic()) return;
    if (!dbus_py_init_abstract()) return;
    if (!dbus_py_init_signature()) return;
    if (!dbus_py_init_int_types()) return;
    if (!dbus_py_init_string_types()) return;
    if (!dbus_py_init_float_types()) return;
    if (!dbus_py_init_container_types()) return;
    if (!dbus_py_init_byte_types()) return;
    if (!dbus_py_init_message_types()) return;
    if (!dbus_py_init_pending_call()) return;
    if (!dbus_py_init_mainloop()) return;
    if (!dbus_py_init_libdbus_conn_types()) return;
    if (!dbus_py_init_conn_types()) return;
    if (!dbus_py_init_server_types()) return;

    this_module = Py_InitModule3("_dbus_bindings", module_functions, module_doc);
    if (!this_module) return;

    if (!dbus_py_insert_abstract_types(this_module)) return;
    if (!dbus_py_insert_signature(this_module)) return;
    if (!dbus_py_insert_int_types(this_module)) return;
    if (!dbus_py_insert_string_types(this_module)) return;
    if (!dbus_py_insert_float_types(this_module)) return;
    if (!dbus_py_insert_container_types(this_module)) return;
    if (!dbus_py_insert_byte_types(this_module)) return;
    if (!dbus_py_insert_message_types(this_module)) return;
    if (!dbus_py_insert_pending_call(this_module)) return;
    if (!dbus_py_insert_mainloop_types(this_module)) return;
    if (!dbus_py_insert_libdbus_conn_types(this_module)) return;
    if (!dbus_py_insert_conn_types(this_module)) return;
    if (!dbus_py_insert_server_types(this_module)) return;

    if (PyModule_AddStringConstant(this_module, "BUS_DAEMON_NAME",
                                   DBUS_SERVICE_DBUS) < 0) return;
    if (PyModule_AddStringConstant(this_module, "BUS_DAEMON_PATH",
                                   DBUS_PATH_DBUS) < 0) return;
    if (PyModule_AddStringConstant(this_module, "BUS_DAEMON_IFACE",
                                   DBUS_INTERFACE_DBUS) < 0) return;
    if (PyModule_AddStringConstant(this_module, "LOCAL_PATH",
                                   DBUS_PATH_LOCAL) < 0) return;
    if (PyModule_AddStringConstant(this_module, "LOCAL_IFACE",
                                   DBUS_INTERFACE_LOCAL) < 0) return;
    if (PyModule_AddStringConstant(this_module, "INTROSPECTABLE_IFACE",
                                   DBUS_INTERFACE_INTROSPECTABLE) < 0) return;
    if (PyModule_AddStringConstant(this_module, "PEER_IFACE",
                                   DBUS_INTERFACE_PEER) < 0) return;
    if (PyModule_AddStringConstant(this_module, "PROPERTIES_IFACE",
                                   DBUS_INTERFACE_PROPERTIES) < 0) return;
    if (PyModule_AddStringConstant(this_module,
                "DBUS_INTROSPECT_1_0_XML_PUBLIC_IDENTIFIER",
                DBUS_INTROSPECT_1_0_XML_PUBLIC_IDENTIFIER) < 0) return;
    if (PyModule_AddStringConstant(this_module,
                "DBUS_INTROSPECT_1_0_XML_SYSTEM_IDENTIFIER",
                DBUS_INTROSPECT_1_0_XML_SYSTEM_IDENTIFIER) < 0) return;
    if (PyModule_AddStringConstant(this_module,
                "DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE",
                DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE) < 0) return;

#define ADD_CONST_VAL(x, v) \
    if (PyModule_AddIntConstant(this_module, x, v) < 0) return;
#define ADD_CONST_PREFIXED(x) ADD_CONST_VAL(#x, DBUS_##x)
#define ADD_CONST(x) ADD_CONST_VAL(#x, x)

    ADD_CONST(DBUS_START_REPLY_SUCCESS)
    ADD_CONST(DBUS_START_REPLY_ALREADY_RUNNING)

    ADD_CONST_PREFIXED(RELEASE_NAME_REPLY_RELEASED)
    ADD_CONST_PREFIXED(RELEASE_NAME_REPLY_NON_EXISTENT)
    ADD_CONST_PREFIXED(RELEASE_NAME_REPLY_NOT_OWNER)

    ADD_CONST_PREFIXED(REQUEST_NAME_REPLY_PRIMARY_OWNER)
    ADD_CONST_PREFIXED(REQUEST_NAME_REPLY_IN_QUEUE)
    ADD_CONST_PREFIXED(REQUEST_NAME_REPLY_EXISTS)
    ADD_CONST_PREFIXED(REQUEST_NAME_REPLY_ALREADY_OWNER)

    ADD_CONST_PREFIXED(NAME_FLAG_ALLOW_REPLACEMENT)
    ADD_CONST_PREFIXED(NAME_FLAG_REPLACE_EXISTING)
    ADD_CONST_PREFIXED(NAME_FLAG_DO_NOT_QUEUE)

    ADD_CONST_PREFIXED(BUS_SESSION)
    ADD_CONST_PREFIXED(BUS_SYSTEM)
    ADD_CONST_PREFIXED(BUS_STARTER)

    ADD_CONST_PREFIXED(MESSAGE_TYPE_INVALID)
    ADD_CONST_PREFIXED(MESSAGE_TYPE_METHOD_CALL)
    ADD_CONST_PREFIXED(MESSAGE_TYPE_METHOD_RETURN)
    ADD_CONST_PREFIXED(MESSAGE_TYPE_ERROR)
    ADD_CONST_PREFIXED(MESSAGE_TYPE_SIGNAL)

    ADD_CONST_PREFIXED(MESSAGE_TYPE_SIGNAL)

    ADD_CONST_PREFIXED(TYPE_INVALID)
    ADD_CONST_PREFIXED(TYPE_BYTE)
    ADD_CONST_PREFIXED(TYPE_BOOLEAN)
    ADD_CONST_PREFIXED(TYPE_INT16)
    ADD_CONST_PREFIXED(TYPE_UINT16)
    ADD_CONST_PREFIXED(TYPE_INT32)
    ADD_CONST_PREFIXED(TYPE_UINT32)
    ADD_CONST_PREFIXED(TYPE_INT64)
    ADD_CONST_PREFIXED(TYPE_UINT64)
    ADD_CONST_PREFIXED(TYPE_DOUBLE)
    ADD_CONST_PREFIXED(TYPE_STRING)
    ADD_CONST_PREFIXED(TYPE_OBJECT_PATH)
    ADD_CONST_PREFIXED(TYPE_SIGNATURE)
    ADD_CONST_PREFIXED(TYPE_ARRAY)
    ADD_CONST_PREFIXED(TYPE_STRUCT)
    ADD_CONST_VAL("STRUCT_BEGIN", DBUS_STRUCT_BEGIN_CHAR)
    ADD_CONST_VAL("STRUCT_END", DBUS_STRUCT_END_CHAR)
    ADD_CONST_PREFIXED(TYPE_VARIANT)
    ADD_CONST_PREFIXED(TYPE_DICT_ENTRY)
    ADD_CONST_VAL("DICT_ENTRY_BEGIN", DBUS_DICT_ENTRY_BEGIN_CHAR)
    ADD_CONST_VAL("DICT_ENTRY_END", DBUS_DICT_ENTRY_END_CHAR)

    ADD_CONST_PREFIXED(HANDLER_RESULT_HANDLED)
    ADD_CONST_PREFIXED(HANDLER_RESULT_NOT_YET_HANDLED)
    ADD_CONST_PREFIXED(HANDLER_RESULT_NEED_MEMORY)

    ADD_CONST_PREFIXED(WATCH_READABLE)
    ADD_CONST_PREFIXED(WATCH_WRITABLE)
    ADD_CONST_PREFIXED(WATCH_HANGUP)
    ADD_CONST_PREFIXED(WATCH_ERROR)

    if (PyModule_AddStringConstant(this_module, "__docformat__",
                                   "restructuredtext") < 0) return;

    if (PyModule_AddStringConstant(this_module, "__version__",
                                   PACKAGE_VERSION) < 0) return;

    if (PyModule_AddIntConstant(this_module, "_python_version",
                                PY_VERSION_HEX) < 0) return;

    c_api = PyCObject_FromVoidPtr ((void *)dbus_bindings_API, NULL);
    if (!c_api) {
        return;
    }
    PyModule_AddObject(this_module, "_C_API", c_api);
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
