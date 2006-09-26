/* Implementation of Bus, a subtype of Connection.
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

PyDoc_STRVAR(Bus_tp_doc,
"Bus([address: str or int])\n\n"
"If the address is an int it must be one of the constants BUS_SESSION,\n"
"BUS_SYSTEM, BUS_STARTER. The default is BUS_SESSION.\n"
);

/* Bus definition =================================================== */

static PyTypeObject BusType;

#define Bus_Check(ob) PyObject_TypeCheck(ob, &BusType)

/* Bus methods ====================================================== */

static PyObject *
Bus_tp_new (PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *first = NULL;
    DBusConnection *conn;
    DBusError error;
    Connection *self;
    dbus_bool_t ret;
    long type;
    static char *argnames[] = {"address_or_type", NULL};

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "|O", argnames, &first)) {
        return NULL;
    }

    dbus_error_init (&error);

    if (first && PyString_Check(first)) {
        /* It's a custom address. First connect to it, then register. */

        self = (Connection *)Connection_tp_new(cls, args, kwargs);
        if (!self) return NULL;

        Py_BEGIN_ALLOW_THREADS
        ret = dbus_bus_register (self->conn, &error);
        Py_END_ALLOW_THREADS
        if (!ret) {
            DBusException_ConsumeError(&error);
            Py_DECREF(self);
            return NULL;
        }
    }

    /* If the first argument isn't a string, it must be an integer
    representing one of the well-known bus types. */

    if (first && !PyInt_Check (first)) {
        PyErr_SetString(PyExc_TypeError, "A string address or an integer "
                                         "bus type is required");
        return NULL;
    }
    if (first) {
        type = PyInt_AsLong (first);
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
    conn = dbus_bus_get_private (type, &error);
    Py_END_ALLOW_THREADS

    if (!conn) {
        DBusException_ConsumeError (&error);
        return NULL;
    }
    return Connection_NewConsumingDBusConnection(cls, conn);
}

PyDoc_STRVAR(Bus_get_unique_name__doc__,
"get_unique_name() -> str\n\n"
"Return this application's unique name on this bus.\n");
static PyObject *
Bus_get_unique_name (Connection *self, PyObject *args)
{
    const char *name;

    Py_BEGIN_ALLOW_THREADS
    name = dbus_bus_get_unique_name (self->conn);
    Py_END_ALLOW_THREADS
    if (!name) {
        /* shouldn't happen, but C subtypes could have done something stupid */
        PyErr_SetString (DBusException, "Unable to retrieve unique name");
        return NULL;
    }
    return PyString_FromString (name);
}

PyDoc_STRVAR(Bus_get_unix_user__doc__,
"get_unix_user(bus_name: str) -> int\n"
"\n"
"Get the numeric uid of the process which owns the given bus name\n"
"on the connected bus daemon.\n"
"\n"
":Parameters:\n"
"    `bus_name` : str\n"
"        A bus name (may be either a unique name or a well-known name)\n"
);
static PyObject *
Bus_get_unix_user (Connection *self, PyObject *args)
{
    DBusError error;
    unsigned long uid;
    const char *bus_name;

    if (!PyArg_ParseTuple(args, "s:get_unix_user", &bus_name)) {
        return NULL;
    }

    dbus_error_init (&error);
    Py_BEGIN_ALLOW_THREADS
    uid = dbus_bus_get_unix_user (self->conn, bus_name, &error);
    Py_END_ALLOW_THREADS
    if (uid == (unsigned long)(-1)) return DBusException_ConsumeError (&error);
    return PyLong_FromUnsignedLong (uid);
}

