/* Glue code to attach the GObject main loop to D-Bus from within Python.
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

#include <Python.h>
#include "dbus-python.h"
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

PyMODINIT_FUNC init_dbus_glib_bindings(void);

#if defined(__GNUC__)
#   if __GNUC__ >= 3
#       define UNUSED __attribute__((__unused__))
#   else
#       define UNUSED /*nothing*/
#   endif
#else
#   define UNUSED /*nothing*/
#endif

static dbus_bool_t
dbus_py_glib_set_up_conn(DBusConnection *conn, void *data)
{
    GMainContext *ctx = (GMainContext *)data;
    Py_BEGIN_ALLOW_THREADS
    dbus_connection_setup_with_g_main(conn, ctx);
    Py_END_ALLOW_THREADS
    return 1;
}

static dbus_bool_t
dbus_py_glib_set_up_srv(DBusServer *srv, void *data)
{
    GMainContext *ctx = (GMainContext *)data;
    Py_BEGIN_ALLOW_THREADS
    dbus_server_setup_with_g_main(srv, ctx);
    Py_END_ALLOW_THREADS
    return 1;
}

static void
dbus_py_glib_unref_mainctx(void *data)
{
    if (data)
        g_main_context_unref((GMainContext *)data);
}

/* Generate a dbus-python NativeMainLoop wrapper from a GLib main loop */
static PyObject *
dbus_glib_native_mainloop(GMainContext *ctx)
{
    PyObject *loop = DBusPyNativeMainLoop_New4(dbus_py_glib_set_up_conn,
                                               dbus_py_glib_set_up_srv,
                                               dbus_py_glib_unref_mainctx,
                                               ctx ? g_main_context_ref(ctx)
                                                   : NULL);
    if (!loop && ctx) {
        g_main_context_unref(ctx);
    }
    return loop;
}

PyDoc_STRVAR(module_doc, "");

PyDoc_STRVAR(DBusGMainLoop__doc__,
"DBusGMainLoop([set_as_default=False]) -> NativeMainLoop\n"
"\n"
"Return a NativeMainLoop object which can be used to\n"
"represent the default GLib main context in dbus-python.\n"
"\n"
"If the keyword argument set_as_default is given and is true, set the new\n"
"main loop as the default for all new Connection or Bus instances.\n"
"\n"
"Non-default main contexts are not currently supported.\n");
static PyObject *
DBusGMainLoop (PyObject *always_null UNUSED, PyObject *args, PyObject *kwargs)
{
    PyObject *mainloop, *function, *result;
    int set_as_default = 0;
    static char *argnames[] = {"set_as_default", NULL};

    if (PyTuple_Size(args) != 0) {
        PyErr_SetString(PyExc_TypeError, "DBusGMainLoop() takes no "
                                         "positional arguments");
        return NULL;
    }
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i", argnames,
                                     &set_as_default)) {
        return NULL;
    }

    mainloop = dbus_glib_native_mainloop(NULL);
    if (mainloop && set_as_default) {
        if (!_dbus_bindings_module) {
            PyErr_SetString(PyExc_ImportError, "_dbus_bindings not imported");
            Py_DECREF(mainloop);
            return NULL;
        }
        function = PyObject_GetAttrString(_dbus_bindings_module,
                                          "set_default_main_loop");
        if (!function) {
            Py_DECREF(mainloop);
            return NULL;
        }
        result = PyObject_CallFunctionObjArgs(function, mainloop, NULL);
        Py_DECREF(function);
        if (!result) {
            Py_DECREF(mainloop);
            return NULL;
        }
        Py_DECREF(result);
    }
    return mainloop;
}

PyDoc_STRVAR(setup_with_g_main__doc__,
"setup_with_g_main(conn: dbus.Connection)\n"
"\n"
"Deprecated.\n");
static PyObject *
setup_with_g_main (PyObject *always_null UNUSED, PyObject *args)
{
    DBusConnection *dbc;
    PyObject *conn;
    if (!PyArg_ParseTuple(args, "O:setup_with_g_main", &conn)) return NULL;

    dbc = DBusPyConnection_BorrowDBusConnection (conn);
    if (!dbc) return NULL;
    dbus_py_glib_set_up_conn(dbc, NULL);
    Py_RETURN_NONE;
}

PyDoc_STRVAR(gthreads_init__doc__,
"gthreads_init()");
static PyObject *
gthreads_init (PyObject *always_null UNUSED, PyObject *no_args UNUSED)
{
    dbus_g_thread_init();
    Py_RETURN_NONE;
}

static PyMethodDef module_functions[] = {
    {"setup_with_g_main", setup_with_g_main, METH_VARARGS,
     setup_with_g_main__doc__},
    {"gthreads_init", gthreads_init, METH_NOARGS, gthreads_init__doc__},
    {"DBusGMainLoop", (PyCFunction)DBusGMainLoop,
     METH_VARARGS|METH_KEYWORDS, DBusGMainLoop__doc__},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
init_dbus_glib_bindings(void)
{
    PyObject *this_module;

    if (import_dbus_bindings("_dbus_glib_bindings") < 0) return;
    this_module = Py_InitModule3 ("_dbus_glib_bindings", module_functions,
                                  module_doc);
    if (!this_module) return;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
