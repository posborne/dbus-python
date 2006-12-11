/* Simple D-Bus types: integers of various sizes, and ObjectPath.
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

/* Specific types =================================================== */

/* Boolean, a subclass of DBusPythonInt ============================= */

PyDoc_STRVAR(Boolean_tp_doc,
"A boolean, represented as a subtype of `int` (not `bool`, because `bool`\n"
"cannot be subclassed).\n"
"\n"
":SupportedUsage:\n"
"    ``from dbus import Boolean`` or ``from dbus.types import Boolean``\n"
"\n"
":Constructor:\n"
"    Boolean(value: int[, variant_level: int]) -> Boolean\n"
"\n"
"    value is converted to 0 or 1.\n"
"\n"
"    variant_level must be non-negative; the default is 0.\n"
"\n"
":IVariables:\n"
"  `variant_level` : int\n"
"    Indicates how many nested Variant containers this object\n"
"    is contained in: if a message's wire format has a variant containing a\n"
"    variant containing a boolean, this is represented in Python by a\n"
"    Boolean with variant_level==2.\n"
);

static PyObject *
Boolean_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *tuple, *self, *value = Py_None;
    long variantness = 0;
    static char *argnames[] = {"_", "variant_level", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Ol:__new__", argnames,
                                     &value, &variantness)) return NULL;
    if (variantness < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "variant_level must be non-negative");
        return NULL;
    }
    tuple = Py_BuildValue("(i)", PyObject_IsTrue(value) ? 1 : 0);
    if (!tuple) return NULL;
    self = (DBusPyIntBase_Type.tp_new)(cls, tuple, kwargs);
    Py_DECREF(tuple);
    return self;
}

static PyObject *
Boolean_tp_repr (PyObject *self)
{
    long variant_level = ((DBusPyIntBase *)self)->variant_level;
    if (variant_level > 0) {
        return PyString_FromFormat("%s(%s, variant_level=%ld)",
                                   self->ob_type->tp_name,
                                   PyInt_AsLong(self) ? "True" : "False",
                                   variant_level);
    }
    return PyString_FromFormat("%s(%s)",
                               self->ob_type->tp_name,
                               PyInt_AsLong(self) ? "True" : "False");
}

PyTypeObject DBusPyBoolean_Type = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
    "dbus.Boolean",
    0,
    0,
    0,                                      /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_compare */
    Boolean_tp_repr,                        /* tp_repr */
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
    Boolean_tp_doc,                         /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    0,                                      /* tp_iter */
    0,                                      /* tp_iternext */
    0,                                      /* tp_methods */
    0,                                      /* tp_members */
    0,                                      /* tp_getset */
    DEFERRED_ADDRESS(&DBusPyIntBase_Type),  /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    Boolean_tp_new,                         /* tp_new */
};

/* Int16 ============================================================ */

PyDoc_STRVAR(Int16_tp_doc,
"A signed 16-bit integer between -0x8000 and +0x7FFF, represented as\n"
"a subtype of `int`.\n"
"\n"
":SupportedUsage:\n"
"    ``from dbus import Int16`` or ``from dbus.types import Int16``\n"
"\n"
":Constructor:\n"
"    Int16(value: int[, variant_level: int]) -> Int16\n"
"    value must be within the allowed range, or OverflowError will be\n"
"    raised.\n"
"    variant_level must be non-negative; the default is 0.\n"
"\n"
":IVariables:\n"
"  `variant_level` : int\n"
"    Indicates how many nested Variant containers this object\n"
"    is contained in: if a message's wire format has a variant containing a\n"
"    variant containing an int16, this is represented in Python by an\n"
"    Int16 with variant_level==2.\n"
);

dbus_int16_t
dbus_py_int16_range_check(PyObject *obj)
{
    long i = PyInt_AsLong (obj);
    if (i == -1 && PyErr_Occurred ()) return -1;
    if (i < -0x8000 || i > 0x7fff) {
        PyErr_Format(PyExc_OverflowError, "Value %d out of range for Int16",
                     (int)i);
        return -1;
    }
    return i;
}

static PyObject *
Int16_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *self = (DBusPyIntBase_Type.tp_new)(cls, args, kwargs);
    if (self && dbus_py_int16_range_check(self) == -1 && PyErr_Occurred()) {
        Py_DECREF(self);
        return NULL;
    }
    return self;
}

