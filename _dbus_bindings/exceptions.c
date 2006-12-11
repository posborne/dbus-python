/* D-Bus exception base classes.
 *
 * Copyright (C) 2006 Collabora Ltd.
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

#include "dbus_bindings-internal.h"

PyObject *DBusPyException;

PyDoc_STRVAR(DBusException__doc__, "Represents any D-Bus-related error.");

PyObject *
DBusPyException_ConsumeError(DBusError *error)
{
    PyErr_Format(DBusPyException, "%s: %s", error->name, error->message);
    dbus_error_free(error);
    return NULL;
}

dbus_bool_t
dbus_py_init_exception_types(void)
{
    PyObject *docstring;

    /* We call it dbus.DBusException because that's where you should import it
    from. */
    DBusPyException = PyErr_NewException("dbus.DBusException", NULL, NULL);
    if (!DBusPyException) return 0;
    docstring = PyString_FromString(DBusException__doc__);
    if (!docstring) return 0;
    if (PyObject_SetAttrString(DBusPyException, "__doc__", docstring)) return 0;
    Py_DECREF(docstring);
    return 1;
}

dbus_bool_t
dbus_py_insert_exception_types(PyObject *this_module)
{
    if (PyModule_AddObject(this_module, "DBusException", DBusPyException) < 0) {
        return 0;
    }
    return 1;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
