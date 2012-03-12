/* Subclasses of built-in Python types supporting extra D-Bus functionality.
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
    long variant_level;

    if (!key) {
        return -1;
    }

    vl_obj = PyDict_GetItem(_dbus_py_variant_levels, key);
    Py_CLEAR(key);

    if (!vl_obj) {
        /* PyDict_GetItem() does not set an exception when the key is missing.
         * In our case, it just means that there was no entry in the variant
         * dictionary for this object.  Semantically, this is equivalent to a
         * variant level of 0.
         */
        return 0;
    }
    variant_level = NATIVEINT_ASLONG(vl_obj);
    if (variant_level == -1 && PyErr_Occurred()) {
        /* variant_level < 0 can never be inserted into the dictionary; see
         * dbus_py_variant_level_set() below.  The semantics of setting
         * variant_level < 0 is to delete it from the dictionary.
         */
        return -1;
    }
    assert(variant_level >= 0);
    return variant_level;
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
                Py_CLEAR(key);
                return FALSE;
            }
        }
    }
    else {
        PyObject *vl_obj = NATIVEINT_FROMLONG(variant_level);
        if (!vl_obj) {
            Py_CLEAR(key);
            return FALSE;
        }
        if (PyDict_SetItem(_dbus_py_variant_levels, key, vl_obj) < 0) {
            Py_CLEAR(vl_obj);
            Py_CLEAR(key);
            return FALSE;
        }
        Py_CLEAR(vl_obj);
    }
    Py_CLEAR(key);
    return TRUE;
}

PyObject *
dbus_py_variant_level_getattro(PyObject *obj, PyObject *name)
{
    PyObject *key, *value;

#ifdef PY3
    if (PyUnicode_CompareWithASCIIString(name, "variant_level"))
        return PyObject_GenericGetAttr(obj, name);
#else
    if (PyBytes_Check(name)) {
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

    if (strcmp(PyBytes_AS_STRING(name), "variant_level")) {
        value = PyObject_GenericGetAttr(obj, name);
        Py_CLEAR(name);
        return value;
    }

    Py_CLEAR(name);
#endif  /* PY3 */

    key = PyLong_FromVoidPtr(obj);

    if (!key) {
        return NULL;
    }

    value = PyDict_GetItem(_dbus_py_variant_levels, key);
    Py_CLEAR(key);

    if (!value)
        return NATIVEINT_FROMLONG(0);
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

#ifndef PY3
/* Support code for int subclasses. ================================== */

PyDoc_STRVAR(DBusPythonInt_tp_doc,\
"Base class for int subclasses with a ``variant_level`` attribute.\n"
"Do not rely on the existence of this class outside dbus-python.\n"
);

static PyMemberDef DBusPythonInt_tp_members[] = {
    {"variant_level", T_LONG, offsetof(DBusPyIntBase, variant_level),
     READONLY,
     "The number of nested variants wrapping the real data. "
     "0 if not in a variant."},
    {NULL},
};

static PyObject *
DBusPythonInt_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
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

    self = (PyInt_Type.tp_new)(cls, args, NULL);
    if (self) {
        ((DBusPyIntBase *)self)->variant_level = variantness;
    }
    return self;
}

static PyObject *
DBusPythonInt_tp_repr(PyObject *self)
{
    PyObject *parent_repr = (PyInt_Type.tp_repr)(self);
    long variant_level = ((DBusPyIntBase *)self)->variant_level;
    PyObject *my_repr;

    if (!parent_repr) return NULL;
    if (variant_level > 0) {
        my_repr = PyUnicode_FromFormat("%s(%V, variant_level=%ld)",
                                       Py_TYPE(self)->tp_name,
                                       REPRV(parent_repr),
                                       variant_level);
    }
    else {
        my_repr = PyUnicode_FromFormat("%s(%V)", Py_TYPE(self)->tp_name,
                                       REPRV(parent_repr));
    }
    /* whether my_repr is NULL or not: */
    Py_CLEAR(parent_repr);
    return my_repr;
}

PyTypeObject DBusPyIntBase_Type = {
    PyVarObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type), 0)
    "_dbus_bindings._IntBase",
    sizeof(DBusPyIntBase),
    0,
    0,                                      /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_compare */
    DBusPythonInt_tp_repr,                  /* tp_repr */
    0,                                      /* tp_as_number */
    0,                                      /* tp_as_sequence */
    0,                                      /* tp_as_mapping */
    0,                                      /* tp_hash */
    0,                                      /* tp_call */
    0,                                      /* tp_str */
    0,                                      /* tp_getattro */
    0,                                      /* tp_setattro */
    0,                                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    DBusPythonInt_tp_doc,                   /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    0,                                      /* tp_iter */
    0,                                      /* tp_iternext */
    0,                                      /* tp_methods */
    DBusPythonInt_tp_members,               /* tp_members */
    0,                                      /* tp_getset */
    DEFERRED_ADDRESS(&PyInt_Type),          /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    PyType_GenericAlloc,                    /* tp_alloc */
    DBusPythonInt_tp_new,                   /* tp_new */
    PyObject_Del,                           /* tp_free */
};
#endif  /* !PY3 */

