/* Simple D-Bus types: integers of various sizes, and ObjectPath.
 *
 * Copyright (C) 2006 Collabora Ltd.
 *
 * Licensed under the Academic Free License version 2.1
 *
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

#include "types-internal.h"

static PyObject *DBusPyBoolean = NULL;
static PyObject *Int16 = NULL;
static PyObject *UInt16 = NULL;
static PyObject *Int32 = NULL;
static PyObject *UInt32 = NULL;
static PyObject *Int64 = NULL;
static PyObject *UInt64 = NULL;

static dbus_bool_t
do_import(void)
{
    PyObject *name;
    PyObject *module;

    if (DBusPyBoolean && Int16 && UInt16 && Int32 && UInt32 &&
        Int64 && UInt64)
        return TRUE;

    Py_CLEAR(DBusPyBoolean);
    Py_CLEAR(Int16);
    Py_CLEAR(UInt16);
    Py_CLEAR(Int32);
    Py_CLEAR(UInt32);
    Py_CLEAR(Int64);
    Py_CLEAR(UInt64);

    name = PyString_FromString("dbus.data");
    if (!name)
        return FALSE;

    module = PyImport_Import(name);
    Py_DECREF(name);
    if (!module)
        return FALSE;

    DBusPyBoolean = PyObject_GetAttrString(module, "Boolean");
    Int16 = PyObject_GetAttrString(module, "Int16");
    UInt16 = PyObject_GetAttrString(module, "UInt16");
    Int32 = PyObject_GetAttrString(module, "Int32");
    UInt32 = PyObject_GetAttrString(module, "UInt32");
    Int64 = PyObject_GetAttrString(module, "Int64");
    UInt64 = PyObject_GetAttrString(module, "UInt64");
    Py_DECREF(module);

    if (DBusPyBoolean && !PyType_Check(DBusPyBoolean)) {
        PyErr_SetString(PyExc_AssertionError, "Assertion failed: "
                        "isinstance(dbus.data.Boolean, type)");
        DBusPyBoolean = NULL;
    }
    if (Int16 && !PyType_Check(Int16)) {
        PyErr_SetString(PyExc_AssertionError, "Assertion failed: "
                        "isinstance(dbus.data.Int16, type)");
        Int16 = NULL;
    }
    if (UInt16 && !PyType_Check(UInt16)) {
        PyErr_SetString(PyExc_AssertionError, "Assertion failed: "
                        "isinstance(dbus.data.UInt16, type)");
        UInt16 = NULL;
    }
    if (Int32 && !PyType_Check(Int32)) {
        PyErr_SetString(PyExc_AssertionError, "Assertion failed: "
                        "isinstance(dbus.data.Int32, type)");
        Int32 = NULL;
    }
    if (UInt32 && !PyType_Check(UInt32)) {
        PyErr_SetString(PyExc_AssertionError, "Assertion failed: "
                        "isinstance(dbus.data.UInt32, type)");
        UInt32 = NULL;
    }
    if (Int64 && !PyType_Check(Int64)) {
        PyErr_SetString(PyExc_AssertionError, "Assertion failed: "
                        "isinstance(dbus.data.Int64, type)");
        Int64 = NULL;
    }
    if (UInt64 && !PyType_Check(UInt64)) {
        PyErr_SetString(PyExc_AssertionError, "Assertion failed: "
                        "isinstance(dbus.data.UInt64, type)");
        UInt64 = NULL;
    }

    return DBusPyBoolean && Int16 && UInt16 && Int32 && UInt32 && Int64 &&
        UInt64;
}

int
DBusPyBoolean_Check(PyObject *o)
{
    if (!DBusPyBoolean && !do_import())
        return 0;
    return PyObject_TypeCheck(o, (PyTypeObject *)DBusPyBoolean);
}

PyObject *
DBusPyBoolean_New(int is_true, long variant_level)
{
    PyObject *args = NULL;
    PyObject *kwargs = NULL;
    PyObject *ret = NULL;

    if (!DBusPyBoolean && !do_import())
        return NULL;

    if (variant_level != 0) {
        kwargs = DBusPy_BuildConstructorKeywordArgs(variant_level, NULL);
        if (!kwargs)
            goto finally;
    }

    args = Py_BuildValue("(i)", is_true);
    if (!args)
        goto finally;

    ret = PyObject_Call(DBusPyBoolean, args, kwargs);

finally:
    Py_XDECREF(args);
    Py_XDECREF(kwargs);
    return ret;
}

int
DBusPyInt16_Check(PyObject *o)
{
    if (!Int16 && !do_import())
        return 0;
    return PyObject_TypeCheck(o, (PyTypeObject *)Int16);
}

int
DBusPyUInt16_Check(PyObject *o)
{
    if (!UInt16 && !do_import())
        return 0;
    return PyObject_TypeCheck(o, (PyTypeObject *)UInt16);
}

int
DBusPyInt32_Check(PyObject *o)
{
    if (!Int32 && !do_import())
        return 0;
    return PyObject_TypeCheck(o, (PyTypeObject *)Int32);
}

int
DBusPyUInt32_Check(PyObject *o)
{
    if (!UInt32 && !do_import())
        return 0;
    return PyObject_TypeCheck(o, (PyTypeObject *)UInt32);
}

int
DBusPyInt64_Check(PyObject *o)
{
    if (!Int64 && !do_import())
        return 0;
    return PyObject_TypeCheck(o, (PyTypeObject *)Int64);
}

int
DBusPyUInt64_Check(PyObject *o)
{
    if (!UInt64 && !do_import())
        return 0;
    return PyObject_TypeCheck(o, (PyTypeObject *)UInt64);
}

PyObject *
DBusPyInt16_New(dbus_int16_t value, long variant_level)
{
    PyObject *args = NULL;
    PyObject *kwargs = NULL;
    PyObject *ret = NULL;

    if (!Int16 && !do_import())
        return NULL;

    if (variant_level != 0) {
        kwargs = DBusPy_BuildConstructorKeywordArgs(variant_level, NULL);
        if (!kwargs)
            goto finally;
    }

    args = Py_BuildValue("(i)", (int)value);
    if (!args)
        goto finally;

    ret = PyObject_Call(Int16, args, kwargs);

finally:
    Py_XDECREF(args);
    Py_XDECREF(kwargs);
    return ret;
}

dbus_int16_t
dbus_py_int16_range_check(PyObject *obj)
{
    long i = PyInt_AsLong (obj);
    if (i == -1 && PyErr_Occurred ()) return -1;
    if (i < -0x8000 || i > 0x7fff) {
        PyErr_Format(PyExc_OverflowError, "Value %d out of range for Int16",
                     (int)i);
        return -1;
    }
    return i;
}

PyObject *
DBusPyUInt16_New(dbus_uint16_t value, long variant_level)
{
    PyObject *args = NULL;
    PyObject *kwargs = NULL;
    PyObject *ret = NULL;

    if (!UInt16 && !do_import())
        return NULL;

    if (variant_level != 0) {
        kwargs = DBusPy_BuildConstructorKeywordArgs(variant_level, NULL);
        if (!kwargs)
            goto finally;
    }

    args = Py_BuildValue("(I)", (unsigned int)value);
    if (!args)
        goto finally;

    ret = PyObject_Call(UInt16, args, kwargs);

finally:
    Py_XDECREF(args);
    Py_XDECREF(kwargs);
    return ret;
}

dbus_uint16_t
dbus_py_uint16_range_check(PyObject *obj)
{
    long i = PyInt_AsLong(obj);
    if (i == -1 && PyErr_Occurred()) return (dbus_uint16_t)(-1);
    if (i < 0 || i > 0xffff) {
        PyErr_Format(PyExc_OverflowError, "Value %d out of range for UInt16",
                     (int)i);
        return (dbus_uint16_t)(-1);
    }
    return i;
}

PyObject *
DBusPyInt32_New(dbus_int32_t value, long variant_level)
{
    PyObject *args = NULL;
    PyObject *kwargs = NULL;
    PyObject *ret = NULL;

    if (!Int32 && !do_import())
        return NULL;

    if (variant_level != 0) {
        kwargs = DBusPy_BuildConstructorKeywordArgs(variant_level, NULL);
        if (!kwargs)
            goto finally;
    }

    args = Py_BuildValue("(l)", (long)value);
    if (!args)
        goto finally;

    ret = PyObject_Call(Int32, args, kwargs);

finally:
    Py_XDECREF(args);
    Py_XDECREF(kwargs);
    return ret;
}

dbus_int32_t
dbus_py_int32_range_check(PyObject *obj)
{
    long i = PyInt_AsLong(obj);
    if (i == -1 && PyErr_Occurred()) return -1;
    if (i < INT32_MIN || i > INT32_MAX) {
        PyErr_Format(PyExc_OverflowError, "Value %d out of range for Int32",
                     (int)i);
        return -1;
    }
    return i;
}

PyObject *
DBusPyUInt32_New(dbus_uint32_t value, long variant_level)
{
    PyObject *args = NULL;
    PyObject *kwargs = NULL;
    PyObject *ret = NULL;

    if (!UInt32 && !do_import())
        return NULL;

    if (variant_level != 0) {
        kwargs = DBusPy_BuildConstructorKeywordArgs(variant_level, NULL);
        if (!kwargs)
            goto finally;
    }

    args = Py_BuildValue("(k)", (unsigned long)value);
    if (!args)
        goto finally;

    ret = PyObject_Call(UInt32, args, kwargs);

finally:
    Py_XDECREF(args);
    Py_XDECREF(kwargs);
    return ret;
}

dbus_uint32_t
dbus_py_uint32_range_check(PyObject *obj)
{
    unsigned long i;
    PyObject *long_obj = PyNumber_Long(obj);

    if (!long_obj) return (dbus_uint32_t)(-1);
    i = PyLong_AsUnsignedLong(long_obj);
    if (i == (unsigned long)(-1) && PyErr_Occurred()) {
        Py_DECREF(long_obj);
        return (dbus_uint32_t)(-1);
    }
    if (i > UINT32_MAX) {
        PyErr_Format(PyExc_OverflowError, "Value %d out of range for UInt32",
                     (int)i);
        Py_DECREF(long_obj);
        return (dbus_uint32_t)(-1);
    }
    Py_DECREF(long_obj);
    return i;
}

PyObject *
DBusPyInt64_New(dbus_int64_t value, long variant_level)
{
    PyObject *args = NULL;
    PyObject *kwargs = NULL;
    PyObject *ret = NULL;

    if (!Int64 && !do_import())
        return NULL;

    if (variant_level != 0) {
        kwargs = DBusPy_BuildConstructorKeywordArgs(variant_level, NULL);
        if (!kwargs)
            goto finally;
    }

    args = Py_BuildValue("(L)", (PY_LONG_LONG)value);
    if (!args)
        goto finally;

    ret = PyObject_Call(Int64, args, kwargs);

finally:
    Py_XDECREF(args);
    Py_XDECREF(kwargs);
    return ret;
}

#ifdef DBUS_PYTHON_64_BIT_WORKS
dbus_int64_t
dbus_py_int64_range_check(PyObject *obj)
{
    PY_LONG_LONG i;
    PyObject *long_obj = PyNumber_Long(obj);

    if (!long_obj) return -1;
    i = PyLong_AsLongLong(long_obj);
    if (i == -1 && PyErr_Occurred()) {
        Py_DECREF(long_obj);
        return -1;
    }
    if (i < INT64_MIN || i > INT64_MAX) {
        PyErr_SetString(PyExc_OverflowError, "Value out of range for Int64");
        Py_DECREF(long_obj);
        return -1;
    }
    Py_DECREF(long_obj);
    return i;
}
#endif

PyObject *
DBusPyUInt64_New(dbus_uint64_t value, long variant_level)
{
    PyObject *args = NULL;
    PyObject *kwargs = NULL;
    PyObject *ret = NULL;

    if (!UInt64 && !do_import())
        return NULL;

    if (variant_level != 0) {
        kwargs = DBusPy_BuildConstructorKeywordArgs(variant_level, NULL);
        if (!kwargs)
            goto finally;
    }

    args = Py_BuildValue("(K)", (unsigned PY_LONG_LONG)value);
    if (!args)
        goto finally;

    ret = PyObject_Call(UInt64, args, kwargs);

finally:
    Py_XDECREF(args);
    Py_XDECREF(kwargs);
    return ret;
}

#ifdef DBUS_PYTHON_64_BIT_WORKS
dbus_uint64_t
dbus_py_uint64_range_check(PyObject *obj)
{
    unsigned PY_LONG_LONG i;
    PyObject *long_obj = PyNumber_Long(obj);

    if (!long_obj) return -1;
    i = PyLong_AsUnsignedLongLong(long_obj);
    if (i == (unsigned PY_LONG_LONG)-1 && PyErr_Occurred()) {
        Py_DECREF(long_obj);
        return -1;
    }
    if (i > UINT64_MAX) {
        PyErr_SetString(PyExc_OverflowError, "Value out of range for UInt64");
        Py_DECREF(long_obj);
        return -1;
    }
    Py_DECREF(long_obj);
    return i;
}
#endif

dbus_bool_t
dbus_py_init_int_types(void)
{
    return 1;
}

dbus_bool_t
dbus_py_insert_int_types(PyObject *this_module UNUSED)
{
    return 1;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
