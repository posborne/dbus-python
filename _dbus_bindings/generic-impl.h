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

#define DEFERRED_ADDRESS(ADDR) 0

static PyObject *empty_tuple = NULL;

/* A generic repr() implementation for int subclasses, returning something
 * like 'dbus.Int16(123)'.
 * Equivalent to the following Python:
 * def __repr__(self):
 *     return '%s(%r)' % (self.__class__, int.__repr__(self))
 */
static PyObject *
int_subclass_tp_repr (PyObject *self)
{
    PyObject *parent_repr = (PyInt_Type.tp_repr)(self);
    PyObject *my_repr;

    if (!parent_repr) return NULL;
    my_repr = PyString_FromFormat("%s(%s)", self->ob_type->tp_name,
                                  PyString_AS_STRING(parent_repr));
    /* whether my_repr is NULL or not: */
    Py_DECREF(parent_repr);
    return my_repr;
}

/* A generic repr() implementation for long subclasses, returning something
 * like 'dbus.Int64(123L)'.
 * Equivalent to the following Python:
 * def __repr__(self):
 *     return '%s(%r)' % (self.__class__, long.__repr__(self))
 */
static PyObject *
long_subclass_tp_repr(PyObject *self)
{
    PyObject *parent_repr = (PyLong_Type.tp_repr)(self);
    PyObject *my_repr;

    if (!parent_repr) return NULL;
    my_repr = PyString_FromFormat("%s(%s)", self->ob_type->tp_name,
                                  PyString_AS_STRING(parent_repr));
    /* whether my_repr is NULL or not: */
    Py_DECREF(parent_repr);
    return my_repr;
}

/* A generic repr() implementation for float subclasses, returning something
 * like 'dbus.Double(123)'.
 * Equivalent to the following Python:
 * def __repr__(self):
 *     return '%s(%r)' % (self.__class__, float.__repr__(self))
 */
static PyObject *
float_subclass_tp_repr(PyObject *self)
{
    PyObject *parent_repr = (PyFloat_Type.tp_repr)(self);
    PyObject *my_repr;

    if (!parent_repr) return NULL;
    my_repr = PyString_FromFormat("%s(%s)", self->ob_type->tp_name,
                                  PyString_AS_STRING(parent_repr));
    /* whether my_repr is NULL or not: */
    Py_DECREF(parent_repr);
    return my_repr;
}

/* A generic repr() implementation for str subclasses, returning something
 * like "dbus.ObjectPath('/foo/bar')".
 * Equivalent to the following Python:
 * def __repr__(self):
 *     return '%s(%r)' % (self.__class__, str.__repr__(self))
 */
static PyObject *
str_subclass_tp_repr(PyObject *self)
{
    PyObject *parent_repr = (PyString_Type.tp_repr)(self);
    PyObject *my_repr;

    if (!parent_repr) return NULL;
    my_repr = PyString_FromFormat("%s(%s)", self->ob_type->tp_name,
                                  PyString_AS_STRING(parent_repr));
    /* whether my_repr is NULL or not: */
    Py_DECREF(parent_repr);
    return my_repr;
}

/* Take the global interpreter lock and decrement the reference count.
 * Suitable for calling from a C callback. */
static void
Glue_TakeGILAndXDecref(PyObject *obj)
{
    PyGILState_STATE gil = PyGILState_Ensure();
    Py_XDECREF(obj);
    PyGILState_Release(gil);
}

static inline int
init_generic(void)
{
    empty_tuple = PyTuple_New(0);
    if (!empty_tuple) return 0;
    return 1;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
