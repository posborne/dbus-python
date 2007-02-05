/* Simple D-Bus types: ObjectPath and other string types.
 *
 * Copyright (C) 2006 Collabora Ltd.
 *
 * Licensed under the Academic Free License version 2.1
 *
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

/* UTF-8 string representation ====================================== */

PyDoc_STRVAR(UTF8String_tp_doc,
"A string represented using UTF-8 - a subtype of `str`.\n"
"\n"
"All strings on D-Bus are required to be valid Unicode; in the \"wire\n"
"protocol\" they're transported as UTF-8.\n"
"\n"
"By default, when byte arrays are converted from D-Bus to Python, they\n"
"come out as a `dbus.String`, which is a subtype of `unicode`.\n"
"If you prefer to get UTF-8 strings (as instances of this class) or you\n"
"want to avoid the conversion overhead of going from UTF-8 to Python's\n"
"internal Unicode representation, you can pass the ``utf8_strings=True``\n"
"keyword argument to any of these methods:\n"
"\n"
"* any D-Bus method proxy, or ``connect_to_signal``, on the objects returned\n"
"  by `Bus.get_object`\n"
"* any D-Bus method on a `dbus.Interface`\n"
"* `dbus.Interface.connect_to_signal`\n"
"* `Bus.add_signal_receiver`\n"
"\n"
"\n"
"Constructor::\n"
"\n"
"    dbus.UTF8String(value: str or unicode[, variant_level: int]) -> UTF8String\n"
"\n"
"If value is a str object it must be valid UTF-8.\n"
"\n"
"variant_level must be non-negative; the default is 0.\n"
"\n"
":IVariables:\n"
"  `variant_level` : int\n"
"    Indicates how many nested Variant containers this object\n"
"    is contained in: if a message's wire format has a variant containing a\n"
"    variant containing a string, this is represented in Python by a\n"
"    String or UTF8String with variant_level==2.\n"
":Since: 0.80 (in older versions, use dbus.String)\n"
);

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
    return (DBusPyStrBase_Type.tp_new)(cls, args, kwargs);
}

PyTypeObject DBusPyUTF8String_Type = {
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
    DEFERRED_ADDRESS(&DBusPyStrBase_Type),   /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    UTF8String_tp_new,                      /* tp_new */
};

/* Object path ====================================================== */

PyDoc_STRVAR(ObjectPath_tp_doc,
"A D-Bus object path, such as '/com/example/MyApp/Documents/abc'.\n"
"\n"
"ObjectPath is a subtype of str, and object-paths behave like strings.\n"
"\n"
"Constructor::\n"
"\n"
"    dbus.ObjectPath(path: str, variant_level: int) -> ObjectPath\n"
"\n"
"path must be an ASCII string following the syntax of object paths.\n"
"variant_level must be non-negative; the default is 0.\n"
"\n"
":IVariables:\n"
"  `variant_level` : int\n"
"    Indicates how many nested Variant containers this object\n"
"    is contained in: if a message's wire format has a variant containing a\n"
"    variant containing an object path, this is represented in Python by an\n"
"    ObjectPath with variant_level==2.\n"
);

static PyObject *
ObjectPath_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    const char *str = NULL;
    long variantness = 0;
    static char *argnames[] = {"object_path", "variant_level", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|l:__new__", argnames,
                                     &str, &variantness)) return NULL;
    if (!dbus_py_validate_object_path(str)) {
        return NULL;
    }
    return (DBusPyStrBase_Type.tp_new)(cls, args, kwargs);
}

PyTypeObject DBusPyObjectPath_Type = {
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
    DEFERRED_ADDRESS(&DBusPyStrBase_Type),   /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    ObjectPath_tp_new,                      /* tp_new */
};

/* Unicode string representation ==================================== */