PyTypeObject DBusPyInt16_Type = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
    "dbus.Int16",
    0,
    0,
    0,                                      /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_compare */
    0,                                      /* tp_repr */
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
    Int16_tp_doc,                           /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    0,                                      /* tp_iter */
    0,                                      /* tp_iternext */
    0,                                      /* tp_methods */
    0,                                      /* tp_members */
    0,                                      /* tp_getset */
    DEFERRED_ADDRESS(&DBusPyIntBase_Type),  /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    Int16_tp_new,                           /* tp_new */
};

/* UInt16 =========================================================== */

PyDoc_STRVAR(UInt16_tp_doc,
"An unsigned 16-bit integer between 0 and 0xFFFF, represented as\n"
"a subtype of `int`.\n"
"\n"
":SupportedUsage:\n"
"    ``from dbus import UInt16`` or ``from dbus.types import UInt16``\n"
"\n"
":Constructor:\n"
"    UInt16(value: int[, variant_level: int]) -> UInt16\n"
"    value must be within the allowed range, or OverflowError will be\n"
"    raised.\n"
"    variant_level must be non-negative; the default is 0.\n"
"\n"
":IVariables:\n"
"  `variant_level` : int\n"
"    Indicates how many nested Variant containers this object\n"
"    is contained in: if a message's wire format has a variant containing a\n"
"    variant containing a uint16, this is represented in Python by a\n"
"    UInt16 with variant_level==2.\n"
);

dbus_uint16_t
dbus_py_uint16_range_check(PyObject *obj)
{
    long i = PyInt_AsLong(obj);
    if (i == -1 && PyErr_Occurred()) return (dbus_uint16_t)(-1);
    if (i < 0 || i > 0xffff) {
        PyErr_Format(PyExc_OverflowError, "Value %d out of range for UInt16",
                     (int)i);
        return (dbus_uint16_t)(-1);
    }
    return i;
}

static PyObject *
UInt16_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *self = (DBusPyIntBase_Type.tp_new)(cls, args, kwargs);
    if (self && dbus_py_uint16_range_check(self) == (dbus_uint16_t)(-1)
        && PyErr_Occurred()) {
        Py_DECREF (self);
        return NULL;
    }
    return self;
}

PyTypeObject DBusPyUInt16_Type = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
    "dbus.UInt16",
    0,
    0,
    0,                                      /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_compare */
    0,                                      /* tp_repr */
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
    UInt16_tp_doc,                          /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    0,                                      /* tp_iter */
    0,                                      /* tp_iternext */
    0,                                      /* tp_methods */
    0,                                      /* tp_members */
    0,                                      /* tp_getset */
    DEFERRED_ADDRESS(&DBusPyIntBase_Type),   /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    UInt16_tp_new,                          /* tp_new */
};

/* Int32 ============================================================ */

PyDoc_STRVAR(Int32_tp_doc,
"A signed 32-bit integer between -0x8000 0000 and +0x7FFF FFFF, represented as\n"
"a subtype of `int`.\n"
"\n"
":SupportedUsage:\n"
"    ``from dbus import Int32`` or ``from dbus.types import Int32``\n"
"\n"
":Constructor:\n"
"    Int32(value: int[, variant_level: int]) -> Int32\n"
"    value must be within the allowed range, or OverflowError will be\n"
"    raised.\n"
"    variant_level must be non-negative; the default is 0.\n"
"\n"
":IVariables:\n"
"  `variant_level` : int\n"
"    Indicates how many nested Variant containers this object\n"
"    is contained in: if a message's wire format has a variant containing a\n"
"    variant containing an int32, this is represented in Python by an\n"
"    Int32 with variant_level==2.\n"
);

dbus_int32_t
dbus_py_int32_range_check(PyObject *obj)
{
    long i = PyInt_AsLong(obj);
    if (i == -1 && PyErr_Occurred()) return -1;
    if (i < INT32_MIN || i > INT32_MAX) {
        PyErr_Format(PyExc_OverflowError, "Value %d out of range for Int32",
                     (int)i);
        return -1;
    }
    return i;
}

static PyObject *
Int32_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *self = (DBusPyIntBase_Type.tp_new)(cls, args, kwargs);
    if (self && dbus_py_int32_range_check(self) == -1 && PyErr_Occurred()) {
        Py_DECREF(self);
        return NULL;
    }
    return self;
}

