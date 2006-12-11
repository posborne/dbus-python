/* D-Bus container types: Array, Dict and Struct.
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

/* Array ============================================================ */

PyDoc_STRVAR(Array_tp_doc,
"An array of similar items, implemented as a subtype of list.\n"
"\n"
"As currently implemented, an Array behaves just like a list, but\n"
"with the addition of a ``signature`` property set by the constructor;\n"
"conversion of its items to D-Bus types is only done when it's sent in\n"
"a Message. This might change in future so validation is done earlier.\n"
"\n"
":SupportedUsage:\n"
"   ``from dbus import Array`` or ``from dbus.types import Array``\n"
"\n"
":Constructor:\n"
"    Array([iterable][, signature][, variant_level])\n"
"\n"
"    variant_level must be non-negative; the default is 0.\n"
"\n"
"    The signature may be None, in which case when the Array is sent over\n"
"    D-Bus, the item signature will be guessed from the first element.\n"
"\n"
":IVariables:\n"
"  `variant_level` : int\n"
"    Indicates how many nested Variant containers this object\n"
"    is contained in: if a message's wire format has a variant containing a\n"
"    variant containing an array, this is represented in Python by an\n"
"    Array with variant_level==2.\n"
);

static struct PyMemberDef Array_tp_members[] = {
    {"signature", T_OBJECT, offsetof(DBusPyArray, signature), READONLY,
     "The D-Bus signature of each element of this Array (a Signature "
     "instance)"},
    {"variant_level", T_LONG, offsetof(DBusPyArray, variant_level),
     READONLY,
     "The number of nested variants wrapping the real data. "
     "0 if not in a variant."},
    {NULL},
};

static void
Array_tp_dealloc (DBusPyArray *self)
{
    Py_XDECREF(self->signature);
    self->signature = NULL;
    (PyList_Type.tp_dealloc)((PyObject *)self);
}

static PyObject *
Array_tp_repr(DBusPyArray *self)
{
    PyObject *parent_repr = (PyList_Type.tp_repr)((PyObject *)self);
    PyObject *sig_repr = PyObject_Repr(self->signature);
    PyObject *my_repr = NULL;
    long variant_level = self->variant_level;

    if (!parent_repr) goto finally;
    if (!sig_repr) goto finally;
    if (variant_level > 0) {
        my_repr = PyString_FromFormat("%s(%s, signature=%s, "
                                      "variant_level=%ld)",
                                      self->super.ob_type->tp_name,
                                      PyString_AS_STRING(parent_repr),
                                      PyString_AS_STRING(sig_repr),
                                      variant_level);
    }
    else {
        my_repr = PyString_FromFormat("%s(%s, signature=%s)",
                                      self->super.ob_type->tp_name,
                                      PyString_AS_STRING(parent_repr),
                                      PyString_AS_STRING(sig_repr));
    }
finally:
    Py_XDECREF(parent_repr);
    Py_XDECREF(sig_repr);
    return my_repr;
}

static PyObject *
Array_tp_new (PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *variant_level = NULL;
    DBusPyArray *self = (DBusPyArray *)(PyList_Type.tp_new)(cls, args, kwargs);

    /* variant_level is immutable, so handle it in __new__ rather than 
    __init__ */
    if (!self) return NULL;
    Py_INCREF(Py_None);
    self->signature = Py_None;
    self->variant_level = 0;
    if (kwargs) {
        variant_level = PyDict_GetItem(kwargs, dbus_py_variant_level_const);
    }
    if (variant_level) {
        self->variant_level = PyInt_AsLong(variant_level);
        if (PyErr_Occurred()) {
            Py_DECREF((PyObject *)self);
            return NULL;
        }
    }
    return (PyObject *)self;
}