/* Support code for float subclasses. ================================ */

/* There's only one subclass at the moment (Double) but these are factored
out to make room for Float later. (Float is implemented and #if'd out) */

PyDoc_STRVAR(DBusPythonFloat_tp_doc,\
"Base class for float subclasses with a ``variant_level`` attribute.\n"
"Do not rely on the existence of this class outside dbus-python.\n"
);

static PyMemberDef DBusPythonFloat_tp_members[] = {
    {"variant_level", T_LONG, offsetof(DBusPyFloatBase, variant_level),
     READONLY,
     "The number of nested variants wrapping the real data. "
     "0 if not in a variant."},
    {NULL},
};

static PyObject *
DBusPythonFloat_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
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

    self = (PyFloat_Type.tp_new)(cls, args, NULL);
    if (self) {
        ((DBusPyFloatBase *)self)->variant_level = variantness;
    }
    return self;
}

static PyObject *
DBusPythonFloat_tp_repr(PyObject *self)
{
    PyObject *parent_repr = (PyFloat_Type.tp_repr)(self);
    long variant_level = ((DBusPyFloatBase *)self)->variant_level;
    PyObject *my_repr;

    if (!parent_repr) return NULL;
    if (variant_level > 0) {
        my_repr = PyUnicode_FromFormat("%s(%V, variant_level=%ld)",
                                       Py_TYPE(self)->tp_name,
                                       REPRV(parent_repr),
                                       variant_level);
    }
    else {
        my_repr = PyUnicode_FromFormat("%s(%V)", Py_TYPE(self)->tp_name,
                                       REPRV(parent_repr));
    }
    /* whether my_repr is NULL or not: */
    Py_CLEAR(parent_repr);
    return my_repr;
}

PyTypeObject DBusPyFloatBase_Type = {
    PyVarObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type), 0)
    "_dbus_bindings._FloatBase",
    sizeof(DBusPyFloatBase),
    0,
    0,                                      /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_compare */
    DBusPythonFloat_tp_repr,                /* tp_repr */
    0,                                      /* tp_as_number */
    0,                                      /* tp_as_sequence */
    0,                                      /* tp_as_mapping */
    0,                                      /* tp_hash */
    0,                                      /* tp_call */
    0,                                      /* tp_str */
    0,                                      /* tp_getattro */
    0,                                      /* tp_setattro */
    0,                                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    DBusPythonFloat_tp_doc,                 /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    0,                                      /* tp_iter */
    0,                                      /* tp_iternext */
    0,                                      /* tp_methods */
    DBusPythonFloat_tp_members,             /* tp_members */
    0,                                      /* tp_getset */
    DEFERRED_ADDRESS(&PyFloat_Type),        /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    DBusPythonFloat_tp_new,                 /* tp_new */
};

#ifdef PY3
/* Support code for bytes subclasses ================================== */

PyDoc_STRVAR(DBusPythonBytes_tp_doc,\
"Base class for bytes subclasses with a ``variant_level`` attribute.\n"
"Do not rely on the existence of this class outside dbus-python.\n"
);

static PyObject *
DBusPythonBytes_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
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
                                     &variantness))
        return NULL;

    if (variantness < 0) {
        PyErr_SetString(PyExc_ValueError,
                       "variant_level must be non-negative");
        return NULL;
    }

    self = (PyBytes_Type.tp_new)(cls, args, NULL);
    if (self) {
        if (!dbus_py_variant_level_set(self, variantness)) {
            Py_CLEAR(self);
            return NULL;
        }
    }
    return self;
}

static PyObject *
DBusPythonBytes_tp_repr(PyObject *self)
{
    PyObject *parent_repr = (PyBytes_Type.tp_repr)(self);
    PyObject *vl_obj;
    PyObject *my_repr;
    long variant_level;

    if (!parent_repr) return NULL;
    vl_obj = PyObject_GetAttr(self, dbus_py_variant_level_const);
    if (!vl_obj) {
        Py_CLEAR(parent_repr);
        return NULL;
    }
    variant_level = NATIVEINT_ASLONG(vl_obj);
    Py_CLEAR(vl_obj);
    if (variant_level == -1 && PyErr_Occurred()) {
        Py_CLEAR(parent_repr);
        return NULL;
    }
    if (variant_level > 0) {
        my_repr = PyUnicode_FromFormat("%s(%V, variant_level=%ld)",
                                       Py_TYPE(self)->tp_name,
                                       REPRV(parent_repr),
                                       variant_level);
    }
    else {
        my_repr = PyUnicode_FromFormat("%s(%V)", Py_TYPE(self)->tp_name,
                                       REPRV(parent_repr));
    }
    /* whether my_repr is NULL or not: */
    Py_CLEAR(parent_repr);
    return my_repr;
}

