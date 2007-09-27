/* D-Bus Byte and ByteArray types.
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
