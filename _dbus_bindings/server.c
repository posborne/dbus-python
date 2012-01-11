/* Implementation of the _dbus_bindings Server type, a Python wrapper
 * for DBusServer.
 *
 * Copyright (C) 2008 Openismus GmbH <http://openismus.com/>
 * Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
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

/* Server definition ================================================ */

typedef struct {
    PyObject_HEAD
    DBusServer *server;

    /* The Connection subtype for which this Server is a factory */
    PyObject *conn_class;

    /* Weak-references list to make server weakly referenceable */
    PyObject *weaklist;

    PyObject *mainloop;
} Server;

PyDoc_STRVAR(Server_tp_doc,
"A D-Bus server.\n"
"\n"
"::\n"
"\n"
"   Server(address, connection_subtype, mainloop=None, auth_mechanisms=None)\n"
"     -> Server\n"
);

/* D-Bus Server user data slot, containing an owned reference to either
 * the Server, or a weakref to the Server.
 */
static dbus_int32_t _server_python_slot;

/* C API for main-loop hooks ======================================== */

/* Return a borrowed reference to the DBusServer which underlies this
 * Server. */
DBusServer *
DBusPyServer_BorrowDBusServer(PyObject *self)
{
    DBusServer *dbs;

    TRACE(self);
    if (!DBusPyServer_Check(self)) {
        PyErr_SetString(PyExc_TypeError, "A dbus.server.Server is required");
        return NULL;
    }
    dbs = ((Server *)self)->server;
    if (!dbs) {
        PyErr_SetString(PyExc_RuntimeError, "Server is in an invalid "
                        "state: no DBusServer");
        return NULL;
    }
    return dbs;
}

/* Internal C API =================================================== */

static dbus_bool_t
DBusPyServer_set_auth_mechanisms(Server *self,
                                 PyObject *auth_mechanisms)
{
    PyObject *fast_seq = NULL, *references = NULL;
    Py_ssize_t length;
    Py_ssize_t i;

    fast_seq = PySequence_Fast(auth_mechanisms,
            "Expecting sequence for auth_mechanisms parameter");

    if (!fast_seq)
        return FALSE;

    length = PySequence_Fast_GET_SIZE(fast_seq);

    /* scope for list */
    {
        const char *list[length + 1];

        if (!(references = PyTuple_New(length)))
            goto error;

        for (i = 0; i < length; ++i) {
            PyObject *am, *am_as_bytes;

            am = PySequence_Fast_GET_ITEM(auth_mechanisms, i);
            if (!am) goto error;

            if (PyUnicode_Check(am)) {
                am_as_bytes = PyUnicode_AsUTF8String(am);
                if (!am_as_bytes)
                    goto error;
            }
            else {
                am_as_bytes = am;
                Py_INCREF(am_as_bytes);
            }
            list[i] = PyBytes_AsString(am_as_bytes);
            if (!list[i])
                goto error;

            PyTuple_SET_ITEM(references, i, am_as_bytes);
        }

        list[length] = NULL;

        Py_BEGIN_ALLOW_THREADS
        dbus_server_set_auth_mechanisms(self->server, list);
        Py_END_ALLOW_THREADS
    }

    Py_CLEAR(fast_seq);
    Py_CLEAR(references);
    return TRUE;
  error:
    Py_CLEAR(fast_seq);
    Py_CLEAR(references);
    return FALSE;
}

/* Return a new reference to a Python Server or subclass corresponding
 * to the DBusServer server. For use in callbacks.
 *
 * Raises AssertionError if the DBusServer does not have a Server.
 */
static PyObject *
DBusPyServer_ExistingFromDBusServer(DBusServer *server)
{
    PyObject *self, *ref;

    Py_BEGIN_ALLOW_THREADS
    ref = (PyObject *)dbus_server_get_data(server,
                                           _server_python_slot);
    Py_END_ALLOW_THREADS
    if (ref) {
        DBG("(DBusServer *)%p has weak reference at %p", server, ref);
        self = PyWeakref_GetObject(ref);   /* still a borrowed ref */
        if (self && self != Py_None && DBusPyServer_Check(self)) {
            DBG("(DBusServer *)%p has weak reference at %p pointing to %p",
                server, ref, self);
            TRACE(self);
            Py_INCREF(self);
            TRACE(self);
            return self;
        }
    }

    PyErr_SetString(PyExc_AssertionError,
                    "D-Bus server does not have a Server "
                    "instance associated with it");
    return NULL;
}

