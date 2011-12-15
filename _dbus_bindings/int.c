/* Simple D-Bus types: integers of various sizes, and ObjectPath.
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

#include "types-internal.h"

#ifdef PY3
#define INTBASE (DBusPyLongBase_Type)
#else
#define INTBASE (DBusPyIntBase_Type)
#endif

/* Specific types =================================================== */

/* Boolean, a subclass of DBusPythonInt ============================= */

PyDoc_STRVAR(Boolean_tp_doc,
"A boolean, represented as a subtype of `int` (not `bool`, because `bool`\n"
"cannot be subclassed).\n"
"\n"
"Constructor::\n"
"\n"
"    dbus.Boolean(value[, variant_level]) -> Boolean\n"
"\n"
"``value`` is converted to 0 or 1 as if by ``int(bool(value))``.\n"
"\n"
"``variant_level`` must be non-negative; the default is 0.\n"
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
    self = (INTBASE.tp_new)(cls, tuple, kwargs);
    Py_CLEAR(tuple);
    return self;
}

static PyObject *
Boolean_tp_repr(PyObject *self)
{
    int is_true = PyObject_IsTrue(self);
#ifdef PY3
    long variant_level = dbus_py_variant_level_get(self);
    if (variant_level < 0)
        return NULL;
#else
    long variant_level = ((DBusPyIntBase *)self)->variant_level;
#endif

    if (is_true == -1)
        return NULL;

    if (variant_level > 0) {
        return PyUnicode_FromFormat("%s(%s, variant_level=%ld)",
                                    Py_TYPE(self)->tp_name,
                                    is_true ? "True" : "False",
                                    variant_level);
    }
    return PyUnicode_FromFormat("%s(%s)",
                                Py_TYPE(self)->tp_name,
                                is_true ? "True" : "False");
}

PyTypeObject DBusPyBoolean_Type = {
    PyVarObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type), 0)
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
    DEFERRED_ADDRESS(&INTBASE),             /* tp_base */
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
"Constructor::\n"
"\n"
"    dbus.Int16(value: int[, variant_level: int]) -> Int16\n"
"\n"
"value must be within the allowed range, or OverflowError will be\n"
"raised.\n"
"\n"
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
    long i = PyLong_AsLong(obj);
    if (i == -1 && PyErr_Occurred())
        return -1;

    if (i < -0x8000 || i > 0x7fff) {
        PyErr_Format(PyExc_OverflowError, "Value %d out of range for Int16",
                     (int)i);
        return -1;
    }
    return (dbus_int16_t)i;
}

static PyObject *
Int16_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *self = (INTBASE.tp_new)(cls, args, kwargs);
    if (self && dbus_py_int16_range_check(self) == -1 && PyErr_Occurred()) {
        Py_CLEAR(self);
        return NULL;
    }
    return self;
}

PyTypeObject DBusPyInt16_Type = {
    PyVarObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type), 0)
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
    DEFERRED_ADDRESS(&INTBASE),             /* tp_base */
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
"Constructor::\n"
"\n"
"    dbus.UInt16(value: int[, variant_level: int]) -> UInt16\n"
"\n"
"``value`` must be within the allowed range, or `OverflowError` will be\n"
"raised.\n"
"\n"
"``variant_level`` must be non-negative; the default is 0.\n"
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
    long i = PyLong_AsLong(obj);
    if (i == -1 && PyErr_Occurred())
        return (dbus_uint16_t)(-1);

    if (i < 0 || i > 0xffff) {
        PyErr_Format(PyExc_OverflowError, "Value %d out of range for UInt16",
                     (int)i);
        return (dbus_uint16_t)(-1);
    }
    return (dbus_uint16_t)i;
}

static PyObject *
UInt16_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *self = (INTBASE.tp_new)(cls, args, kwargs);
    if (self && dbus_py_uint16_range_check(self) == (dbus_uint16_t)(-1)
        && PyErr_Occurred())
    {
        Py_CLEAR (self);
        return NULL;
    }
    return self;
}

PyTypeObject DBusPyUInt16_Type = {
    PyVarObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type), 0)
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
    DEFERRED_ADDRESS(&INTBASE),             /* tp_base */
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
"Constructor::\n"
"\n"
"    dbus.Int32(value: int[, variant_level: int]) -> Int32\n"
"\n"
"``value`` must be within the allowed range, or `OverflowError` will be\n"
"raised.\n"
"\n"
"``variant_level`` must be non-negative; the default is 0.\n"
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
    long i = PyLong_AsLong(obj);
    if (i == -1 && PyErr_Occurred())
        return -1;

    if (i < INT32_MIN || i > INT32_MAX) {
        PyErr_Format(PyExc_OverflowError, "Value %d out of range for Int32",
                     (int)i);
        return -1;
    }
    return (dbus_int32_t)i;
}

