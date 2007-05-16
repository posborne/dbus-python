/* D-Bus exception base classes.
 *
 * Copyright (C) 2006 Collabora Ltd.
 *
 * Licensed under the Academic Free License version 2.1
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "dbus_bindings-internal.h"

static PyObject *imported_dbus_exception = NULL;

static dbus_bool_t
import_exception(void)
{
    PyObject *name;
    PyObject *exceptions;

    if (imported_dbus_exception != NULL) {
        return TRUE;
    }

    name = PyString_FromString("dbus.exceptions");
    if (name == NULL) {
        return FALSE;
    }
    exceptions = PyImport_Import(name);
    Py_DECREF(name);
    if (exceptions == NULL) {
        return FALSE;
    }
    imported_dbus_exception = PyObject_GetAttrString(exceptions,
                                                     "DBusException");
    Py_DECREF(exceptions);

    return (imported_dbus_exception != NULL);
}

PyObject *
DBusPyException_SetString(const char *msg)
{
    if (imported_dbus_exception != NULL || import_exception()) {
        PyErr_SetString(imported_dbus_exception, msg);
    }
    return NULL;
}

PyObject *
DBusPyException_ConsumeError(DBusError *error)
{
    PyObject *exc_value = NULL;

    if (imported_dbus_exception == NULL && !import_exception()) {
        goto finally;
    }

    exc_value = PyObject_CallFunction(imported_dbus_exception,
                                      "s",
                                      error->message ? error->message
                                                     : "");
    if (error->name) {
        PyObject *name = PyString_FromString(error->name);
        int ret;

        if (!name)
            goto finally;
        ret = PyObject_SetAttrString(exc_value, "_dbus_error_name", name);
        Py_DECREF(name);
        if (ret < 0) {
            goto finally;
        }
    }

    PyErr_SetObject(imported_dbus_exception, exc_value);

finally:
    Py_XDECREF(exc_value);
    dbus_error_free(error);
    return NULL;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
