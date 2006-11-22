/* C API for _dbus_bindings, used by _dbus_glib_bindings.
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

#ifndef DBUS_BINDINGS_H
#define DBUS_BINDINGS_H

#define DBUS_API_SUBJECT_TO_CHANGE 1
#include <dbus/dbus.h>

DBUS_BEGIN_DECLS

#define DBUS_BINDINGS_API_COUNT 3

#ifdef INSIDE_DBUS_BINDINGS

static DBusConnection *Connection_BorrowDBusConnection(PyObject *);
static PyObject *NativeMainLoop_New4(dbus_bool_t (*)(DBusConnection *, void *),
                                     dbus_bool_t (*)(DBusServer *, void *),
                                     void (*)(void *),
                                     void *);

#else

static void **dbus_bindings_API;

#define Connection_BorrowDBusConnection \
        (*(DBusConnection *(*)(PyObject *))dbus_bindings_API[1])
#define NativeMainLoop_New4 \
    ((PyObject *(*)(dbus_bool_t (*)(DBusConnection *, void *),\
                    dbus_bool_t (*)(DBusServer *, void *),\
                    void (*)(void *),\
                    void *))dbus_bindings_API[2])

static int
import_dbus_bindings(const char *this_module_name)
{
  PyObject *module = PyImport_ImportModule ("_dbus_bindings");
  int count;

  if (module != NULL) {
      PyObject *c_api = PyObject_GetAttrString (module, "_C_API");
      if (c_api == NULL) return -1;
      if (PyCObject_Check (c_api)) {
          dbus_bindings_API = (void **)PyCObject_AsVoidPtr (c_api);
      }
      else {
          Py_DECREF(c_api);
          PyErr_SetString(PyExc_RuntimeError, "C API is not a PyCObject");
          return -1;
      }
      Py_DECREF (c_api);
      count = *(int *)dbus_bindings_API[0];
      if (count < DBUS_BINDINGS_API_COUNT) {
          PyErr_Format(PyExc_RuntimeError,
                       "_dbus_bindings has API version %d but %s needs "
                       "_dbus_bindings API version at least %d",
                       count, this_module_name,
                       DBUS_BINDINGS_API_COUNT);
          return -1;
      }
  }
  return 0;
}

#endif

DBUS_END_DECLS

#endif