static int
Array_tp_init (DBusPyArray *self, PyObject *args, PyObject *kwargs)
{
    PyObject *obj = dbus_py_empty_tuple;
    PyObject *signature = NULL;
    PyObject *tuple;
    PyObject *variant_level;
    /* variant_level is accepted but ignored - it's immutable, so
     * __new__ handles it */
    static char *argnames[] = {"iterable", "signature", "variant_level", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OOO:__init__", argnames,
                                     &obj, &signature, &variant_level)) {
        return -1;
    }

  /* convert signature from a borrowed ref of unknown type to an owned ref
  of type Signature (or None) */
    if (!signature) signature = Py_None;
    if (signature == Py_None
        || PyObject_IsInstance(signature, (PyObject *)&DBusPySignature_Type)) {
        Py_INCREF(signature);
    }
    else {
        signature = PyObject_CallFunction((PyObject *)&DBusPySignature_Type,
                                          "(O)", signature);
        if (!signature) return -1;
    }

    tuple = Py_BuildValue("(O)", obj);
    if (!tuple) {
        Py_DECREF(signature);
        return -1;
    }
    if ((PyList_Type.tp_init)((PyObject *)self, tuple, NULL) < 0) {
        Py_DECREF(tuple);
        Py_DECREF(signature);
        return -1;
    }
    Py_DECREF(tuple);

    Py_XDECREF(self->signature);
    self->signature = signature;
    return 0;
}

PyTypeObject DBusPyArray_Type = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
    "dbus.Array",
    sizeof(DBusPyArray),
    0,
    (destructor)Array_tp_dealloc,           /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_compare */
    (reprfunc)Array_tp_repr,                /* tp_repr */
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
    Array_tp_doc,                           /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    0,                                      /* tp_iter */
    0,                                      /* tp_iternext */
    0,                                      /* tp_methods */
    Array_tp_members,                       /* tp_members */
    0,                                      /* tp_getset */
    0,                                      /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    (initproc)Array_tp_init,                /* tp_init */
    0,                                      /* tp_alloc */
    Array_tp_new,                           /* tp_new */
};

/* Dict ============================================================= */

PyDoc_STRVAR(Dict_tp_doc,
"An mapping whose keys are similar and whose values are similar,\n"
"implemented as a subtype of dict.\n"
"\n"
"As currently implemented, a Dictionary behaves just like a dict, but\n"
"with the addition of a ``signature`` property set by the constructor;\n"
"conversion of its items to D-Bus types is only done when it's sent in\n"
"a Message. This may change in future so validation is done earlier.\n"
"\n"
":SupportedUsage:\n"
"   ``from dbus import Dictionary`` or ``from dbus.types import Dictionary``\n"
"\n"
":Constructor:\n"
"    Dictionary([mapping_or_iterable][, signature][, variant_level])\n"
"\n"
"    variant_level must be non-negative; the default is 0.\n"
"\n"
"    The signature may be None, in which case when the Dictionary is sent over\n"
"    D-Bus, the key and value signatures will be guessed from some arbitrary.\n"
"    element.\n"
"\n"
":IVariables:\n"
"  `variant_level` : int\n"
"    Indicates how many nested Variant containers this object\n"
"    is contained in: if a message's wire format has a variant containing a\n"
"    variant containing an array of DICT_ENTRY, this is represented in\n"
"    Python by a Dictionary with variant_level==2.\n"
);

static struct PyMemberDef Dict_tp_members[] = {
    {"signature", T_OBJECT, offsetof(DBusPyDict, signature), READONLY,
     "The D-Bus signature of each key in this Dictionary, followed by "
     "that of each value in this Dictionary, as a Signature instance."},
    {"variant_level", T_LONG, offsetof(DBusPyDict, variant_level),
     READONLY,
     "The number of nested variants wrapping the real data. "
     "0 if not in a variant."},
    {NULL},
};

static void
Dict_tp_dealloc (DBusPyDict *self)
{
    Py_XDECREF(self->signature);
    self->signature = NULL;
    (PyDict_Type.tp_dealloc)((PyObject *)self);
}

static PyObject *
Dict_tp_repr(DBusPyDict *self)
{
    PyObject *parent_repr = (PyDict_Type.tp_repr)((PyObject *)self);
    PyObject *sig_repr = PyObject_Repr(self->signature);
    PyObject *my_repr = NULL;
    long variant_level = self->variant_level;

    if (!parent_repr) goto finally;
    if (!sig_repr) goto finally;
    if (variant_level > 0) {
        my_repr = PyString_FromFormat("%s(%s, signature=%s, "
                                      "variant_level=%ld)",
                                      self->super.ob_type->tp_name,
                                      PyString_AS_STRING(parent_repr),
                                      PyString_AS_STRING(sig_repr),
                                      variant_level);
    }
    else {
        my_repr = PyString_FromFormat("%s(%s, signature=%s)",
                                      self->super.ob_type->tp_name,
                                      PyString_AS_STRING(parent_repr),
                                      PyString_AS_STRING(sig_repr));
    }
finally:
    Py_XDECREF(parent_repr);
    Py_XDECREF(sig_repr);
    return my_repr;
}