PyTypeObject DBusPyInt32_Type = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
    "dbus.Int32",
    0,
    0,
    0,                                      /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_compare */
    0,                                      /* tp_repr */
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
    Int32_tp_doc,                           /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    0,                                      /* tp_iter */
    0,                                      /* tp_iternext */
    0,                                      /* tp_methods */
    0,                                      /* tp_members */
    0,                                      /* tp_getset */
    DEFERRED_ADDRESS(&DBusPyIntBase_Type),   /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    Int32_tp_new,                           /* tp_new */
};

/* UInt32 =========================================================== */

PyDoc_STRVAR(UInt32_tp_doc,
"An unsigned 32-bit integer between 0 and 0xFFFF FFFF, represented as a\n"
"subtype of `long`.\n"
"\n"
"Note that this may be changed in future to be a subtype of `int` on\n"
"64-bit platforms; applications should not rely on either behaviour.\n"
"\n"
":SupportedUsage:\n"
"    ``from dbus import UInt32`` or ``from dbus.types import UInt32``\n"
"\n"
":Constructor:\n"
"    UInt32(value: long[, variant_level: int]) -> UInt32\n"
"    value must be within the allowed range, or OverflowError will be\n"
"    raised.\n"
"    variant_level must be non-negative; the default is 0.\n"
"\n"
":IVariables:\n"
"  `variant_level` : int\n"
"    Indicates how many nested Variant containers this object\n"
"    is contained in: if a message's wire format has a variant containing a\n"
"    variant containing a uint32, this is represented in Python by a\n"
"    UInt32 with variant_level==2.\n"
);

dbus_uint32_t
dbus_py_uint32_range_check(PyObject *obj)
{
    unsigned long i;
    PyObject *long_obj = PyNumber_Long(obj);

    if (!long_obj) return (dbus_uint32_t)(-1);
    i = PyLong_AsUnsignedLong(long_obj);
    if (i == (unsigned long)(-1) && PyErr_Occurred()) {
        Py_DECREF(long_obj);
        return (dbus_uint32_t)(-1);
    }
    if (i > UINT32_MAX) {
        PyErr_Format(PyExc_OverflowError, "Value %d out of range for UInt32",
                     (int)i);
        Py_DECREF(long_obj);
        return (dbus_uint32_t)(-1);
    }
    Py_DECREF(long_obj);
    return i;
}

static PyObject *
UInt32_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *self = (DBusPyLongBase_Type.tp_new)(cls, args, kwargs);
    if (self && dbus_py_uint32_range_check(self) == (dbus_uint32_t)(-1)
        && PyErr_Occurred()) {
        Py_DECREF(self);
        return NULL;
    }
    return self;
}

PyTypeObject DBusPyUInt32_Type = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
    "dbus.UInt32",
    0,
    0,
    0,                                      /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_compare */
    0,                                      /* tp_repr */
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
    UInt32_tp_doc,                          /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    0,                                      /* tp_iter */
    0,                                      /* tp_iternext */
    0,                                      /* tp_methods */
    0,                                      /* tp_members */
    0,                                      /* tp_getset */
    DEFERRED_ADDRESS(&DBusPyLongBase_Type),  /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    UInt32_tp_new,                          /* tp_new */
};

/* Int64 =========================================================== */

PyDoc_STRVAR(Int64_tp_doc,
"A signed 64-bit integer between -0x8000 0000 0000 0000 and\n"
"+0x7FFF FFFF FFFF FFFF, represented as a subtype of `long`.\n"
"\n"
"Note that this may be changed in future to be a subtype of `int` on\n"
"64-bit platforms; applications should not rely on either behaviour.\n"
"\n"
"This type only works on platforms where the C compiler has suitable\n"
"64-bit types, such as C99 ``long long``.\n"
#ifdef DBUS_PYTHON_64_BIT_WORKS
    "This is the case on your current platform.\n"
#else /* !defined(DBUS_PYTHON_64_BIT_WORKS) */
    "This is not the case on your current platform, so this type's\n"
    "constructor will always raise NotImplementedError. Try a better\n"
    "compiler, if one is available.\n"
#endif /* !defined(DBUS_PYTHON_64_BIT_WORKS) */
"\n"
":SupportedUsage:\n"
"    ``from dbus import Int64`` or ``from dbus.types import Int64``\n"
"\n"
":Constructor:\n"
"    Int64(value: long[, variant_level: int]) -> Int64\n"
"    value must be within the allowed range, or OverflowError will be\n"
"    raised.\n"
"    variant_level must be non-negative; the default is 0.\n"
"\n"
":IVariables:\n"
"  `variant_level` : int\n"
"    Indicates how many nested Variant containers this object\n"
"    is contained in: if a message's wire format has a variant containing a\n"
"    variant containing an int64, this is represented in Python by an\n"
"    Int64 with variant_level==2.\n"
);