static void
DBusPyBytesBase_tp_dealloc(PyObject *self)
{
    dbus_py_variant_level_clear(self);
    (PyBytes_Type.tp_dealloc)(self);
}

PyTypeObject DBusPyBytesBase_Type = {
    PyVarObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type), 0)
    "_dbus_bindings._BytesBase",
    0,
    0,
    DBusPyBytesBase_tp_dealloc,             /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_compare */
    DBusPythonBytes_tp_repr,                /* tp_repr */
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
    DBusPythonBytes_tp_doc,                 /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    0,                                      /* tp_iter */
    0,                                      /* tp_iternext */
    0,                                      /* tp_methods */
    0,                                      /* tp_members */
    0,                                      /* tp_getset */
    DEFERRED_ADDRESS(&PyBytes_Type),        /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    DBusPythonBytes_tp_new,                 /* tp_new */
};
#endif  /* PY3 */

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

    self = (NATIVESTR_TYPE.tp_new)(cls, args, NULL);
    if (self) {
        if (!dbus_py_variant_level_set(self, variantness)) {
            Py_CLEAR(self);
            return NULL;
        }
    }
    return self;
}

static PyObject *
DBusPythonString_tp_repr(PyObject *self)
{
    PyObject *parent_repr = (NATIVESTR_TYPE.tp_repr)(self);
    PyObject *vl_obj;
    PyObject *my_repr;
    long variant_level;

    if (!parent_repr) return NULL;
    vl_obj = PyObject_GetAttr(self, dbus_py_variant_level_const);
    if (!vl_obj) {
        Py_CLEAR(parent_repr);
        return NULL;
    }
    variant_level = NATIVEINT_ASLONG(vl_obj);
    Py_CLEAR(vl_obj);
    if (variant_level == -1 && PyErr_Occurred()) {
        Py_CLEAR(parent_repr);
        return NULL;
    }

    if (variant_level > 0) {
        my_repr = PyUnicode_FromFormat("%s(%V, variant_level=%ld)",
                                       Py_TYPE(self)->tp_name,
                                       REPRV(parent_repr),
                                       variant_level);
    }
    else {
        my_repr = PyUnicode_FromFormat("%s(%V)", Py_TYPE(self)->tp_name,
                                       REPRV(parent_repr));
    }
    /* whether my_repr is NULL or not: */
    Py_CLEAR(parent_repr);
    return my_repr;
}

static void
DBusPyStrBase_tp_dealloc(PyObject *self)
{
    dbus_py_variant_level_clear(self);
    (NATIVESTR_TYPE.tp_dealloc)(self);
}

PyTypeObject DBusPyStrBase_Type = {
    PyVarObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type), 0)
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
    DEFERRED_ADDRESS(&NATIVESTR_TYPE),      /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    DBusPythonString_tp_new,                /* tp_new */
};

/* Support code for long subclasses ================================= */

PyDoc_STRVAR(DBusPythonLong_tp_doc,\
"Base class for ``long`` subclasses with a ``variant_level`` attribute.\n"
"Do not rely on the existence of this class outside dbus-python.\n"
);

static PyObject *
DBusPythonLong_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
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

    self = (PyLong_Type.tp_new)(cls, args, NULL);
    if (self) {
        if (!dbus_py_variant_level_set(self, variantness)) {
            Py_CLEAR(self);
            return NULL;
        }
    }
    return self;
}

static PyObject *
DBusPythonLong_tp_repr(PyObject *self)
{
    PyObject *parent_repr = (PyLong_Type.tp_repr)(self);
    PyObject *vl_obj;
    PyObject *my_repr;
    long variant_level;

    if (!parent_repr) return NULL;
    vl_obj = PyObject_GetAttr(self, dbus_py_variant_level_const);
    if (!vl_obj) {
        Py_CLEAR(parent_repr);
        return NULL;
    }
    variant_level = NATIVEINT_ASLONG(vl_obj);
    Py_CLEAR(vl_obj);
    if (variant_level < 0 && PyErr_Occurred()) {
        Py_CLEAR(parent_repr);
        return NULL;
    }

    if (variant_level) {
        my_repr = PyUnicode_FromFormat("%s(%V, variant_level=%ld)",
                                       Py_TYPE(self)->tp_name,
                                       REPRV(parent_repr),
                                       variant_level);
    }
    else {
        my_repr = PyUnicode_FromFormat("%s(%V)", Py_TYPE(self)->tp_name,
                                       REPRV(parent_repr));
    }
    /* whether my_repr is NULL or not: */
    Py_CLEAR(parent_repr);
    return my_repr;
}

