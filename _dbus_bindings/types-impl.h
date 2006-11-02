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

/* IntNN, UIntNN ==================================================== */

static PyTypeObject Int16Type, Int32Type, Int64Type;
static PyTypeObject UInt16Type, UInt32Type, UInt64Type;
static PyTypeObject ObjectPathType, BooleanType;
static PyTypeObject DoubleType;
#ifdef WITH_DBUS_FLOAT32
static PyTypeObject FloatType;
#endif

#define DEFINE_CHECK(type) \
static inline int type##_Check (PyObject *o) \
{ \
    return (o->ob_type == &type##Type) \
            || PyObject_IsInstance(o, (PyObject *)&type##Type); \
}
DEFINE_CHECK(Int16)
DEFINE_CHECK(Int32)
DEFINE_CHECK(Int64)
DEFINE_CHECK(UInt16)
DEFINE_CHECK(UInt32)
DEFINE_CHECK(UInt64)
DEFINE_CHECK(ObjectPath)
DEFINE_CHECK(Double)
DEFINE_CHECK(Boolean)
#ifdef WITH_DBUS_FLOAT32
DEFINE_CHECK(Float)
#endif
#undef DEFINE_CHECK

/* The Int and UInt types are nice and simple.
FIXME: use Python int as base for UInt32 and Int64 on 64-bit platforms?
*/

PyDoc_STRVAR(Boolean_tp_doc,
"A boolean (a subtype of int).");

PyDoc_STRVAR(Int16_tp_doc,
"A signed 16-bit integer (a subtype of int).");

PyDoc_STRVAR(UInt16_tp_doc,
"An unsigned 16-bit integer (a subtype of int).");

PyDoc_STRVAR(Int32_tp_doc,
"A signed 32-bit integer (a subtype of int).");

PyDoc_STRVAR(UInt32_tp_doc,
"An unsigned 32-bit integer (a subtype of long).");

PyDoc_STRVAR(Int64_tp_doc,
"A signed 64-bit integer (a subtype of long).");

PyDoc_STRVAR(UInt64_tp_doc,
"An unsigned 64-bit integer (a subtype of long).");

PyDoc_STRVAR(Double_tp_doc,
"A double-precision floating point number (a subtype of float).");

#ifdef WITH_DBUS_FLOAT32
PyDoc_STRVAR(Float_tp_doc,
"A single-precision floating point number (a subtype of float).");
#endif

static PyObject *
Boolean_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *tuple, *self;
    int i;
    static char *argnames[] = {"value", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i:__new__", argnames,
                                     &i)) return NULL;
    tuple = Py_BuildValue("(i)", i ? 1 : 0);
    if (!tuple) return NULL;
    self = (PyInt_Type.tp_new)(cls, tuple, NULL);
    Py_DECREF(tuple);
    return self;
}

static PyObject *
Boolean_from_long(long b)
{
    PyObject *self;
    PyObject *tuple = Py_BuildValue("(i)", b ? 1 : 0);
    if (!tuple) return NULL;
    self = (PyInt_Type.tp_new)(&BooleanType, tuple, NULL);
    Py_DECREF(tuple);
    return self;
}

static PyObject *
Boolean_tp_repr (PyObject *self)
{
    return PyString_FromFormat("%s(%s)", self->ob_type->tp_name,
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
    DEFERRED_ADDRESS(&PyInt_Type),          /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    Boolean_tp_new,                         /* tp_new */
};

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
    PyObject *self = (PyInt_Type.tp_new)(cls, args, kwargs);
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
    int_subclass_tp_repr,                   /* tp_repr */
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
    DEFERRED_ADDRESS(&PyInt_Type),          /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    Int16_tp_new,                           /* tp_new */
};

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
    PyObject *self = (PyInt_Type.tp_new)(cls, args, kwargs);
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
    int_subclass_tp_repr,                   /* tp_repr */
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
    DEFERRED_ADDRESS(&PyInt_Type),          /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    UInt16_tp_new,                          /* tp_new */
};

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
    PyObject *self = (PyInt_Type.tp_new)(cls, args, kwargs);
    if (int32_range_check(self) == -1 && PyErr_Occurred()) return NULL;
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
    int_subclass_tp_repr,                   /* tp_repr */
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
    DEFERRED_ADDRESS(&PyInt_Type),          /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    Int32_tp_new,                           /* tp_new */
};

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
    if (i < 0 || i > UINT32_MAX) {
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
    PyObject *self = (PyLong_Type.tp_new)(cls, args, kwargs);
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
    long_subclass_tp_repr,                  /* tp_repr */
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
    DEFERRED_ADDRESS(&PyLong_Type),         /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    UInt32_tp_new,                          /* tp_new */
};

#if defined(DBUS_HAVE_INT64) && defined(HAVE_LONG_LONG)

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
    PyObject *self = (PyLong_Type.tp_new)(cls, args, kwargs);
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
    long_subclass_tp_repr,                  /* tp_repr */
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
    DEFERRED_ADDRESS(&PyLong_Type),         /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    Int64_tp_new,                           /* tp_new */
};

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
    if (i < 0 || i > UINT64_MAX) {
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
    PyObject *self = (PyLong_Type.tp_new)(cls, args, kwargs);
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
    long_subclass_tp_repr,                  /* tp_repr */
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
    DEFERRED_ADDRESS(&PyLong_Type),         /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    UInt64_tp_new,                          /* tp_new */
};

#endif /* defined(DBUS_HAVE_INT64) && defined(HAVE_LONG_LONG) */

/* Float types ====================================================== */