#ifdef DBUS_PYTHON_64_BIT_WORKS
dbus_int64_t
dbus_py_int64_range_check(PyObject *obj)
{
    PY_LONG_LONG i;
    PyObject *long_obj = PyNumber_Long(obj);

    if (!long_obj) return -1;
    i = PyLong_AsLongLong(long_obj);
    if (i == -1 && PyErr_Occurred()) {
        Py_DECREF(long_obj);
        return -1;
    }
    if (i < INT64_MIN || i > INT64_MAX) {
        PyErr_SetString(PyExc_OverflowError, "Value out of range for Int64");
        Py_DECREF(long_obj);
        return -1;
    }
    Py_DECREF(long_obj);
    return i;
}
#endif

static PyObject *
Int64_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
#ifdef DBUS_PYTHON_64_BIT_WORKS
    PyObject *self = (DBusPyLongBase_Type.tp_new)(cls, args, kwargs);
    if (self && dbus_py_int64_range_check(self) == -1 && PyErr_Occurred()) {
        Py_DECREF(self);
        return NULL;
    }
    return self;
#else
    PyErr_SetString(PyExc_NotImplementedError,
                    "64-bit types are not available on this platform");
    return NULL;
#endif
}

PyTypeObject DBusPyInt64_Type = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
    "dbus.Int64",
    0,
    0,
    0,                                      /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_compare */
    0,                                      /* tp_repr */
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
    Int64_tp_doc,                           /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    0,                                      /* tp_iter */
    0,                                      /* tp_iternext */
    0,                                      /* tp_methods */
    0,                                      /* tp_members */
    0,                                      /* tp_getset */
    DEFERRED_ADDRESS(&DBusPyLongBase_Type),  /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    Int64_tp_new,                           /* tp_new */
};

/* UInt64 =========================================================== */

PyDoc_STRVAR(UInt64_tp_doc,
"An unsigned 64-bit integer between 0 and 0xFFFF FFFF FFFF FFFF,\n"
"represented as a subtype of `long`.\n"
"\n"
"This type only exists on platforms where the C compiler has suitable\n"
"64-bit types, such as C99 ``unsigned long long``.\n"
#ifdef DBUS_PYTHON_64_BIT_WORKS
    "This is the case on your current platform.\n"
#else /* !defined(DBUS_PYTHON_64_BIT_WORKS) */
    "This is not the case on your current platform, so this type's\n"
    "constructor will always raise NotImplementedError. Try a better\n"
    "compiler, if one is available.\n"
#endif /* !defined(DBUS_PYTHON_64_BIT_WORKS) */
"\n"
":Constructor:\n"
"    UInt64(value: long[, variant_level: int]) -> UInt64\n"
"    value must be within the allowed range, or OverflowError will be\n"
"    raised.\n"
"    variant_level must be non-negative; the default is 0.\n"
"\n"
":IVariables:\n"
"  `variant_level` : int\n"
"    Indicates how many nested Variant containers this object\n"
"    is contained in: if a message's wire format has a variant containing a\n"
"    variant containing a uint64, this is represented in Python by a\n"
"    UInt64 with variant_level==2.\n"
);

dbus_uint64_t
dbus_py_uint64_range_check(PyObject *obj)
{
    unsigned PY_LONG_LONG i;
    PyObject *long_obj = PyNumber_Long(obj);

    if (!long_obj) return (dbus_uint64_t)(-1);
    i = PyLong_AsUnsignedLongLong(long_obj);
    if (i == (unsigned PY_LONG_LONG)(-1) && PyErr_Occurred()) {
        Py_DECREF(long_obj);
        return (dbus_uint64_t)(-1);
    }
    if (i > UINT64_MAX) {
        PyErr_SetString(PyExc_OverflowError, "Value out of range for UInt64");
        Py_DECREF(long_obj);
        return (dbus_uint64_t)(-1);
    }
    Py_DECREF(long_obj);
    return i;
}

