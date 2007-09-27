/* D-Bus Byte and ByteArray types.
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

#include <Python.h>
#include <structmember.h>

#include <stdint.h>

#include "dbus_bindings-internal.h"
#include "types-internal.h"

PyDoc_STRVAR(Byte_tp_doc,
"An unsigned byte: a subtype of int, with range restricted to [0, 255].\n"
"\n"
"A Byte b may be converted to a str of length 1 via str(b) == chr(b).\n"
"\n"
"Most of the time you don't want to use this class - it mainly exists\n"
"for symmetry with the other D-Bus types. See `dbus.ByteArray` for a\n"
"better way to handle arrays of Byte.\n"
"\n"
"Constructor::\n"
"\n"
"   dbus.Byte(integer or str of length 1[, variant_level])\n"
"\n"
"``variant_level`` must be non-negative; the default is 0.\n"
"\n"
":IVariables:\n"
"  `variant_level` : int\n"
"    Indicates how many nested Variant containers this object\n"
"    is contained in: if a message's wire format has a variant containing a\n"
"    variant containing a byte, this is represented in Python by a\n"
"    Byte with variant_level==2.\n"
);

static PyObject *
Byte_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *obj;
    PyObject *tuple;
    long variantness = 0;
    static char *argnames[] = {"variant_level", NULL};

    if (PyTuple_Size(args) > 1) {
        PyErr_SetString(PyExc_TypeError, "Byte constructor takes no more "
                        "than one positional argument");
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

    /* obj is only a borrowed ref for the moment */
    obj = PyTuple_GetItem(args, 0);

    if (PyString_Check(obj)) {
        /* string of length 1, we hope */
        if (PyString_GET_SIZE(obj) != 1) {
            goto bad_arg;
        }
        obj = PyInt_FromLong((unsigned char)(PyString_AS_STRING(obj)[0]));
    }
    else if (PyInt_Check(obj)) {
        long i = PyInt_AS_LONG(obj);

        if (obj->ob_type == cls &&
            ((DBusPyIntBase *)obj)->variant_level == variantness) {
            Py_INCREF(obj);
            return obj;
        }
        if (i < 0 || i > 255) goto bad_range;
        /* else make it a new reference */
        Py_INCREF(obj);
    }
    else {
        goto bad_arg;
    }

    tuple = Py_BuildValue("(O)", obj);
    if (!tuple) return NULL;
    Py_DECREF(obj);
    obj = NULL;

    obj = DBusPyIntBase_Type.tp_new(cls, tuple, kwargs);
    Py_DECREF(tuple);
    tuple = NULL;
    return obj;

bad_arg:
    PyErr_SetString(PyExc_TypeError, "Expected a string of length 1, "
                    "or an int in the range 0-255");
    return NULL;
bad_range:
    PyErr_SetString(PyExc_ValueError, "Integer outside range 0-255");
    return NULL;
}

static PyObject *
Byte_tp_str(PyObject *self)
{
    unsigned char str[2] = { (unsigned char)PyInt_AS_LONG(self), 0 };
    return PyString_FromStringAndSize((char *)str, 1);
}

PyTypeObject DBusPyByte_Type = {
        PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
        0,
        "dbus.Byte",
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
        Byte_tp_str,                            /* tp_str */
        0,                                      /* tp_getattro */
        0,                                      /* tp_setattro */
        0,                                      /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
        Byte_tp_doc,                            /* tp_doc */
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
        Byte_new,                               /* tp_new */
};

PyDoc_STRVAR(ByteArray_tp_doc,
"ByteArray is a subtype of str which can be used when you want an\n"
"efficient immutable representation of a D-Bus byte array (signature 'ay').\n"
"\n"
"By default, when byte arrays are converted from D-Bus to Python, they\n"
"come out as a `dbus.Array` of `dbus.Byte`. This is just for symmetry with\n"
"the other D-Bus types - in practice, what you usually want is the byte\n"
"array represented as a string, using this class. To get this, pass the\n"
"``byte_arrays=True`` keyword argument to any of these methods:\n"
"\n"
"* any D-Bus method proxy, or ``connect_to_signal``, on the objects returned\n"
"  by `Bus.get_object`\n"
"* any D-Bus method on a `dbus.Interface`\n"
"* `dbus.Interface.connect_to_signal`\n"
"* `Bus.add_signal_receiver`\n"
"\n"
"Import via::\n"
"\n"
"   from dbus import ByteArray\n"
"\n"
"Constructor::\n"
"\n"
"   ByteArray(str)\n"
);

PyTypeObject DBusPyByteArray_Type = {
        PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
        0,
        "dbus.ByteArray",
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
        ByteArray_tp_doc,                       /* tp_doc */
        0,                                      /* tp_traverse */
        0,                                      /* tp_clear */
        0,                                      /* tp_richcompare */
        0,                                      /* tp_weaklistoffset */
        0,                                      /* tp_iter */
        0,                                      /* tp_iternext */
        0,                                      /* tp_methods */
        0,                                      /* tp_members */
        0,                                      /* tp_getset */
        DEFERRED_ADDRESS(&DBusPyStrBase_Type),  /* tp_base */
        0,                                      /* tp_dict */
        0,                                      /* tp_descr_get */
        0,                                      /* tp_descr_set */
        0,                                      /* tp_dictoffset */
        0,                                      /* tp_init */
        0,                                      /* tp_alloc */
        0,                                      /* tp_new */
};

dbus_bool_t
dbus_py_init_byte_types(void)
{
    DBusPyByte_Type.tp_base = &DBusPyIntBase_Type;
    if (PyType_Ready(&DBusPyByte_Type) < 0) return 0;
    DBusPyByte_Type.tp_print = NULL;

    DBusPyByteArray_Type.tp_base = &DBusPyStrBase_Type;
    if (PyType_Ready(&DBusPyByteArray_Type) < 0) return 0;
    DBusPyByteArray_Type.tp_print = NULL;

    return 1;
}

dbus_bool_t
dbus_py_insert_byte_types(PyObject *this_module)
{
    Py_INCREF(&DBusPyByte_Type);
    if (PyModule_AddObject(this_module, "Byte",
                           (PyObject *)&DBusPyByte_Type) < 0) return 0;
    Py_INCREF(&DBusPyByteArray_Type);
    if (PyModule_AddObject(this_module, "ByteArray",
                           (PyObject *)&DBusPyByteArray_Type) < 0) return 0;

    return 1;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