static PyObject *
Double_from_double(double d)
{
    PyObject *self;
    PyObject *tuple = Py_BuildValue("(f)", d);
    if (!tuple) return NULL;
    self = (PyFloat_Type.tp_new)(&DoubleType, tuple, NULL);
    Py_DECREF(tuple);
    return self;
}

static PyTypeObject DoubleType = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
    "dbus.Double",
    0,
    0,
    0,                                      /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_compare */
    float_subclass_tp_repr,                 /* tp_repr */
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
    Double_tp_doc,                          /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    0,                                      /* tp_iter */
    0,                                      /* tp_iternext */
    0,                                      /* tp_methods */
    0,                                      /* tp_members */
    0,                                      /* tp_getset */
    DEFERRED_ADDRESS(&PyFloat_Type),        /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    0,                                      /* tp_new */
};

#ifdef WITH_DBUS_FLOAT32
static PyObject *
Float_from_double(double d)
{
    PyObject *self;
    PyObject *tuple = Py_BuildValue("(f)", d);
    if (!tuple) return NULL;
    self = (PyFloat_Type.tp_new)(&DoubleType, tuple, NULL);
    Py_DECREF(tuple);
    return self;
}

static PyTypeObject FloatType = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
    "dbus.Float",
    0,
    0,
    0,                                      /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_compare */
    float_subclass_tp_repr,                 /* tp_repr */
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
    Float_tp_doc,                           /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    0,                                      /* tp_iter */
    0,                                      /* tp_iternext */
    0,                                      /* tp_methods */
    0,                                      /* tp_members */
    0,                                      /* tp_getset */
    DEFERRED_ADDRESS(&PyFloat_Type),        /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    0,                                      /* tp_new */
};
#endif /* defined(WITH_DBUS_FLOAT32) */

/* Object path ====================================================== */

PyDoc_STRVAR(ObjectPath_tp_doc,
"ObjectPath(path: str)\n\n"
"A D-Bus object path, such as '/com/example/MyApp/Documents/abc'.\n"
"\n"
"ObjectPath is a subtype of str, and object-paths behave like strings.\n");

static PyObject *
ObjectPath_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *tuple, *self;
    const char *str = NULL;
    static char *argnames[] = {"object_path", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s:__new__", argnames,
                                     &str)) return NULL;
    if (!_validate_object_path(str)) {
        return NULL;
    }
    tuple = Py_BuildValue("(s)", str);
    if (!tuple) return NULL;
    self = PyString_Type.tp_new(cls, tuple, NULL);
    Py_DECREF(tuple);
    return self;
}

/* Slight subtlety: In conn-methods-impl.h we assume that object paths
 * are nicely simple: in particular, that they can never reference
 * other objects, and that they share strings' concept of equality
 * and hash value. If you break these assumptions, alter
 * _register_object_path and _unregister_object_path to coerce
 * ObjectPaths into ordinary Python strings. */
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
    str_subclass_tp_repr,                   /* tp_repr */
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
    DEFERRED_ADDRESS(&PyString_Type),       /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    ObjectPath_tp_new,                      /* tp_new */
};

static inline int
init_types(void)
{
    Int16Type.tp_base = &PyInt_Type;
    if (PyType_Ready(&Int16Type) < 0) return 0;
    /* disable the tp_print copied from PyInt_Type, so tp_repr gets called as
    desired */
    Int16Type.tp_print = NULL;

    UInt16Type.tp_base = &PyInt_Type;
    if (PyType_Ready(&UInt16Type) < 0) return 0;
    UInt16Type.tp_print = NULL;

    Int32Type.tp_base = &PyInt_Type;
    if (PyType_Ready(&Int32Type) < 0) return 0;
    Int32Type.tp_print = NULL;

    UInt32Type.tp_base = &PyLong_Type;
    if (PyType_Ready(&UInt32Type) < 0) return 0;
    UInt32Type.tp_print = NULL;

    Int64Type.tp_base = &PyLong_Type;
    if (PyType_Ready(&Int64Type) < 0) return 0;
    Int64Type.tp_print = NULL;

    UInt64Type.tp_base = &PyLong_Type;
    if (PyType_Ready(&UInt64Type) < 0) return 0;
    UInt64Type.tp_print = NULL;

    ObjectPathType.tp_base = &PyString_Type;
    if (PyType_Ready(&ObjectPathType) < 0) return 0;
    ObjectPathType.tp_print = NULL;

    BooleanType.tp_base = &PyInt_Type;
    if (PyType_Ready(&BooleanType) < 0) return 0;
    BooleanType.tp_print = NULL;

    DoubleType.tp_base = &PyFloat_Type;
    if (PyType_Ready(&DoubleType) < 0) return 0;
    DoubleType.tp_print = NULL;

#ifdef WITH_DBUS_FLOAT32
    FloatType.tp_base = &PyFloat_Type;
    if (PyType_Ready(&FloatType) < 0) return 0;
    FloatType.tp_print = NULL;
#endif

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
#ifdef WITH_DBUS_FLOAT32
    Py_INCREF(&FloatType);
#endif
    Py_INCREF(&DoubleType);
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
#ifdef WITH_DBUS_FLOAT32
    if (PyModule_AddObject(this_module, "Float",
                           (PyObject *)&FloatType) < 0) return 0;
#endif
    if (PyModule_AddObject(this_module, "Double",
                           (PyObject *)&DoubleType) < 0) return 0;

    Py_INCREF(&ObjectPathType);
    if (PyModule_AddObject(this_module, "ObjectPath",
                           (PyObject *)&ObjectPathType) < 0) return 0;

    return 1;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
