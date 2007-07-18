/* D-Bus container types: Array, Dict and Struct.
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

/* Array ============================================================ */

static PyObject *Array = NULL;
/* static PyObject *Struct = NULL; */
static PyObject *Dictionary = NULL;

static dbus_bool_t
do_import(void)
{
    PyObject *name;
    PyObject *module;

    if (Array /* && Struct */ && Dictionary)
        return TRUE;

    Py_CLEAR(Array);
    /* Py_CLEAR(Struct); */
    Py_CLEAR(Dictionary);

    name = PyString_FromString("dbus.data");
    if (!name)
        return FALSE;

    module = PyImport_Import(name);
    Py_DECREF(name);
    if (!module)
        return FALSE;

    Array = PyObject_GetAttrString(module, "Array");
    /* Struct = PyObject_GetAttrString(module, "Struct"); */
    Dictionary = PyObject_GetAttrString(module, "Dictionary");

    Py_DECREF(module);

    if (Array && !PyType_Check(Array)) {
        PyErr_SetString(PyExc_AssertionError, "Assertion failed: "
                        "isinstance(dbus.data.Array, type)");
        Array = NULL;
    }
    /* if (Struct && !PyType_Check(Struct)) {
        PyErr_SetString(PyExc_AssertionError, "Assertion failed: "
                        "isinstance(dbus.data.Struct, type)");
        Struct = NULL;
    } */
    if (Dictionary && !PyType_Check(Dictionary)) {
        PyErr_SetString(PyExc_AssertionError, "Assertion failed: "
                        "isinstance(dbus.data.Dictionary, type)");
        Dictionary = NULL;
    }

    return (Array != NULL /* && Struct != NULL */ && Dictionary != NULL);
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

static PyTypeObject DBusPyStruct_Type;

int
DBusPyStruct_Check(PyObject *o)
{
    return PyObject_TypeCheck(o, &DBusPyStruct_Type);
}

PyObject *
DBusPyStruct_New(PyObject *iterable, long variant_level)
{
    PyObject *args = NULL;
    PyObject *kwargs = NULL;
    PyObject *ret = NULL;

    if (variant_level != 0) {
        kwargs = DBusPy_BuildConstructorKeywordArgs(variant_level, NULL);
        if (!kwargs)
            goto finally;
    }

    args = Py_BuildValue("(O)", iterable);
    if (!args)
        goto finally;

    ret = PyObject_Call((PyObject *)&DBusPyStruct_Type, args,
                        kwargs);

finally:
    Py_XDECREF(args);
    Py_XDECREF(kwargs);
    return ret;
}

static PyObject *struct_signatures;

PyDoc_STRVAR(Struct_tp_doc,
"An structure containing items of possibly distinct types.\n"
"\n"
"Constructor::\n"
"\n"
"    dbus.Struct(iterable, signature=None, variant_level=0) -> Struct\n"
"\n"
"D-Bus structs may not be empty, so the iterable argument is required and\n"
"may not be an empty iterable.\n"
"\n"
"``signature`` is either None, or a string representing the contents of the\n"
"struct as one or more complete type signatures. The overall signature of\n"
"the struct will be the given signature enclosed in parentheses, ``()``.\n"
"\n"
"If the signature is None (default) it will be guessed\n"
"from the types of the items during construction.\n"
"\n"
"``variant_level`` must be non-negative; the default is 0.\n"
"\n"
":IVariables:\n"
"  `variant_level` : int\n"
"    Indicates how many nested Variant containers this object\n"
"    is contained in: if a message's wire format has a variant containing a\n"
"    variant containing a struct, this is represented in Python by a\n"
"    Struct with variant_level==2.\n"
);

static PyObject *
Struct_tp_repr(PyObject *self)
{
    PyObject *parent_repr = (PyTuple_Type.tp_repr)((PyObject *)self);
    PyObject *sig;
    PyObject *sig_repr = NULL;
    PyObject *key;
    long variant_level;
    PyObject *my_repr = NULL;

    if (!parent_repr) goto finally;
    key = PyLong_FromVoidPtr(self);
    if (!key) goto finally;
    sig = PyDict_GetItem(struct_signatures, key);
    Py_DECREF(key);
    if (!sig) sig = Py_None;
    sig_repr = PyObject_Repr(sig);
    if (!sig_repr) goto finally;
    variant_level = dbus_py_variant_level_get(self);
    if (variant_level > 0) {
        my_repr = PyString_FromFormat("%s(%s, signature=%s, "
                                      "variant_level=%ld)",
                                      self->ob_type->tp_name,
                                      PyString_AS_STRING(parent_repr),
                                      PyString_AS_STRING(sig_repr),
                                      variant_level);
    }
    else {
        my_repr = PyString_FromFormat("%s(%s, signature=%s)",
                                      self->ob_type->tp_name,
                                      PyString_AS_STRING(parent_repr),
                                      PyString_AS_STRING(sig_repr));
    }

finally:
    Py_XDECREF(parent_repr);
    Py_XDECREF(sig_repr);
    return my_repr;
}

static PyObject *
Struct_tp_new (PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *signature = Py_None;
    long variantness = 0;
    PyObject *self, *key;
    static char *argnames[] = {"signature", "variant_level", NULL};

    if (PyTuple_Size(args) != 1) {
        PyErr_SetString(PyExc_TypeError,
                        "__new__ takes exactly one positional parameter");
        return NULL;
    }
    if (!PyArg_ParseTupleAndKeywords(dbus_py_empty_tuple, kwargs,
                                     "|Ol:__new__", argnames,
                                     &signature, &variantness)) {
        return NULL;
    }
    if (variantness < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "variant_level must be non-negative");
        return NULL;
    }

    self = (PyTuple_Type.tp_new)(cls, args, NULL);
    if (!self)
        return NULL;
    if (PyTuple_Size(self) < 1) {
        PyErr_SetString(PyExc_ValueError, "D-Bus structs may not be empty");
        Py_DECREF(self);
        return NULL;
    }

    if (!dbus_py_variant_level_set(self, variantness)) {
        Py_DECREF(self);
        return NULL;
    }

    /* convert signature from a borrowed ref of unknown type to an owned ref
    of type Signature (or None) */
    signature = DBusPySignature_FromStringObject(signature, TRUE);
    if (!signature)
        return NULL;

    key = PyLong_FromVoidPtr(self);
    if (!key) {
        Py_DECREF(self);
        Py_DECREF(signature);
        return NULL;
    }
    if (PyDict_SetItem(struct_signatures, key, signature) < 0) {
        Py_DECREF(key);
        Py_DECREF(self);
        Py_DECREF(signature);
        return NULL;
    }

    Py_DECREF(key);
    Py_DECREF(signature);
    return self;
}