PyDoc_STRVAR(Bus_start_service_by_name__doc__,
"start_service_by_name(bus_name: str) -> (True, int)\n\
\n\
Start a service which will implement the given bus name on this\n\
Bus.\n\
\n\
:Parameters:\n\
    `bus_name` : str\n\
        The well-known bus name for which an implementation is required\n\
\n\
:Returns: A tuple of 2 elements. The first is always True, the second is\n\
          either START_REPLY_SUCCESS or START_REPLY_ALREADY_RUNNING.\n\
\n\
:Raises DBusException: if the service could not be started.\n\
\n\
FIXME: Fix return signature?\n\
");
static PyObject *
Bus_start_service_by_name (Connection *self, PyObject *args)
{
    DBusError error;
    const char *bus_name;
    dbus_uint32_t ret;
    dbus_bool_t success;

    if (!PyArg_ParseTuple(args, "s:start_service_by_name", &bus_name)) {
        return NULL;
    }
    dbus_error_init (&error);
    Py_BEGIN_ALLOW_THREADS
    success = dbus_bus_start_service_by_name (self->conn, bus_name,
                                              0 /* flags */, &ret, &error);
    Py_END_ALLOW_THREADS
    if (!success) {
        return DBusException_ConsumeError (&error);
    }
    return Py_BuildValue ("(Ol)", Py_True, (long)ret);
}

/* FIXME: signal IN_QUEUE, EXISTS by exception? */
PyDoc_STRVAR(Bus_request_name__doc__, "");
static PyObject *
Bus_request_name (Connection *self, PyObject *args)
{
    unsigned int flags = 0;
    const char *bus_name;
    int ret;
    DBusError error;

    if (!PyArg_ParseTuple(args, "s|I:request_name", &bus_name, &flags)) {
        return NULL;
    }
    if (!_validate_bus_name(bus_name, 0, 1)) return NULL;

    dbus_error_init (&error);
    Py_BEGIN_ALLOW_THREADS
    ret = dbus_bus_request_name(self->conn, bus_name, flags, &error);
    Py_END_ALLOW_THREADS
    if (ret == -1) return DBusException_ConsumeError (&error);

    return PyInt_FromLong(ret);
}

PyDoc_STRVAR(Bus_release_name__doc__, "");
static PyObject *
Bus_release_name (Connection *self, PyObject *args)
{
    const char *bus_name;
    int ret;
    DBusError error;

    if (!PyArg_ParseTuple(args, "s:release_name", &bus_name)) return NULL;

    dbus_error_init (&error);
    Py_BEGIN_ALLOW_THREADS
    ret = dbus_bus_release_name(self->conn, bus_name, &error);
    Py_END_ALLOW_THREADS
    if (ret == -1) return DBusException_ConsumeError (&error);

    return PyInt_FromLong(ret);
}

PyDoc_STRVAR (Bus_name_has_owner__doc__,
"name_has_owner(bus_name: str) -> bool\n\n"
"Return True if and only if the given bus name has an owner on this bus.\n");
static PyObject *
Bus_name_has_owner (Connection *self, PyObject *args)
{
    const char *bus_name;
    int ret;
    DBusError error;

    if (!PyArg_ParseTuple(args, "s:name_has_owner", &bus_name)) return NULL;
    dbus_error_init (&error);
    Py_BEGIN_ALLOW_THREADS
    ret = dbus_bus_name_has_owner(self->conn, bus_name, &error);
    Py_END_ALLOW_THREADS
    if (dbus_error_is_set (&error)) {
        return DBusException_ConsumeError (&error);
    }
    return PyBool_FromLong(ret);
}