PyDoc_STRVAR(String_tp_doc,
"A string represented using Unicode - a subtype of `unicode`.\n"
"\n"
"All strings on D-Bus are required to be valid Unicode; in the \"wire\n"
"protocol\" they're transported as UTF-8.\n"
"\n"
"By default, when strings are converted from D-Bus to Python, they\n"
"come out as this class. If you prefer to get UTF-8 strings (as instances\n"
"of a subtype of `str`) or you want to avoid the conversion overhead of\n"
"going from UTF-8 to Python's internal Unicode representation, see the\n"
"documentation for `dbus.UTF8String`.\n"
"\n"
"Constructor::\n"
"\n"
"    String(value: str or unicode[, variant_level: int]) -> String\n"
"\n"
"variant_level must be non-negative; the default is 0.\n"
"\n"
":IVariables:\n"
"  `variant_level` : int\n"
"    Indicates how many nested Variant containers this object\n"
"    is contained in: if a message's wire format has a variant containing a\n"
"    variant containing a string, this is represented in Python by a\n"
"    String or UTF8String with variant_level==2.\n"
);

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
    if (!PyArg_ParseTupleAndKeywords(dbus_py_empty_tuple, kwargs,
                                     "|O!:__new__", argnames,
                                     &PyInt_Type, &variantness)) return NULL;
    if (variantness) {
        if (PyInt_AS_LONG(variantness) < 0) {
            PyErr_SetString(PyExc_ValueError,
                            "variant_level must be non-negative");
            return NULL;
        }
        /* own a reference */
        Py_INCREF(variantness);
    }
    else {
        variantness = PyInt_FromLong(0);
        if (!variantness) return NULL;
    }

    self = (PyUnicode_Type.tp_new)(cls, args, NULL);
    if (self) {
        if (PyObject_GenericSetAttr(self, dbus_py_variant_level_const,
                                    variantness) < 0) {
            Py_DECREF(variantness);
            Py_DECREF(self);
            return NULL;
        }
    }
    Py_DECREF(variantness);
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

PyTypeObject DBusPyString_Type = {
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
    dbus_py_immutable_setattro,                /* tp_setattro */
    0,                                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    String_tp_doc,                          /* tp_doc */
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

dbus_bool_t
dbus_py_init_string_types(void)
{
    DBusPyString_Type.tp_basicsize = PyUnicode_Type.tp_basicsize
                                     + 2*sizeof(PyObject *) - 1;
    DBusPyString_Type.tp_basicsize /= sizeof(PyObject *);
    DBusPyString_Type.tp_basicsize *= sizeof(PyObject *);
    DBusPyString_Type.tp_base = &PyUnicode_Type;
    if (PyType_Ready(&DBusPyString_Type) < 0) return 0;
    DBusPyString_Type.tp_print = NULL;

    DBusPyUTF8String_Type.tp_basicsize = PyUnicode_Type.tp_basicsize
                                  + 2*sizeof(PyObject *) - 1;
    DBusPyUTF8String_Type.tp_basicsize /= sizeof(PyObject *);
    DBusPyUTF8String_Type.tp_basicsize *= sizeof(PyObject *);
    DBusPyUTF8String_Type.tp_base = &DBusPyStrBase_Type;
    if (PyType_Ready(&DBusPyUTF8String_Type) < 0) return 0;
    DBusPyUTF8String_Type.tp_print = NULL;

    DBusPyObjectPath_Type.tp_base = &DBusPyStrBase_Type;
    if (PyType_Ready(&DBusPyObjectPath_Type) < 0) return 0;
    DBusPyObjectPath_Type.tp_print = NULL;

    DBusPyBoolean_Type.tp_base = &DBusPyIntBase_Type;
    if (PyType_Ready(&DBusPyBoolean_Type) < 0) return 0;
    DBusPyBoolean_Type.tp_print = NULL;

    return 1;
}

dbus_bool_t
dbus_py_insert_string_types(PyObject *this_module)
{
    Py_INCREF(&DBusPyObjectPath_Type);
    Py_INCREF(&DBusPyUTF8String_Type);
    Py_INCREF(&DBusPyString_Type);
    if (PyModule_AddObject(this_module, "ObjectPath",
                           (PyObject *)&DBusPyObjectPath_Type) < 0) return 0;
    if (PyModule_AddObject(this_module, "UTF8String",
                           (PyObject *)&DBusPyUTF8String_Type) < 0) return 0;
    if (PyModule_AddObject(this_module, "String",
                           (PyObject *)&DBusPyString_Type) < 0) return 0;

    return 1;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