static void
DBusPyLongBase_tp_dealloc(PyObject *self)
{
    dbus_py_variant_level_clear(self);
    (PyLong_Type.tp_dealloc)(self);
}

PyTypeObject DBusPyLongBase_Type = {
    PyVarObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type), 0)
    "_dbus_bindings._LongBase",
    0,
    0,
    DBusPyLongBase_tp_dealloc,              /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_compare */
    DBusPythonLong_tp_repr,                 /* tp_repr */
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
    DBusPythonLong_tp_doc,                  /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    0,                                      /* tp_iter */
    0,                                      /* tp_iternext */
    0,                                      /* tp_methods */
    0,                                      /* tp_members */
    0,                                      /* tp_getset */
    DEFERRED_ADDRESS(&PyLong_Type),         /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    DBusPythonLong_tp_new,                  /* tp_new */
};

PyObject *dbus_py_variant_level_const = NULL;
PyObject *dbus_py_signature_const = NULL;
PyObject *dbus_py__dbus_object_path__const = NULL;

#ifdef PY3
#define INTERN (PyUnicode_InternFromString)
#else
/* Neither Python 2.6 nor 2.7 define the expected PyBytes_InternFromString
 * alias in bytesobject.h.
 */
#define INTERN (PyString_InternFromString)
#endif

dbus_bool_t
dbus_py_init_abstract(void)
{
    _dbus_py_variant_levels = PyDict_New();
    if (!_dbus_py_variant_levels) return 0;

    dbus_py__dbus_object_path__const = INTERN("__dbus_object_path__");
    if (!dbus_py__dbus_object_path__const) return 0;

    dbus_py_variant_level_const = INTERN("variant_level");
    if (!dbus_py_variant_level_const) return 0;

    dbus_py_signature_const = INTERN("signature");
    if (!dbus_py_signature_const) return 0;

#ifdef PY3
    DBusPyBytesBase_Type.tp_base = &PyBytes_Type;
    if (PyType_Ready(&DBusPyBytesBase_Type) < 0) return 0;
    DBusPyBytesBase_Type.tp_print = NULL;
#else
    DBusPyIntBase_Type.tp_base = &PyInt_Type;
    if (PyType_Ready(&DBusPyIntBase_Type) < 0) return 0;
    /* disable the tp_print copied from PyInt_Type, so tp_repr gets called as
    desired */
    DBusPyIntBase_Type.tp_print = NULL;
#endif

    DBusPyFloatBase_Type.tp_base = &PyFloat_Type;
    if (PyType_Ready(&DBusPyFloatBase_Type) < 0) return 0;
    DBusPyFloatBase_Type.tp_print = NULL;

    DBusPyLongBase_Type.tp_base = &PyLong_Type;
    if (PyType_Ready(&DBusPyLongBase_Type) < 0) return 0;
    DBusPyLongBase_Type.tp_print = NULL;

    DBusPyStrBase_Type.tp_base = &NATIVESTR_TYPE;
    if (PyType_Ready(&DBusPyStrBase_Type) < 0) return 0;
    DBusPyStrBase_Type.tp_print = NULL;

    return 1;
}

dbus_bool_t
dbus_py_insert_abstract_types(PyObject *this_module)
{
    /* PyModule_AddObject steals a ref */
#ifdef PY3
    Py_INCREF(&DBusPyBytesBase_Type);
    if (PyModule_AddObject(this_module, "_BytesBase",
                           (PyObject *)&DBusPyBytesBase_Type) < 0) return 0;
#else
    Py_INCREF(&DBusPyIntBase_Type);
    if (PyModule_AddObject(this_module, "_IntBase",
                           (PyObject *)&DBusPyIntBase_Type) < 0) return 0;
#endif
    Py_INCREF(&DBusPyLongBase_Type);
    Py_INCREF(&DBusPyStrBase_Type);
    Py_INCREF(&DBusPyFloatBase_Type);
    if (PyModule_AddObject(this_module, "_LongBase",
                           (PyObject *)&DBusPyLongBase_Type) < 0) return 0;
    if (PyModule_AddObject(this_module, "_StrBase",
                           (PyObject *)&DBusPyStrBase_Type) < 0) return 0;
    if (PyModule_AddObject(this_module, "_FloatBase",
                           (PyObject *)&DBusPyFloatBase_Type) < 0) return 0;

    return 1;
}
