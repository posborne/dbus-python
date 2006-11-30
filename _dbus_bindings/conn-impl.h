/* Implementation of the _dbus_bindings Connection type, a Python wrapper
 * for DBusConnection. See also conn-methods-impl.h.
 *
 * Copyright (C) 2006 Collabora Ltd. <http://www.collabora.co.uk/>
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

/* Connection definition ============================================ */

PyDoc_STRVAR(Connection_tp_doc,
"A D-Bus connection.\n\n"
"Connection(address: str, mainloop=None) -> Connection\n"
);

typedef struct Connection {
    PyObject_HEAD
    DBusConnection *conn;
    /* A list of filter callbacks. */
    PyObject *filters;
    /* A dict mapping object paths to one of:
     * - tuples (unregister_callback or None, message_callback)
     * - None (meaning unregistration from libdbus is in progress and nobody
     *         should touch this entry til we're finished)
     */
    PyObject *object_paths;

    PyObject *weaklist;
} Connection;

static PyTypeObject ConnectionType;

static inline int Connection_Check(PyObject *o)
{
    return PyObject_TypeCheck(o, &ConnectionType);
}

/* Helpers ========================================================== */

static PyObject *Connection_ExistingFromDBusConnection(DBusConnection *);
static PyObject *Connection_GetObjectPathHandlers(Connection *, PyObject *);
static DBusHandlerResult Connection_HandleMessage(Connection *, Message *,
                                                  PyObject *);

static void
_object_path_unregister(DBusConnection *conn, void *user_data)
{
    PyGILState_STATE gil = PyGILState_Ensure();
    PyObject *tuple = NULL;
    Connection *conn_obj = NULL;
    PyObject *callable;

    conn_obj = (Connection *)Connection_ExistingFromDBusConnection(conn);
    if (!conn_obj) goto out;

    DBG("Connection at %p unregistering object path %s",
        conn_obj, PyString_AS_STRING((PyObject *)user_data));
    tuple = Connection_GetObjectPathHandlers(conn_obj, (PyObject *)user_data);
    if (!tuple) goto out;
    if (tuple == Py_None) goto out;

    DBG("%s", "... yes we have handlers for that object path");

    /* 0'th item is the unregisterer (if that's a word) */
    callable = PyTuple_GetItem(tuple, 0);
    if (callable && callable != Py_None) {
        DBG("%s", "... and we even have an unregisterer");
        /* any return from the unregisterer is ignored */
        Py_XDECREF(PyObject_CallFunctionObjArgs(callable, conn_obj, NULL));
    }
out:
    Py_XDECREF(conn_obj);
    Py_XDECREF(tuple);
    /* the user_data (a Python str) is no longer ref'd by the DBusConnection */
    Py_XDECREF((PyObject *)user_data);
    if (PyErr_Occurred()) {
        PyErr_Print();
    }
    PyGILState_Release(gil);
}

