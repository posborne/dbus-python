/* Glue code to attach the GObject main loop to D-Bus from within Python.
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

#include <Python.h>
#include "dbus_bindings.h"
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

PyDoc_STRVAR(module_doc, "");

PyDoc_STRVAR(setup_with_g_main__doc__,
"setup_with_g_main(conn: dbus.Connection)");
static PyObject *
setup_with_g_main (PyObject *unused, PyObject *args)
{
    DBusConnection *dbc;
    PyObject *conn;
    if (!PyArg_ParseTuple(args, "O:setup_with_g_main", &conn)) return NULL;

    dbc = Connection_BorrowDBusConnection (conn);
    if (!dbc) return NULL;
    dbus_connection_setup_with_g_main (dbc, NULL);
    Py_RETURN_NONE;
}

PyDoc_STRVAR(gthreads_init__doc__,
"gthreads_init()");
static PyObject *
gthreads_init (PyObject *unused, PyObject *also_unused)
{
    dbus_g_thread_init();
    Py_RETURN_NONE;
}

static PyMethodDef module_functions[] = {
    {"setup_with_g_main", setup_with_g_main, METH_VARARGS,
     setup_with_g_main__doc__},
    {"gthreads_init", gthreads_init, METH_NOARGS, gthreads_init__doc__},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
init_dbus_glib_bindings (void)
{
    PyObject *this_module;

    if (import_dbus_bindings () < 0) return;
    this_module = Py_InitModule3 ("_dbus_glib_bindings", module_functions,
                                  module_doc);
    if (!this_module) return;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
