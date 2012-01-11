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

static void
_object_path_unregister(DBusConnection *conn, void *user_data)
{
    PyGILState_STATE gil = PyGILState_Ensure();
    PyObject *tuple = NULL;
    Connection *conn_obj = NULL;
    PyObject *callable;

    conn_obj = (Connection *)DBusPyConnection_ExistingFromDBusConnection(conn);
    if (!conn_obj) goto out;
    TRACE(conn_obj);

    DBG("Connection at %p unregistering object path %s",
        conn_obj, PyBytes_AS_STRING((PyObject *)user_data));
    tuple = DBusPyConnection_GetObjectPathHandlers(
        (PyObject *)conn_obj, (PyObject *)user_data);
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
    Py_CLEAR(conn_obj);
    Py_CLEAR(tuple);
    /* the user_data (a Python str) is no longer ref'd by the DBusConnection */
    Py_CLEAR(user_data);
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
    PyObject *msg_obj;
    PyObject *callable;             /* borrowed */

    dbus_message_ref(message);
    msg_obj = DBusPyMessage_ConsumeDBusMessage(message);
    if (!msg_obj) {
        ret = DBUS_HANDLER_RESULT_NEED_MEMORY;
        goto out;
    }

    conn_obj = (Connection *)DBusPyConnection_ExistingFromDBusConnection(conn);
    if (!conn_obj) {
        ret = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        goto out;
    }
    TRACE(conn_obj);

    DBG("Connection at %p messaging object path %s",
        conn_obj, PyBytes_AS_STRING((PyObject *)user_data));
    DBG_DUMP_MESSAGE(message);
    tuple = DBusPyConnection_GetObjectPathHandlers(
        (PyObject *)conn_obj, (PyObject *)user_data);
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
        ret = DBusPyConnection_HandleMessage(conn_obj, msg_obj, callable);
    }

out:
    Py_CLEAR(msg_obj);
    Py_CLEAR(conn_obj);
    Py_CLEAR(tuple);
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
            break;
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
    Py_CLEAR(msg_obj);
    Py_CLEAR(conn_obj);
    Py_CLEAR(callable);
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

