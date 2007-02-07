/* C API for _dbus_bindings, used by _dbus_glib_bindings and any third-party
 * main loop integration which might happen in future.
 *
 * This file is currently Python-version-independent - please keep it that way.
 *
 * Copyright (C) 2006 Collabora Ltd.
 *
 * Licensed under the Academic Free License version 2.1
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef DBUS_PYTHON_H
#define DBUS_PYTHON_H

#include <Python.h>
#define DBUS_API_SUBJECT_TO_CHANGE 1
#include <dbus/dbus.h>

DBUS_BEGIN_DECLS

typedef void (*_dbus_py_func_ptr)(void);

typedef dbus_bool_t (*_dbus_py_conn_setup_func)(DBusConnection *, void *);
typedef dbus_bool_t (*_dbus_py_srv_setup_func)(DBusServer *, void *);
typedef void (*_dbus_py_free_func)(void *);

#define DBUS_BINDINGS_API_COUNT 3

#ifdef INSIDE_DBUS_PYTHON_BINDINGS

extern DBusConnection *DBusPyConnection_BorrowDBusConnection(PyObject *);
extern PyObject *DBusPyNativeMainLoop_New4(_dbus_py_conn_setup_func,
                                           _dbus_py_srv_setup_func,
                                           _dbus_py_free_func,
                                           void *);

#else

static PyObject *_dbus_bindings_module = NULL;
static _dbus_py_func_ptr *dbus_bindings_API;

#define DBusPyConnection_BorrowDBusConnection \
        (*(DBusConnection *(*)(PyObject *))dbus_bindings_API[1])
#define DBusPyNativeMainLoop_New4 \
    ((PyObject *(*)(_dbus_py_conn_setup_func, _dbus_py_srv_setup_func, \
                    _dbus_py_free_func, void *))dbus_bindings_API[2])

static int
import_dbus_bindings(const char *this_module_name)
{
    PyObject *c_api;
    int count;

    _dbus_bindings_module = PyImport_ImportModule("_dbus_bindings");
    if (!_dbus_bindings_module) {
        return -1;
    }
    c_api = PyObject_GetAttrString(_dbus_bindings_module, "_C_API");
    if (c_api == NULL) return -1;
    if (PyCObject_Check(c_api)) {
        dbus_bindings_API = (_dbus_py_func_ptr *)PyCObject_AsVoidPtr(c_api);
    }
    else {
        Py_DECREF(c_api);
        PyErr_SetString(PyExc_RuntimeError, "C API is not a PyCObject");
        return -1;
    }
    Py_DECREF (c_api);
    count = *(int *)dbus_bindings_API[0];
    if (count < DBUS_BINDINGS_API_COUNT) {
        PyErr_Format(PyExc_RuntimeError,
                     "_dbus_bindings has API version %d but %s needs "
                     "_dbus_bindings API version at least %d",
                     count, this_module_name,
                     DBUS_BINDINGS_API_COUNT);
        return -1;
    }
    return 0;
}

#endif

DBUS_END_DECLS

#endif