static PyObject *
Dict_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    DBusPyDict *self = (DBusPyDict *)(PyDict_Type.tp_new)(cls, args, kwargs);
    PyObject *variant_level = NULL;

    /* variant_level is immutable, so handle it in __new__ rather than 
    __init__ */
    if (!self) return NULL;
    Py_INCREF(Py_None);
    self->signature = Py_None;
    self->variant_level = 0;
    if (kwargs) {
        variant_level = PyDict_GetItem(kwargs, dbus_py_variant_level_const);
    }
    if (variant_level) {
        self->variant_level = PyInt_AsLong(variant_level);
        if (PyErr_Occurred()) {
            Py_DECREF((PyObject *)self);
            return NULL;
        }
    }
    return (PyObject *)self;
}

static int
Dict_tp_init(DBusPyDict *self, PyObject *args, PyObject *kwargs)
{
    PyObject *obj = dbus_py_empty_tuple;
    PyObject *signature = NULL;
    PyObject *tuple;
    PyObject *variant_level;    /* ignored here - __new__ uses it */
    static char *argnames[] = {"mapping_or_iterable", "signature",
                               "variant_level", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OOO:__init__", argnames,
                                     &obj, &signature, &variant_level)) {
        return -1;
    }

  /* convert signature from a borrowed ref of unknown type to an owned ref
  of type Signature (or None) */
    if (!signature) signature = Py_None;
    if (signature == Py_None
        || PyObject_IsInstance(signature, (PyObject *)&DBusPySignature_Type)) {
        Py_INCREF(signature);
    }
    else {
        signature = PyObject_CallFunction((PyObject *)&DBusPySignature_Type,
                                          "(O)", signature);
        if (!signature) return -1;
    }

    tuple = Py_BuildValue("(O)", obj);
    if (!tuple) {
        Py_DECREF(signature);
        return -1;
    }

    if ((PyDict_Type.tp_init((PyObject *)self, tuple, NULL)) < 0) {
        Py_DECREF(tuple);
        Py_DECREF(signature);
        return -1;
    }
    Py_DECREF(tuple);

    Py_XDECREF(self->signature);
    self->signature = signature;
    return 0;
}

PyTypeObject DBusPyDict_Type = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
    "dbus.Dictionary",
    sizeof(DBusPyDict),
    0,
    (destructor)Dict_tp_dealloc,            /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_compare */
    (reprfunc)Dict_tp_repr,                 /* tp_repr */
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
    Dict_tp_doc,                            /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    0,                                      /* tp_iter */
    0,                                      /* tp_iternext */
    0,                                      /* tp_methods */
    Dict_tp_members,                        /* tp_members */
    0,                                      /* tp_getset */
    0,                                      /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    (initproc)Dict_tp_init,                 /* tp_init */
    0,                                      /* tp_alloc */
    Dict_tp_new,                            /* tp_new */
};

/* Struct =========================================================== */

