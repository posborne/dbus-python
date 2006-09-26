/* D-Bus container types: Variant, Array and Dict.
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

#include <stdint.h>

/* IntNN, UIntNN ==================================================== */

static PyTypeObject VariantType, ArrayType, DictType;

#define DEFINE_CHECK(type) \
static inline int type##_Check (PyObject *o) \
{ \
    return (o->ob_type == &type##Type) \
            || PyObject_IsInstance(o, (PyObject *)&type##Type); \
}
DEFINE_CHECK(Variant)
DEFINE_CHECK(Array)
DEFINE_CHECK(Dict)
#undef DEFINE_CHECK

PyDoc_STRVAR(Array_tp_doc,
"Array([iterable, ][signature=Signature(...)])\n\n"
"An array of similar items, implemented as a subtype of list.\n"
"\n"
"As currently implemented, an Array behaves just like a list, but\n"
"with the addition of a ``signature`` property set by the constructor;\n"
"conversion of its items to D-Bus types is only done when it's sent in\n"
"a Message. This may change in future so validation is done earlier.\n"
"\n"
"The signature may be None, in which case when the Array is sent over\n"
"D-Bus, the item signature will be guessed from the first element.\n");

typedef struct {
    PyListObject super;
    PyObject *signature;
} Array;

static struct PyMemberDef Array_tp_members[] = {
    {"signature", T_OBJECT, offsetof(Array, signature), READONLY,
     "The D-Bus signature of each element of this Array (a Signature "
     "instance)"},
    {NULL},
};

static void
Array_tp_dealloc (Array *self)
{
    Py_XDECREF(self->signature);
    self->signature = NULL;
    (PyList_Type.tp_dealloc)((PyObject *)self);
}

static PyObject *
Array_tp_repr(Array *self)
{
    PyObject *parent_repr = (PyList_Type.tp_repr)((PyObject *)self);
    PyObject *sig_repr = PyObject_Repr(self->signature);
    PyObject *my_repr = NULL;

    if (!parent_repr) goto finally;
    if (!sig_repr) goto finally;
    my_repr = PyString_FromFormat("%s(%s, signature=%s)",
                                  self->super.ob_type->tp_name,
                                  PyString_AS_STRING(parent_repr),
                                  PyString_AS_STRING(sig_repr));
finally:
    Py_XDECREF(parent_repr);
    Py_XDECREF(sig_repr);
    return my_repr;
}

static PyObject *
Array_tp_new (PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    Array *self = (Array *)(PyList_Type.tp_new)(cls, args, kwargs);
    if (!self) return NULL;
    Py_INCREF(Py_None);
    self->signature = Py_None;
    return (PyObject *)self;
}

static int
Array_tp_init (Array *self, PyObject *args, PyObject *kwargs)
{
    PyObject *obj = empty_tuple;
    PyObject *signature = NULL;
    PyObject *tuple;
    static char *argnames[] = {"iterable", "signature", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OO:__init__", argnames,
                                     &obj, &signature)) {
        return -1;
    }

  /* convert signature from a borrowed ref of unknown type to an owned ref
  of type Signature (or None) */
    if (!signature) signature = Py_None;
    if (signature == Py_None
        || PyObject_IsInstance(signature, (PyObject *)&SignatureType)) {
        Py_INCREF(signature);
    }
    else {
        signature = PyObject_CallFunction((PyObject *)&SignatureType, "(O)",
                                          signature);
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

static PyTypeObject ArrayType = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
    "dbus.Array",
    sizeof(Array),
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

PyDoc_STRVAR(Dict_tp_doc,
"Dictionary([mapping_or_iterable, ][signature=Signature(...)])\n\n"
"An mapping whose keys are similar and whose values are similar,\n"
"implemented as a subtype of dict.\n"
"\n"
"As currently implemented, a Dictionary behaves just like a dict, but\n"
"with the addition of a ``signature`` property set by the constructor;\n"
"conversion of its items to D-Bus types is only done when it's sent in\n"
"a Message. This may change in future so validation is done earlier.\n"
"\n"
"The signature may be None, in which case when the Dictionary is sent over\n"
"D-Bus, the key and value signatures will be guessed from some arbitrary.\n"
"element.\n");

typedef struct {
    PyDictObject super;
    PyObject *signature;
} Dict;

static struct PyMemberDef Dict_tp_members[] = {
    {"signature", T_OBJECT, offsetof(Dict, signature), READONLY,
     "The D-Bus signature of each key in this Dictionary, followed by "
     "that of each value in this Dictionary, as a Signature instance."},
    {NULL},
};

static void
Dict_tp_dealloc (Dict *self)
{
    Py_XDECREF(self->signature);
    self->signature = NULL;
    (PyDict_Type.tp_dealloc)((PyObject *)self);
}

static PyObject *
Dict_tp_repr(Dict *self)
{
    PyObject *parent_repr = (PyDict_Type.tp_repr)((PyObject *)self);
    PyObject *sig_repr = PyObject_Repr(self->signature);
    PyObject *my_repr = NULL;

    if (!parent_repr) goto finally;
    if (!sig_repr) goto finally;
    my_repr = PyString_FromFormat("%s(%s, signature=%s)",
                                  self->super.ob_type->tp_name,
                                  PyString_AS_STRING(parent_repr),
                                  PyString_AS_STRING(sig_repr));
finally:
    Py_XDECREF(parent_repr);
    Py_XDECREF(sig_repr);
    return my_repr;
}

static PyObject *
Dict_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    Dict *self = (Dict *)(PyDict_Type.tp_new)(cls, args, kwargs);
    if (!self) return NULL;
    Py_INCREF(Py_None);
    self->signature = Py_None;
    return (PyObject *)self;
}

static int
Dict_tp_init(Dict *self, PyObject *args, PyObject *kwargs)
{
    PyObject *obj = empty_tuple;
    PyObject *signature = NULL;
    PyObject *tuple;
    static char *argnames[] = {"mapping_or_iterable", "signature", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OO:__init__", argnames,
                                     &obj, &signature)) {
        return -1;
    }

  /* convert signature from a borrowed ref of unknown type to an owned ref
  of type Signature (or None) */
    if (!signature) signature = Py_None;
    if (signature == Py_None
        || PyObject_IsInstance(signature, (PyObject *)&SignatureType)) {
        Py_INCREF(signature);
    }
    else {
        signature = PyObject_CallFunction((PyObject *)&SignatureType, "(O)",
                                          signature);
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

static PyTypeObject DictType = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
    "dbus.Dictionary",
    sizeof(Dict),
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

PyDoc_STRVAR(Variant_tp_doc,
"Variant(object[, signature=Signature(...)])\n"
"\n"
"A container for D-Bus types. This is an immutable 'value object' like str\n"
"and int.\n"
"\n"
"The signature may be omitted or None, in which case it will be guessed \n"
"at construction time from the wrapped object, with the same algorithm\n"
"used when objects are appended to a message.\n"
"\n"
"A Variant v supports the following operations:\n"
"\n"
"* v(), v.object (the contained object)\n\n"
"* v.signature (the signature as a dbus.Signature)\n\n"
"* v == w if and only if v, w are Variants with the same signature and\n"
"  the objects they contain compare equal\n\n"
"* bool(v) == bool(v.object)\n"
"* str(v) == str(v.object)\n\n"
"* repr(v) returns something like \"Variant(123, signature='i')\"\n\n"
"* int(v) == int(v.object), if supported; ditto long(v), float(v),\n"
"  hex(v), oct(v)\n\n"
"* iter(v) iterates over the contained object, if it's iterable\n\n"
);

typedef struct {
    PyObject_HEAD
    PyObject *object;
    PyObject *signature;
} Variant;

static struct PyMemberDef Variant_tp_members[] = {
    {"object", T_OBJECT, offsetof(Variant, object), READONLY,
     "The wrapped object"},
    {"signature", T_OBJECT, offsetof(Variant, signature), READONLY,
     "The D-Bus signature of the contained object (a Signature instance)"},
    {NULL},
};

static void
Variant_tp_dealloc (Variant *self)
{
    Py_XDECREF(self->object);
    self->object = NULL;
    Py_XDECREF(self->signature);
    self->signature = NULL;
    (self->ob_type->tp_free)(self);
}

/* forward declaration */
static PyObject *Message_guess_signature(PyObject *unused, PyObject *args);

static PyObject *
Variant_tp_new (PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    Variant *self;
    PyObject *obj;
    PyObject *signature = NULL;
    static char *argnames[] = {"object", "signature", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:__new__", argnames,
                                     &obj, &signature)) {
        return NULL;
    }

    /* convert signature from a borrowed ref of unknown type to an owned ref
    of type Signature, somehow... */
    if (!signature || signature == Py_None) {
        /* There's no signature, we'll have to guess what the user wanted */
        PyObject *tuple = Py_BuildValue("(O)", obj);

        if (!tuple) return NULL;
        signature = Message_guess_signature(NULL, tuple);
        Py_DECREF(tuple);
        if (!signature) return NULL;
    }
    else if (PyObject_IsInstance(signature, (PyObject *)&SignatureType)) {
        /* Already a Signature, so that's easy */
        Py_INCREF(signature);
    }
    else {
        /* See what the Signature constructor thinks of it */
        signature = PyObject_CallFunction((PyObject *)&SignatureType, "(O)",
                                          signature);
        if (!signature) return NULL;
    }

    self = (Variant *)(cls->tp_alloc)(cls, 0);
    if (!self) {
        Py_DECREF(signature);
        return NULL;
    }

    self->signature = signature;
    Py_INCREF (obj);
    self->object = obj;
    return (PyObject *)self;
}

static PyObject *
Variant_tp_repr (Variant *self)
{
    PyObject *my_repr = NULL;
    PyObject *obj_repr;
    PyObject *sig_repr;

    if (!self->object) {
        return PyString_FromString("<invalid dbus.Variant with no object>");
    }

    obj_repr = PyObject_Repr(self->object);
    sig_repr = PyObject_Repr(self->signature);
    if (obj_repr && sig_repr) {
        my_repr = PyString_FromFormat("%s(%s, signature=%s)",
                                      self->ob_type->tp_name,
                                      PyString_AS_STRING(obj_repr),
                                      PyString_AS_STRING(sig_repr));
    }
    /* whether my_repr is NULL or not: */
    Py_XDECREF(obj_repr);
    Py_XDECREF(sig_repr);
    return my_repr;
}

static PyObject *
Variant_tp_iter(Variant *self)
{
    return PyObject_GetIter(self->object);
}

static PyObject *
Variant_tp_call(Variant *self, PyObject *args, PyObject *kwargs)
{
    if ((args && PyObject_Size(args)) || (kwargs && PyObject_Size(kwargs))) {
        PyErr_SetString(PyExc_TypeError, "Variant.__call__ takes no arguments");
        return NULL;
    }
    Py_INCREF(self->object);
    return self->object;
}

static long
Variant_tp_hash(Variant *self)
{
    return PyObject_Hash(self->object);
}

static PyObject *
Variant_tp_richcompare(Variant *self, PyObject *other, int opid)
{
    PyObject *ret;

    if (opid == Py_EQ || opid == Py_NE) {
        if (Variant_Check(other)) {
            Variant *vo = (Variant *)other;
            ret = PyObject_RichCompare(self->signature, vo->signature, opid);
            if (!ret) return NULL;
            if (!PyObject_IsTrue(ret)) {
                if (opid == Py_EQ) {
                    /* Signatures are unequal, we require equal. Fail. */
                    return ret;
                }
            }
            else {
                if (opid == Py_NE) {
                    /* Signatures are unequal, succeed. */
                    return ret;
                }
            }
            Py_DECREF(ret);
            ret = PyObject_RichCompare(self->object, vo->object, opid);
            if (!ret) return NULL;
            if (!PyObject_IsTrue(ret)) {
                if (opid == Py_EQ) {
                    /* Objects are unequal, we require equal. Fail. */
                    return ret;
                }
            }
            else {
                if (opid == Py_NE) {
                    /* Objects are unequal, succeed. */
                    return ret;
                }
            }
            Py_DECREF(ret);
            if (opid == Py_NE) {
                Py_RETURN_FALSE;
            }
            else {
                Py_RETURN_TRUE;
            }
        }
        /* Types are dissimilar */
        if (opid == Py_NE) {
            Py_RETURN_TRUE;
        }
        else {
            Py_RETURN_FALSE;
        }
    }
    else {
        PyErr_SetString(PyExc_TypeError, "Variant objects do not support "
                        "ordered comparison (<, >, <=, >=)");
        return NULL;
    }
}

static PyObject *
Variant_tp_str(Variant *self)
{
    if (PyString_Check(self->object)) {
        Py_INCREF(self->object);
        return self->object;
    }
    return PyObject_Str(self->object);
}

static int
Variant_tp_nonzero(Variant *self)
{
    return PyObject_IsTrue(self->object);
}

static PyObject *
Variant_nb_int(Variant *self)
{
    if (PyInt_Check(self->object) || PyLong_Check(self->object)) {
        Py_INCREF(self->object);
        return self->object;
    }
    return PyNumber_Int(self->object);
}

static PyObject *
Variant_nb_oct(Variant *self)
{
    PyObject *ret;
    PyObject *i = Variant_nb_int(self);

    if (!i) return NULL;
    if (!PyNumber_Check(i) || !i->ob_type->tp_as_number->nb_oct) {
        PyErr_SetString(PyExc_TypeError, "Value stored in Variant does not "
                        "support conversion to octal");
        Py_DECREF(i);
        return NULL;
    }
    ret = (i->ob_type->tp_as_number->nb_oct)(i);
    Py_DECREF(i);
    return ret;
}

static PyObject *
Variant_nb_hex(Variant *self)
{
    PyObject *ret;
    PyObject *i = Variant_nb_int(self);

    if (!i) return NULL;
    if (!PyNumber_Check(i) || !i->ob_type->tp_as_number->nb_hex) {
        PyErr_SetString(PyExc_TypeError, "Value stored in Variant does not "
                        "support conversion to hexadecimal");
        Py_DECREF(i);
        return NULL;
    }
    ret = (i->ob_type->tp_as_number->nb_hex)(i);
    Py_DECREF(i);
    return ret;
}

static PyObject *
Variant_nb_long(Variant *self)
{
    if (PyLong_Check(self->object)) {
        Py_INCREF(self->object);
        return self->object;
    }
    return PyNumber_Long(self->object);
}

static PyObject *
Variant_nb_float(Variant *self)
{
    if (PyFloat_Check(self->object)) {
        Py_INCREF(self->object);
        return self->object;
    }
    return PyNumber_Float(self->object);
}

static PyNumberMethods Variant_tp_as_number = {
    NULL, /* nb_add */
    NULL, /* nb_subtract */
    NULL, /* nb_multiply */
    NULL, /* nb_divide */
    NULL, /* nb_remainder */
    NULL, /* nb_divmod */
    NULL, /* nb_power */
    NULL, /* nb_negative */
    NULL, /* tp_positive */
    NULL, /* tp_absolute */
    (inquiry)Variant_tp_nonzero, /* tp_nonzero */
    NULL, /* nb_invert */
    NULL, /* nb_lshift */
    NULL, /* nb_rshift */
    NULL, /* nb_and */
    NULL, /* nb_xor */
    NULL, /* nb_or */
    NULL, /* nb_coerce */
    (unaryfunc)Variant_nb_int,
    (unaryfunc)Variant_nb_long,
    (unaryfunc)Variant_nb_float,
    (unaryfunc)Variant_nb_oct,
    (unaryfunc)Variant_nb_hex,
    NULL, /* nb_inplace_add */
    NULL, /* nb_inplace_subtract */
    NULL, /* nb_inplace_multiply */
    NULL, /* nb_inplace_divide */
    NULL, /* nb_inplace_remainder */
    NULL, /* nb_inplace_power */
    NULL, /* nb_inplace_lshift */
    NULL, /* nb_inplace_rshift */
    NULL, /* nb_inplace_and */
    NULL, /* nb_inplace_xor */
    NULL, /* nb_inplace_or */
    NULL, /* nb_floor_divide */
    NULL, /* nb_true_divide */
    NULL, /* nb_inplace_floor_divide */
    NULL, /* nb_inplace_true_divide */
};

static PyTypeObject VariantType = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
    "dbus.Variant",
    sizeof(Variant),
    0,
    (destructor)Variant_tp_dealloc,         /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_compare */
    (reprfunc)Variant_tp_repr,              /* tp_repr */
    &Variant_tp_as_number,                  /* tp_as_number */
    0,                                      /* tp_as_sequence */
    0,                                      /* tp_as_mapping */
    (hashfunc)Variant_tp_hash,              /* tp_hash */
    (ternaryfunc)Variant_tp_call,           /* tp_call */
    (reprfunc)Variant_tp_str,               /* tp_str */
    0,                                      /* tp_getattro */
    0,                                      /* tp_setattro */
    0,                                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    Variant_tp_doc,                         /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    (richcmpfunc)Variant_tp_richcompare,    /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    (getiterfunc)Variant_tp_iter,           /* tp_iter */
    0,                                      /* tp_iternext */
    0,                                      /* tp_methods */
    Variant_tp_members,                     /* tp_members */
    0,                                      /* tp_getset */
    0,                                      /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    Variant_tp_new,                         /* tp_new */
};

static inline int
init_container_types(void)
{
    ArrayType.tp_base = &PyList_Type;
    if (PyType_Ready(&ArrayType) < 0) return 0;
    ArrayType.tp_print = NULL;

    DictType.tp_base = &PyDict_Type;
    if (PyType_Ready(&DictType) < 0) return 0;
    DictType.tp_print = NULL;

    if (PyType_Ready(&VariantType) < 0) return 0;

    return 1;
}

static inline int
insert_container_types(PyObject *this_module)
{
    Py_INCREF(&ArrayType);
    if (PyModule_AddObject(this_module, "Array",
                           (PyObject *)&ArrayType) < 0) return 0;

    Py_INCREF(&DictType);
    if (PyModule_AddObject(this_module, "Dictionary",
                           (PyObject *)&DictType) < 0) return 0;

    Py_INCREF(&VariantType);
    if (PyModule_AddObject(this_module, "Variant",
                           (PyObject *)&VariantType) < 0) return 0;

    return 1;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */

