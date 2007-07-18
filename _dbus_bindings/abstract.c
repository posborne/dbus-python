/* Subclasses of built-in Python types supporting extra D-Bus functionality.
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

#include <Python.h>
#include <structmember.h>

#include <stdint.h>

#include "dbus_bindings-internal.h"
#include "types-internal.h"

/* Dict indexed by object IDs, whose values are nonzero variant levels
 * for immutable variable-sized D-Bus data types (_LongBase, _StrBase, Struct).
 *
 * This is a strange way to store them, but adding a __dict__ to the offending
 * objects seems even more error-prone, given that their sizes are variable!
 */
PyObject *_dbus_py_variant_levels = NULL;

long
dbus_py_variant_level_get(PyObject *obj)
{
    PyObject *vl_obj;
    PyObject *key = PyLong_FromVoidPtr(obj);

    if (!key) {
        return 0;
    }

    vl_obj = PyDict_GetItem(_dbus_py_variant_levels, key);
    Py_DECREF(key);

    if (!vl_obj)
        return 0;
    return PyInt_AsLong(vl_obj);
}

dbus_bool_t
dbus_py_variant_level_set(PyObject *obj, long variant_level)
{
    /* key is the object's ID (= pointer) to avoid referencing it */
    PyObject *key = PyLong_FromVoidPtr(obj);

    if (!key) {
        return FALSE;
    }

    if (variant_level <= 0) {
        if (PyDict_GetItem (_dbus_py_variant_levels, key)) {
            if (PyDict_DelItem (_dbus_py_variant_levels, key) < 0) {
                Py_DECREF(key);
                return FALSE;
            }
        }
    }
    else {
        PyObject *vl_obj = PyInt_FromLong(variant_level);
        if (!vl_obj) {
            Py_DECREF(key);
            return FALSE;
        }
        if (PyDict_SetItem (_dbus_py_variant_levels, key, vl_obj) < 0) {
            Py_DECREF(key);
            return FALSE;
        }
    }
    Py_DECREF(key);
    return TRUE;
}

PyObject *
dbus_py_variant_level_getattro(PyObject *obj, PyObject *name)
{
    PyObject *key, *value;

    if (PyString_Check(name)) {
        Py_INCREF(name);
    }
    else if (PyUnicode_Check(name)) {
        name = PyUnicode_AsEncodedString(name, NULL, NULL);
        if (!name) {
            return NULL;
        }
    }
    else {
        PyErr_SetString(PyExc_TypeError, "attribute name must be string");
        return NULL;
    }

    if (strcmp(PyString_AS_STRING(name), "variant_level")) {
        value = PyObject_GenericGetAttr(obj, name);
        Py_DECREF(name);
        return value;
    }

    Py_DECREF(name);

    key = PyLong_FromVoidPtr(obj);

    if (!key) {
        return NULL;
    }

    value = PyDict_GetItem(_dbus_py_variant_levels, key);
    Py_DECREF(key);

    if (!value)
        return PyInt_FromLong(0);
    Py_INCREF(value);
    return value;
}

/* To be invoked by destructors. Clear the variant level without touching the
 * exception state */
void
dbus_py_variant_level_clear(PyObject *self)
{
    PyObject *et, *ev, *etb;

    /* avoid clobbering any pending exception */
    PyErr_Fetch(&et, &ev, &etb);
    if (!dbus_py_variant_level_set(self, 0)) {
        /* should never happen */
        PyErr_WriteUnraisable(self);
    }
    PyErr_Restore(et, ev, etb);
}

/* Support code for str subclasses ================================== */

PyDoc_STRVAR(DBusPythonString_tp_doc,\
"Base class for str subclasses with a ``variant_level`` attribute.\n"
"Do not rely on the existence of this class outside dbus-python.\n"
);

static PyObject *
DBusPythonString_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *self;
    long variantness = 0;
    static char *argnames[] = {"variant_level", NULL};

    if (PyTuple_Size(args) > 1) {
        PyErr_SetString(PyExc_TypeError,
                        "__new__ takes at most one positional parameter");
        return NULL;
    }
    if (!PyArg_ParseTupleAndKeywords(dbus_py_empty_tuple, kwargs,
                                     "|l:__new__", argnames,
                                     &variantness)) return NULL;
    if (variantness < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "variant_level must be non-negative");
        return NULL;
    }

    self = (PyString_Type.tp_new)(cls, args, NULL);
    if (self) {
        if (!dbus_py_variant_level_set(self, variantness)) {
            Py_DECREF(self);
            return NULL;
        }
    }
    return self;
}

