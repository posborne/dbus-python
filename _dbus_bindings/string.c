/* Simple D-Bus types: ObjectPath and other string types.
 *
 * Copyright (C) 2006, 2007 Collabora Ltd.
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

#include "types-internal.h"
#include <structmember.h>

static PyObject *String = NULL;
static PyObject *UTF8String = NULL;
static PyObject *ObjectPath = NULL;

static dbus_bool_t
do_import(void)
{
    PyObject *name;
    PyObject *module;

    if (String && ObjectPath && UTF8String)
        return TRUE;

    Py_CLEAR(String);
    Py_CLEAR(UTF8String);
    Py_CLEAR(ObjectPath);

    name = PyString_FromString("dbus.data");
    if (!name)
        return FALSE;

    module = PyImport_Import(name);
    Py_DECREF(name);
    if (!module)
        return FALSE;

    String = PyObject_GetAttrString(module, "String");
    UTF8String = PyObject_GetAttrString(module, "UTF8String");
    ObjectPath = PyObject_GetAttrString(module, "ObjectPath");
    Py_DECREF(module);

    return (String != NULL && UTF8String != NULL && ObjectPath != NULL);
}

int
DBusPyString_Check(PyObject *o)
{
    if (!String && !do_import())
        return 0;
    return PyObject_TypeCheck(o, (PyTypeObject *)String);
}

PyObject *
DBusPyString_New(const char *utf8, long variant_level)
{
    PyObject *args = NULL;
    PyObject *kwargs = NULL;
    PyObject *ret = NULL;

    if (!String && !do_import())
        return NULL;

    if (variant_level != 0) {
        kwargs = DBusPy_BuildConstructorKeywordArgs(variant_level, NULL);
        if (!kwargs)
            goto finally;
    }

    args = Py_BuildValue("(s)", utf8);
    if (!args)
        goto finally;

    ret = PyObject_Call(String, args, kwargs);

finally:
    Py_XDECREF(args);
    Py_XDECREF(kwargs);
    return ret;
}

int
DBusPyObjectPath_Check(PyObject *o)
{
    if (!ObjectPath && !do_import())
        return 0;
    return PyObject_TypeCheck(o, (PyTypeObject *)ObjectPath);
}

PyObject *
DBusPyObjectPath_New(const char *utf8, long variant_level)
{
    PyObject *args = NULL;
    PyObject *kwargs = NULL;
    PyObject *ret = NULL;

    if (!ObjectPath && !do_import())
        return NULL;

    if (variant_level != 0) {
        kwargs = DBusPy_BuildConstructorKeywordArgs(variant_level, NULL);
        if (!kwargs)
            goto finally;
    }

    args = Py_BuildValue("(s)", utf8);
    if (!args)
        goto finally;

    ret = PyObject_Call(ObjectPath, args, kwargs);

finally:
    Py_XDECREF(args);
    Py_XDECREF(kwargs);
    return ret;
}

int
DBusPyUTF8String_Check(PyObject *o)
{
    if (!UTF8String && !do_import())
        return 0;
    return PyObject_TypeCheck(o, (PyTypeObject *)UTF8String);
}

PyObject *
DBusPyUTF8String_New(const char *utf8, long variant_level)
{
    PyObject *args = NULL;
    PyObject *kwargs = NULL;
    PyObject *ret = NULL;

    if (!UTF8String && !do_import())
        return NULL;

    if (variant_level != 0) {
        kwargs = DBusPy_BuildConstructorKeywordArgs(variant_level, NULL);
        if (!kwargs)
            goto finally;
    }

    args = Py_BuildValue("(s)", utf8);
    if (!args)
        goto finally;

    ret = PyObject_Call(UTF8String, args, kwargs);

finally:
    Py_XDECREF(args);
    Py_XDECREF(kwargs);
    return ret;
}

dbus_bool_t
dbus_py_init_string_types(void)
{
    return 1;
}

dbus_bool_t
dbus_py_insert_string_types(PyObject *this_module UNUSED)
{
    return 1;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