PyDoc_STRVAR(Connection_set_allow_anonymous__doc__,
"set_allow_anonymous(bool)\n\n"
"Allows anonymous clients. Call this on the server side of a connection in a on_connection_added callback"
);
static PyObject *
Connection_set_allow_anonymous(Connection *self, PyObject *args)
{
    dbus_bool_t t;

    TRACE(self);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(self->conn);
    if (!PyArg_ParseTuple(args, "i", &t)) {
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    dbus_connection_set_allow_anonymous(self->conn, t ? 1 : 0);
    Py_END_ALLOW_THREADS
    Py_RETURN_NONE;
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
    return NATIVEINT_FROMLONG(fd);
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
    Py_CLEAR(obj);

    Py_BEGIN_ALLOW_THREADS
    dbus_connection_remove_filter(self->conn, _filter_message, callable);
    Py_END_ALLOW_THREADS

    Py_RETURN_NONE;
}

PyDoc_STRVAR(Connection__register_object_path__doc__,
"register_object_path(path, on_message, on_unregister=None, fallback=False)\n"
"\n"
"Register a callback to be called when messages arrive at the given\n"
"object-path. Used to export objects' methods on the bus in a low-level\n"
"way. For the high-level interface to this functionality (usually\n"
"recommended) see the `dbus.service.Object` base class.\n"
"\n"
":Parameters:\n"
"   `path` : str\n"
"       Object path to be acted on\n"
"   `on_message` : callable\n"
"       Called when a message arrives at the given object-path, with\n"
"       two positional parameters: the first is this Connection,\n"
"       the second is the incoming `dbus.lowlevel.Message`.\n"
"   `on_unregister` : callable or None\n"
"       If not None, called when the callback is unregistered.\n"
"   `fallback` : bool\n"
"       If True (the default is False), when a message arrives for a\n"
"       'subdirectory' of the given path and there is no more specific\n"
"       handler, use this handler. Normally this handler is only run if\n"
"       the paths match exactly.\n"
);
static PyObject *
Connection__register_object_path(Connection *self, PyObject *args,
                                 PyObject *kwargs)
{
    dbus_bool_t ok;
    int fallback = 0;
    char *path_bytes;
    PyObject *callbacks, *path, *tuple, *on_message, *on_unregister = Py_None;
    static char *argnames[] = {"path", "on_message", "on_unregister",
                               "fallback", NULL};

    TRACE(self);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(self->conn);
    if (!Connection__require_main_loop(self, NULL)) {
        return NULL;
    }
    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "OO|Oi:_register_object_path",
                                     argnames,
                                     &path,
                                     &on_message, &on_unregister,
                                     &fallback)) return NULL;

    /* Take a reference to path, which we give away to libdbus in a moment.

    Also, path needs to be a string (not a subclass which could do something
    mad) to preserve the desirable property that the DBusConnection can
    never strongly reference the Connection, even indirectly.
    */
    if (PyBytes_CheckExact(path)) {
        Py_INCREF(path);
    }
    else if (PyUnicode_Check(path)) {
        path = PyUnicode_AsUTF8String(path);
        if (!path) return NULL;
    }
    else if (PyBytes_Check(path)) {
        path = PyBytes_FromString(PyBytes_AS_STRING(path));
        if (!path) return NULL;
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "path must be a str, bytes, or unicode object");
        return NULL;
    }

    path_bytes = PyBytes_AS_STRING(path);
    if (!dbus_py_validate_object_path(path_bytes)) {
        Py_CLEAR(path);
        return NULL;
    }

    tuple = Py_BuildValue("(OO)", on_unregister, on_message);
    if (!tuple) {
        Py_CLEAR(path);
        return NULL;
    }

    /* Guard against registering a handler that already exists. */
    callbacks = PyDict_GetItem(self->object_paths, path);
    if (callbacks && callbacks != Py_None) {
        PyErr_Format(PyExc_KeyError, "Can't register the object-path "
                     "handler for '%s': there is already a handler",
                     path_bytes);
        Py_CLEAR(tuple);
        Py_CLEAR(path);
        return NULL;
    }

    /* Pre-allocate a slot in the dictionary, so we know we'll be able
     * to replace it with the callbacks without OOM.
     * This ensures we can keep libdbus' opinion of whether those
     * paths are handled in sync with our own. */
    if (PyDict_SetItem(self->object_paths, path, Py_None) < 0) {
        Py_CLEAR(tuple);
        Py_CLEAR(path);
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    if (fallback) {
        ok = dbus_connection_register_fallback(self->conn,
                                               path_bytes,
                                               &_object_path_vtable,
                                               path);
    }
    else {
        ok = dbus_connection_register_object_path(self->conn,
                                                  path_bytes,
                                                  &_object_path_vtable,
                                                  path);
    }
    Py_END_ALLOW_THREADS

    if (ok) {
        if (PyDict_SetItem(self->object_paths, path, tuple) < 0) {
            /* That shouldn't have happened, we already allocated enough
            memory for it. Oh well, try to undo the registration to keep
            things in sync. If this fails too, we've leaked a bit of
            memory in libdbus, but tbh we should never get here anyway. */
            Py_BEGIN_ALLOW_THREADS
            ok = dbus_connection_unregister_object_path(self->conn,
                                                        path_bytes);
            Py_END_ALLOW_THREADS
            return NULL;
        }
        /* don't DECREF path: libdbus owns a ref now */
        Py_CLEAR(tuple);
        Py_RETURN_NONE;
    }
    else {
        /* Oops, OOM. Tidy up, if we can, ignoring any error. */
        PyDict_DelItem(self->object_paths, path);
        PyErr_Clear();
        Py_CLEAR(tuple);
        Py_CLEAR(path);
        PyErr_NoMemory();
        return NULL;
    }
}

PyDoc_STRVAR(Connection__unregister_object_path__doc__,
"unregister_object_path(path)\n\n"
"Remove a previously registered handler for the given object path.\n"
"\n"
":Parameters:\n"
"   `path` : str\n"
"       The object path whose handler is to be removed\n"
":Raises KeyError: if there is no handler registered for exactly that\n"
"   object path.\n"
);
static PyObject *
Connection__unregister_object_path(Connection *self, PyObject *args,
                                   PyObject *kwargs)
{
    dbus_bool_t ok;
    char *path_bytes;
    PyObject *path;
    PyObject *callbacks;
    static char *argnames[] = {"path", NULL};

    TRACE(self);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(self->conn);
    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "O:_unregister_object_path",
                                     argnames, &path)) return NULL;

    /* Take a ref to the path. Same comments as for _register_object_path. */
    if (PyBytes_CheckExact(path)) {
        Py_INCREF(path);
    }
    else if (PyUnicode_Check(path)) {
        path = PyUnicode_AsUTF8String(path);
        if (!path) return NULL;
    }
    else if (PyBytes_Check(path)) {
        path = PyBytes_FromString(PyBytes_AS_STRING(path));
        if (!path) return NULL;
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "path must be a str, bytes, or unicode object");
        return NULL;
    }

    path_bytes = PyBytes_AS_STRING(path);

    /* Guard against unregistering a handler that doesn't, in fact, exist,
    or whose unregistration is already in progress. */
    callbacks = PyDict_GetItem(self->object_paths, path);
    if (!callbacks || callbacks == Py_None) {
        PyErr_Format(PyExc_KeyError, "Can't unregister the object-path "
                     "handler for '%s': there is no such handler",
                     path_bytes);
        Py_CLEAR(path);
        return NULL;
    }

    /* Hang on to a reference to the callbacks for the moment. */
    Py_INCREF(callbacks);

    /* Get rid of the object-path while we still have the GIL, to
    guard against unregistering twice from different threads (which
    causes undefined behaviour in libdbus).

    Because deletion would make it possible for the re-insertion below
    to fail, we instead set the handler to None as a placeholder.
    */
    if (PyDict_SetItem(self->object_paths, path, Py_None) < 0) {
        /* If that failed, there's no need to be paranoid as below - the
        callbacks are still set, so we failed, but at least everything
        is in sync. */
        Py_CLEAR(callbacks);
        Py_CLEAR(path);
        return NULL;
    }

    /* BEGIN PARANOIA
    This is something of a critical section - the dict of object-paths
    and libdbus' internal structures are out of sync for a bit. We have
    to be able to cope with that.

    It's really annoying that dbus_connection_unregister_object_path
    can fail, *and* has undefined behaviour if the object path has
    already been unregistered. Either/or would be fine.
    */

    Py_BEGIN_ALLOW_THREADS
    ok = dbus_connection_unregister_object_path(self->conn, path_bytes);
    Py_END_ALLOW_THREADS

    if (ok) {
        Py_CLEAR(callbacks);
        PyDict_DelItem(self->object_paths, path);
        /* END PARANOIA on successful code path */
        /* The above can't fail unless by some strange trickery the key is no
        longer present. Ignore any errors. */
        Py_CLEAR(path);
        PyErr_Clear();
        Py_RETURN_NONE;
    }
    else {
        /* Oops, OOM. Put the callbacks back in the dict so
         * we'll have another go if/when the user frees some memory
         * and tries calling this method again. */
        PyDict_SetItem(self->object_paths, path, callbacks);
        /* END PARANOIA on failing code path */
        /* If the SetItem failed, there's nothing we can do about it - but
        since we know it's an existing entry, it shouldn't be able to fail
        anyway. */
        Py_CLEAR(path);
        Py_CLEAR(callbacks);
        return PyErr_NoMemory();
    }
}

