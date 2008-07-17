/* Implementation of Bus, a subtype of Connection.
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

#include "dbus_bindings-internal.h"
#include "conn-internal.h"

PyObject *
DBusPyConnection_NewForBus(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *first = NULL, *mainloop = NULL;
    DBusConnection *conn;
    DBusError error;
    Connection *self;
    static char *argnames[] = {"address_or_type", "mainloop", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OO", argnames,
                                     &first, &mainloop)) {
        return NULL;
    }

    dbus_error_init(&error);

    if (first && PyString_Check(first)) {
        dbus_bool_t ret;

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
    else if (!first || PyInt_Check(first)) {
        long type;
        PyObject *libdbusconn;
        PyObject *new_args;
        PyObject *new_kwargs;

        /* If the first argument isn't a string, it must be an integer
        representing one of the well-known bus types. The default is
        DBUS_BUS_SESSION. */

        if (first) {
            type = PyInt_AsLong(first);

            if (type != DBUS_BUS_SESSION && type != DBUS_BUS_SYSTEM
                && type != DBUS_BUS_STARTER) {
                PyErr_Format(PyExc_ValueError, "Unknown bus type %ld", type);
                return NULL;
            }
        }
        else {
            type = DBUS_BUS_SESSION;
        }

        Py_BEGIN_ALLOW_THREADS
        conn = dbus_bus_get_private(type, &error);
        Py_END_ALLOW_THREADS

        if (!conn) {
            DBusPyException_ConsumeError(&error);
            return NULL;
        }

        libdbusconn = DBusPyLibDBusConnection_New (conn);
        dbus_connection_unref (conn);

        if (!libdbusconn)
            return NULL;

        new_args = PyTuple_Pack(2, libdbusconn, mainloop ? mainloop : Py_None);
        Py_DECREF(libdbusconn);

        if (!new_args) {
            return NULL;
        }

        new_kwargs = PyDict_New();

        if (!new_kwargs) {
            Py_DECREF(new_args);
            return NULL;
        }

        self = (Connection *)(DBusPyConnection_Type.tp_new)(cls, new_args,
                new_kwargs);
        Py_DECREF(new_args);
        Py_DECREF(new_kwargs);

        return (PyObject *)self;    /* whether NULL or not */
    }
    else {
        PyErr_SetString(PyExc_TypeError, "A string address or an integer "
                                         "bus type is required");
        return NULL;
    }
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
        return DBusPyException_SetString("This connection has no unique name "
                                         "yet");
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