static PyObject *
DBusPythonString_tp_repr(PyObject *self)
{
    PyObject *parent_repr = (PyString_Type.tp_repr)(self);
    PyObject *vl_obj;
    PyObject *my_repr;
    long variant_level;

    if (!parent_repr) return NULL;
    vl_obj = PyObject_GetAttr(self, dbus_py_variant_level_const);
    if (!vl_obj) {
        Py_DECREF(parent_repr);
        return NULL;
    }
    variant_level = PyInt_AsLong(vl_obj);
    Py_DECREF(vl_obj);
    if (variant_level > 0) {
        my_repr = PyString_FromFormat("%s(%s, variant_level=%ld)",
                                      self->ob_type->tp_name,
                                      PyString_AS_STRING(parent_repr),
                                      variant_level);
    }
    else {
        my_repr = PyString_FromFormat("%s(%s)", self->ob_type->tp_name,
                                      PyString_AS_STRING(parent_repr));
    }
    /* whether my_repr is NULL or not: */
    Py_DECREF(parent_repr);
    return my_repr;
}

static void
DBusPyStrBase_tp_dealloc(PyObject *self)
{
    dbus_py_variant_level_clear(self);
    (PyString_Type.tp_dealloc)(self);
}

PyTypeObject DBusPyStrBase_Type = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
    "_dbus_bindings._StrBase",
    0,
    0,
    DBusPyStrBase_tp_dealloc,               /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_compare */
    DBusPythonString_tp_repr,               /* tp_repr */
    0,                                      /* tp_as_number */
    0,                                      /* tp_as_sequence */
    0,                                      /* tp_as_mapping */
    0,                                      /* tp_hash */
    0,                                      /* tp_call */
    0,                                      /* tp_str */
    dbus_py_variant_level_getattro,         /* tp_getattro */
    dbus_py_immutable_setattro,             /* tp_setattro */
    0,                                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    DBusPythonString_tp_doc,                /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    0,                                      /* tp_iter */
    0,                                      /* tp_iternext */
    0,                                      /* tp_methods */
    0,                                      /* tp_members */
    0,                                      /* tp_getset */
    DEFERRED_ADDRESS(&PyString_Type),       /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    DBusPythonString_tp_new,                /* tp_new */
};

PyObject *dbus_py_variant_level_const = NULL;
PyObject *dbus_py_signature_const = NULL;
PyObject *dbus_py__dbus_object_path__const = NULL;

PyObject *
DBusPy_BuildConstructorKeywordArgs(long variant_level, const char *signature)
{
    PyObject *dict = PyDict_New();
    PyObject *value;
    int status;

    if (!dict)
        return NULL;

    if (variant_level != 0) {
        value = PyInt_FromLong(variant_level);
        if (!value) {
            Py_DECREF(dict);
            return NULL;
        }
        status = PyDict_SetItem(dict, dbus_py_variant_level_const, value);
        Py_DECREF(value);
        if (status < 0) {
            Py_DECREF(dict);
            return NULL;
        }
    }

    if (signature != NULL) {
        value = DBusPySignature_FromString(signature);
        if (!value) {
            Py_DECREF(dict);
            return NULL;
        }
        status = PyDict_SetItem(dict, dbus_py_signature_const, value);
        Py_DECREF(value);
        if (status < 0) {
            Py_DECREF(dict);
            return NULL;
        }
    }

    return dict;
}

dbus_bool_t
dbus_py_init_abstract(void)
{
    _dbus_py_variant_levels = PyDict_New();
    if (!_dbus_py_variant_levels) return 0;

    dbus_py__dbus_object_path__const = PyString_InternFromString("__dbus_object_path__");
    if (!dbus_py__dbus_object_path__const) return 0;

    dbus_py_variant_level_const = PyString_InternFromString("variant_level");
    if (!dbus_py_variant_level_const) return 0;

    dbus_py_signature_const = PyString_InternFromString("signature");
    if (!dbus_py_signature_const) return 0;

    DBusPyStrBase_Type.tp_base = &PyString_Type;
    if (PyType_Ready(&DBusPyStrBase_Type) < 0) return 0;
    DBusPyStrBase_Type.tp_print = NULL;

    return 1;
}

dbus_bool_t
dbus_py_insert_abstract_types(PyObject *this_module)
{
    Py_INCREF(&DBusPyStrBase_Type);
    if (PyModule_AddObject(this_module, "_StrBase",
                           (PyObject *)&DBusPyStrBase_Type) < 0) return 0;

    return 1;
}
