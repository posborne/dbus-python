/* Simple D-Bus types: ObjectPath and other string types.
 *
 * Copyright (C) 2006-2007 Collabora Ltd. <http://www.collabora.co.uk/>
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

    if (String && !PyType_Check(String)) {
        PyErr_SetString(PyExc_AssertionError, "Assertion failed: "
                        "isinstance(dbus.data.String, type)");
        String = NULL;
    }
    if (UTF8String && !PyType_Check(UTF8String)) {
        PyErr_SetString(PyExc_AssertionError, "Assertion failed: "
                        "isinstance(dbus.data.UTF8String, type)");
        UTF8String = NULL;
    }
    if (ObjectPath && !PyType_Check(ObjectPath)) {
        PyErr_SetString(PyExc_AssertionError, "Assertion failed: "
                        "isinstance(dbus.data.ObjectPath, type)");
        ObjectPath = NULL;
    }

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
