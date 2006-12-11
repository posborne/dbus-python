/* Subclasses of built-in Python types supporting extra D-Bus functionality.
 *
 * Copyright (C) 2006 Collabora Ltd.
 *
 * Licensed under the Academic Free License version 2.1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
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

/* Support code for int subclasses. ================================== */

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

PyTypeObject DBusPyIntBase_Type = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
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
    "",                                     /* tp_doc */
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
    0,                                      /* tp_alloc */
    DBusPythonInt_tp_new,                   /* tp_new */
};

/* Support code for float subclasses. ================================ */

/* There's only one subclass at the moment (Double) but these are factored
out to make room for Float later. (Float is implemented and #if'd out) */

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

PyTypeObject DBusPyFloatBase_Type = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
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
    "",                                     /* tp_doc */
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

/* Support code for str subclasses ================================== */

static PyObject *
DBusPythonString_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *self;
    PyObject *variantness = NULL;
    static char *argnames[] = {"variant_level", NULL};

    if (PyTuple_Size(args) > 1) {
        PyErr_SetString(PyExc_TypeError,
                        "__new__ takes at most one positional parameter");
        return NULL;
    }
    if (!PyArg_ParseTupleAndKeywords(dbus_py_empty_tuple, kwargs,
                                     "|O!:__new__", argnames,
                                     &PyInt_Type, &variantness)) return NULL;
    if (!variantness) {
        variantness = PyInt_FromLong(0);
        if (!variantness) return NULL;
    }
    if (PyInt_AS_LONG(variantness) < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "variant_level must be non-negative");
        return NULL;
    }

    self = (PyString_Type.tp_new)(cls, args, NULL);
    if (self) {
        PyObject_GenericSetAttr(self, dbus_py_variant_level_const, variantness);
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
    if (!vl_obj) return NULL;
    variant_level = PyInt_AsLong(vl_obj);
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

PyTypeObject DBusPyStrBase_Type = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
    "_dbus_bindings._StrBase",
    INT_MAX, /* placeholder */
    0,
    0,                                      /* tp_dealloc */
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
    PyObject_GenericGetAttr,                /* tp_getattro */
    Glue_immutable_setattro,                /* tp_setattro */
    0,                                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    "",                                     /* tp_doc */
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
    -sizeof(void *),                        /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    DBusPythonString_tp_new,                /* tp_new */
};

/* Support code for long subclasses ================================= */

static PyObject *
DBusPythonLong_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *self;
    PyObject *variantness = NULL;
    static char *argnames[] = {"variant_level", NULL};

    if (PyTuple_Size(args) > 1) {
        PyErr_SetString(PyExc_TypeError,
                        "__new__ takes at most one positional parameter");
        return NULL;
    }
    if (!PyArg_ParseTupleAndKeywords(dbus_py_empty_tuple, kwargs,
                                     "|O!:__new__", argnames,
                                     &PyInt_Type, &variantness)) return NULL;
    if (!variantness) {
        variantness = PyInt_FromLong(0);
        if (!variantness) return NULL;
    }
    if (PyInt_AS_LONG(variantness) < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "variant_level must be non-negative");
        return NULL;
    }

    self = (PyLong_Type.tp_new)(cls, args, NULL);
    if (self) {
        PyObject_GenericSetAttr(self, dbus_py_variant_level_const, variantness);
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
    if (!vl_obj) return 0;
    variant_level = PyInt_AsLong(vl_obj);
    if (variant_level) {
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

PyTypeObject DBusPyLongBase_Type = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
    "_dbus_bindings._LongBase",
    INT_MAX, /* placeholder */
    0,
    0,                                      /* tp_dealloc */
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
    PyObject_GenericGetAttr,                /* tp_getattro */
    Glue_immutable_setattro,                /* tp_setattro */
    0,                                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    "",                                     /* tp_doc */
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
    -sizeof(PyObject *),                    /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    DBusPythonLong_tp_new,                  /* tp_new */
};

static inline int
init_abstract(void)
{
    dbus_py_variant_level_const = PyString_InternFromString("variant_level");
    if (!dbus_py_variant_level_const) return 0;

    dbus_py_signature_const = PyString_InternFromString("signature");
    if (!dbus_py_signature_const) return 0;

    DBusPyIntBase_Type.tp_base = &PyInt_Type;
    if (PyType_Ready(&DBusPyIntBase_Type) < 0) return 0;
    /* disable the tp_print copied from PyInt_Type, so tp_repr gets called as
    desired */
    DBusPyIntBase_Type.tp_print = NULL;

    DBusPyFloatBase_Type.tp_base = &PyFloat_Type;
    if (PyType_Ready(&DBusPyFloatBase_Type) < 0) return 0;
    DBusPyFloatBase_Type.tp_print = NULL;

    /* Add a pointer for the instance dict, aligning to sizeof(PyObject *)
     * to make sure the offset of -sizeof(PyObject *) is right. */
    DBusPyLongBase_Type.tp_basicsize = PyLong_Type.tp_basicsize
                                       + 2*sizeof(PyObject *) - 1;
    DBusPyLongBase_Type.tp_basicsize /= sizeof(PyObject *);
    DBusPyLongBase_Type.tp_basicsize *= sizeof(PyObject *);
    DBusPyLongBase_Type.tp_base = &PyLong_Type;
    if (PyType_Ready(&DBusPyLongBase_Type) < 0) return 0;
    DBusPyLongBase_Type.tp_print = NULL;

    DBusPyStrBase_Type.tp_basicsize = PyString_Type.tp_basicsize
                                      + 2*sizeof(PyObject *) - 1;
    DBusPyStrBase_Type.tp_basicsize /= sizeof(PyObject *);
    DBusPyStrBase_Type.tp_basicsize *= sizeof(PyObject *);
    DBusPyStrBase_Type.tp_base = &PyString_Type;
    if (PyType_Ready(&DBusPyStrBase_Type) < 0) return 0;
    DBusPyStrBase_Type.tp_print = NULL;

    return 1;
}

static inline int
insert_abstract_types(PyObject *this_module)
{
    Py_INCREF(&DBusPyIntBase_Type);
    Py_INCREF(&DBusPyLongBase_Type);
    Py_INCREF(&DBusPyStrBase_Type);
    Py_INCREF(&DBusPyFloatBase_Type);
    if (PyModule_AddObject(this_module, "_IntBase",
                           (PyObject *)&DBusPyIntBase_Type) < 0) return 0;
    if (PyModule_AddObject(this_module, "_LongBase",
                           (PyObject *)&DBusPyLongBase_Type) < 0) return 0;
    if (PyModule_AddObject(this_module, "_StrBase",
                           (PyObject *)&DBusPyStrBase_Type) < 0) return 0;
    if (PyModule_AddObject(this_module, "_FloatBase",
                           (PyObject *)&DBusPyFloatBase_Type) < 0) return 0;

    return 1;
}
