/* Implementation of Signature type for D-Bus bindings.
 *
 * Copyright (C) 2006 Collabora Ltd.
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

#include <Python.h>
#include <structmember.h>

#include <stdint.h>

#include "dbus_bindings-internal.h"
#include "types-internal.h"

static PyObject *Signature = NULL;

static dbus_bool_t
do_import(void)
{
    PyObject *name;
    PyObject *module;

    if (Signature != NULL)
        return TRUE;

    name = PyString_FromString("dbus.data");
    if (!name)
        return FALSE;

    module = PyImport_Import(name);
    Py_DECREF(name);
    if (!module)
        return FALSE;

    Signature = PyObject_GetAttrString(module, "Signature");
    Py_DECREF(module);

    if (Signature && !PyType_Check(Signature)) {
        PyErr_SetString(PyExc_AssertionError, "Assertion failed: "
                        "isinstance(dbus.data.Signature, type)");
        Signature = NULL;
    }

    return (Signature != NULL);
}

typedef struct {
    PyObject_HEAD
    PyObject *string;
    DBusSignatureIter iter;
} SignatureIter;

static void
SignatureIter_tp_dealloc (SignatureIter *self)
{
    Py_XDECREF(self->string);
    self->string = NULL;
    PyObject_Del(self);
}

static PyObject *
SignatureIter_tp_iternext (SignatureIter *self)
{
    char *sig;
    PyObject *obj;

    /* Stop immediately if finished or not correctly initialized */
    if (!self->string) return NULL;

    sig = dbus_signature_iter_get_signature(&(self->iter));
    if (!sig) return PyErr_NoMemory();
    obj = PyObject_CallFunction(Signature, "s", sig);
    dbus_free(sig);
    if (!obj) return NULL;

    if (!dbus_signature_iter_next(&(self->iter))) {
        /* mark object as having been finished with */
        Py_DECREF(self->string);
        self->string = NULL;
    }

    return obj;
}

static PyObject *
SignatureIter_tp_iter(PyObject *self)
{
    Py_INCREF(self);
    return self;
}

static PyTypeObject SignatureIterType = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
    "_dbus_bindings._SignatureIter",
    sizeof(SignatureIter),
    0,
    (destructor)SignatureIter_tp_dealloc,   /* tp_dealloc */
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
    Py_TPFLAGS_DEFAULT,                     /* tp_flags */
    0,                                      /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    SignatureIter_tp_iter,                  /* tp_iter */
    (iternextfunc)SignatureIter_tp_iternext,  /* tp_iternext */
    0,                                      /* tp_methods */
    0,                                      /* tp_members */
    0,                                      /* tp_getset */
    0,                                      /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    /* deliberately not callable! Use iter(Signature) instead */
    0,                                      /* tp_new */
    0,                                      /* tp_free */
};

PyObject *
dbus_py_get_signature_iter (PyObject *unused UNUSED, PyObject *sig)
{
    SignatureIter *iter;

    if (!DBusPySignature_Check (sig)) {
        PyErr_SetString(PyExc_TypeError, "A dbus.data.Signature is required");
    }

    iter = PyObject_New(SignatureIter, &SignatureIterType);

    if (!iter)
        return NULL;

    if (PyString_AS_STRING (sig)[0]) {
        Py_INCREF(sig);
        iter->string = sig;
        dbus_signature_iter_init(&(iter->iter), PyString_AS_STRING(sig));
    }
    else {
        /* this is a null string, make a null iterator */
        iter->string = NULL;
    }
    return (PyObject *)iter;
}

PyObject *
DBusPySignature_FromStringObject(PyObject *o, int allow_none)
{
    if (!Signature && !do_import())
        return NULL;

    if (allow_none && o == Py_None) {
        Py_INCREF(o);
        return o;
    }
    if (PyObject_TypeCheck(o, (PyTypeObject *)Signature)) {
        Py_INCREF(o);
        return o;
    }
    return PyObject_CallFunction(Signature, "(O)", o);
}

PyObject *
DBusPySignature_FromStringAndVariantLevel(const char *str, long variant_level)
{
    if (!Signature && !do_import())
        return NULL;
    return PyObject_CallFunction(Signature, "(sl)", str, variant_level);
}

PyObject *
DBusPySignature_FromStringAndSize(const char *str, Py_ssize_t size)
{
    if (!Signature && !do_import())
        return NULL;
    return PyObject_CallFunction(Signature, "(s#)", str, size);
}

PyObject *
DBusPySignature_FromString(const char *str)
{
    if (!Signature && !do_import())
        return NULL;
    return PyObject_CallFunction(Signature, "(s)", str);
}

int
DBusPySignature_Check(PyObject *o)
{
    if (!Signature && !do_import())
        return 0;
    return PyObject_TypeCheck(o, (PyTypeObject *)Signature);
}

dbus_bool_t
dbus_py_init_signature(void)
{
    if (PyType_Ready(&SignatureIterType) < 0) return 0;

    return 1;
}

dbus_bool_t
dbus_py_insert_signature(PyObject *this_module)
{
    Py_INCREF(&SignatureIterType);
    if (PyModule_AddObject(this_module, "_SignatureIter",
                           (PyObject *)&SignatureIterType) < 0) return 0;

    return 1;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