static DBusHandlerResult
_object_path_message(DBusConnection *conn, DBusMessage *message,
                     void *user_data)
{
    DBusHandlerResult ret;
    PyGILState_STATE gil = PyGILState_Ensure();
    Connection *conn_obj = NULL;
    PyObject *tuple = NULL;
    Message *msg_obj;
    PyObject *callable;             /* borrowed */

    dbus_message_ref(message);
    msg_obj = (Message *)Message_ConsumeDBusMessage(message);
    if (!msg_obj) {
        ret = DBUS_HANDLER_RESULT_NEED_MEMORY;
        goto out;
    }

    conn_obj = (Connection *)Connection_ExistingFromDBusConnection(conn);
    if (!conn_obj) {
        ret = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        goto out;
    }

    DBG("Connection at %p messaging object path %s",
        conn_obj, PyString_AS_STRING((PyObject *)user_data));
    DBG_DUMP_MESSAGE(message);
    tuple = Connection_GetObjectPathHandlers(conn_obj, (PyObject *)user_data);
    if (!tuple || tuple == Py_None) {
        ret = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        goto out;
    }

    DBG("%s", "... yes we have handlers for that object path");

    /* 1st item (0-based) is the message callback */
    callable = PyTuple_GetItem(tuple, 1);
    if (!callable) {
        DBG("%s", "... error getting message handler from tuple");
        ret = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    else if (callable == Py_None) {
        /* there was actually no handler after all */
        DBG("%s", "... but those handlers don't do messages");
        ret = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    else {
        DBG("%s", "... and we have a message handler for that object path");
        ret = Connection_HandleMessage(conn_obj, msg_obj, callable);
    }

out:
    Py_XDECREF(msg_obj);
    Py_XDECREF(conn_obj);
    Py_XDECREF(tuple);
    if (PyErr_Occurred()) {
        PyErr_Print();
    }
    PyGILState_Release(gil);
    return ret;
}

static const DBusObjectPathVTable _object_path_vtable = {
    _object_path_unregister,
    _object_path_message,
};

static DBusHandlerResult
_filter_message(DBusConnection *conn, DBusMessage *message, void *user_data)
{
    DBusHandlerResult ret;
    PyGILState_STATE gil = PyGILState_Ensure();
    Connection *conn_obj = NULL;
    PyObject *callable = NULL;
    Message *msg_obj;
#ifndef DBUS_PYTHON_DISABLE_CHECKS
    int i, size;
#endif

    dbus_message_ref(message);
    msg_obj = (Message *)Message_ConsumeDBusMessage(message);
    if (!msg_obj) {
        DBG("%s", "OOM while trying to construct Message");
        ret = DBUS_HANDLER_RESULT_NEED_MEMORY;
        goto out;
    }

    conn_obj = (Connection *)Connection_ExistingFromDBusConnection(conn);
    if (!conn_obj) {
        DBG("%s", "failed to traverse DBusConnection -> Connection weakref");
        ret = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        goto out;
    }

    /* The user_data is a pointer to a Python object. To avoid
     * cross-library reference cycles, the DBusConnection isn't allowed
     * to reference it. However, as long as the Connection is still
     * alive, its ->filters list owns a reference to the same Python
     * object, so the object should also still be alive.
     *
     * To ensure that this works, be careful whenever manipulating the
     * filters list! (always put things in the list *before* giving
     * them to libdbus, etc.)
     */
#ifdef DBUS_PYTHON_DISABLE_CHECKS
    callable = (PyObject *)user_data;
#else
    size = PyList_GET_SIZE(conn_obj->filters);
    for (i = 0; i < size; i++) {
        callable = PyList_GET_ITEM(conn_obj->filters, i);
        if (callable == user_data) {
            Py_INCREF(callable);
        }
        else {
            callable = NULL;
        }
    }

    if (!callable) {
        DBG("... filter %p has vanished from ->filters, so not calling it",
            user_data);
        ret = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        goto out;
    }
#endif

    ret = Connection_HandleMessage(conn_obj, msg_obj, callable);
out:
    Py_XDECREF(msg_obj);
    Py_XDECREF(conn_obj);
    Py_XDECREF(callable);
    PyGILState_Release(gil);
    return ret;
}

/* D-Bus Connection user data slot, containing an owned reference to either
 * the Connection, or a weakref to the Connection.
 */
static dbus_int32_t _connection_python_slot;

/* C API for main-loop hooks ======================================== */

/* Return a borrowed reference to the DBusConnection which underlies this
 * Connection. */
static DBusConnection *
Connection_BorrowDBusConnection(PyObject *self)
{
    if (!Connection_Check(self)) {
        PyErr_SetString(PyExc_TypeError, "A dbus.Connection is required");
        return NULL;
    }
    return ((Connection *)self)->conn;
}

/* Internal C API =================================================== */

/* Pass a message through a handler. */
static DBusHandlerResult
Connection_HandleMessage(Connection *conn, Message *msg, PyObject *callable)
{
    PyObject *obj = PyObject_CallFunctionObjArgs(callable, conn, msg,
                                                 NULL);
    if (obj == Py_None) {
        DBG("%p: OK, handler %p returned None", conn, callable);
        Py_DECREF(obj);
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    else if (obj == Py_NotImplemented) {
        DBG("%p: handler %p returned NotImplemented, continuing",
            conn, callable);
        Py_DECREF(obj);
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    else if (!obj) {
        if (PyErr_ExceptionMatches(PyExc_MemoryError)) {
            DBG_EXC("%p: handler %p caused OOM", conn, callable);
            PyErr_Clear();
            return DBUS_HANDLER_RESULT_NEED_MEMORY;
        }
        DBG_EXC("%p: handler %p raised exception", conn, callable);
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    else {
        long i = PyInt_AsLong(obj);
        DBG("%p: handler %p returned %ld", conn, callable, i);
        Py_DECREF(obj);
        if (i == -1 && PyErr_Occurred()) {
            PyErr_SetString(PyExc_TypeError, "Return from D-Bus message "
                            "handler callback should be None, "
                            "NotImplemented or integer");
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }
        else if (i == DBUS_HANDLER_RESULT_HANDLED ||
            i == DBUS_HANDLER_RESULT_NOT_YET_HANDLED ||
            i == DBUS_HANDLER_RESULT_NEED_MEMORY) {
            return i;
        }
        else {
            PyErr_Format(PyExc_ValueError, "Integer return from "
                        "D-Bus message handler callback should "
                        "be a DBUS_HANDLER_RESULT_... constant, "
                        "not %d", (int)i);
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }
    }
}

/* On KeyError or if unregistration is in progress, return None. */
static PyObject *
Connection_GetObjectPathHandlers(Connection *self, PyObject *path)
{
    PyObject *callbacks = PyDict_GetItem(self->object_paths, path);
    if (!callbacks) {
        if (PyErr_ExceptionMatches(PyExc_KeyError)) {
            PyErr_Clear();
            Py_RETURN_NONE;
        }
    }
    Py_INCREF(callbacks);
    return callbacks;
}

/* Return a new reference to a Python Connection or subclass corresponding
 * to the DBusConnection conn. For use in callbacks.
 *
 * Raises AssertionError if the DBusConnection does not have a Connection.
 */
static PyObject *
Connection_ExistingFromDBusConnection(DBusConnection *conn)
{
    PyObject *self, *ref;

    Py_BEGIN_ALLOW_THREADS
    ref = (PyObject *)dbus_connection_get_data(conn,
                                               _connection_python_slot);
    Py_END_ALLOW_THREADS
    if (ref) {
        self = PyWeakref_GetObject(ref);   /* still a borrowed ref */
        if (self && self != Py_None && Connection_Check(self)) {
            Py_INCREF(self);
            return self;
        }
    }

    PyErr_SetString(PyExc_AssertionError,
                    "D-Bus connection does not have a Connection "
                    "instance associated with it");
    return NULL;
}

/* Return a new reference to a Python Connection or subclass (given by cls)
 * corresponding to the DBusConnection conn, which must have been newly
 * created. For use by the Connection and Bus constructors.
 *
 * Raises AssertionError if the DBusConnection already has a Connection.
 */
static PyObject *
Connection_NewConsumingDBusConnection(PyTypeObject *cls,
                                      DBusConnection *conn,
                                      PyObject *mainloop)
{
    Connection *self = NULL;
    PyObject *ref;
    dbus_bool_t ok;

    Py_BEGIN_ALLOW_THREADS
    ref = (PyObject *)dbus_connection_get_data(conn,
                                               _connection_python_slot);
    Py_END_ALLOW_THREADS
    if (ref) {
        self = (Connection *)PyWeakref_GetObject(ref);
        ref = NULL;
        if (self && (PyObject *)self != Py_None) {
            self = NULL;
            PyErr_SetString(PyExc_AssertionError,
                            "Newly created D-Bus connection already has a "
                            "Connection instance associated with it");
            return NULL;
        }
    }
    ref = NULL;

    if (!mainloop || mainloop == Py_None) {
        mainloop = default_main_loop;
        if (!mainloop || mainloop == Py_None) {
            PyErr_SetString(PyExc_ValueError,
                            "D-Bus connections must be attached to a main "
                            "loop by passing mainloop=... to the constructor "
                            "or calling dbus.Bus.set_default_main_loop(...)");
            goto err;
        }
    }
    /* Make sure there's a ref to the main loop (in case someone changes the
     * default) */
    Py_INCREF(mainloop);

    DBG("Constructing Connection from DBusConnection at %p", conn);

    self = (Connection *)(cls->tp_alloc(cls, 0));
    if (!self) goto err;

    self->conn = NULL;
    self->filters = PyList_New(0);
    if (!self->filters) goto err;
    self->object_paths = PyDict_New();
    if (!self->object_paths) goto err;

    ref = PyWeakref_NewRef((PyObject *)self, NULL);
    if (!ref) goto err;

    Py_BEGIN_ALLOW_THREADS
    ok = dbus_connection_set_data(conn, _connection_python_slot,
                                  (void *)ref,
                                  (DBusFreeFunction)Glue_TakeGILAndXDecref);
    Py_END_ALLOW_THREADS

    if (!ok) {
        PyErr_NoMemory();
        goto err;
    }

    self->conn = conn;

    if (!dbus_python_set_up_connection((PyObject *)self, mainloop)) {
        goto err;
    }

    Py_DECREF(mainloop);

    return (PyObject *)self;

err:
    DBG("Failed to construct Connection from DBusConnection at %p", conn);
    Py_XDECREF(mainloop);
    Py_XDECREF(self);
    Py_XDECREF(ref);
    if (conn) {
        Py_BEGIN_ALLOW_THREADS
        dbus_connection_close(conn);
        dbus_connection_unref(conn);
        Py_END_ALLOW_THREADS
    }
    return NULL;
}

/* Connection type-methods ========================================== */

/* "Constructor" (the real constructor is Connection_NewFromDBusConnection,
 * to which this delegates). */
static PyObject *
Connection_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    DBusConnection *conn;
    const char *address;
    DBusError error;
    PyObject *self, *mainloop = NULL;
    static char *argnames[] = {"address", "mainloop", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|O", argnames,
                                     &address, &mainloop)) {
        return NULL;
    }

    dbus_error_init(&error);

    /* We always open a private connection (at the libdbus level). Sharing
     * is done in Python, to keep things simple. */
    Py_BEGIN_ALLOW_THREADS
    conn = dbus_connection_open_private(address, &error);
    Py_END_ALLOW_THREADS

    if (!conn) {
        DBusException_ConsumeError(&error);
        return NULL;
    }
    self = Connection_NewConsumingDBusConnection(cls, conn, mainloop);

    return self;
}

/* Destructor */
static void Connection_tp_dealloc(Connection *self)
{
    DBusConnection *conn = self->conn;
    self->conn = NULL;

    DBG("Deallocating Connection at %p (DBusConnection at %p)", self, conn);

    if (conn) {
        Py_BEGIN_ALLOW_THREADS
        dbus_connection_close(conn);
        Py_END_ALLOW_THREADS

        dbus_connection_unref(conn);
    }

    Py_XDECREF(self->filters);
    Py_XDECREF(self->object_paths);
    (self->ob_type->tp_free)((PyObject *)self);
}

/* Connection_tp_methods */
#include "conn-methods-impl.h"

/* Connection type object =========================================== */

static PyTypeObject ConnectionType = {
    PyObject_HEAD_INIT(NULL)
    0,                      /*ob_size*/
    "_dbus_bindings.Connection", /*tp_name*/
    sizeof(Connection),     /*tp_basicsize*/
    0,                      /*tp_itemsize*/
    /* methods */
    (destructor)Connection_tp_dealloc,
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_WEAKREFS | Py_TPFLAGS_BASETYPE,
    Connection_tp_doc,      /*tp_doc*/
    0,                      /*tp_traverse*/
    0,                      /*tp_clear*/
    0,                      /*tp_richcompare*/
    offsetof(Connection, weaklist),   /*tp_weaklistoffset*/
    0,                      /*tp_iter*/
    0,                      /*tp_iternext*/
    Connection_tp_methods,  /*tp_methods*/
    0,                      /*tp_members*/
    0,                      /*tp_getset*/
    0,                      /*tp_base*/
    0,                      /*tp_dict*/
    0,                      /*tp_descr_get*/
    0,                      /*tp_descr_set*/
    0,                      /*tp_dictoffset*/
    0,                      /*tp_init*/
    0,                      /*tp_alloc*/
    Connection_tp_new,      /*tp_new*/
    0,                      /*tp_free*/
    0,                      /*tp_is_gc*/
};

static inline dbus_bool_t
init_conn_types(void)
{
    default_main_loop = NULL;

    /* Get a slot to store our weakref on DBus Connections */
    _connection_python_slot = -1;
    if (!dbus_connection_allocate_data_slot(&_connection_python_slot))
        return FALSE;
    if (PyType_Ready(&ConnectionType) < 0)
        return FALSE;
    return TRUE;
}

static inline dbus_bool_t
insert_conn_types(PyObject *this_module)
{
    if (PyModule_AddObject(this_module, "Connection",
                           (PyObject *)&ConnectionType) < 0) return FALSE;
    return TRUE;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