static PyObject *
Int32_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *self = (INTBASE.tp_new)(cls, args, kwargs);
    if (self && dbus_py_int32_range_check(self) == -1 && PyErr_Occurred()) {
        Py_CLEAR(self);
        return NULL;
    }
    return self;
}

PyTypeObject DBusPyInt32_Type = {
    PyVarObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type), 0)
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
    DEFERRED_ADDRESS(&INTBASE),             /* tp_base */
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
"Constructor::\n"
"\n"
"    dbus.UInt32(value: long[, variant_level: int]) -> UInt32\n"
"\n"
"``value`` must be within the allowed range, or `OverflowError` will be\n"
"raised.\n"
"\n"
"``variant_level`` must be non-negative; the default is 0.\n"
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
        Py_CLEAR(long_obj);
        return (dbus_uint32_t)(-1);
    }
    if (i > UINT32_MAX) {
        PyErr_Format(PyExc_OverflowError, "Value %d out of range for UInt32",
                     (int)i);
        Py_CLEAR(long_obj);
        return (dbus_uint32_t)(-1);
    }
    Py_CLEAR(long_obj);
    return i;
}

static PyObject *
UInt32_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *self = (DBusPyLongBase_Type.tp_new)(cls, args, kwargs);
    if (self && dbus_py_uint32_range_check(self) == (dbus_uint32_t)(-1)
        && PyErr_Occurred()) {
        Py_CLEAR(self);
        return NULL;
    }
    return self;
}

PyTypeObject DBusPyUInt32_Type = {
    PyVarObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type), 0)
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
"\n"
"Constructor::\n"
"\n"
"    dbus.Int64(value: long[, variant_level: int]) -> Int64\n"
"\n"
"``value`` must be within the allowed range, or `OverflowError` will be\n"
"raised.\n"
"\n"
"``variant_level`` must be non-negative; the default is 0.\n"
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
        Py_CLEAR(long_obj);
        return -1;
    }
    if (i < INT64_MIN || i > INT64_MAX) {
        PyErr_SetString(PyExc_OverflowError, "Value out of range for Int64");
        Py_CLEAR(long_obj);
        return -1;
    }
    Py_CLEAR(long_obj);
    return i;
}
#endif

static PyObject *
Int64_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
#ifdef DBUS_PYTHON_64_BIT_WORKS
    PyObject *self = (DBusPyLongBase_Type.tp_new)(cls, args, kwargs);
    if (self && dbus_py_int64_range_check(self) == -1 && PyErr_Occurred()) {
        Py_CLEAR(self);
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
    PyVarObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type), 0)
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
"\n"
"Constructor::\n"
"\n"
"    dbus.UInt64(value: long[, variant_level: int]) -> UInt64\n"
"\n"
"``value`` must be within the allowed range, or `OverflowError` will be\n"
"raised.\n"
"\n"
"``variant_level`` must be non-negative; the default is 0.\n"
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
        Py_CLEAR(long_obj);
        return (dbus_uint64_t)(-1);
    }
    if (i > UINT64_MAX) {
        PyErr_SetString(PyExc_OverflowError, "Value out of range for UInt64");
        Py_CLEAR(long_obj);
        return (dbus_uint64_t)(-1);
    }
    Py_CLEAR(long_obj);
    return i;
}

static PyObject *
UInt64_tp_new (PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
#ifdef DBUS_PYTHON_64_BIT_WORKS
    PyObject *self = (DBusPyLongBase_Type.tp_new)(cls, args, kwargs);
    if (self && dbus_py_uint64_range_check(self) == (dbus_uint64_t)(-1)
        && PyErr_Occurred()) {
        Py_CLEAR(self);
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
    PyVarObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type), 0)
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
    DBusPyInt16_Type.tp_base = &INTBASE;
    if (PyType_Ready(&DBusPyInt16_Type) < 0) return 0;
    /* disable the tp_print copied from PyInt_Type, so tp_repr gets called as
    desired */
    DBusPyInt16_Type.tp_print = NULL;

    DBusPyUInt16_Type.tp_base = &INTBASE;
    if (PyType_Ready(&DBusPyUInt16_Type) < 0) return 0;
    DBusPyUInt16_Type.tp_print = NULL;

    DBusPyInt32_Type.tp_base = &INTBASE;
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

    DBusPyBoolean_Type.tp_base = &INTBASE;
    if (PyType_Ready(&DBusPyBoolean_Type) < 0) return 0;
    DBusPyBoolean_Type.tp_print = NULL;

    return 1;
}

dbus_bool_t
dbus_py_insert_int_types(PyObject *this_module)
{
    /* PyModule_AddObject steals a ref */
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
