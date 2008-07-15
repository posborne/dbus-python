/* _dbus_bindings internal API. For use within _dbus_bindings only.
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

#ifndef DBUS_BINDINGS_CONN_H
#define DBUS_BINDINGS_CONN_H

#include "dbus_bindings-internal.h"

typedef struct {
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

    /* Weak-references list to make Connections weakly referenceable */
    PyObject *weaklist;

    dbus_bool_t has_mainloop;
} Connection;

typedef struct {
    PyObject_HEAD
    DBusConnection *conn;
} DBusPyLibDBusConnection;

extern struct PyMethodDef DBusPyConnection_tp_methods[];
extern DBusHandlerResult DBusPyConnection_HandleMessage(Connection *,
                                                        PyObject *,
                                                        PyObject *);
extern PyObject *DBusPyConnection_ExistingFromDBusConnection(DBusConnection *);
extern PyObject *DBusPyConnection_GetObjectPathHandlers(PyObject *self,
                                                        PyObject *path);

extern PyObject *DBusPyConnection_NewForBus(PyTypeObject *cls, PyObject *args,
                                            PyObject *kwargs);
extern PyObject *DBusPyConnection_SetUniqueName(Connection *, PyObject *);
extern PyObject *DBusPyConnection_GetUniqueName(Connection *, PyObject *);

#endif
