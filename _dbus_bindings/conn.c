/* Implementation of the _dbus_bindings Connection type, a Python wrapper
 * for DBusConnection. See also conn-methods.c.
 *
 * Copyright (C) 2006-2008 Collabora Ltd. <http://www.collabora.co.uk/>
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

/* Connection definition ============================================ */

PyDoc_STRVAR(Connection_tp_doc,
"A D-Bus connection.\n"
"\n"
"::\n"
"\n"
"   Connection(address, mainloop=None) -> Connection\n"
);

/* D-Bus Connection user data slot, containing an owned reference to either
 * the Connection, or a weakref to the Connection.
 */
static dbus_int32_t _connection_python_slot;

/* C API for main-loop hooks ======================================== */

/* Return a borrowed reference to the DBusConnection which underlies this
 * Connection. */
DBusConnection *
DBusPyConnection_BorrowDBusConnection(PyObject *self)
{
    DBusConnection *dbc;

    TRACE(self);
    if (!DBusPyConnection_Check(self)) {
        PyErr_SetString(PyExc_TypeError, "A dbus.Connection is required");
        return NULL;
    }
    dbc = ((Connection *)self)->conn;
    if (!dbc) {
        PyErr_SetString(PyExc_RuntimeError, "Connection is in an invalid "
                        "state: no DBusConnection");
        return NULL;
    }
    return dbc;
}

/* Internal C API =================================================== */

