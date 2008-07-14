/* Implementation of normal Python-accessible methods on the _dbus_bindings
 * Connection type; separated out to keep the file size manageable.
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

PyDoc_STRVAR(Server_disconnect__doc__,
"disconnect()\n\n"
"Releases the server's address and stops listening for new clients.\n");
static PyObject *
Server_disconnect (Server *self, PyObject *args UNUSED)
{
    TRACE(self);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(self->server);
    Py_BEGIN_ALLOW_THREADS
    dbus_server_disconnect(self->server);
    Py_END_ALLOW_THREADS
    
    Py_INCREF(Py_None);
    return Py_None;
}



PyDoc_STRVAR(Server_get_address__doc__,
"get_address() -> String\n\n"
"Return the address of the server.\n");
static PyObject *
Server_get_address (Server *self, PyObject *args UNUSED)
{
    char *address;
    PyObject *ret;

    TRACE(self);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(self->server);
    Py_BEGIN_ALLOW_THREADS
    address = dbus_server_get_address(self->server);
    Py_END_ALLOW_THREADS
    ret = PyString_FromString(address);
    dbus_free (address);
    return ret;
}

PyDoc_STRVAR(Server_get_id__doc__,
"get_id() -> String\n\n"
"Return the id of the server.\n");
static PyObject *
Server_get_id (Server *self, PyObject *args UNUSED)
{
    char *id;
    PyObject *ret;

    TRACE(self);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(self->server);
    Py_BEGIN_ALLOW_THREADS
    id = dbus_server_get_id(self->server);
    Py_END_ALLOW_THREADS
    
    ret = PyString_FromString(id);
    dbus_free (id);
    
    return ret;
}

PyDoc_STRVAR(Server_get_is_connected__doc__,
"get_is_connected() -> bool\n\n"
"Return True if server is istill listening new connections.\n");
static PyObject *
Server_get_is_connected (Server *self, PyObject *args UNUSED)
{
    PyObject *ret;

    TRACE(self);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(self->server);
    Py_BEGIN_ALLOW_THREADS
    if (dbus_server_get_is_connected(self->server))
        ret = Py_True;
    else
        ret = Py_False;
    Py_END_ALLOW_THREADS
   
   Py_INCREF(ret);

    return ret;
}


PyDoc_STRVAR(Server_set_new_connection_function__doc__,
"set_new_connection_function()\n\n"
"Set a function for handling new connections.\n");
static PyObject *
Server_set_new_connection_function (Server *self, PyObject *args)
{
    
    PyObject *callback;
    PyTypeObject *conn_type = NULL;

    if (!PyArg_ParseTuple(args, "O|O:set_new_connection_function",
                          &callback, &conn_type)) {
        return NULL;
    }

    if (callback != Py_None && ! PyCallable_Check (callback)) {
        PyErr_SetString (PyExc_TypeError, 
                            "The first argument must be a callable object");
        return NULL;
    }

    if (!conn_type || (PyObject *)conn_type == Py_None) {
        conn_type = &DBusPyConnection_Type; 
    }

    if (!PyType_IsSubtype (conn_type, &DBusPyConnection_Type)) {
        PyErr_SetString (PyExc_TypeError, 
                            "The second argument must be None or Subclass of _dbus_bindings_.Connection");
        return NULL;
    }
 
    TRACE(self);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(self->server);
    Py_BEGIN_ALLOW_THREADS
    Py_XDECREF (self->new_connection_callback);
    if (callback != Py_None) {
        Py_INCREF (callback);
        self->new_connection_callback = callback;
    }
    else
        self->new_connection_callback = NULL;
    
    Py_XDECREF (self->new_connection_type);
    Py_INCREF (conn_type);
    self->new_connection_type = conn_type;
    
    Py_END_ALLOW_THREADS
    
    Py_INCREF (Py_None);
    return Py_None;
}


PyDoc_STRVAR(Server_set_auth_mechanisms__doc__,
"set_auth_mechanisms (...)\n\n"
"Set the authentication mechanisms that this server offers to clients.\n");
static PyObject *
Server_set_auth_mechanisms (Server *self, PyObject *args)
{
    
    int len, i;
    const char ** _mechanisms = NULL;

    len = PyTuple_Size (args);

    if (len > 0) {
        _mechanisms = (const char **)malloc (sizeof (char *) * (len + 1));
        for (i = 0; i < len; i++) {
            PyObject *mechanism = PyTuple_GetItem (args, i);
            if (!PyString_Check (mechanism)) {
                free (_mechanisms);
                PyErr_SetString(PyExc_TypeError, 
                    "arguments must be strings");
                return NULL;
            }
            _mechanisms[i] = PyString_AS_STRING (mechanism);
        }
        _mechanisms[len] = NULL;
    }
 
    TRACE(self);
    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(self->server);
    Py_BEGIN_ALLOW_THREADS
    dbus_server_set_auth_mechanisms (self->server, _mechanisms);
    Py_END_ALLOW_THREADS

    if (_mechanisms != NULL)
        free (_mechanisms);
    Py_INCREF (Py_None);
    return Py_None;
}

struct PyMethodDef DBusPyServer_tp_methods[] = {
#define ENTRY(name, flags) {#name, (PyCFunction)Server_##name, flags, Server_##name##__doc__}
    ENTRY(disconnect, METH_NOARGS),
    ENTRY(get_address, METH_NOARGS),
    ENTRY(get_id, METH_NOARGS),
    ENTRY(get_is_connected, METH_NOARGS),
    ENTRY(set_new_connection_function, METH_VARARGS),
    ENTRY(set_auth_mechanisms, METH_VARARGS),
    {NULL},
#undef ENTRY
};

/* vim:set ft=c cino< sw=4 sts=4 et: */
