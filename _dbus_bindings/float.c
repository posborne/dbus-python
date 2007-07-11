/* Simple D-Bus types: Double and (with appropriate #defines) Float
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

static PyObject *Double = NULL;

static dbus_bool_t
do_import(void)
{
    PyObject *name;
    PyObject *module;

    if (Double != NULL)
        return TRUE;

    Py_CLEAR(Double);

    name = PyString_FromString("dbus.data");
    if (!name)
        return FALSE;

    module = PyImport_Import(name);
    Py_DECREF(name);
    if (!module)
        return FALSE;

    Double = PyObject_GetAttrString(module, "Double");
    Py_DECREF(module);

    if (Double && !PyType_Check(Double)) {
        PyErr_SetString(PyExc_AssertionError, "Assertion failed: "
                        "isinstance(dbus.data.Double, type)");
        Double = NULL;
    }

    return (Double != NULL);
}

int
DBusPyDouble_Check(PyObject *o)
{
    if (!Double && !do_import())
        return 0;
    return PyObject_TypeCheck(o, (PyTypeObject *)Double);
}

PyObject *
DBusPyDouble_New(double value, long variant_level)
{
    PyObject *args = NULL;
    PyObject *kwargs = NULL;
    PyObject *ret = NULL;

    if (!Double && !do_import())
        return NULL;

    if (variant_level != 0) {
        kwargs = DBusPy_BuildConstructorKeywordArgs(variant_level, NULL);
        if (!kwargs)
            goto finally;
    }

    args = Py_BuildValue("(d)", value);
    if (!args)
        goto finally;

    ret = PyObject_Call(Double, args, kwargs);

finally:
    Py_XDECREF(args);
    Py_XDECREF(kwargs);
    return ret;
}

dbus_bool_t
dbus_py_init_float_types(void)
{
    return 1;
}

dbus_bool_t
dbus_py_insert_float_types(PyObject *this_module UNUSED)
{
    return 1;
}