static void
DBusPyServer_new_connection_cb(DBusServer *server,
                               DBusConnection *conn,
                               void *data UNUSED)
{
    PyGILState_STATE gil = PyGILState_Ensure();
    PyObject *self = NULL;
    PyObject *method = NULL;

    self = DBusPyServer_ExistingFromDBusServer(server);
    if (!self) goto out;
    TRACE(self);

    method = PyObject_GetAttrString(self, "_on_new_connection");
    TRACE(method);

    if (method) {
        PyObject *conn_class = ((Server *)self)->conn_class;
        PyObject *wrapper = DBusPyLibDBusConnection_New(conn);
        PyObject *conn_obj;
        PyObject *result;

        if (!wrapper)
            goto out;

        conn_obj = PyObject_CallFunctionObjArgs((PyObject *)conn_class,
                wrapper, ((Server*) self)->mainloop, NULL);
        Py_CLEAR(wrapper);

        if (!conn_obj)
            goto out;

        result = PyObject_CallFunctionObjArgs(method, conn_obj, NULL);
        Py_CLEAR (conn_obj);

        /* discard result if not NULL, and fall through regardless */
        Py_CLEAR(result);
    }

out:
    Py_CLEAR(method);
    Py_CLEAR(self);

    if (PyErr_Occurred())
        PyErr_Print();

    PyGILState_Release(gil);
}

/* Return a new reference to a Python Server or subclass (given by cls)
 * corresponding to the DBusServer server, which must have been newly
 * created. For use by the Server constructor.
 *
 * Raises AssertionError if the DBusServer already has a Server.
 *
 * One reference to server is stolen - either the returned DBusPyServer
 * claims it, or it's unreffed.
 */
static PyObject *
DBusPyServer_NewConsumingDBusServer(PyTypeObject *cls,
                                    DBusServer *server,
                                    PyObject *conn_class,
                                    PyObject *mainloop,
                                    PyObject *auth_mechanisms)
{
    Server *self = NULL;
    PyObject *ref;
    dbus_bool_t ok;

    DBG("%s(cls=%p, server=%p, mainloop=%p, auth_mechanisms=%p)",
        __func__, cls, server, mainloop, auth_mechanisms);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(server);

    Py_BEGIN_ALLOW_THREADS
    ref = (PyObject *)dbus_server_get_data(server,
                                           _server_python_slot);
    Py_END_ALLOW_THREADS
    if (ref) {
        self = (Server *)PyWeakref_GetObject(ref);
        ref = NULL;
        if (self && (PyObject *)self != Py_None) {
            self = NULL;
            PyErr_SetString(PyExc_AssertionError,
                            "Newly created D-Bus server already has a "
                            "Server instance associated with it");
            DBG("%s() fail - assertion failed, DBusPyServer has a DBusServer already", __func__);
            DBG_WHEREAMI;
            return NULL;
        }
    }
    ref = NULL;

    /* Change mainloop from a borrowed reference to an owned reference */
    if (!mainloop || mainloop == Py_None) {
        mainloop = dbus_py_get_default_main_loop();

        if (!mainloop || mainloop == Py_None) {
            PyErr_SetString(PyExc_RuntimeError,
                            "To run a D-Bus server, you need to either "
                            "pass mainloop=... to the constructor or call "
                            "dbus.set_default_main_loop(...)");
            goto err;
        }
    }
    else {
        Py_INCREF(mainloop);
    }

    DBG("Constructing Server from DBusServer at %p", server);

    self = (Server *)(cls->tp_alloc(cls, 0));
    if (!self) goto err;
    TRACE(self);

    DBG_WHEREAMI;

    self->server = NULL;

    Py_INCREF(conn_class);
    self->conn_class = conn_class;

    self->mainloop = mainloop;
    mainloop = NULL;    /* don't DECREF it - the DBusServer owns it now */

    ref = PyWeakref_NewRef((PyObject *)self, NULL);
    if (!ref) goto err;
    DBG("Created weak ref %p to (Server *)%p for (DBusServer *)%p",
        ref, self, server);

    Py_BEGIN_ALLOW_THREADS
    ok = dbus_server_set_data(server, _server_python_slot,
                              (void *)ref,
                              (DBusFreeFunction)dbus_py_take_gil_and_xdecref);
    Py_END_ALLOW_THREADS

    if (ok) {
        DBG("Attached weak ref %p ((Server *)%p) to (DBusServer *)%p",
            ref, self, server);

        ref = NULL;     /* don't DECREF it - the DBusServer owns it now */
    }
    else {
        DBG("Failed to attached weak ref %p ((Server *)%p) to "
            "(DBusServer *)%p - will dispose of it", ref, self, server);
        PyErr_NoMemory();
        goto err;
    }

    DBUS_PY_RAISE_VIA_GOTO_IF_FAIL(server, err);
    self->server = server;
    /* the DBusPyServer will close it now */
    server = NULL;

    if (self->mainloop != Py_None &&
        !dbus_py_set_up_server((PyObject *)self, self->mainloop))
        goto err;

    if (auth_mechanisms && auth_mechanisms != Py_None &&
        !DBusPyServer_set_auth_mechanisms(self, auth_mechanisms))
        goto err;

    Py_BEGIN_ALLOW_THREADS
    dbus_server_set_new_connection_function(self->server,
                                            DBusPyServer_new_connection_cb,
                                            NULL, NULL);
    Py_END_ALLOW_THREADS

    DBG("%s() -> %p", __func__, self);
    TRACE(self);
    return (PyObject *)self;

err:
    DBG("Failed to construct Server from DBusServer at %p", server);
    Py_CLEAR(mainloop);
    Py_CLEAR(self);
    Py_CLEAR(ref);

    if (server) {
        Py_BEGIN_ALLOW_THREADS
        dbus_server_disconnect(server);
        dbus_server_unref(server);
        Py_END_ALLOW_THREADS
    }

    DBG("%s() fail", __func__);
    DBG_WHEREAMI;
    return NULL;
}

/* Server type-methods ============================================== */

static PyObject *
Server_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    DBusServer *server;
    const char *address;
    DBusError error;
    PyObject *self, *conn_class, *mainloop = NULL, *auth_mechanisms = NULL;
    static char *argnames[] = { "address", "connection_class", "mainloop",
        "auth_mechanisms", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sO|OO", argnames,
            &address, &conn_class, &mainloop, &auth_mechanisms)) {
        return NULL;
    }

    if (!PyType_Check(conn_class) ||
        !PyType_IsSubtype((PyTypeObject *) conn_class, &DBusPyConnection_Type)) {
        /* strictly speaking, it can be any subtype of
         * _dbus_bindings._Connection - but nobody else should be subtyping
         * that, so let's keep this slightly inaccurate message */
        PyErr_SetString(PyExc_TypeError, "connection_class must be "
                "dbus.connection.Connection or a subtype");
        return NULL;
    }

    dbus_error_init(&error);

    Py_BEGIN_ALLOW_THREADS
    server = dbus_server_listen(address, &error);
    Py_END_ALLOW_THREADS

    if (!server) {
        DBusPyException_ConsumeError(&error);
        return NULL;
    }

    self = DBusPyServer_NewConsumingDBusServer(cls, server, conn_class,
            mainloop, auth_mechanisms);
    ((Server *)self)->weaklist = NULL;
    TRACE(self);

    return self;
}

/* Destructor */
static void Server_tp_dealloc(Server *self)
{
    DBusServer *server = self->server;
    PyObject *et, *ev, *etb;

    /* avoid clobbering any pending exception */
    PyErr_Fetch(&et, &ev, &etb);

    if (self->weaklist) {
        PyObject_ClearWeakRefs((PyObject *)self);
    }

    TRACE(self);
    DBG("Deallocating Server at %p (DBusServer at %p)", self, server);
    DBG_WHEREAMI;

    if (server) {
        DBG("Server at %p has a server, disconnecting it...", self);
        Py_BEGIN_ALLOW_THREADS
        dbus_server_disconnect(server);
        Py_END_ALLOW_THREADS
    }

    Py_CLEAR(self->mainloop);

    /* make sure to do this last to preserve the invariant that
     * self->server is always non-NULL for any referenced Server.
     */
    DBG("Server at %p: nulling self->server", self);
    self->server = NULL;

    if (server) {
        DBG("Server at %p: unreffing server", self);
        dbus_server_unref(server);
    }

    DBG("Server at %p: freeing self", self);
    PyErr_Restore(et, ev, etb);
    (Py_TYPE(self)->tp_free)((PyObject *)self);
}

PyDoc_STRVAR(Server_disconnect__doc__,
"disconnect()\n\n"
"Releases the server's address and stops listening for new clients.\n\n"
"If called more than once, only the first call has an effect.");
static PyObject *
Server_disconnect (Server *self, PyObject *args UNUSED)
{
    TRACE(self);
    if (self->server) {
        Py_BEGIN_ALLOW_THREADS
        dbus_server_disconnect(self->server);
        Py_END_ALLOW_THREADS
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(Server_get_address__doc__,
"get_address() -> str\n\n"
"Returns the address of the server.");
static PyObject *
Server_get_address(Server *self, PyObject *args UNUSED)
{
    const char *address;

    TRACE(self);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(self->server);
    Py_BEGIN_ALLOW_THREADS
    address = dbus_server_get_address(self->server);
    Py_END_ALLOW_THREADS

    return NATIVESTR_FROMSTR(address);
}

PyDoc_STRVAR(Server_get_id__doc__,
"get_id() -> str\n\n"
"Returns the unique ID of the server.");
static PyObject *
Server_get_id(Server *self, PyObject *args UNUSED)
{
    const char *id;

    TRACE(self);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(self->server);
    Py_BEGIN_ALLOW_THREADS
    id = dbus_server_get_id(self->server);
    Py_END_ALLOW_THREADS

    return NATIVESTR_FROMSTR(id);
}

PyDoc_STRVAR(Server_get_is_connected__doc__,
"get_is_connected() -> bool\n\n"
"Return true if this Server is still listening for new connections.\n");
static PyObject *
Server_get_is_connected (Server *self, PyObject *args UNUSED)
{
    dbus_bool_t ret;

    TRACE(self);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(self->server);
    Py_BEGIN_ALLOW_THREADS
    ret = dbus_server_get_is_connected(self->server);
    Py_END_ALLOW_THREADS
    return PyBool_FromLong(ret);
}

/* Server type object =============================================== */

struct PyMethodDef DBusPyServer_tp_methods[] = {
#define ENTRY(name, flags) {#name, (PyCFunction)Server_##name, flags, Server_##name##__doc__}
    ENTRY(disconnect,       METH_NOARGS),
    ENTRY(get_address,      METH_NOARGS),
    ENTRY(get_id,           METH_NOARGS),
    ENTRY(get_is_connected, METH_NOARGS),
    {NULL},
#undef ENTRY
};

PyTypeObject DBusPyServer_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_dbus_bindings._Server",/*tp_name*/
    sizeof(Server),         /*tp_basicsize*/
    0,                      /*tp_itemsize*/
    /* methods */
    (destructor)Server_tp_dealloc,
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
#ifdef PY3
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
#else
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_WEAKREFS | Py_TPFLAGS_BASETYPE,
#endif
    Server_tp_doc,          /*tp_doc*/
    0,                      /*tp_traverse*/
    0,                      /*tp_clear*/
    0,                      /*tp_richcompare*/
    offsetof(Server, weaklist),   /*tp_weaklistoffset*/
    0,                      /*tp_iter*/
    0,                      /*tp_iternext*/
    DBusPyServer_tp_methods,/*tp_methods*/
    0,                      /*tp_members*/
    0,                      /*tp_getset*/
    0,                      /*tp_base*/
    0,                      /*tp_dict*/
    0,                      /*tp_descr_get*/
    0,                      /*tp_descr_set*/
    0,                      /*tp_dictoffset*/
    0,                      /*tp_init*/
    0,                      /*tp_alloc*/
    Server_tp_new,          /*tp_new*/
    0,                      /*tp_free*/
    0,                      /*tp_is_gc*/
};

dbus_bool_t
dbus_py_init_server_types(void)
{
    /* Get a slot to store our weakref on DBus Server */
    _server_python_slot = -1;
    if (!dbus_server_allocate_data_slot(&_server_python_slot))
        return FALSE;

    if (PyType_Ready(&DBusPyServer_Type) < 0)
        return FALSE;

    return TRUE;
}

dbus_bool_t
dbus_py_insert_server_types(PyObject *this_module)
{
    /* PyModule_AddObject steals a ref */
    Py_INCREF (&DBusPyServer_Type);
    if (PyModule_AddObject(this_module, "_Server",
                           (PyObject *)&DBusPyServer_Type) < 0) return FALSE;

    return TRUE;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
