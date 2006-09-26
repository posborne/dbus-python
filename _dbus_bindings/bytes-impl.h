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
"Byte is a subtype of str, restricted to length exactly 1.\n"
"\n"
"A Byte b also supports the following operations:\n"
"\n"
"* int(b) == long(b) == float(b) == ord(b)\n"
"\n"
"* oct(b), hex(b)\n"
);

static inline unsigned char
Byte_as_uchar(PyObject *self)
{
    return (unsigned char) (PyString_AS_STRING (self)[0]);
}

PyObject *allocated_bytes[256] = {NULL};

static PyObject *
Byte_from_uchar(unsigned char data)
{
    PyObject *ret;
    ret = allocated_bytes[data];
    if (!ret) {
        char c[] = { (char)data, '\0' };
        PyObject *tuple = Py_BuildValue("(s#)", c, 1);
        DBG("Allocating a new Byte of value \\x%02x", data);

        if (!tuple) return NULL;

        ret = PyString_Type.tp_new(&ByteType, tuple, NULL);
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
    PyObject *arg;
    PyObject *string;
    PyObject *tuple;

    if (PyTuple_Size(args) != 1 || (kwargs && PyDict_Size(kwargs) != 0)) {
        PyErr_SetString(PyExc_TypeError, "Byte constructor takes exactly one "
                        "positional argument");
        return NULL;
    }

    /* arg is only a borrowed ref for the moment */
    arg = PyTuple_GetItem(args, 0);

    if (PyString_Check(arg)) {
        /* string of length 1, we hope */
        if (PyString_GET_SIZE(arg) != 1) {
            goto bad_arg;
        }
        if (arg->ob_type == &ByteType) {
            Py_INCREF(arg);
            return arg;
        }
        if (cls == &ByteType) {
            /* we know that the Byte type is the same as a string
             * internally, so fast-track it */
            return Byte_from_uchar((unsigned char)(PyString_AS_STRING(arg)[0]));
        }
        /* only a borrowed reference so far, take ownership */
        Py_INCREF(arg);
    }
    else if (PyInt_Check(arg)) {
        long i = PyInt_AS_LONG(arg);

        if (i < 0 || i > 255) goto bad_range;
        /* make a byte object, which is a length-1 string - now it's a new
        reference */
        arg = Byte_from_uchar((unsigned char)i);
        if (!arg) return NULL;
        /* if that's the type we wanted, there's nothing more to do */
        if (cls == &ByteType) return arg;
    }
    else {
        goto bad_arg;
    }

    /* so here's the complicated case: we're trying to instantiate a subclass
    so we can't just allocate a Byte and go "it'll all be fine".

    By now, we do have a string-of-length-1 to use as an argument to the c'tor
    */
    tuple = Py_BuildValue("(O)", arg);
    if (!tuple) return NULL;
    Py_DECREF(arg);
    arg = NULL;

    string = PyString_Type.tp_new(cls, tuple, NULL);
    Py_DECREF(tuple);
    tuple = NULL;
    return string;

bad_arg:
    PyErr_SetString(PyExc_TypeError, "Expected a string of length 1, "
                    "or an int in the range 0-255");
    return NULL;
bad_range:
    PyErr_SetString(PyExc_ValueError, "Integer outside range 0-255");
    return NULL;
}

static PyObject *Byte_nb_int (PyObject *self)
{
    return PyInt_FromLong(Byte_as_uchar(self));
}

static PyObject *Byte_nb_float (PyObject *self)
{
    return PyFloat_FromDouble(Byte_as_uchar(self));
}

static PyObject *Byte_nb_long (PyObject *self)
{
    return PyLong_FromLong(Byte_as_uchar(self));
}

static PyObject *Byte_nb_oct (PyObject *self)
{
    PyObject *i = PyInt_FromLong(Byte_as_uchar(self));
    if (!i) return NULL;
    PyObject *s = (i->ob_type->tp_as_number->nb_oct) (i);
    Py_XDECREF (i);
    return s;
}

static PyObject *Byte_nb_hex (PyObject *self)
{
    PyObject *i = PyInt_FromLong(Byte_as_uchar(self));
    if (!i) return NULL;
    PyObject *s = (i->ob_type->tp_as_number->nb_hex) (i);
    Py_XDECREF (i);
    return s;
}

static PyNumberMethods Byte_tp_as_number = {
        0,                                      /* nb_add */
        0,                                      /* nb_subtract */
        0,                                      /* nb_multiply */
        0,                                      /* nb_divide */
        0,                                      /* nb_remainder */
        0,                                      /* nb_divmod */
        0,                                      /* nb_power */
        0,                                      /* nb_negative */
        0,                                      /* nb_positive */
        0,                                      /* nb_absolute */
        0,                                      /* nb_nonzero */
        0,                                      /* nb_invert */
        0,                                      /* nb_lshift */
        0,                                      /* nb_rshift */
        0,                                      /* nb_and */
        0,                                      /* nb_xor */
        0,                                      /* nb_or */
        0,                                      /* nb_coerce */
        (unaryfunc)Byte_nb_int,                 /* nb_int */
        (unaryfunc)Byte_nb_long,                /* nb_long */
        (unaryfunc)Byte_nb_float,               /* nb_float */
        (unaryfunc)Byte_nb_oct,                 /* nb_oct */
        (unaryfunc)Byte_nb_hex,                 /* nb_hex */
        0,                                      /* nb_inplace_add */
        0,                                      /* nb_inplace_subtract */
        0,                                      /* nb_inplace_multiply */
        0,                                      /* nb_inplace_divide */
        0,                                      /* nb_inplace_remainder */
        0,                                      /* nb_inplace_power */
        0,                                      /* nb_inplace_lshift */
        0,                                      /* nb_inplace_rshift */
        0,                                      /* nb_inplace_and */
        0,                                      /* nb_inplace_xor */
        0,                                      /* nb_inplace_or */
        0,                                      /* nb_floor_divide */
        0,                                      /* nb_true_divide */
        0,                                      /* nb_inplace_floor_divide */
        0,                                      /* nb_inplace_true_divide */
};

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
        str_subclass_tp_repr,                   /* tp_repr */
        &Byte_tp_as_number,
        0,                                      /* tp_as_sequence */
        0,                                      /* tp_as_mapping */
        0,                                      /* tp_hash */
        0,                                      /* tp_call */
        0,                                      /* tp_str */
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
        DEFERRED_ADDRESS(&PyString_Type),       /* tp_base */
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
    ByteType.tp_base = &PyString_Type;
    if (PyType_Ready(&ByteType) < 0) return 0;
    /* disable the tp_print copied from PyString_Type, so tp_repr gets called as
    desired */
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