PyDoc_STRVAR(Connection_list_exported_child_objects__doc__,
"list_exported_child_objects(path: str) -> list of str\n\n"
"Return a list of the names of objects exported on this Connection as\n"
"direct children of the given object path.\n"
"\n"
"Each name returned may be converted to a valid object path using\n"
"``dbus.ObjectPath('%s%s%s' % (path, (path != '/' and '/' or ''), name))``.\n"
"For the purposes of this function, every parent or ancestor of an exported\n"
"object is considered to be an exported object, even if it's only an object\n"
"synthesized by the library to support introspection.\n");
static PyObject *
Connection_list_exported_child_objects (Connection *self, PyObject *args,
                                        PyObject *kwargs)
{
    const char *path;
    char **kids, **kid_ptr;
    dbus_bool_t ok;
    PyObject *ret;
    static char *argnames[] = {"path", NULL};

    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(self->conn);
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", argnames, &path)) {
        return NULL;
    }

    if (!dbus_py_validate_object_path(path)) {
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    ok = dbus_connection_list_registered(self->conn, path, &kids);
    Py_END_ALLOW_THREADS

    if (!ok) {
        return PyErr_NoMemory();
    }

    ret = PyList_New(0);
    if (!ret) {
        return NULL;
    }
    for (kid_ptr = kids; *kid_ptr; kid_ptr++) {
        PyObject *tmp = NATIVESTR_FROMSTR(*kid_ptr);

        if (!tmp) {
            Py_CLEAR(ret);
            return NULL;
        }
        if (PyList_Append(ret, tmp) < 0) {
            Py_CLEAR(tmp);
            Py_CLEAR(ret);
            return NULL;
        }
        Py_CLEAR(tmp);
    }

    dbus_free_string_array(kids);

    return ret;
}

    /* dbus_connection_get_object_path_data - not useful to Python,
     * the object path data is just a PyBytes containing the path */
    /* dbus_connection_list_registered could be useful, though */

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
    ENTRY(_register_object_path, METH_VARARGS|METH_KEYWORDS),
    ENTRY(remove_message_filter, METH_O),
    ENTRY(send_message, METH_VARARGS),
    ENTRY(send_message_with_reply, METH_VARARGS|METH_KEYWORDS),
    ENTRY(send_message_with_reply_and_block, METH_VARARGS),
    ENTRY(_unregister_object_path, METH_VARARGS|METH_KEYWORDS),
    ENTRY(list_exported_child_objects, METH_VARARGS|METH_KEYWORDS),
    {"_new_for_bus", (PyCFunction)DBusPyConnection_NewForBus,
        METH_CLASS|METH_VARARGS|METH_KEYWORDS,
        new_for_bus__doc__},
    {"get_unique_name", (PyCFunction)DBusPyConnection_GetUniqueName,
        METH_NOARGS,
        get_unique_name__doc__},
    {"set_unique_name", (PyCFunction)DBusPyConnection_SetUniqueName,
        METH_VARARGS,
        set_unique_name__doc__},
    ENTRY(set_allow_anonymous, METH_VARARGS),
    {NULL},
#undef ENTRY
};

/* vim:set ft=c cino< sw=4 sts=4 et: */
