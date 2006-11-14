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

#include <stdint.h>

/* Specific types =================================================== */

/* Boolean, a subclass of DBusPythonInt ============================= */

static PyTypeObject BooleanType;

DEFINE_CHECK(Boolean)

PyDoc_STRVAR(Boolean_tp_doc,
"Boolean(value: int[, variant_level: int]) -> Boolean\n\n"
"A boolean (a subtype of int).\n\n"
"value is converted to 0 or 1.\n"
"variant_level must be non-negative; the default is 0.\n"
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
    self = (DBusPythonIntType.tp_new)(cls, tuple, kwargs);
    Py_DECREF(tuple);
    return self;
}

static PyObject *
Boolean_tp_repr (PyObject *self)
{
    long variant_level = ((DBusPythonInt *)self)->variant_level;
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

static PyTypeObject BooleanType = {
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
    DEFERRED_ADDRESS(&DBusPythonInt_Type),  /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    Boolean_tp_new,                         /* tp_new */
};

/* Int16 ============================================================ */

static PyTypeObject Int16Type;

DEFINE_CHECK(Int16)

PyDoc_STRVAR(Int16_tp_doc,
"Int16(value: any integer[, variant_level: int]) -> Int16\n\n"
"A signed 16-bit integer (a subtype of int).\n\n"
"value must be in the range -0x8000 to +0x7fff.\n"
"variant_level must be non-negative; the default is 0.\n"
);

static dbus_int16_t
int16_range_check(PyObject *obj)
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
    PyObject *self = (DBusPythonIntType.tp_new)(cls, args, kwargs);
    if (self && int16_range_check(self) == -1 && PyErr_Occurred()) {
        Py_DECREF(self);
        return NULL;
    }
    return self;
}

static PyTypeObject Int16Type = {
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
    DEFERRED_ADDRESS(&DBusPythonIntType),   /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    Int16_tp_new,                           /* tp_new */
};

/* UInt16 =========================================================== */

static PyTypeObject UInt16Type;

DEFINE_CHECK(UInt16)

PyDoc_STRVAR(UInt16_tp_doc,
"UInt16(value: any integer[, variant_level: int]) -> UInt16\n\n"
"An unsigned 16-bit integer (a subtype of int).\n\n"
"value must be in the range 0 to 0xffff.\n"
"variant_level must be non-negative; the default is 0.\n"
);

static dbus_uint16_t
uint16_range_check(PyObject *obj)
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
    PyObject *self = (DBusPythonIntType.tp_new)(cls, args, kwargs);
    if (self && uint16_range_check(self) == (dbus_uint16_t)(-1)
        && PyErr_Occurred()) {
        Py_DECREF (self);
        return NULL;
    }
    return self;
}

static PyTypeObject UInt16Type = {
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
    DEFERRED_ADDRESS(&DBusPythonIntType),   /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    UInt16_tp_new,                          /* tp_new */
};

/* Int32 ============================================================ */

static PyTypeObject Int32Type;

DEFINE_CHECK(Int32)

PyDoc_STRVAR(Int32_tp_doc,
"Int32(value: any integer[, variant_level: int]) -> Int32\n\n"
"A signed 32-bit integer (a subtype of int).\n\n"
"value must be in the range -0x80000000 to +0x7fffffff.\n"
"variant_level must be non-negative; the default is 0.\n"
);

static dbus_int32_t
int32_range_check(PyObject *obj)
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
    PyObject *self = (DBusPythonIntType.tp_new)(cls, args, kwargs);
    if (self && int32_range_check(self) == -1 && PyErr_Occurred()) {
        Py_DECREF(self);
        return NULL;
    }
    return self;
}

static PyTypeObject Int32Type = {
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
    DEFERRED_ADDRESS(&DBusPythonIntType),   /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    Int32_tp_new,                           /* tp_new */
};

/* UInt32 =========================================================== */

static PyTypeObject UInt32Type;

DEFINE_CHECK(UInt32)

PyDoc_STRVAR(UInt32_tp_doc,
"UInt32(value: any integer[, variant_level: int]) -> UInt32\n\n"
"An unsigned 32-bit integer (a subtype of long).\n\n"
"value must be in the range 0 to 0xffffffff.\n"
"variant_level must be non-negative; the default is 0.\n"
);

static dbus_uint32_t
uint32_range_check(PyObject *obj)
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
    PyObject *self = (DBusPythonLongType.tp_new)(cls, args, kwargs);
    if (self && uint32_range_check(self) == (dbus_uint32_t)(-1)
        && PyErr_Occurred()) {
        Py_DECREF(self);
        return NULL;
    }
    return self;
}

static PyTypeObject UInt32Type = {
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
    DEFERRED_ADDRESS(&DBusPythonLongType),  /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    UInt32_tp_new,                          /* tp_new */
};

#if defined(DBUS_HAVE_INT64) && defined(HAVE_LONG_LONG)

/* Int64 =========================================================== */

static PyTypeObject Int64Type;

DEFINE_CHECK(Int64)

PyDoc_STRVAR(Int64_tp_doc,
"Int64(value: any integer[, variant_level: int]) -> Int64\n\n"
"A signed 64-bit integer (a subtype of long).\n\n"
"value must be in the range -0x8000000000000000 to 0x7fffffffffffffff.\n"
"variant_level must be non-negative; the default is 0.\n"
);

static dbus_int64_t
int64_range_check(PyObject *obj)
{
    long long i;
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

static PyObject *
Int64_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *self = (DBusPythonLongType.tp_new)(cls, args, kwargs);
    if (self && int64_range_check(self) == -1 && PyErr_Occurred()) {
        Py_DECREF(self);
        return NULL;
    }
    return self;
}

static PyTypeObject Int64Type = {
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
    DEFERRED_ADDRESS(&DBusPythonLongType),  /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    Int64_tp_new,                           /* tp_new */
};

/* UInt64 =========================================================== */

static PyTypeObject UInt64Type;

DEFINE_CHECK(UInt64)

PyDoc_STRVAR(UInt64_tp_doc,
"UInt64(value: any integer[, variant_level: int]) -> UInt64\n\n"
"An unsigned 64-bit integer (a subtype of long).\n\n"
"value must be in the range 0 to 0xffffffffffffffff.\n"
"variant_level must be non-negative; the default is 0.\n"
);

static dbus_uint64_t
uint64_range_check(PyObject *obj)
{
    unsigned long long i;
    PyObject *long_obj = PyNumber_Long(obj);

    if (!long_obj) return (dbus_uint64_t)(-1);
    i = PyLong_AsUnsignedLongLong(long_obj);
    if (i == (unsigned long long)(-1) && PyErr_Occurred()) {
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
    PyObject *self = (DBusPythonLongType.tp_new)(cls, args, kwargs);
    if (self && uint64_range_check(self) == (dbus_uint64_t)(-1)
        && PyErr_Occurred()) {
        Py_DECREF(self);
        return NULL;
    }
    return self;
}

static PyTypeObject UInt64Type = {
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
    DEFERRED_ADDRESS(&DBusPythonLongType),  /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    UInt64_tp_new,                          /* tp_new */
};

#endif /* defined(DBUS_HAVE_INT64) && defined(HAVE_LONG_LONG) */

/* UTF-8 string representation ====================================== */

static PyTypeObject UTF8StringType;

DEFINE_CHECK(UTF8String)

PyDoc_STRVAR(UTF8String_tp_doc,
"UTF8String(value: str[, variant_level: int])\n\n"
"A string represented using UTF-8.\n"
"\n"
"The variant_level must be non-negative; the default is 0.");

static PyObject *
UTF8String_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    const char *str = NULL;
    long variantness = 0;
    static char *argnames[] = {"value", "variant_level", NULL};
    PyObject *unicode;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|l:__new__", argnames,
                                     &str, &variantness)) return NULL;
    unicode = PyUnicode_DecodeUTF8(str, strlen(str), NULL);
    if (!unicode) return NULL;
    Py_DECREF(unicode);
    return (DBusPythonStringType.tp_new)(cls, args, kwargs);
}

static PyTypeObject UTF8StringType = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
    "dbus.UTF8String",
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
    UTF8String_tp_doc,                      /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    0,                                      /* tp_iter */
    0,                                      /* tp_iternext */
    0,                                      /* tp_methods */
    0,                                      /* tp_members */
    0,                                      /* tp_getset */
    DEFERRED_ADDRESS(&DBusPythonStringType),   /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    UTF8String_tp_new,                      /* tp_new */
};

/* Object path ====================================================== */

static PyTypeObject ObjectPathType;

DEFINE_CHECK(ObjectPath)

PyDoc_STRVAR(ObjectPath_tp_doc,
"ObjectPath(path: str[, variant_level: int])\n\n"
"A D-Bus object path, such as '/com/example/MyApp/Documents/abc'.\n"
"\n"
"ObjectPath is a subtype of str, and object-paths behave like strings.\n"
"\n"
"The variant_level must be non-negative; the default is 0.");

static PyObject *
ObjectPath_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    const char *str = NULL;
    long variantness = 0;
    static char *argnames[] = {"object_path", "variant_level", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|l:__new__", argnames,
                                     &str, &variantness)) return NULL;
    if (!_validate_object_path(str)) {
        return NULL;
    }
    return (DBusPythonStringType.tp_new)(cls, args, kwargs);
}

static PyTypeObject ObjectPathType = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
    "dbus.ObjectPath",
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
    ObjectPath_tp_doc,                      /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    0,                                      /* tp_iter */
    0,                                      /* tp_iternext */
    0,                                      /* tp_methods */
    0,                                      /* tp_members */
    0,                                      /* tp_getset */
    DEFERRED_ADDRESS(&DBusPythonStringType),   /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    ObjectPath_tp_new,                      /* tp_new */
};

/* Unicode string representation ==================================== */

static PyTypeObject StringType;
DEFINE_CHECK(String)

