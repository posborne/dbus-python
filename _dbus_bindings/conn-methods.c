/* Implementation of normal Python-accessible methods on the _dbus_bindings
 * Connection type; separated out to keep the file size manageable.
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

static DBusHandlerResult
_filter_message(DBusConnection *conn, DBusMessage *message, void *user_data)
{
    DBusHandlerResult ret;
    PyGILState_STATE gil = PyGILState_Ensure();
    Connection *conn_obj = NULL;
    PyObject *callable = NULL;
    PyObject *msg_obj;
#ifndef DBUS_PYTHON_DISABLE_CHECKS
    Py_ssize_t i, size;
#endif

    dbus_message_ref(message);
    msg_obj = DBusPyMessage_ConsumeDBusMessage(message);
    if (!msg_obj) {
        DBG("%s", "OOM while trying to construct Message");
        ret = DBUS_HANDLER_RESULT_NEED_MEMORY;
        goto out;
    }

    conn_obj = (Connection *)DBusPyConnection_ExistingFromDBusConnection(conn);
    if (!conn_obj) {
        DBG("%s", "failed to traverse DBusConnection -> Connection weakref");
        ret = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        goto out;
    }
    TRACE(conn_obj);

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

    ret = DBusPyConnection_HandleMessage(conn_obj, msg_obj, callable);
out:
    Py_XDECREF(msg_obj);
    Py_XDECREF(conn_obj);
    Py_XDECREF(callable);
    PyGILState_Release(gil);
    return ret;
}

PyDoc_STRVAR(Connection__require_main_loop__doc__,
"_require_main_loop()\n\n"
"Raise an exception if this Connection is not bound to any main loop -\n"
"in this state, asynchronous calls, receiving signals and exporting objects\n"
"will not work.\n"
"\n"
"`dbus.mainloop.NULL_MAIN_LOOP` is treated like a valid main loop - if you're\n"
"using that, you presumably know what you're doing.\n");
static PyObject *
Connection__require_main_loop (Connection *self, PyObject *args UNUSED)
{
    if (!self->has_mainloop) {
        PyErr_SetString(PyExc_RuntimeError,
                        "To make asynchronous calls, receive signals or "
                        "export objects, D-Bus connections must be attached "
                        "to a main loop by passing mainloop=... to the "
                        "constructor or calling "
                        "dbus.set_default_main_loop(...)");
        return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(Connection_close__doc__,
"close()\n\n"
"Close the connection.");
static PyObject *
Connection_close (Connection *self, PyObject *args UNUSED)
{
    TRACE(self);
    /* Because the user explicitly asked to close the connection, we'll even
    let them close shared connections. */
    if (self->conn) {
        Py_BEGIN_ALLOW_THREADS
        dbus_connection_close(self->conn);
        Py_END_ALLOW_THREADS
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(Connection_get_is_connected__doc__,
"get_is_connected() -> bool\n\n"
"Return true if this Connection is connected.\n");
static PyObject *
Connection_get_is_connected (Connection *self, PyObject *args UNUSED)
{
    dbus_bool_t ret;

    TRACE(self);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(self->conn);
    Py_BEGIN_ALLOW_THREADS
    ret = dbus_connection_get_is_connected(self->conn);
    Py_END_ALLOW_THREADS
    return PyBool_FromLong(ret);
}

PyDoc_STRVAR(Connection_get_is_authenticated__doc__,
"get_is_authenticated() -> bool\n\n"
"Return true if this Connection was ever authenticated.\n");
static PyObject *
Connection_get_is_authenticated (Connection *self, PyObject *args UNUSED)
{
    dbus_bool_t ret;

    TRACE(self);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(self->conn);
    Py_BEGIN_ALLOW_THREADS
    ret = dbus_connection_get_is_authenticated(self->conn);
    Py_END_ALLOW_THREADS
    return PyBool_FromLong(ret);
}

PyDoc_STRVAR(Connection_set_exit_on_disconnect__doc__,
"set_exit_on_disconnect(bool)\n\n"
"Set whether the C function ``_exit`` will be called when this Connection\n"
"becomes disconnected. This will cause the program to exit without calling\n"
"any cleanup code or exit handlers.\n"
"\n"
"The default is for this feature to be disabled for Connections and enabled\n"
"for Buses.\n");
static PyObject *
Connection_set_exit_on_disconnect (Connection *self, PyObject *args)
{
    int exit_on_disconnect;

    TRACE(self);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(self->conn);
    if (!PyArg_ParseTuple(args, "i:set_exit_on_disconnect",
                          &exit_on_disconnect)) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    dbus_connection_set_exit_on_disconnect(self->conn,
                                           exit_on_disconnect ? 1 : 0);
    Py_END_ALLOW_THREADS
    Py_RETURN_NONE;
}

PyDoc_STRVAR(Connection_send_message__doc__,
"send_message(msg) -> long\n\n"
"Queue the given message for sending, and return the message serial number.\n"
"\n"
":Parameters:\n"
"   `msg` : dbus.lowlevel.Message\n"
"       The message to be sent.\n"
);
static PyObject *
Connection_send_message(Connection *self, PyObject *args)
{
    dbus_bool_t ok;
    PyObject *obj;
    DBusMessage *msg;
    dbus_uint32_t serial;

    TRACE(self);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(self->conn);
    if (!PyArg_ParseTuple(args, "O", &obj)) return NULL;

    msg = DBusPyMessage_BorrowDBusMessage(obj);
    if (!msg) return NULL;

    Py_BEGIN_ALLOW_THREADS
    ok = dbus_connection_send(self->conn, msg, &serial);
    Py_END_ALLOW_THREADS

    if (!ok) {
        return PyErr_NoMemory();
    }

    return PyLong_FromUnsignedLong(serial);
}

/* The timeout is in seconds here, since that's conventional in Python. */
PyDoc_STRVAR(Connection_send_message_with_reply__doc__,
"send_message_with_reply(msg, reply_handler, timeout_s=-1, "
"require_main_loop=False) -> dbus.lowlevel.PendingCall\n\n"
"Queue the message for sending; expect a reply via the returned PendingCall,\n"
"which can also be used to cancel the pending call.\n"
"\n"
":Parameters:\n"
"   `msg` : dbus.lowlevel.Message\n"
"       The message to be sent\n"
"   `reply_handler` : callable\n"
"       Asynchronous reply handler: will be called with one positional\n"
"       parameter, a Message instance representing the reply.\n"
"   `timeout_s` : float\n"
"       If the reply takes more than this many seconds, a timeout error\n"
"       will be created locally and raised instead. If this timeout is\n"
"       negative (default), a sane default (supplied by libdbus) is used.\n"
"   `require_main_loop` : bool\n"
"       If True, raise RuntimeError if this Connection does not have a main\n"
"       loop configured. If False (default) and there is no main loop, you are\n"
"       responsible for calling block() on the PendingCall.\n"
"\n"
);
static PyObject *
Connection_send_message_with_reply(Connection *self, PyObject *args, PyObject *kw)
{
    dbus_bool_t ok;
    double timeout_s = -1.0;
    int timeout_ms;
    PyObject *obj, *callable;
    DBusMessage *msg;
    DBusPendingCall *pending;
    int require_main_loop = 0;
    static char *argnames[] = {"msg", "reply_handler", "timeout_s",
                               "require_main_loop", NULL};

    TRACE(self);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(self->conn);
    if (!PyArg_ParseTupleAndKeywords(args, kw,
                                     "OO|di:send_message_with_reply",
                                     argnames,
                                     &obj, &callable, &timeout_s,
                                     &require_main_loop)) {
        return NULL;
    }
    if (require_main_loop && !Connection__require_main_loop(self, NULL)) {
        return NULL;
    }

    msg = DBusPyMessage_BorrowDBusMessage(obj);
    if (!msg) return NULL;

    if (timeout_s < 0) {
        timeout_ms = -1;
    }
    else {
        if (timeout_s > ((double)INT_MAX) / 1000.0) {
            PyErr_SetString(PyExc_ValueError, "Timeout too long");
            return NULL;
        }
        timeout_ms = (int)(timeout_s * 1000.0);
    }

    Py_BEGIN_ALLOW_THREADS
    ok = dbus_connection_send_with_reply(self->conn, msg, &pending,
                                         timeout_ms);
    Py_END_ALLOW_THREADS

    if (!ok) {
        return PyErr_NoMemory();
    }

    if (!pending) {
        /* connection is disconnected (doesn't return FALSE!) */
        return DBusPyException_SetString ("Connection is disconnected - "
                                          "unable to make method call");
    }

    return DBusPyPendingCall_ConsumeDBusPendingCall(pending, callable);
}

/* Again, the timeout is in seconds, since that's conventional in Python. */
PyDoc_STRVAR(Connection_send_message_with_reply_and_block__doc__,
"send_message_with_reply_and_block(msg, timeout_s=-1)"
" -> dbus.lowlevel.Message\n\n"
"Send the message and block while waiting for a reply.\n"
"\n"
"This does not re-enter the main loop, so it can lead to a deadlock, if\n"
"the called method tries to make a synchronous call to a method in this\n"
"application. As such, it's probably a bad idea.\n"
"\n"
":Parameters:\n"
"   `msg` : dbus.lowlevel.Message\n"
"       The message to be sent\n"
"   `timeout_s` : float\n"
"       If the reply takes more than this many seconds, a timeout error\n"
"       will be created locally and raised instead. If this timeout is\n"
"       negative (default), a sane default (supplied by libdbus) is used.\n"
":Returns:\n"
"   A `dbus.lowlevel.Message` instance (probably a `dbus.lowlevel.MethodReturnMessage`) on success\n"
":Raises dbus.DBusException:\n"
"   On error (including if the reply arrives but is an\n"
"   error message)\n"
"\n"
);
static PyObject *
Connection_send_message_with_reply_and_block(Connection *self, PyObject *args)
{
    double timeout_s = -1.0;
    int timeout_ms;
    PyObject *obj;
    DBusMessage *msg, *reply;
    DBusError error;

    TRACE(self);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(self->conn);
    if (!PyArg_ParseTuple(args, "O|d:send_message_with_reply_and_block", &obj,
                          &timeout_s)) {
        return NULL;
    }

    msg = DBusPyMessage_BorrowDBusMessage(obj);
    if (!msg) return NULL;

    if (timeout_s < 0) {
        timeout_ms = -1;
    }
    else {
        if (timeout_s > ((double)INT_MAX) / 1000.0) {
            PyErr_SetString(PyExc_ValueError, "Timeout too long");
            return NULL;
        }
        timeout_ms = (int)(timeout_s * 1000.0);
    }

    dbus_error_init(&error);
    Py_BEGIN_ALLOW_THREADS
    reply = dbus_connection_send_with_reply_and_block(self->conn, msg,
                                                      timeout_ms, &error);
    Py_END_ALLOW_THREADS

    /* FIXME: if we instead used send_with_reply and blocked on the resulting
     * PendingCall, then we could get all args from the error, not just
     * the first */
    if (!reply) {
        return DBusPyException_ConsumeError(&error);
    }
    return DBusPyMessage_ConsumeDBusMessage(reply);
}

PyDoc_STRVAR(Connection_flush__doc__,
"flush()\n\n"
"Block until the outgoing message queue is empty.\n");
static PyObject *
Connection_flush (Connection *self, PyObject *args UNUSED)
{
    TRACE(self);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(self->conn);
    Py_BEGIN_ALLOW_THREADS
    dbus_connection_flush (self->conn);
    Py_END_ALLOW_THREADS
    Py_RETURN_NONE;
}

/* Unsupported:
 * dbus_connection_preallocate_send
 * dbus_connection_free_preallocated_send
 * dbus_connection_send_preallocated
 * dbus_connection_borrow_message
 * dbus_connection_return_message
 * dbus_connection_steal_borrowed_message
 * dbus_connection_pop_message
 */

/* Non-main-loop handling not yet implemented: */
    /* dbus_connection_read_write_dispatch */
    /* dbus_connection_read_write */

/* Main loop handling not yet implemented: */
    /* dbus_connection_get_dispatch_status */
    /* dbus_connection_dispatch */
    /* dbus_connection_set_watch_functions */
    /* dbus_connection_set_timeout_functions */
    /* dbus_connection_set_wakeup_main_function */
    /* dbus_connection_set_dispatch_status_function */

/* Normally in Python this would be called fileno(), but I don't want to
 * encourage people to select() on it */
PyDoc_STRVAR(Connection_get_unix_fd__doc__,
"get_unix_fd() -> int or None\n\n"
"Get the connection's UNIX file descriptor, if any.\n\n"
"This can be used for SELinux access control checks with ``getpeercon()``\n"
"for example. **Do not** read or write to the file descriptor, or try to\n"
"``select()`` on it.\n");
static PyObject *
Connection_get_unix_fd (Connection *self, PyObject *unused UNUSED)
{
    int fd;
    dbus_bool_t ok;

    TRACE(self);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(self->conn);
    Py_BEGIN_ALLOW_THREADS
    ok = dbus_connection_get_unix_fd (self->conn, &fd);
    Py_END_ALLOW_THREADS
    if (!ok) Py_RETURN_NONE;
    return PyInt_FromLong(fd);
}

PyDoc_STRVAR(Connection_get_peer_unix_user__doc__,
"get_peer_unix_user() -> long or None\n\n"
"Get the UNIX user ID at the other end of the connection, if it has been\n"
"authenticated. Return None if this is a non-UNIX platform or the\n"
"connection has not been authenticated.\n");
static PyObject *
Connection_get_peer_unix_user (Connection *self, PyObject *unused UNUSED)
{
    unsigned long uid;
    dbus_bool_t ok;

    TRACE(self);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(self->conn);
    Py_BEGIN_ALLOW_THREADS
    ok = dbus_connection_get_unix_user (self->conn, &uid);
    Py_END_ALLOW_THREADS
    if (!ok) Py_RETURN_NONE;
    return PyLong_FromUnsignedLong (uid);
}

PyDoc_STRVAR(Connection_get_peer_unix_process_id__doc__,
"get_peer_unix_process_id() -> long or None\n\n"
"Get the UNIX process ID at the other end of the connection, if it has been\n"
"authenticated. Return None if this is a non-UNIX platform or the\n"
"connection has not been authenticated.\n");
static PyObject *
Connection_get_peer_unix_process_id (Connection *self, PyObject *unused UNUSED)
{
    unsigned long pid;
    dbus_bool_t ok;

    TRACE(self);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(self->conn);
    Py_BEGIN_ALLOW_THREADS
    ok = dbus_connection_get_unix_process_id (self->conn, &pid);
    Py_END_ALLOW_THREADS
    if (!ok) Py_RETURN_NONE;
    return PyLong_FromUnsignedLong (pid);
}

/* TODO: wrap dbus_connection_set_unix_user_function Pythonically */

PyDoc_STRVAR(Connection_add_message_filter__doc__,
"add_message_filter(callable)\n\n"
"Add the given message filter to the internal list.\n\n"
"Filters are handlers that are run on all incoming messages, prior to the\n"
"objects registered to handle object paths.\n"
"\n"
"Filters are run in the order that they were added. The same handler can\n"
"be added as a filter more than once, in which case it will be run more\n"
"than once. Filters added during a filter callback won't be run on the\n"
"message being processed.\n"
);
static PyObject *
Connection_add_message_filter(Connection *self, PyObject *callable)
{
    dbus_bool_t ok;

    TRACE(self);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(self->conn);
    /* The callable must be referenced by ->filters *before* it is
     * given to libdbus, which does not own a reference to it.
     */
    if (PyList_Append(self->filters, callable) < 0) {
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    ok = dbus_connection_add_filter(self->conn, _filter_message, callable,
                                    NULL);
    Py_END_ALLOW_THREADS

    if (!ok) {
        Py_XDECREF(PyObject_CallMethod(self->filters, "remove", "(O)",
                                       callable));
        PyErr_NoMemory();
        return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(Connection_remove_message_filter__doc__,
"remove_message_filter(callable)\n\n"
"Remove the given message filter (see `add_message_filter` for details).\n"
"\n"
":Raises LookupError:\n"
"   The given callable is not among the registered filters\n");
static PyObject *
Connection_remove_message_filter(Connection *self, PyObject *callable)
{
    PyObject *obj;

    TRACE(self);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(self->conn);
    /* It's safe to do this before removing it from libdbus, because
     * the presence of callable in our arguments means we have a ref
     * to it. */
    obj = PyObject_CallMethod(self->filters, "remove", "(O)", callable);
    if (!obj) return NULL;
    Py_DECREF(obj);

    Py_BEGIN_ALLOW_THREADS
    dbus_connection_remove_filter(self->conn, _filter_message, callable);
    Py_END_ALLOW_THREADS

    Py_RETURN_NONE;
}

/* dbus_connection_set_change_sigpipe - sets global state */

/* Maxima. Does Python code ever need to manipulate these?
 * OTOH they're easy to wrap */
    /* dbus_connection_set_max_message_size */
    /* dbus_connection_get_max_message_size */
    /* dbus_connection_set_max_received_size */
    /* dbus_connection_get_max_received_size */

/* dbus_connection_get_outgoing_size - almost certainly unneeded */

PyDoc_STRVAR(new_for_bus__doc__,
"Connection._new_for_bus([address: str or int]) -> Connection\n"
"\n"
"If the address is an int it must be one of the constants BUS_SESSION,\n"
"BUS_SYSTEM, BUS_STARTER; if a string, it must be a D-Bus address.\n"
"The default is BUS_SESSION.\n"
);

PyDoc_STRVAR(get_unique_name__doc__,
"get_unique_name() -> str\n\n"
"Return this application's unique name on this bus.\n"
"\n"
":Raises DBusException: if the connection has no unique name yet\n"
"   (for Bus objects this can't happen, for peer-to-peer connections\n"
"   this means you haven't called `set_unique_name`)\n");

PyDoc_STRVAR(set_unique_name__doc__,
"set_unique_name(str)\n\n"
"Set this application's unique name on this bus. Raise ValueError if it has\n"
"already been set.\n");

struct PyMethodDef DBusPyConnection_tp_methods[] = {
#define ENTRY(name, flags) {#name, (PyCFunction)Connection_##name, flags, Connection_##name##__doc__}
    ENTRY(_require_main_loop, METH_NOARGS),
    ENTRY(close, METH_NOARGS),
    ENTRY(flush, METH_NOARGS),
    ENTRY(get_is_connected, METH_NOARGS),
    ENTRY(get_is_authenticated, METH_NOARGS),
    ENTRY(set_exit_on_disconnect, METH_VARARGS),
    ENTRY(get_unix_fd, METH_NOARGS),
    ENTRY(get_peer_unix_user, METH_NOARGS),
    ENTRY(get_peer_unix_process_id, METH_NOARGS),
    ENTRY(add_message_filter, METH_O),
    ENTRY(remove_message_filter, METH_O),
    ENTRY(send_message, METH_VARARGS),
    ENTRY(send_message_with_reply, METH_VARARGS|METH_KEYWORDS),
    ENTRY(send_message_with_reply_and_block, METH_VARARGS),
    {"_new_for_bus", (PyCFunction)DBusPyConnection_NewForBus,
        METH_CLASS|METH_VARARGS|METH_KEYWORDS,
        new_for_bus__doc__},
    {"get_unique_name", (PyCFunction)DBusPyConnection_GetUniqueName,
        METH_NOARGS,
        get_unique_name__doc__},
    {"set_unique_name", (PyCFunction)DBusPyConnection_SetUniqueName,
        METH_VARARGS,
        set_unique_name__doc__},
    {NULL},
#undef ENTRY
};

/* vim:set ft=c cino< sw=4 sts=4 et: */
