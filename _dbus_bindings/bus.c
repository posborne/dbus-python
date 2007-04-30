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

PyObject *
DBusPyConnection_NewForBus(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
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

PyObject *
DBusPyConnection_GetUniqueName(Connection *self, PyObject *args UNUSED)
{
    const char *name;

    TRACE(self);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(self->conn);
    Py_BEGIN_ALLOW_THREADS
    name = dbus_bus_get_unique_name(self->conn);
    Py_END_ALLOW_THREADS
    if (!name) {
        PyErr_SetString(DBusPyException, "This connection has no unique "
                        "name yet");
        return NULL;
    }
    return PyString_FromString(name);
}

PyObject *
DBusPyConnection_SetUniqueName(Connection *self, PyObject *args)
{
    const char *old_name, *new_name;

    if (!PyArg_ParseTuple(args, "s:set_unique_name", &new_name)) {
        return NULL;
    }

    TRACE(self);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(self->conn);

    /* libdbus will assert if we try to set a unique name when there's
     * already one, so we need to make sure that can't happen.
     * (Thanks, libdbus.)
     *
     * The things that can set the unique name are:
     * - this function - but we don't release the GIL, so only one instance of
     *   this function can run
     * - dbus_bus_get - but this is only called in a __new__ or __new__-like
     *   function, so the new connection isn't available to other code yet
     *   and this function can't be called on it
     * - dbus_bus_register - same as dbus_bus_get
     *
     * Code outside dbus-python shouldn't be setting the unique name, because
     * we're using a private connection; we have to trust the authors
     * of mainloop bindings not to do silly things like that.
     */
    old_name = dbus_bus_get_unique_name(self->conn);
    if (old_name != NULL) {
        PyErr_Format(PyExc_ValueError, "This connection already has a "
                     "unique name: '%s'", old_name);
        return NULL;
    }
    dbus_bus_set_unique_name(self->conn, new_name);

    Py_RETURN_NONE;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
