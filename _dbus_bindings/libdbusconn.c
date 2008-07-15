/* An extremely thin wrapper around a libdbus Connection, for use by
 * Server.
 *
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
#include "conn-internal.h"

PyDoc_STRVAR(DBusPyLibDBusConnection_tp_doc,
"A reference to a ``DBusConnection`` from ``libdbus``, which might not\n"
"have been attached to a `dbus.connection.Connection` yet.\n"
"\n"
"Cannot be instantiated from Python. The only use of this object is to\n"
"pass it to the ``dbus.connection.Connection`` constructor instead of an\n"
"address.\n"
);

/** Create a DBusPyLibDBusConnection from a DBusConnection.
 */
PyObject *
DBusPyLibDBusConnection_New(DBusConnection *conn)
{
    DBusPyLibDBusConnection *self = NULL;

    DBUS_PY_RAISE_VIA_NULL_IF_FAIL(conn);

    self = (DBusPyLibDBusConnection *)(DBusPyLibDBusConnection_Type.tp_alloc(
        &DBusPyLibDBusConnection_Type, 0));

    if (!self)
        return NULL;

    self->conn = dbus_connection_ref (conn);

    return (PyObject *)self;
}

/* Destructor */
static void
DBusPyLibDBusConnection_tp_dealloc(Connection *self)
{
    DBusConnection *conn = self->conn;
    PyObject *et, *ev, *etb;

    /* avoid clobbering any pending exception */
    PyErr_Fetch(&et, &ev, &etb);

    self->conn = NULL;

    if (conn) {
        dbus_connection_unref(conn);
    }

    PyErr_Restore(et, ev, etb);
    (self->ob_type->tp_free)((PyObject *) self);
}

PyTypeObject DBusPyLibDBusConnection_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                      /*ob_size*/
    "_dbus_bindings._LibDBusConnection",
    sizeof(DBusPyLibDBusConnection),
    0,                      /*tp_itemsize*/
    /* methods */
    (destructor)DBusPyLibDBusConnection_tp_dealloc,
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
    Py_TPFLAGS_DEFAULT,
    DBusPyLibDBusConnection_tp_doc,
};

dbus_bool_t
dbus_py_init_libdbus_conn_types(void)
{
    if (PyType_Ready(&DBusPyLibDBusConnection_Type) < 0)
        return FALSE;

    return TRUE;
}

dbus_bool_t
dbus_py_insert_libdbus_conn_types(PyObject *this_module)
{
    if (PyModule_AddObject(this_module, "_LibDBusConnection",
                           (PyObject *)&DBusPyLibDBusConnection_Type) < 0)
        return FALSE;

    return TRUE;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
