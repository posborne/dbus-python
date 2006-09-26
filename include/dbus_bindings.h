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

#ifdef INSIDE_DBUS_BINDINGS

static DBusConnection *Connection_BorrowDBusConnection (PyObject *);

#else

static void **dbus_bindings_API;
#define Connection_BorrowDBusConnection \
        (*(DBusConnection *(*)(PyObject *))dbus_bindings_API[0])
static int
import_dbus_bindings(void)
{
  PyObject *module = PyImport_ImportModule ("_dbus_bindings");

  if (module != NULL)
    {
      PyObject *c_api = PyObject_GetAttrString (module, "_C_API");
      if (c_api == NULL) return -1;
      if (PyCObject_Check (c_api))
        {
          dbus_bindings_API = (void **)PyCObject_AsVoidPtr (c_api);
        }
      Py_DECREF (c_api);
    }
  return 0;
}

#endif

DBUS_END_DECLS

#endif
