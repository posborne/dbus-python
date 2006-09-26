/* Implementation of normal Python-accessible methods on the _dbus_bindings
 * Connection type; separated out to keep the file size manageable.
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

PyDoc_STRVAR(Connection_close__doc__,
"close()\n\n"
"Close the connection.");
static PyObject *
Connection_close (Connection *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":close")) return NULL;
    /* Because the user explicitly asked to close the connection, we'll even
    let them close shared connections. */
    if (self->conn) {
        Py_BEGIN_ALLOW_THREADS
        dbus_connection_close (self->conn);
        Py_END_ALLOW_THREADS
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(Connection_get_is_connected__doc__,
"get_is_connected() -> bool\n\n"
"Return true if this Connection is connected.\n");
static PyObject *
Connection_get_is_connected (Connection *self, PyObject *args)
{
    dbus_bool_t ret;
    if (!PyArg_ParseTuple(args, ":get_is_connected")) return NULL;
    Py_BEGIN_ALLOW_THREADS
    ret = dbus_connection_get_is_connected (self->conn);
    Py_END_ALLOW_THREADS
    return PyBool_FromLong (ret);
}

PyDoc_STRVAR(Connection_get_is_authenticated__doc__,
"get_is_authenticated() -> bool\n\n"
"Return true if this Connection was ever authenticated.\n");
static PyObject *
Connection_get_is_authenticated (Connection *self, PyObject *args)
{
    dbus_bool_t ret;
    if (!PyArg_ParseTuple(args, ":get_is_authenticated")) return NULL;
    Py_BEGIN_ALLOW_THREADS
    ret = dbus_connection_get_is_authenticated (self->conn);
    Py_END_ALLOW_THREADS
    return PyBool_FromLong (ret);
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
    if (!PyArg_ParseTuple(args, "i:set_exit_on_disconnect",
                          &exit_on_disconnect)) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    dbus_connection_set_exit_on_disconnect (self->conn,
                                            exit_on_disconnect ? 1 : 0);
    Py_END_ALLOW_THREADS
    Py_RETURN_NONE;
}

PyDoc_STRVAR(Connection__send__doc__,
"_send(msg: Message) -> long\n\n"
"Queue the given message for sending, and return the message serial number.\n"
);
static PyObject *
Connection__send (Connection *self, PyObject *args)
{
    dbus_bool_t ok;
    PyObject *obj;
    DBusMessage *msg;
    dbus_uint32_t serial;

    if (!PyArg_ParseTuple(args, "O", &obj)) return NULL;

    msg = Message_BorrowDBusMessage(obj);
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
PyDoc_STRVAR(Connection__send_with_reply__doc__,
"_send_with_reply(msg: Message, reply_handler: callable[, timeout_s: float])"
" -> PendingCall\n\n"
"Queue the message for sending; expect a reply via the returned PendingCall.\n"
"\n"
":Parameters:\n"
"   `msg` : Message\n"
"       The message to be sent\n"
"   `reply_handler` : callable\n"
"       Asynchronous reply handler: will be called with one positional\n"
"       parameter, a Message instance representing the reply.\n"
"   `timeout_s` : float\n"
"       If the reply takes more than this many seconds, a timeout error\n"
"       will be created locally and raised instead. If this timeout is\n"
"       negative (default), a sane default (supplied by libdbus) is used.\n"
":Returns\n"
"   A `PendingCall` instance which can be used to cancel the pending call.\n"
"\n"
);
static PyObject *
Connection__send_with_reply(Connection *self, PyObject *args)
{
    dbus_bool_t ok;
    double timeout_s = -1.0;
    int timeout_ms;
    PyObject *obj, *callable;
    DBusMessage *msg;
    DBusPendingCall *pending;

    if (!PyArg_ParseTuple(args, "OO|f:send_with_reply", &obj, &callable,
                          &timeout_s)) {
        return NULL;
    }

    msg = Message_BorrowDBusMessage(obj);
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

    return PendingCall_ConsumeDBusPendingCall(pending, callable);
}

/* Again, the timeout is in seconds, since that's conventional in Python. */
PyDoc_STRVAR(Connection__send_with_reply_and_block__doc__,
"_send_with_reply_and_block(msg: Message, [, timeout_s: float])"
" -> Message\n\n"
"Send the message and block while waiting for a reply.\n"
"\n"
"This does not re-enter the main loop, so it can lead to a deadlock, if\n"
"the called method tries to make a synchronous call to a method in this\n"
"application. As such, it's probably a bad idea.\n"
"\n"
":Parameters:\n"
"   `msg` : Message\n"
"       The message to be sent\n"
"   `timeout_s` : float\n"
"       If the reply takes more than this many seconds, a timeout error\n"
"       will be created locally and raised instead. If this timeout is\n"
"       negative (default), a sane default (supplied by libdbus) is used.\n"
":Returns\n"
"   A `Message` instance (probably a `MethodReturnMessage`) on success\n"
":Raises\n"
"   A `DBusException` on error (including if the reply arrives but is an\n"
"   error message)\n"
"\n"
);
static PyObject *
Connection__send_with_reply_and_block(Connection *self, PyObject *args)
{
    double timeout_s = -1.0;
    int timeout_ms;
    PyObject *obj;
    DBusMessage *msg, *reply;
    DBusError error;

    if (!PyArg_ParseTuple(args, "O|f:_send_with_reply_and_block", &obj,
                          &timeout_s)) {
        return NULL;
    }

    msg = Message_BorrowDBusMessage(obj);
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

    if (!reply) {
        return DBusException_ConsumeError(&error);
    }
    return Message_ConsumeDBusMessage(reply);
}

PyDoc_STRVAR(Connection_flush__doc__,
"flush()\n\n"
"Block until the outgoing message queue is empty.\n");
static PyObject *
Connection_flush (Connection *self, PyObject *args)
{
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
Connection_get_unix_fd (Connection *self, PyObject *unused)
{
    int fd;
    dbus_bool_t ok;

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
Connection_get_peer_unix_user (Connection *self, PyObject *unused)
{
    unsigned long uid;
    dbus_bool_t ok;

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
Connection_get_peer_unix_process_id (Connection *self, PyObject *unused)
{
    unsigned long pid;
    dbus_bool_t ok;

    Py_BEGIN_ALLOW_THREADS
    ok = dbus_connection_get_unix_process_id (self->conn, &pid);
    Py_END_ALLOW_THREADS
    if (!ok) Py_RETURN_NONE;
    return PyLong_FromUnsignedLong (pid);
}

/* TODO: wrap dbus_connection_set_unix_user_function Pythonically */

PyDoc_STRVAR(Connection__add_filter__doc__,
"_add_filter(callable)\n\n"
"Add the given message filter to the internal list.\n\n"
"Filters are handlers that are run on all incoming messages, prior to the\n"
"objects registered with `_register_object_path`.\n"
"Filters are run in the order that they were added. The same handler can\n"
"be added as a filter more than once, in which case it will be run more\n"
"than once. Filters added during a filter callback won't be run on the\n"
"message being processed.\n"
);
static PyObject *
Connection__add_filter(Connection *self, PyObject *callable)
{
    dbus_bool_t ok;

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

PyDoc_STRVAR(Connection__remove_filter__doc__,
"_remove_filter(callable)\n\n"
"Remove the given message filter (see `_add_filter` for details).\n");
static PyObject *
Connection__remove_filter(Connection *self, PyObject *callable)
{
    PyObject *obj;

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

/* object exports not yet implemented: */

PyDoc_STRVAR(Connection__register_object_path__doc__,
"_register_object_path(path: str, on_message: callable[, on_unregister: "
"callable][, **kwargs])\n\n"
"Keyword arguments accepted:\n"
"fallback: bool (default False): if True, when a message arrives for a\n"
"'subdirectory' of the given path and there is no more specific handler,\n"
"use this handler. Normally this handler is only run if the paths match\n"
"exactly.\n"
);
static PyObject *
Connection__register_object_path(Connection *self, PyObject *args,
                                 PyObject *kwargs)
{
    dbus_bool_t ok;
    int fallback = 0;
    PyObject *callbacks, *path, *tuple, *on_message, *on_unregister = Py_None;
    static char *argnames[] = {"path", "on_message", "on_unregister",
                               "fallback", NULL};

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
    We make an exception for ObjectPaths because they're equally simple,
    are known to have the same __eq__ and __hash__, and are what calling code
    ought to be using. */
    if (PyString_CheckExact(path) || path->ob_type == &ObjectPathType) {
        Py_INCREF(path);
    }
    else if (PyUnicode_Check(path)) {
        path = PyUnicode_AsUTF8String(path);
        if (!path) return NULL;
    }
    else if (PyString_Check(path)) {
        path = PyString_FromString(PyString_AS_STRING(path));
        if (!path) return NULL;
    }

    if (!_validate_object_path(PyString_AS_STRING(path))) {
        Py_DECREF(path);
        return NULL;
    }

    tuple = Py_BuildValue("(OO)", on_unregister, on_message);
    if (!tuple) {
        Py_DECREF(path);
        return NULL;
    }

    /* Guard against registering a handler that already exists. */
    callbacks = PyDict_GetItem(self->object_paths, path);
    if (callbacks && callbacks != Py_None) {
        PyErr_Format(PyExc_KeyError, "Can't register the object-path "
                     "handler for '%s': there is already a handler",
                     PyString_AS_STRING(path));
        Py_DECREF(tuple);
        Py_DECREF(path);
        return NULL;
    }

    /* Pre-allocate a slot in the dictionary, so we know we'll be able
     * to replace it with the callbacks without OOM.
     * This ensures we can keep libdbus' opinion of whether those
     * paths are handled in sync with our own. */
    if (PyDict_SetItem(self->object_paths, path, Py_None) < 0) {
        Py_DECREF(tuple);
        Py_DECREF(path);
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    if (fallback) {
        ok = dbus_connection_register_fallback(self->conn,
                                               PyString_AS_STRING(path),
                                               &_object_path_vtable,
                                               path);
    }
    else {
        ok = dbus_connection_register_object_path(self->conn,
                                                  PyString_AS_STRING(path),
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
                                                    PyString_AS_STRING(path));
            Py_END_ALLOW_THREADS
            return NULL;
        }
        /* don't DECREF path: libdbus owns a ref now */
        Py_DECREF(tuple);
        Py_RETURN_NONE;
    }
    else {
        /* Oops, OOM. Tidy up, if we can, ignoring any error. */
        PyDict_DelItem(self->object_paths, path);
        PyErr_Clear();
        Py_DECREF(tuple);
        Py_DECREF(path);
        PyErr_NoMemory();
        return NULL;
    }
}

PyDoc_STRVAR(Connection__unregister_object_path__doc__,
"_unregister_object_path(path: str)\n\n"
""
);
static PyObject *
Connection__unregister_object_path(Connection *self, PyObject *args,
                                   PyObject *kwargs)
{
    dbus_bool_t ok;
    PyObject *path;
    PyObject *callbacks;
    static char *argnames[] = {"path", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "O!:_unregister_object_path",
                                     argnames, 
                                     &PyString_Type, &path)) return NULL;

    /* Take a ref to the path. Same comments as for _register_object_path. */
    if (PyString_CheckExact(path) || path->ob_type == &ObjectPathType) {
        Py_INCREF(path);
    }
    else {
        path = PyString_FromString(PyString_AS_STRING(path));
        if (!path) return NULL;
    }

    /* Guard against unregistering a handler that doesn't, in fact, exist,
    or whose unregistration is already in progress. */
    callbacks = PyDict_GetItem(self->object_paths, path);
    if (!callbacks || callbacks == Py_None) {
        PyErr_Format(PyExc_KeyError, "Can't unregister the object-path "
                     "handler for '%s': there is no such handler",
                     PyString_AS_STRING(path));
        Py_DECREF(path);
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
        Py_DECREF(callbacks);
        Py_DECREF(path);
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
    ok = dbus_connection_unregister_object_path(self->conn,
                                                PyString_AS_STRING(path));
    Py_END_ALLOW_THREADS

    if (ok) {
        Py_DECREF(callbacks);
        PyDict_DelItem(self->object_paths, path);
        /* END PARANOIA on successful code path */
        /* The above can't fail unless by some strange trickery the key is no
        longer present. Ignore any errors. */
        Py_DECREF(path);
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
        Py_DECREF(path);
        Py_DECREF(callbacks);
        return PyErr_NoMemory();
    }
}

    /* dbus_connection_get_object_path_data - not useful to Python,
     * the object path data is just a PyString containing the path */
    /* dbus_connection_list_registered could be useful, though */

/* dbus_connection_set_change_sigpipe - sets global state */

/* Maxima. Does Python code ever need to manipulate these?
 * OTOH they're easy to wrap */
    /* dbus_connection_set_max_message_size */
    /* dbus_connection_get_max_message_size */
    /* dbus_connection_set_max_received_size */
    /* dbus_connection_get_max_received_size */

/* dbus_connection_get_outgoing_size - almost certainly unneeded */

static struct PyMethodDef Connection_tp_methods[] = {
#define ENTRY(name, flags) {#name, (PyCFunction)Connection_##name, flags, Connection_##name##__doc__}
    ENTRY(close, METH_NOARGS),
    ENTRY(flush, METH_NOARGS),
    ENTRY(get_is_connected, METH_NOARGS),
    ENTRY(get_is_authenticated, METH_NOARGS),
    ENTRY(set_exit_on_disconnect, METH_VARARGS),
    ENTRY(get_unix_fd, METH_NOARGS),
    ENTRY(get_peer_unix_user, METH_NOARGS),
    ENTRY(get_peer_unix_process_id, METH_NOARGS),
    ENTRY(_add_filter, METH_O),
    ENTRY(_register_object_path, METH_VARARGS|METH_KEYWORDS),
    ENTRY(_remove_filter, METH_O),
    ENTRY(_send, METH_VARARGS),
    ENTRY(_send_with_reply, METH_VARARGS),
    ENTRY(_send_with_reply_and_block, METH_VARARGS),
    ENTRY(_unregister_object_path, METH_VARARGS|METH_KEYWORDS),
    {NULL},
#undef ENTRY
};

/* vim:set ft=c cino< sw=4 sts=4 et: */