PyDoc_STRVAR (Bus_add_match_string__doc__,
"add_match_string(rule: str)\n\n"
"Arrange for this application to receive messages on the bus that match\n"
"the given rule. This version will block and raises DBusException on error.\n");
static PyObject *
Bus_add_match_string (Connection *self, PyObject *args)
{
    const char *rule;
    DBusError error;

    if (!PyArg_ParseTuple(args, "s:add_match", &rule)) return NULL;
    dbus_error_init (&error);
    Py_BEGIN_ALLOW_THREADS
    dbus_bus_add_match (self->conn, rule, &error);
    Py_END_ALLOW_THREADS
    if (dbus_error_is_set (&error)) {
        return DBusException_ConsumeError (&error);
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR (Bus_add_match_string_non_blocking__doc__,
"add_match_string_non_blocking(rule: str)\n\n"
"Arrange for this application to receive messages on the bus that match\n"
"the given rule. This version does not block, but any errors will be\n"
"ignored.\n");
static PyObject *
Bus_add_match_string_non_blocking (Connection *self, PyObject *args)
{
    const char *rule;

    if (!PyArg_ParseTuple(args, "s:add_match", &rule)) return NULL;
    Py_BEGIN_ALLOW_THREADS
    dbus_bus_add_match (self->conn, rule, NULL);
    Py_END_ALLOW_THREADS
    Py_RETURN_NONE;
}

PyDoc_STRVAR (Bus_remove_match_string__doc__,
"remove_match_string_non_blocking(rule: str)\n\n"
"Remove the given match rule; if it has been added more than once,\n"
"remove one of the identical copies, leaving the others active.\n"
"This version blocks, and raises DBusException on error.\n");
static PyObject *
Bus_remove_match_string (Connection *self, PyObject *args)
{
    const char *rule;
    DBusError error;

    if (!PyArg_ParseTuple(args, "s:remove_match", &rule)) return NULL;
    dbus_error_init (&error);
    Py_BEGIN_ALLOW_THREADS
    dbus_bus_remove_match (self->conn, rule, &error);
    Py_END_ALLOW_THREADS
    if (dbus_error_is_set (&error)) {
        return DBusException_ConsumeError (&error);
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR (Bus_remove_match_string_non_blocking__doc__,
"remove_match_string_non_blocking(rule: str)\n\n"
"Remove the given match rule; if it has been added more than once,\n"
"remove one of the identical copies, leaving the others active.\n"
"This version does not block, but causes any errors to be ignored.\n");
static PyObject *
Bus_remove_match_string_non_blocking (Connection *self, PyObject *args)
{
    const char *rule;

    if (!PyArg_ParseTuple(args, "s:remove_match", &rule)) return NULL;
    Py_BEGIN_ALLOW_THREADS
    dbus_bus_remove_match (self->conn, rule, NULL);
    Py_END_ALLOW_THREADS
    Py_RETURN_NONE;
}

/* Bus type object ================================================== */

static struct PyMethodDef Bus_tp_methods[] = {
#define ENTRY(name, flags) {#name, (PyCFunction)Bus_##name, flags, Bus_##name##__doc__},
    ENTRY(get_unique_name, METH_NOARGS)
    ENTRY(get_unix_user, METH_VARARGS)
    ENTRY(start_service_by_name, METH_VARARGS)
    ENTRY(request_name, METH_VARARGS)
    ENTRY(release_name, METH_VARARGS)
    ENTRY(name_has_owner, METH_VARARGS)
    ENTRY(name_has_owner, METH_VARARGS)
    ENTRY(add_match_string, METH_VARARGS)
    ENTRY(add_match_string_non_blocking, METH_VARARGS)
    ENTRY(remove_match_string, METH_VARARGS)
    ENTRY(remove_match_string_non_blocking, METH_VARARGS)
#undef ENTRY
    {NULL},
};

/* TODO: Call this dbus.Bus rather than _dbus_bindings._Bus if it ever gets
 * all the functionality of the current dbus._dbus.Bus (mainly creation of
 * proxies).
 */
static PyTypeObject BusType = {
        PyObject_HEAD_INIT(NULL)
        0,                      /*ob_size*/
        "_dbus_bindings._Bus",  /*tp_name*/
        0,                      /*tp_basicsize*/
        0,                      /*tp_itemsize*/
        /* methods */
        (destructor)Connection_tp_dealloc,  /*tp_dealloc*/
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
        &ConnectionType,        /*tp_base*/
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

static inline int
init_bus_types (void)
{
    if (PyType_Ready (&BusType) < 0) return 0;
    return 1;
}

static inline int
insert_bus_types (PyObject *this_module)
{
    if (PyModule_AddObject (this_module, "_Bus",
                            (PyObject *)&BusType) < 0) return 0;
    return 1;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