/* Pass a message through a handler. */
DBusHandlerResult
DBusPyConnection_HandleMessage(Connection *conn,
                               PyObject *msg,
                               PyObject *callable)
{
    PyObject *obj;

    TRACE(conn);
    obj = PyObject_CallFunctionObjArgs(callable, conn, msg,
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
PyObject *
DBusPyConnection_GetObjectPathHandlers(PyObject *self, PyObject *path)
{
    PyObject *callbacks;

    TRACE(self);
    callbacks = PyDict_GetItem(((Connection *)self)->object_paths, path);
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
PyObject *
DBusPyConnection_ExistingFromDBusConnection(DBusConnection *conn)
{
    PyObject *self, *ref;

    Py_BEGIN_ALLOW_THREADS
    ref = (PyObject *)dbus_connection_get_data(conn,
                                               _connection_python_slot);
    Py_END_ALLOW_THREADS
    if (ref) {
        DBG("(DBusConnection *)%p has weak reference at %p", conn, ref);
        self = PyWeakref_GetObject(ref);   /* still a borrowed ref */
        if (self && self != Py_None && DBusPyConnection_Check(self)) {
            DBG("(DBusConnection *)%p has weak reference at %p pointing to %p",
                conn, ref, self);
            TRACE(self);
            Py_INCREF(self);
            TRACE(self);
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
DBusPyConnection_NewConsumingDBusConnection(PyTypeObject *cls,
                                            DBusConnection *conn,
                                            PyObject *mainloop)
{
    Connection *self = NULL;
    PyObject *ref;
    dbus_bool_t ok;

    DBG("%s(cls=%p, conn=%p, mainloop=%p)", __func__, cls, conn, mainloop);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(conn);

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
            DBG("%s() fail - assertion failed, DBusPyConn has a DBusConn already", __func__);
            DBG_WHEREAMI;
            return NULL;
        }
    }
    ref = NULL;

    /* Change mainloop from a borrowed reference to an owned reference */
    if (!mainloop || mainloop == Py_None) {
        mainloop = dbus_py_get_default_main_loop();
        if (!mainloop)
            goto err;
    }
    else {
        Py_INCREF(mainloop);
    }

    DBG("Constructing Connection from DBusConnection at %p", conn);

    self = (Connection *)(cls->tp_alloc(cls, 0));
    if (!self) goto err;
    TRACE(self);

    DBG_WHEREAMI;

    self->has_mainloop = (mainloop != Py_None);
    self->conn = NULL;
    self->filters = PyList_New(0);
    if (!self->filters) goto err;
    self->object_paths = PyDict_New();
    if (!self->object_paths) goto err;

    ref = PyWeakref_NewRef((PyObject *)self, NULL);
    if (!ref) goto err;
    DBG("Created weak ref %p to (Connection *)%p for (DBusConnection *)%p",
        ref, self, conn);

    Py_BEGIN_ALLOW_THREADS
    ok = dbus_connection_set_data(conn, _connection_python_slot,
                                  (void *)ref,
                                  (DBusFreeFunction)dbus_py_take_gil_and_xdecref);
    Py_END_ALLOW_THREADS

    if (ok) {
        DBG("Attached weak ref %p ((Connection *)%p) to (DBusConnection *)%p",
            ref, self, conn);
        ref = NULL;     /* don't DECREF it - the DBusConnection owns it now */
    }
    else {
        DBG("Failed to attached weak ref %p ((Connection *)%p) to "
            "(DBusConnection *)%p - will dispose of it", ref, self, conn);
        PyErr_NoMemory();
        goto err;
    }

    DBUS_PY_RAISE_VIA_GOTO_IF_FAIL(conn, err);
    self->conn = conn;
    /* the DBusPyConnection will close it now */
    conn = NULL;

    if (self->has_mainloop
        && !dbus_py_set_up_connection((PyObject *)self, mainloop)) {
        goto err;
    }

    Py_DECREF(mainloop);

    DBG("%s() -> %p", __func__, self);
    TRACE(self);
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
    DBG("%s() fail", __func__);
    DBG_WHEREAMI;
    return NULL;
}

/* Connection type-methods ========================================== */

/* Constructor */
static PyObject *
Connection_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    DBusConnection *conn;
    const char *address;
    PyObject *address_or_conn;
    DBusError error;
    PyObject *self, *mainloop = NULL;
    static char *argnames[] = {"address", "mainloop", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O", argnames,
                                     &address_or_conn, &mainloop)) {
        return NULL;
    }

    if (DBusPyLibDBusConnection_CheckExact(address_or_conn)) {
        DBusPyLibDBusConnection *wrapper =
            (DBusPyLibDBusConnection *) address_or_conn;

        DBUS_PY_RAISE_VIA_NULL_IF_FAIL(wrapper->conn);

        conn = dbus_connection_ref (wrapper->conn);
    }
    else if ((address = PyString_AsString(address_or_conn)) != NULL) {
        dbus_error_init(&error);

        /* We always open a private connection (at the libdbus level). Sharing
         * is done in Python, to keep things simple. */
        Py_BEGIN_ALLOW_THREADS
        conn = dbus_connection_open_private(address, &error);
        Py_END_ALLOW_THREADS

        if (!conn) {
            DBusPyException_ConsumeError(&error);
            return NULL;
        }
    }
    else {
        return NULL;
    }

    self = DBusPyConnection_NewConsumingDBusConnection(cls, conn, mainloop);
    TRACE(self);

    return self;
}

/* Destructor */
static void Connection_tp_dealloc(Connection *self)
{
    DBusConnection *conn = self->conn;
    PyObject *et, *ev, *etb;
    PyObject *filters = self->filters;
    PyObject *object_paths = self->object_paths;

    /* avoid clobbering any pending exception */
    PyErr_Fetch(&et, &ev, &etb);

    if (self->weaklist) {
        PyObject_ClearWeakRefs((PyObject *)self);
    }

    TRACE(self);
    DBG("Deallocating Connection at %p (DBusConnection at %p)", self, conn);
    DBG_WHEREAMI;

    DBG("Connection at %p: deleting callbacks", self);
    self->filters = NULL;
    Py_XDECREF(filters);
    self->object_paths = NULL;
    Py_XDECREF(object_paths);

    if (conn) {
        /* Might trigger callbacks if we're unlucky... */
        DBG("Connection at %p has a conn, closing it...", self);
        Py_BEGIN_ALLOW_THREADS
        dbus_connection_close(conn);
        Py_END_ALLOW_THREADS
    }

    /* make sure to do this last to preserve the invariant that
     * self->conn is always non-NULL for any referenced Connection
     * (until the filters and object paths were freed, we might have been
     * in a reference cycle!)
     */
    DBG("Connection at %p: nulling self->conn", self);
    self->conn = NULL;

    if (conn) {
        DBG("Connection at %p: unreffing conn", self);
        dbus_connection_unref(conn);
    }

    DBG("Connection at %p: freeing self", self);
    PyErr_Restore(et, ev, etb);
    (self->ob_type->tp_free)((PyObject *)self);
}

/* Connection type object =========================================== */

PyTypeObject DBusPyConnection_Type = {
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
    DBusPyConnection_tp_methods,  /*tp_methods*/
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

dbus_bool_t
dbus_py_init_conn_types(void)
{
    /* Get a slot to store our weakref on DBus Connections */
    _connection_python_slot = -1;
    if (!dbus_connection_allocate_data_slot(&_connection_python_slot))
        return FALSE;
    if (PyType_Ready(&DBusPyConnection_Type) < 0)
        return FALSE;
    return TRUE;
}

dbus_bool_t
dbus_py_insert_conn_types(PyObject *this_module)
{
    if (PyModule_AddObject(this_module, "Connection",
                           (PyObject *)&DBusPyConnection_Type) < 0) return FALSE;
    return TRUE;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