static void
Struct_tp_dealloc(PyObject *self)
{
    PyObject *et, *ev, *etb, *key;

    dbus_py_variant_level_clear(self);
    PyErr_Fetch(&et, &ev, &etb);

    key = PyLong_FromVoidPtr(self);
    if (key) {
        if (PyDict_GetItem(struct_signatures, key)) {
            if (PyDict_DelItem(struct_signatures, key) < 0) {
                /* should never happen */
                PyErr_WriteUnraisable(self);
            }
        }
        Py_DECREF(key);
    }
    else {
        /* not enough memory to free all the memory... leak the signature,
         * there's not much else we could do here */
        PyErr_WriteUnraisable(self);
    }

    PyErr_Restore(et, ev, etb);
    (PyTuple_Type.tp_dealloc)(self);
}

PyObject *
Struct_tp_getattro(PyObject *obj, PyObject *name)
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

    if (strcmp(PyString_AS_STRING(name), "signature")) {
        value = dbus_py_variant_level_getattro(obj, name);
        Py_DECREF(name);
        return value;
    }

    Py_DECREF(name);

    key = PyLong_FromVoidPtr(obj);

    if (!key) {
        return NULL;
    }

    value = PyDict_GetItem(struct_signatures, key);
    Py_DECREF(key);

    if (!value)
        value = Py_None;
    Py_INCREF(value);
    return value;
}

static PyTypeObject DBusPyStruct_Type = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
    "dbus.Struct",
    0,
    0,
    Struct_tp_dealloc,                      /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_compare */
    (reprfunc)Struct_tp_repr,               /* tp_repr */
    0,                                      /* tp_as_number */
    0,                                      /* tp_as_sequence */
    0,                                      /* tp_as_mapping */
    0,                                      /* tp_hash */
    0,                                      /* tp_call */
    0,                                      /* tp_str */
    Struct_tp_getattro,                     /* tp_getattro */
    dbus_py_immutable_setattro,             /* tp_setattro */
    0,                                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    Struct_tp_doc,                          /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    0,                                      /* tp_iter */
    0,                                      /* tp_iternext */
    0,                                      /* tp_methods */
    0,                                      /* tp_members */
    0,                                      /* tp_getset */
    0,                                      /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    Struct_tp_new,                          /* tp_new */
};

dbus_bool_t
dbus_py_init_container_types(void)
{
    struct_signatures = PyDict_New();
    if (!struct_signatures) return 0;

    DBusPyStruct_Type.tp_base = &PyTuple_Type;
    if (PyType_Ready(&DBusPyStruct_Type) < 0) return 0;
    DBusPyStruct_Type.tp_print = NULL;

    return 1;
}

dbus_bool_t
dbus_py_insert_container_types(PyObject *this_module)
{
    Py_INCREF(&DBusPyStruct_Type);
    if (PyModule_AddObject(this_module, "Struct",
                           (PyObject *)&DBusPyStruct_Type) < 0) return 0;

    return 1;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */

