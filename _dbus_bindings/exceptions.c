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
    PyErr_Format(DBusPyException, "%s: %s",
                 error->name, error->message);
    dbus_error_free(error);
    return NULL;
}

dbus_bool_t
dbus_py_init_exception_types(void)
{
    PyObject *bases;
    PyObject *dict = PyDict_New();
    PyObject *name;

    if (!dict)
        return 0;
    if (PyDict_SetItemString(dict, "__doc__",
                             PyString_FromString(DBusException__doc__)) < 0)
        return 0;
    if (PyDict_SetItemString(dict, "__module__",
                             PyString_FromString("dbus")) < 0)
        return 0;
    bases = Py_BuildValue("(O)", (PyObject *)PyExc_Exception);
    if (!bases) {
        Py_DECREF(dict);
        return 0;
    }
    name = PyString_FromString("DBusException");
    if (!name) {
        Py_DECREF(dict);
        Py_DECREF(bases);
        return 0;
    }
    DBusPyException = PyClass_New(bases, dict, name);
    Py_DECREF(bases);
    Py_DECREF(dict);
    if (!DBusPyException)
        return 0;
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
