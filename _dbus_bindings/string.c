/* Simple D-Bus types: ObjectPath and other string types.
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

#include "types-internal.h"
#include <structmember.h>

#ifndef PY3
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
    Py_CLEAR(unicode);
    return (DBusPyStrBase_Type.tp_new)(cls, args, kwargs);
}

PyTypeObject DBusPyUTF8String_Type = {
    PyVarObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type), 0)
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
#endif  /* !PY3 */

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
    PyVarObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type), 0)
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
String_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
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
    self = (PyUnicode_Type.tp_new)(cls, args, NULL);
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
        my_repr = PyUnicode_FromFormat("%s(%V, variant_level=%ld)",
                                       Py_TYPE(self)->tp_name,
                                       REPRV(parent_repr),
                                       ((DBusPyString *)self)->variant_level);
    }
    else {
        my_repr = PyUnicode_FromFormat("%s(%V)", Py_TYPE(self)->tp_name,
                                       REPRV(parent_repr));
    }
    /* whether my_repr is NULL or not: */
    Py_CLEAR(parent_repr);
    return my_repr;
}

PyTypeObject DBusPyString_Type = {
    PyVarObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type), 0)
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

#ifndef PY3
    DBusPyUTF8String_Type.tp_base = &DBusPyStrBase_Type;
    if (PyType_Ready(&DBusPyUTF8String_Type) < 0) return 0;
    DBusPyUTF8String_Type.tp_print = NULL;
#endif

    DBusPyObjectPath_Type.tp_base = &DBusPyStrBase_Type;
    if (PyType_Ready(&DBusPyObjectPath_Type) < 0) return 0;
    DBusPyObjectPath_Type.tp_print = NULL;

    return 1;
}

dbus_bool_t
dbus_py_insert_string_types(PyObject *this_module)
{
    /* PyModule_AddObject steals a ref */
    Py_INCREF(&DBusPyObjectPath_Type);
    Py_INCREF(&DBusPyString_Type);
    if (PyModule_AddObject(this_module, "ObjectPath",
                           (PyObject *)&DBusPyObjectPath_Type) < 0) return 0;
    if (PyModule_AddObject(this_module, "String",
                           (PyObject *)&DBusPyString_Type) < 0) return 0;

#ifndef PY3
    Py_INCREF(&DBusPyUTF8String_Type);
    if (PyModule_AddObject(this_module, "UTF8String",
                           (PyObject *)&DBusPyUTF8String_Type) < 0) return 0;
#endif

    return 1;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
