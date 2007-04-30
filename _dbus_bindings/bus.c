/* Implementation of Bus, a subtype of Connection.
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

#include "dbus_bindings-internal.h"
#include "conn-internal.h"

PyDoc_STRVAR(Bus_tp_doc,
"If the address is an int it must be one of the constants BUS_SESSION,\n"
"BUS_SYSTEM, BUS_STARTER; if a string, it must be a D-Bus address.\n"
"The default is BUS_SESSION.\n"
"\n"
"Constructor::\n"
"\n"
"   BusImplementation([address: str or int])\n"
);

/* Bus definition =================================================== */

static PyTypeObject BusType;

#define Bus_Check(ob) PyObject_TypeCheck(ob, &BusType)

/* Bus methods ====================================================== */

static PyObject *
Bus_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *first = NULL, *mainloop = NULL;
    DBusConnection *conn;
    DBusError error;
    Connection *self;
    dbus_bool_t ret;
    long type;
    static char *argnames[] = {"address_or_type", "mainloop", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OO", argnames,
                                     &first, &mainloop)) {
        return NULL;
    }

    dbus_error_init(&error);

    if (first && PyString_Check(first)) {
        /* It's a custom address. First connect to it, then register. */

        self = (Connection *)(DBusPyConnection_Type.tp_new)(cls, args, kwargs);
        if (!self) return NULL;
        TRACE(self);

        Py_BEGIN_ALLOW_THREADS
        ret = dbus_bus_register(self->conn, &error);
        Py_END_ALLOW_THREADS
        if (!ret) {
            DBusPyException_ConsumeError(&error);
            Py_DECREF(self);
            return NULL;
        }

        return (PyObject *)self;
    }

    /* If the first argument isn't a string, it must be an integer
    representing one of the well-known bus types. */

    if (first && !PyInt_Check(first)) {
        PyErr_SetString(PyExc_TypeError, "A string address or an integer "
                                         "bus type is required");
        return NULL;
    }
    if (first) {
        type = PyInt_AsLong(first);
    }
    else {
        type = DBUS_BUS_SESSION;
    }

    if (type != DBUS_BUS_SESSION && type != DBUS_BUS_SYSTEM
        && type != DBUS_BUS_STARTER) {
        PyErr_Format(PyExc_ValueError, "Unknown bus type %d", (int)type);
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    conn = dbus_bus_get_private(type, &error);
    Py_END_ALLOW_THREADS

    if (!conn) {
        DBusPyException_ConsumeError(&error);
        return NULL;
    }
    return DBusPyConnection_NewConsumingDBusConnection(cls, conn, mainloop);
}

PyDoc_STRVAR(Bus_get_unique_name__doc__,
"get_unique_name() -> str\n\n"
"Return this application's unique name on this bus.\n");
static PyObject *
Bus_get_unique_name(Connection *self, PyObject *args UNUSED)
{
    const char *name;

    TRACE(self);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(self->conn);
    Py_BEGIN_ALLOW_THREADS
    name = dbus_bus_get_unique_name(self->conn);
    Py_END_ALLOW_THREADS
    if (!name) {
        /* shouldn't happen, but C subtypes could have done something stupid */
        PyErr_SetString(DBusPyException, "Unable to retrieve unique name");
        return NULL;
    }
    return PyString_FromString(name);
}

/* Bus type object ================================================== */

static struct PyMethodDef Bus_tp_methods[] = {
#define ENTRY(name, flags) {#name, (PyCFunction)Bus_##name, flags, Bus_##name##__doc__},
    ENTRY(get_unique_name, METH_NOARGS)
#undef ENTRY
    {NULL},
};

static PyTypeObject BusType = {
        PyObject_HEAD_INIT(NULL)
        0,                      /*ob_size*/
        "_dbus_bindings.BusImplementation",  /*tp_name*/
        0,                      /*tp_basicsize*/
        0,                      /*tp_itemsize*/
        /* methods */
        0,                      /*tp_dealloc*/
        0,                      /*tp_print*/
        0,                      /*tp_getattr*/
        0,                      /*tp_setattr*/
        0,                      /*tp_compare*/
        0,                      /*tp_repr*/
        0,                      /*tp_as_number*/
        0,                      /*tp_as_sequence*/
        0,                      /*tp_as_mapping*/
        0,                      /*tp_hash*/
        0,                      /*tp_call*/
        0,                      /*tp_str*/
        0,                      /*tp_getattro*/
        0,                      /*tp_setattro*/
        0,                      /*tp_as_buffer*/
        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /*tp_flags*/
        Bus_tp_doc,             /*tp_doc*/
        0,                      /*tp_traverse*/
        0,                      /*tp_clear*/
        0,                      /*tp_richcompare*/
        0,                      /*tp_weaklistoffset*/
        0,                      /*tp_iter*/
        0,                      /*tp_iternext*/
        Bus_tp_methods,         /*tp_methods*/
        0,                      /*tp_members*/
        0,                      /*tp_getset*/
        DEFERRED_ADDRESS(&ConnectionType), /*tp_base*/
        0,                      /*tp_dict*/
        0,                      /*tp_descr_get*/
        0,                      /*tp_descr_set*/
        0,                      /*tp_dictoffset*/
        0,                      /*tp_init*/
        0,                      /*tp_alloc*/
        Bus_tp_new,             /*tp_new*/
        0,                      /*tp_free*/
        0,                      /*tp_is_gc*/
};

dbus_bool_t
dbus_py_init_bus_types(void)
{
    BusType.tp_base = &DBusPyConnection_Type;
    if (PyType_Ready(&BusType) < 0) return 0;
    return 1;
}

dbus_bool_t
dbus_py_insert_bus_types(PyObject *this_module)
{
    if (PyModule_AddObject(this_module, "BusImplementation",
                           (PyObject *)&BusType) < 0) return 0;
    return 1;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
