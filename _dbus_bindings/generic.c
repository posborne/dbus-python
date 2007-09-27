/* General Python glue code, used in _dbus_bindings but not actually anything
 * to do with D-Bus.
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

#include "dbus_bindings-internal.h"

/* The empty tuple, held globally since dbus-python turns out to use it quite
 * a lot
 */
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
