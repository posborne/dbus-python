/* D-Bus container types: Array, Dict and Struct.
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

#include <Python.h>
#include <structmember.h>

#include <stdint.h>

#include "dbus_bindings-internal.h"
#include "types-internal.h"

/* Array ============================================================ */

static PyObject *Array = NULL;
static PyObject *Struct = NULL;
static PyObject *Dictionary = NULL;

static dbus_bool_t
do_import(void)
{
    PyObject *name;
    PyObject *module;

    if (Array && Struct && Dictionary)
        return TRUE;

    Py_CLEAR(Array);
    Py_CLEAR(Struct);
    Py_CLEAR(Dictionary);

    name = PyString_FromString("dbus.data");
    if (!name)
        return FALSE;

    module = PyImport_Import(name);
    Py_DECREF(name);
    if (!module)
        return FALSE;

    Array = PyObject_GetAttrString(module, "Array");
    Struct = PyObject_GetAttrString(module, "Struct");
    Dictionary = PyObject_GetAttrString(module, "Dictionary");

    Py_DECREF(module);

    if (Array && !PyType_Check(Array)) {
        PyErr_SetString(PyExc_AssertionError, "Assertion failed: "
                        "isinstance(dbus.data.Array, type)");
        Array = NULL;
    }
    if (Struct && !PyType_Check(Struct)) {
        PyErr_SetString(PyExc_AssertionError, "Assertion failed: "
                        "isinstance(dbus.data.Struct, type)");
        Struct = NULL;
    }
    if (Dictionary && !PyType_Check(Dictionary)) {
        PyErr_SetString(PyExc_AssertionError, "Assertion failed: "
                        "isinstance(dbus.data.Dictionary, type)");
        Dictionary = NULL;
    }

    return (Array != NULL && Struct != NULL && Dictionary != NULL);
}

int
DBusPyArray_Check(PyObject *o)
{
    if (!Array && !do_import())
        return 0;
    return PyObject_TypeCheck(o, (PyTypeObject *)Array);
}

PyObject *
DBusPyArray_New(const char *signature, long variant_level)
{
    PyObject *kwargs = NULL;
    PyObject *ret = NULL;

    if (!Array && !do_import())
        return NULL;

    if (variant_level != 0) {
        kwargs = DBusPy_BuildConstructorKeywordArgs(variant_level, signature);
        if (!kwargs)
            goto finally;
    }

    ret = PyObject_Call(Array, dbus_py_empty_tuple, kwargs);

finally:
    Py_XDECREF(kwargs);
    return ret;
}

/* Dict ============================================================= */

int
DBusPyDictionary_Check(PyObject *o)
{
    if (!Dictionary && !do_import())
        return 0;
    return PyObject_TypeCheck(o, (PyTypeObject *)Dictionary);
}

PyObject *
DBusPyDictionary_New(const char *signature, long variant_level)
{
    PyObject *kwargs = NULL;
    PyObject *ret = NULL;

    if (!Dictionary && !do_import())
        return NULL;

    if (variant_level != 0) {
        kwargs = DBusPy_BuildConstructorKeywordArgs(variant_level, signature);
        if (!kwargs)
            goto finally;
    }

    ret = PyObject_Call(Dictionary, dbus_py_empty_tuple, kwargs);

finally:
    Py_XDECREF(kwargs);
    return ret;
}

/* Struct =========================================================== */

int
DBusPyStruct_Check(PyObject *o)
{
    if (!Struct && !do_import())
        return 0;
    return PyObject_TypeCheck(o, (PyTypeObject *)Struct);
}

PyObject *
DBusPyStruct_New(PyObject *iterable, long variant_level)
{
    PyObject *args = NULL;
    PyObject *kwargs = NULL;
    PyObject *ret = NULL;

    if (!Struct && !do_import())
        return NULL;

    if (variant_level != 0) {
        kwargs = DBusPy_BuildConstructorKeywordArgs(variant_level, NULL);
        if (!kwargs)
            goto finally;
    }

    args = Py_BuildValue("(O)", iterable);
    if (!args)
        goto finally;

    ret = PyObject_Call(Struct, args, kwargs);

finally:
    Py_XDECREF(args);
    Py_XDECREF(kwargs);
    return ret;
}

dbus_bool_t
dbus_py_init_container_types(void)
{
    return 1;
}

dbus_bool_t
dbus_py_insert_container_types(PyObject *this_module UNUSED)
{
    return 1;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */

