/* D-Bus Byte and ByteArray types.
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

#include <Python.h>
#include <structmember.h>

#include <stdint.h>

#include "dbus_bindings-internal.h"
#include "types-internal.h"

static PyObject *Byte = NULL;
static PyObject *ByteArray = NULL;

static dbus_bool_t
do_import(void)
{
    PyObject *name;
    PyObject *module;

    if (ByteArray != NULL && Byte != NULL)
        return TRUE;

    Py_CLEAR(Byte);
    Py_CLEAR(ByteArray);

    name = PyString_FromString("dbus.data");
    if (!name)
        return FALSE;

    module = PyImport_Import(name);
    Py_DECREF(name);
    if (!module)
        return FALSE;

    Byte = PyObject_GetAttrString(module, "Byte");
    ByteArray = PyObject_GetAttrString(module, "ByteArray");
    Py_DECREF(module);

    if (Byte && !PyType_Check(Byte)) {
        PyErr_SetString(PyExc_AssertionError, "Assertion failed: "
                        "isinstance(dbus.data.Byte, type)");
        Byte = NULL;
    }
    if (ByteArray && !PyType_Check(ByteArray)) {
        PyErr_SetString(PyExc_AssertionError, "Assertion failed: "
                        "isinstance(dbus.data.ByteArray, type)");
        ByteArray = NULL;
    }

    return (ByteArray != NULL && Byte != NULL);
}

int
DBusPyByte_Check(PyObject *o)
{
    if (!Byte && !do_import())
        return 0;
    return PyObject_TypeCheck(o, (PyTypeObject *)Byte);
}

int
DBusPyByteArray_Check(PyObject *o)
{
    if (!ByteArray && !do_import())
        return 0;
    return PyObject_TypeCheck(o, (PyTypeObject *)ByteArray);
}

PyObject *
DBusPyByte_New(unsigned char value, long variant_level)
{
    PyObject *args = NULL;
    PyObject *kwargs = NULL;
    PyObject *ret = NULL;

    if (!Byte && !do_import())
        return NULL;

    if (variant_level != 0) {
        kwargs = DBusPy_BuildConstructorKeywordArgs(variant_level, NULL);
        if (!kwargs)
            goto finally;
    }

    args = Py_BuildValue("(I)", (unsigned int) value);
    if (!args)
        goto finally;

    ret = PyObject_Call(Byte, args, kwargs);

finally:
    Py_XDECREF(args);
    Py_XDECREF(kwargs);
    return ret;
}

PyObject *
DBusPyByteArray_New(const char *bytes, Py_ssize_t count, long variant_level)
{
    PyObject *args = NULL;
    PyObject *kwargs = NULL;
    PyObject *ret = NULL;

    if (!ByteArray && !do_import())
        return NULL;

    if (variant_level != 0) {
        kwargs = DBusPy_BuildConstructorKeywordArgs(variant_level, NULL);
        if (!kwargs)
            goto finally;
    }

    args = Py_BuildValue("(s#)", bytes, count);
    if (!args)
        goto finally;

    ret = PyObject_Call(ByteArray, args, kwargs);

finally:
    Py_XDECREF(args);
    Py_XDECREF(kwargs);
    return ret;
}

dbus_bool_t
dbus_py_init_byte_types(void)
{
    return 1;
}

dbus_bool_t
dbus_py_insert_byte_types(PyObject *this_module UNUSED)
{
    return 1;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