static PyObject *
UInt64_tp_new (PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
#ifdef DBUS_PYTHON_64_BIT_WORKS
    PyObject *self = (DBusPyLongBase_Type.tp_new)(cls, args, kwargs);
    if (self && dbus_py_uint64_range_check(self) == (dbus_uint64_t)(-1)
        && PyErr_Occurred()) {
        Py_DECREF(self);
        return NULL;
    }
    return self;
#else
    PyErr_SetString(PyExc_NotImplementedError,
                    "64-bit integer types are not supported on this platform");
    return NULL;
#endif
}

PyTypeObject DBusPyUInt64_Type = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
    "dbus.UInt64",
    0,
    0,
    0,                                      /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_compare */
    0,                                      /* tp_repr */
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
    UInt64_tp_doc,                          /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    0,                                      /* tp_iter */
    0,                                      /* tp_iternext */
    0,                                      /* tp_methods */
    0,                                      /* tp_members */
    0,                                      /* tp_getset */
    DEFERRED_ADDRESS(&DBusPyLongBase_Type),  /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    UInt64_tp_new,                          /* tp_new */
};

dbus_bool_t
dbus_py_init_int_types(void)
{
    DBusPyInt16_Type.tp_base = &DBusPyIntBase_Type;
    if (PyType_Ready(&DBusPyInt16_Type) < 0) return 0;
    /* disable the tp_print copied from PyInt_Type, so tp_repr gets called as
    desired */
    DBusPyInt16_Type.tp_print = NULL;

    DBusPyUInt16_Type.tp_base = &DBusPyIntBase_Type;
    if (PyType_Ready(&DBusPyUInt16_Type) < 0) return 0;
    DBusPyUInt16_Type.tp_print = NULL;

    DBusPyInt32_Type.tp_base = &DBusPyIntBase_Type;
    if (PyType_Ready(&DBusPyInt32_Type) < 0) return 0;
    DBusPyInt32_Type.tp_print = NULL;

    DBusPyUInt32_Type.tp_base = &DBusPyLongBase_Type;
    if (PyType_Ready(&DBusPyUInt32_Type) < 0) return 0;
    DBusPyUInt32_Type.tp_print = NULL;

#if defined(DBUS_HAVE_INT64) && defined(HAVE_LONG_LONG)
    DBusPyInt64_Type.tp_base = &DBusPyLongBase_Type;
    if (PyType_Ready(&DBusPyInt64_Type) < 0) return 0;
    DBusPyInt64_Type.tp_print = NULL;

    DBusPyUInt64_Type.tp_base = &DBusPyLongBase_Type;
    if (PyType_Ready(&DBusPyUInt64_Type) < 0) return 0;
    DBusPyUInt64_Type.tp_print = NULL;
#endif
    return 1;
}

dbus_bool_t
dbus_py_insert_int_types(PyObject *this_module)
{
    Py_INCREF(&DBusPyInt16_Type);
    Py_INCREF(&DBusPyUInt16_Type);
    Py_INCREF(&DBusPyInt32_Type);
    Py_INCREF(&DBusPyUInt32_Type);
    Py_INCREF(&DBusPyInt64_Type);
    Py_INCREF(&DBusPyUInt64_Type);
    Py_INCREF(&DBusPyBoolean_Type);
    if (PyModule_AddObject(this_module, "Int16",
                           (PyObject *)&DBusPyInt16_Type) < 0) return 0;
    if (PyModule_AddObject(this_module, "UInt16",
                           (PyObject *)&DBusPyUInt16_Type) < 0) return 0;
    if (PyModule_AddObject(this_module, "Int32",
                           (PyObject *)&DBusPyInt32_Type) < 0) return 0;
    if (PyModule_AddObject(this_module, "UInt32",
                           (PyObject *)&DBusPyUInt32_Type) < 0) return 0;
    if (PyModule_AddObject(this_module, "Int64",
                           (PyObject *)&DBusPyInt64_Type) < 0) return 0;
    if (PyModule_AddObject(this_module, "UInt64",
                           (PyObject *)&DBusPyUInt64_Type) < 0) return 0;
    if (PyModule_AddObject(this_module, "Boolean",
                           (PyObject *)&DBusPyBoolean_Type) < 0) return 0;

    return 1;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