PyDoc_STRVAR(Struct_tp_doc,
"An structure containing items of possibly distinct types.\n"
"\n"
":SupportedUsage:\n"
"   ``from dbus import Struct`` or ``from dbus.types import Struct``\n"
"\n"
":Constructor:\n"
"    ``Struct(iterable[, signature: str][, variant_level: int]) -> Struct``\n"
"\n"
"    D-Bus structs may not be empty, so the iterable argument is required.\n"
"\n"
"    The signature may be omitted or None, in which case it will be guessed\n"
"    from the types of the items during construction.\n"
"\n"
"    variant_level must be non-negative; the default is 0.\n"
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
    PyObject *sig, *sig_repr = NULL;
    PyObject *vl_obj;
    long variant_level;
    PyObject *my_repr = NULL;

    if (!parent_repr) goto finally;
    sig = PyObject_GetAttr(self, dbus_py_signature_const);
    if (!sig) goto finally;
    sig_repr = PyObject_Repr(sig);
    if (!sig_repr) goto finally;
    vl_obj = PyObject_GetAttr(self, dbus_py_variant_level_const);
    if (!vl_obj) goto finally;
    variant_level = PyInt_AsLong(vl_obj);
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
    PyObject *signature = NULL;
    PyObject *variantness = NULL;
    PyObject *self;
    static char *argnames[] = {"signature", "variant_level", NULL};

    if (PyTuple_Size(args) != 1) {
        PyErr_SetString(PyExc_TypeError,
                        "__new__ takes exactly one positional parameter");
        return NULL;
    }
    if (!PyArg_ParseTupleAndKeywords(dbus_py_empty_tuple, kwargs,
                                     "|OO!:__new__", argnames,
                                     &signature, &PyInt_Type,
                                     &variantness)) {
        return NULL;
    }
    if (!variantness) {
        variantness = PyInt_FromLong(0);
    }

    self = (PyTuple_Type.tp_new)(cls, args, NULL);
    if (!self)
        return NULL;
    if (PyTuple_Size(self) < 1) {
        PyErr_SetString(PyExc_ValueError, "D-Bus structs may not be empty");
        Py_DECREF(self);
        return NULL;
    }

    if (PyObject_GenericSetAttr(self, dbus_py_variant_level_const, variantness) < 0) {
        Py_DECREF(self);
        return NULL;
    }

    /* convert signature from a borrowed ref of unknown type to an owned ref
    of type Signature (or None) */
    if (!signature) signature = Py_None;
    if (signature == Py_None
        || PyObject_IsInstance(signature, (PyObject *)&DBusPySignature_Type)) {
        Py_INCREF(signature);
    }
    else {
        signature = PyObject_CallFunction((PyObject *)&DBusPySignature_Type,
                                          "(O)", signature);
        if (!signature) {
            Py_DECREF(self);
            return NULL;
        }
    }

    if (PyObject_GenericSetAttr(self, dbus_py_signature_const, signature) < 0) {
        Py_DECREF(self);
        Py_DECREF(signature);
        return NULL;
    }
    Py_DECREF(signature);
    return self;
}

PyTypeObject DBusPyStruct_Type = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
    "dbus.Struct",
    INT_MAX, /* placeholder */
    0,
    0,                                      /* tp_dealloc */
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
    PyObject_GenericGetAttr,                /* tp_getattro */
    Glue_immutable_setattro,                /* tp_setattro */
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
    -sizeof(PyObject *),                    /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    Struct_tp_new,                          /* tp_new */
};

static inline int
init_container_types(void)
{
    DBusPyArray_Type.tp_base = &PyList_Type;
    if (PyType_Ready(&DBusPyArray_Type) < 0) return 0;
    DBusPyArray_Type.tp_print = NULL;

    DBusPyDict_Type.tp_base = &PyDict_Type;
    if (PyType_Ready(&DBusPyDict_Type) < 0) return 0;
    DBusPyDict_Type.tp_print = NULL;

    DBusPyStruct_Type.tp_basicsize = PyTuple_Type.tp_basicsize
                                     + 2*sizeof(PyObject *) - 1;
    DBusPyStruct_Type.tp_basicsize /= sizeof(PyObject *);
    DBusPyStruct_Type.tp_basicsize *= sizeof(PyObject *);
    DBusPyStruct_Type.tp_base = &PyTuple_Type;
    if (PyType_Ready(&DBusPyStruct_Type) < 0) return 0;
    DBusPyStruct_Type.tp_print = NULL;

    return 1;
}

static inline int
insert_container_types(PyObject *this_module)
{
    Py_INCREF(&DBusPyArray_Type);
    if (PyModule_AddObject(this_module, "Array",
                           (PyObject *)&DBusPyArray_Type) < 0) return 0;

    Py_INCREF(&DBusPyDict_Type);
    if (PyModule_AddObject(this_module, "Dictionary",
                           (PyObject *)&DBusPyDict_Type) < 0) return 0;

    Py_INCREF(&DBusPyStruct_Type);
    if (PyModule_AddObject(this_module, "Struct",
                           (PyObject *)&DBusPyStruct_Type) < 0) return 0;

    return 1;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */

