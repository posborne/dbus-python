/* Implementation of the _dbus_bindings Server type, a Python wrapper
 * for DBusServer. See also server-methods.c.
 *
 * Copyright (C) 2008 Huang Peng <phuang@redhat.com> .
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
#include "server-internal.h"

/* Server definition ============================================ */

PyDoc_STRVAR(Server_tp_doc,
"A D-Bus server.\n"
"\n"
"::\n"
"\n"
"   Server(address, mainloop=None) -> Server\n"
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
        PyErr_SetString(PyExc_TypeError, "A dbus.Server is required");
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

/* Return a new reference to a Python Server or subclass corresponding
 * to the DBusServer server. For use in callbacks.
 *
 * Raises AssertionError if the DBusServer does not have a Server.
 */
PyObject *
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

/* Return a new reference to a Python Server or subclass (given by cls)
 * corresponding to the DBusServer server, which must have been newly
 * created. For use by the Server and Bus constructors.
 *
 * Raises AssertionError if the DBusServer already has a Server.
 */

static void
Server_new_connection_callback (DBusServer *server, DBusConnection *connection, void * data UNUSED)
{
    PyObject *conn = NULL;
    PyObject *result = NULL;
    PyObject *args = NULL;
    Server *self = NULL;
    
    self = (Server *)DBusPyServer_ExistingFromDBusServer(server);
    if (self == NULL)
        goto error;

    if (self->new_connection_callback) {
        conn = DBusPyConnection_NewConsumingDBusConnection(self->new_connection_type, connection, self->mainloop);
        if (conn == NULL)
            goto error;
        dbus_connection_ref (connection);

        args = Py_BuildValue ("(OO)", self, conn);
        if (args == NULL)
            goto error;
        result = PyObject_Call (self->new_connection_callback, args, NULL);
        if (result == NULL)
            goto error;
        
    }
    goto out;
error:
    PyErr_Print ();
out:
    Py_XDECREF (conn);
    Py_XDECREF (result);
    Py_XDECREF (args);
    Py_XDECREF ((PyObject *)self);
}

/* Server type-methods ========================================== */

/* "Constructor" (the real constructor is Server_NewFromDBusServer,
 * to which this delegates). */
static PyObject *
Server_tp_new(PyTypeObject *cls, PyObject *args UNUSED, PyObject *kwargs UNUSED)
{
    PyObject *self;
    self = cls->tp_alloc (cls, 0);
    return (PyObject *)self;

}


static int
Server_tp_init(Server *self, PyObject *args, PyObject *kwargs)
{
    PyObject *mainloop = NULL;
    DBusServer *server = NULL;
    char *address = NULL;

    PyObject *ref = NULL;
    DBusError error;
    dbus_bool_t ok;

    static char *argnames[] = {"address", "mainloop", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|O", argnames,
                                     &address, &mainloop)) {
        return -1;
    }

    if (!mainloop || mainloop == Py_None) {
        mainloop = dbus_py_get_default_main_loop();
        if (!mainloop) {
            PyErr_SetString (PyExc_Exception, 
                                "Can not get default mainloop.");
            goto err;
        }
    }
    else {
        Py_INCREF (mainloop);
    }
    

    dbus_error_init(&error);

    Py_BEGIN_ALLOW_THREADS
    server = dbus_server_listen(address, &error);
    Py_END_ALLOW_THREADS

    if (!server) {
        DBusPyException_ConsumeError(&error);
        goto err;
    }

    dbus_server_set_new_connection_function (server, Server_new_connection_callback, NULL, NULL);

    Py_INCREF (mainloop);
    self->mainloop = mainloop;
    self->server = server;
    self->new_connection_callback = NULL;
    self->new_connection_type = NULL;

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

    if (!dbus_py_set_up_server((PyObject *)self, mainloop)) {
        goto err;
    }

    Py_DECREF(mainloop);

    DBG("%s() -> %p", __func__, self);
    TRACE(self);
    return 0;

err:
    DBG("Failed to construct Server.");
    Py_XDECREF(mainloop);
    Py_XDECREF(ref);
    if (server) {
        Py_BEGIN_ALLOW_THREADS
        dbus_server_unref(server);
        Py_END_ALLOW_THREADS
    }
    DBG("%s() fail", __func__);
    DBG_WHEREAMI;
    return -1;
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

    DBG("Server at %p: deleting callbacks", self);

    if (server) {
        /* Might trigger callbacks if we're unlucky... */
        DBG("Server at %p has a server, closing it...", self);
        Py_BEGIN_ALLOW_THREADS
        Py_END_ALLOW_THREADS
    }

    /* make sure to do this last to preserve the invariant that
     * self->server is always non-NULL for any referenced Server
     * (until the filters and object paths were freed, we might have been
     * in a reference cycle!)
     */
    DBG("Server at %p: nulling self->server", self);
    self->server = server;

    if (server) {
        DBG("Server at %p: unreffing server", self);
        dbus_server_unref(server);
    }

    Py_XDECREF (self->mainloop);

    DBG("Server at %p: freeing self", self);
    PyErr_Restore(et, ev, etb);
    (self->ob_type->tp_free)((PyObject *)self);
}

/* Server type object =========================================== */

PyTypeObject DBusPyServer_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                      /*ob_size*/
    "_dbus_bindings.Server",/*tp_name*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_WEAKREFS | Py_TPFLAGS_BASETYPE,
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
    (initproc) Server_tp_init,
                            /*tp_init*/
    0,                      /*tp_alloc*/
    Server_tp_new,          /*tp_new*/
    0,                      /*tp_free*/
    0,                      /*tp_is_gc*/
};

dbus_bool_t
dbus_py_init_server_types(void)
{
    /* Get a slot to store our weakref on DBus Servers */
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
    if (PyModule_AddObject(this_module, "Server",
                           (PyObject *)&DBusPyServer_Type) < 0) return FALSE;
    return TRUE;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
