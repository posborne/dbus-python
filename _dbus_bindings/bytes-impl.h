/* D-Bus Byte and ByteArray types.
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

static PyTypeObject ByteType, ByteArrayType;

static inline int Byte_Check(PyObject *o)
{
    return (o->ob_type == &ByteType)
           || PyObject_IsInstance(o, (PyObject *)&ByteType);
}

static inline int ByteArray_Check(PyObject *o)
{
    return (o->ob_type == &ByteArrayType)
           || PyObject_IsInstance(o, (PyObject *)&ByteArrayType);
}

PyDoc_STRVAR(Byte_tp_doc,
"Byte(integer or str of length 1)\n"
"\n"
"Byte is a subtype of int, with range restricted to [0, 255].\n"
"\n"
"A Byte b may be converted to a str of length 1 via str(b) == chr(b).\n"
);

static inline unsigned char
Byte_as_uchar(PyObject *self)
{
    return (unsigned char) (PyInt_AsLong (self));
}

PyObject *allocated_bytes[256] = {NULL};

static PyObject *
Byte_from_uchar(unsigned char data)
{
    PyObject *ret;
    ret = allocated_bytes[data];
    if (!ret) {
        PyObject *tuple = Py_BuildValue("(i)", data);
        DBG("Allocating a new Byte of value \\x%02x", data);

        if (!tuple) return NULL;

        ret = PyInt_Type.tp_new(&ByteType, tuple, NULL);
        Py_DECREF(tuple);
        tuple = NULL;

#ifdef USING_DBG
        fprintf(stderr, "New Byte of value \\x%02x: ", data);
        PyObject_Print(ret, stderr, 0);
        fprintf(stderr, "\n");
#endif

        Py_INCREF(ret);
        allocated_bytes[data] = ret;
      }
    Py_INCREF(ret);
    return ret;
}

static PyObject *
Byte_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *obj;
    PyObject *tuple;

    if (PyTuple_Size(args) != 1 || (kwargs && PyDict_Size(kwargs) != 0)) {
        PyErr_SetString(PyExc_TypeError, "Byte constructor takes exactly one "
                        "positional argument");
        return NULL;
    }

    /* obj is only a borrowed ref for the moment */
    obj = PyTuple_GetItem(args, 0);

    if (PyString_Check(obj)) {
        /* string of length 1, we hope */
        if (PyString_GET_SIZE(obj) != 1) {
            goto bad_arg;
        }
        /* here arg goes from a borrowed to owned reference */
        obj = Byte_from_uchar((unsigned char)(PyString_AS_STRING(obj)[0]));
        if (cls == &ByteType) {
            /* we know that the Byte type is the same as an int internally */
            return obj;
        }
    }
    else if (PyInt_Check(obj)) {
        long i = PyInt_AS_LONG(obj);

        if (obj->ob_type == cls) {
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

    /* so here's the complicated case: we're trying to instantiate a subclass
    so we can't just allocate a Byte and go "it'll all be fine".

    By now, we do have an int (or Byte) to use as an argument to the c'tor.
    */
    tuple = Py_BuildValue("(O)", obj);
    if (!tuple) return NULL;
    Py_DECREF(obj);
    obj = NULL;

    obj = PyInt_Type.tp_new(cls, tuple, NULL);
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
    char str[] = { (char)Byte_as_uchar(self), 0 };
    return PyString_FromStringAndSize(str, 1);
}

static PyTypeObject ByteType = {
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
        int_subclass_tp_repr,                   /* tp_repr */
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
"ByteArray(str)\n"
"\n"
"ByteArray is a subtype of str which can be used when you want an\n"
"efficient immutable representation of a D-Bus byte array (signature 'ay').\n"
"\n"
"The subscript operation byte_array[i] returns Byte instances. Otherwise,\n"
"it's just a string.\n"
);

static inline unsigned char *
ByteArray_as_ucharptr(PyObject *self)
{
    return (unsigned char *)PyString_AsString(self);
}

static inline PyObject *
ByteArray_from_uchars(const unsigned char *data, int size)
{
    return PyObject_CallFunction((PyObject *)&ByteArrayType, "(s#)",
                                 (char *)data, size);
}

static PyObject *
ByteArray_sq_item (PyObject *self, int i)
{
    unsigned char c;
    if (i < 0 || i >= PyString_GET_SIZE(self)) {
        PyErr_SetString(PyExc_IndexError, "ByteArray index out of range");
        return NULL;
    }
    c = ((unsigned char *)PyString_AS_STRING(self))[i];
    return Byte_from_uchar(c);
}

static PyObject *
ByteArray_mp_subscript(PyObject *self, PyObject *other)
{
    long i;
    if (PyInt_Check(other)) {
        i = PyInt_AS_LONG(other);
        if (i < 0) i += PyString_GET_SIZE (self);
        return ByteArray_sq_item(self, i);
    }
    else if (PyLong_Check(other)) {
        i = PyLong_AsLong(other);
        if (i == -1 && PyErr_Occurred()) return NULL;
        if (i < 0) i += PyString_GET_SIZE(self);
        return ByteArray_sq_item(self, i);
    }
    else {
        /* slices just return strings */
        return (PyString_Type.tp_as_mapping->mp_subscript)(self, other);
    }
}

static PyMappingMethods ByteArray_tp_as_mapping = {
    0, /* mp_length */
    ByteArray_mp_subscript, /* mp_subscript */
    0, /* mp_ass_subscript */
};

static PySequenceMethods ByteArray_tp_as_sequence = {
    0, /* sq_length */
    0, /* sq_concat */
    0, /* sq_repeat */
    ByteArray_sq_item, /* sq_item */
    0, /* sq_slice */
    0, /* sq_ass_item */
    0, /* sq_ass_slice */
    0, /* sq_contains */
};

static PyTypeObject ByteArrayType = {
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
        str_subclass_tp_repr,                   /* tp_repr */
        0,                                      /* tp_as_number */
        &ByteArray_tp_as_sequence,              /* tp_as_sequence */
        &ByteArray_tp_as_mapping,               /* tp_as_mapping */
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
        DEFERRED_ADDRESS(&PyString_Type),       /* tp_base */
        0,                                      /* tp_dict */
        0,                                      /* tp_descr_get */
        0,                                      /* tp_descr_set */
        0,                                      /* tp_dictoffset */
        0,                                      /* tp_init */
        0,                                      /* tp_alloc */
        0,                                      /* tp_new */
};

static inline int
init_byte_types(void)
{
    ByteType.tp_base = &PyInt_Type;
    if (PyType_Ready(&ByteType) < 0) return 0;
    ByteType.tp_print = NULL;

    ByteArrayType.tp_base = &PyString_Type;
    if (PyType_Ready(&ByteArrayType) < 0) return 0;
    ByteArrayType.tp_print = NULL;

    return 1;
}

static inline int
insert_byte_types(PyObject *this_module)
{
    Py_INCREF(&ByteType);
    if (PyModule_AddObject(this_module, "Byte",
                           (PyObject *)&ByteType) < 0) return 0;
    Py_INCREF(&ByteArrayType);
    if (PyModule_AddObject(this_module, "ByteArray",
                           (PyObject *)&ByteArrayType) < 0) return 0;

    return 1;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