static PyObject *
String_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *self;
    PyObject *variantness = NULL;
    static char *argnames[] = {"variant_level", NULL};

    if (PyTuple_Size(args) > 1) {
        PyErr_SetString(PyExc_TypeError,
                        "__new__ takes at most one positional parameter");
        return NULL;
    }
    if (!PyArg_ParseTupleAndKeywords(empty_tuple, kwargs,
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

    self = (PyUnicode_Type.tp_new)(cls, args, NULL);
    if (self) {
        PyObject_GenericSetAttr(self, variant_level_const, variantness);
    }
    return self;
}

static PyObject *
String_tp_repr(PyObject *self)
{
    PyObject *parent_repr = (PyUnicode_Type.tp_repr)(self);
    PyObject *vl_obj;
    PyObject *my_repr;
    long variant_level;

    if (!parent_repr) return NULL;
    vl_obj = PyObject_GetAttr(self, variant_level_const);
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

static PyTypeObject StringType = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
    "dbus.String",
    INT_MAX, /* placeholder */
    0,
    0,                                      /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_compare */
    String_tp_repr,                         /* tp_repr */
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
    DEFERRED_ADDRESS(&PyUnicode_Type),      /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    -sizeof(void *),                        /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    String_tp_new,                          /* tp_new */
};

static inline int
init_types(void)
{
    Int16Type.tp_base = &DBusPythonIntType;
    if (PyType_Ready(&Int16Type) < 0) return 0;
    /* disable the tp_print copied from PyInt_Type, so tp_repr gets called as
    desired */
    Int16Type.tp_print = NULL;

    UInt16Type.tp_base = &DBusPythonIntType;
    if (PyType_Ready(&UInt16Type) < 0) return 0;
    UInt16Type.tp_print = NULL;

    Int32Type.tp_base = &DBusPythonIntType;
    if (PyType_Ready(&Int32Type) < 0) return 0;
    Int32Type.tp_print = NULL;

    UInt32Type.tp_base = &DBusPythonLongType;
    if (PyType_Ready(&UInt32Type) < 0) return 0;
    UInt32Type.tp_print = NULL;

#if defined(DBUS_HAVE_INT64) && defined(HAVE_LONG_LONG)
    Int64Type.tp_base = &DBusPythonLongType;
    if (PyType_Ready(&Int64Type) < 0) return 0;
    Int64Type.tp_print = NULL;

    UInt64Type.tp_base = &DBusPythonLongType;
    if (PyType_Ready(&UInt64Type) < 0) return 0;
    UInt64Type.tp_print = NULL;
#endif

    StringType.tp_basicsize = PyUnicode_Type.tp_basicsize
                              + 2*sizeof(PyObject *) - 1;
    StringType.tp_basicsize /= sizeof(PyObject *);
    StringType.tp_basicsize *= sizeof(PyObject *);
    StringType.tp_base = &PyUnicode_Type;
    if (PyType_Ready(&StringType) < 0) return 0;
    StringType.tp_print = NULL;

    UTF8StringType.tp_basicsize = PyUnicode_Type.tp_basicsize
                                  + 2*sizeof(PyObject *) - 1;
    UTF8StringType.tp_basicsize /= sizeof(PyObject *);
    UTF8StringType.tp_basicsize *= sizeof(PyObject *);
    UTF8StringType.tp_base = &DBusPythonStringType;
    if (PyType_Ready(&UTF8StringType) < 0) return 0;
    UTF8StringType.tp_print = NULL;

    ObjectPathType.tp_base = &DBusPythonStringType;
    if (PyType_Ready(&ObjectPathType) < 0) return 0;
    ObjectPathType.tp_print = NULL;

    BooleanType.tp_base = &DBusPythonIntType;
    if (PyType_Ready(&BooleanType) < 0) return 0;
    BooleanType.tp_print = NULL;

    return 1;
}

static inline int
insert_types(PyObject *this_module)
{
    Py_INCREF(&Int16Type);
    Py_INCREF(&UInt16Type);
    Py_INCREF(&Int32Type);
    Py_INCREF(&UInt32Type);
    Py_INCREF(&Int64Type);
    Py_INCREF(&UInt64Type);
    Py_INCREF(&BooleanType);
    if (PyModule_AddObject(this_module, "Int16",
                           (PyObject *)&Int16Type) < 0) return 0;
    if (PyModule_AddObject(this_module, "UInt16",
                           (PyObject *)&UInt16Type) < 0) return 0;
    if (PyModule_AddObject(this_module, "Int32",
                           (PyObject *)&Int32Type) < 0) return 0;
    if (PyModule_AddObject(this_module, "UInt32",
                           (PyObject *)&UInt32Type) < 0) return 0;
    if (PyModule_AddObject(this_module, "Int64",
                           (PyObject *)&Int64Type) < 0) return 0;
    if (PyModule_AddObject(this_module, "UInt64",
                           (PyObject *)&UInt64Type) < 0) return 0;
    if (PyModule_AddObject(this_module, "Boolean",
                           (PyObject *)&BooleanType) < 0) return 0;

    Py_INCREF(&ObjectPathType);
    Py_INCREF(&UTF8StringType);
    Py_INCREF(&StringType);
    if (PyModule_AddObject(this_module, "ObjectPath",
                           (PyObject *)&ObjectPathType) < 0) return 0;
    if (PyModule_AddObject(this_module, "UTF8String",
                           (PyObject *)&UTF8StringType) < 0) return 0;
    if (PyModule_AddObject(this_module, "String",
                           (PyObject *)&StringType) < 0) return 0;

    return 1;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
