/* Simple D-Bus types: ObjectPath and other string types.
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

#include "types-internal.h"
#include <structmember.h>

static PyTypeObject DBusPyString_Type;

int
DBusPyString_Check(PyObject *o)
{
    return PyObject_TypeCheck(o, &DBusPyString_Type);
}

PyObject *
DBusPyString_New(const char *utf8, long variant_level)
{
    PyObject *args = NULL;
    PyObject *kwargs = NULL;
    PyObject *ret = NULL;

    if (variant_level != 0) {
        kwargs = DBusPy_BuildConstructorKeywordArgs(variant_level, NULL);
        if (!kwargs)
            goto finally;
    }

    args = Py_BuildValue("(s)", utf8);
    if (!args)
        goto finally;

    ret = PyObject_Call((PyObject *)&DBusPyString_Type, args, kwargs);

finally:
    Py_XDECREF(args);
    Py_XDECREF(kwargs);
    return ret;
}

static PyTypeObject DBusPyObjectPath_Type;

int
DBusPyObjectPath_Check(PyObject *o)
{
    return PyObject_TypeCheck(o, &DBusPyObjectPath_Type);
}

PyObject *
DBusPyObjectPath_New(const char *utf8, long variant_level)
{
    PyObject *args = NULL;
    PyObject *kwargs = NULL;
    PyObject *ret = NULL;

    if (variant_level != 0) {
        kwargs = DBusPy_BuildConstructorKeywordArgs(variant_level, NULL);
        if (!kwargs)
            goto finally;
    }

    args = Py_BuildValue("(s)", utf8);
    if (!args)
        goto finally;

    ret = PyObject_Call((PyObject *)&DBusPyObjectPath_Type, args, kwargs);

finally:
    Py_XDECREF(args);
    Py_XDECREF(kwargs);
    return ret;
}

static PyTypeObject DBusPyUTF8String_Type;

int
DBusPyUTF8String_Check(PyObject *o)
{
    return PyObject_TypeCheck(o, &DBusPyUTF8String_Type);
}

PyObject *
DBusPyUTF8String_New(const char *utf8, long variant_level)
{
    PyObject *args = NULL;
    PyObject *kwargs = NULL;
    PyObject *ret = NULL;

    if (variant_level != 0) {
        kwargs = DBusPy_BuildConstructorKeywordArgs(variant_level, NULL);
        if (!kwargs)
            goto finally;
    }

    args = Py_BuildValue("(s)", utf8);
    if (!args)
        goto finally;

    ret = PyObject_Call((PyObject *)&DBusPyUTF8String_Type, args, kwargs);

finally:
    Py_XDECREF(args);
    Py_XDECREF(kwargs);
    return ret;
}

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

static PyTypeObject DBusPyUTF8String_Type = {
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

static PyTypeObject DBusPyObjectPath_Type = {
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

static PyMemberDef String_tp_members[] = {
    {"variant_level", T_LONG, offsetof(DBusPyString, variant_level),
     READONLY,
     "The number of nested variants wrapping the real data. "
     "0 if not in a variant"},
    {NULL},
};

static PyObject *
unicode_or_utf8_to_unicode(PyObject *s)
{
    if (PyUnicode_Check(s)) {
        Py_INCREF(s);
        return s;
    }
    if (!PyString_Check(s)) {
        PyErr_SetString(PyExc_TypeError,
                        "A unicode object or UTF-8 string was expected");
        return NULL;
    }
    return PyUnicode_DecodeUTF8(PyString_AS_STRING(s),
                                strlen(PyString_AS_STRING(s)), NULL);
}

static PyObject *
String_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *self;
    PyObject *new_args;
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

    new_args = Py_BuildValue("(O&)", unicode_or_utf8_to_unicode,
                             PyTuple_GET_ITEM(args, 0));
    if (!new_args)
        return NULL;

    self = (PyUnicode_Type.tp_new)(cls, new_args, NULL);
    Py_DECREF(new_args);
    if (self) {
        ((DBusPyString *)self)->variant_level = variantness;
    }
    return self;
}

static PyObject *
String_tp_repr(PyObject *self)
{
    PyObject *parent_repr = (PyUnicode_Type.tp_repr)(self);
    PyObject *my_repr;

    if (!parent_repr) {
        return NULL;
    }
    if (((DBusPyString *)self)->variant_level > 0) {
        my_repr = PyString_FromFormat("%s(%s, variant_level=%ld)",
                                      self->ob_type->tp_name,
                                      PyString_AS_STRING(parent_repr),
                                      ((DBusPyString *)self)->variant_level);
    }
    else {
        my_repr = PyString_FromFormat("%s(%s)", self->ob_type->tp_name,
                                      PyString_AS_STRING(parent_repr));
    }
    /* whether my_repr is NULL or not: */
    Py_DECREF(parent_repr);
    return my_repr;
}

static PyTypeObject DBusPyString_Type = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
    "dbus.String",
    sizeof(DBusPyString),
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
    String_tp_members,                      /* tp_members */
    0,                                      /* tp_getset */
    DEFERRED_ADDRESS(&PyUnicode_Type),      /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    String_tp_new,                          /* tp_new */
};

dbus_bool_t
dbus_py_init_string_types(void)
{
    /* don't need to do strange contortions for unicode, since it's not a
     * "variable-size" object (it has a pointer to its data instead)
     */
    if (PyUnicode_Type.tp_itemsize != 0) {
        fprintf(stderr, "dbus-python is not compatible with this version of "
                "Python (unicode objects are assumed to be fixed-size)");
        return 0;
    }
    DBusPyString_Type.tp_base = &PyUnicode_Type;
    if (PyType_Ready(&DBusPyString_Type) < 0) return 0;
    DBusPyString_Type.tp_print = NULL;

    DBusPyUTF8String_Type.tp_base = &DBusPyStrBase_Type;
    if (PyType_Ready(&DBusPyUTF8String_Type) < 0) return 0;
    DBusPyUTF8String_Type.tp_print = NULL;

    DBusPyObjectPath_Type.tp_base = &DBusPyStrBase_Type;
    if (PyType_Ready(&DBusPyObjectPath_Type) < 0) return 0;
    DBusPyObjectPath_Type.tp_print = NULL;

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
