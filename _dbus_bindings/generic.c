/* General Python glue code, used in _dbus_bindings but not actually anything
 * to do with D-Bus.
 *
 * Copyright (C) 2006 Collabora Ltd.
 *
 * Licensed under the Academic Free License version 2.1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "dbus_bindings-internal.h"

PyObject *dbus_py_empty_tuple = NULL;

PyObject *
dbus_py_tp_richcompare_by_pointer(PyObject *self,
                                  PyObject *other,
                                  int op)
{
    if (op == Py_EQ || op == Py_NE) {
        if (self == other) {
            return PyInt_FromLong(op == Py_EQ);
        }
        return PyInt_FromLong(op == Py_NE);
    }
    PyErr_SetString(PyExc_TypeError,
                    "Instances of this type are not ordered");
    return NULL;
}

long
dbus_py_tp_hash_by_pointer(PyObject *self)
{
    long hash = (long)self;
    return (hash == -1L ? -2L : hash);
}

int
dbus_py_immutable_setattro(PyObject *obj UNUSED,
                        PyObject *name UNUSED,
                        PyObject *value UNUSED)
{
    PyErr_SetString(PyExc_AttributeError, "Object is immutable");
    return -1;
}

/* Take the global interpreter lock and decrement the reference count.
 * Suitable for calling from a C callback. */
void
dbus_py_take_gil_and_xdecref(PyObject *obj)
{
    PyGILState_STATE gil = PyGILState_Ensure();
    Py_XDECREF(obj);
    PyGILState_Release(gil);
}

dbus_bool_t
dbus_py_init_generic(void)
{
    dbus_py_empty_tuple = PyTuple_New(0);
    if (!dbus_py_empty_tuple) return 0;
    return 1;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
