/* _dbus_bindings internal API. For use within _dbus_bindings only.
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

#ifndef DBUS_BINDINGS_SERVER_H
#define DBUS_BINDINGS_SERVER_H

#include "dbus_bindings-internal.h"

typedef struct {
    PyObject_HEAD
    DBusServer *server;

	PyObject *new_connection_callback;
	PyTypeObject *new_connection_type;
    /* Weak-references list to make Connections weakly referenceable */
    PyObject *weaklist;

    PyObject *mainloop; 
} Server;

extern struct PyMethodDef DBusPyServer_tp_methods[];
extern DBusHandlerResult DBusPyServer_HandleMessage(Server *,
                                                    PyObject *,
                                                    PyObject *);
extern PyObject *DBusPyServer_ExistingFromDBusServer(DBusServer *);

extern PyObject *DBusPyServer_GetAddress(Server *, PyObject *);

#endif
