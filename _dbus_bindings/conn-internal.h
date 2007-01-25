/* _dbus_bindings internal API. For use within _dbus_bindings only.
 *
 * Copyright (C) 2006 Collabora Ltd. <http://www.collabora.co.uk/>
 *
 * Licensed under the Academic Free License version 2.1
 *
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

extern struct PyMethodDef DBusPyConnection_tp_methods[];
extern DBusHandlerResult DBusPyConnection_HandleMessage(Connection *,
                                                        PyObject *,
                                                        PyObject *);
extern PyObject *DBusPyConnection_ExistingFromDBusConnection(DBusConnection *);
extern PyObject *DBusPyConnection_GetObjectPathHandlers(PyObject *self,
                                                        PyObject *path);

#endif
