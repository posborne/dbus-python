/* Debug code for _dbus_bindings.
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
#include <stdlib.h>

void
_dbus_py_assertion_failed(const char *assertion)
{
    PyErr_SetString(PyExc_AssertionError, assertion);
#if 1 || defined(USING_DBG) || defined(FATAL_ASSERTIONS)
    /* print the Python stack, and dump core so we can see the C stack too */
    PyErr_Print();
    abort();
#endif
}

#ifdef USING_DBG
void
_dbus_py_whereami(void)
{
    PyObject *c, *v, *t;
    /* This is a little mad. We want to get the traceback without
    clearing the error indicator, if any. */
    PyErr_Fetch(&c, &v, &t); /* 3 new refs */
    Py_XINCREF(c); Py_XINCREF(v); Py_XINCREF(t); /* now we own 6 refs */
    PyErr_Restore(c, v, t); /* steals 3 refs */

    if (!PyErr_Occurred()) {
        PyErr_SetString(PyExc_AssertionError,
                        "No error, but plz provide traceback kthx");
    }
    PyErr_Print();

    PyErr_Restore(c, v, t); /* steals another 3 refs */
}

void
_dbus_py_dbg_exc(void)
{
    PyObject *c, *v, *t;
    /* This is a little mad. We want to get the traceback without
    clearing the error indicator. */
    PyErr_Fetch(&c, &v, &t); /* 3 new refs */
    Py_XINCREF(c); Py_XINCREF(v); Py_XINCREF(t); /* now we own 6 refs */
    PyErr_Restore(c, v, t); /* steals 3 refs */
    PyErr_Print();
    PyErr_Restore(c, v, t); /* steals another 3 refs */
}

void
_dbus_py_dbg_dump_message(DBusMessage *message)
{
    const char *s;
    fprintf(stderr, "DBusMessage at %p\n", message);

    s = dbus_message_get_destination(message);
    if (!s) s = "(null)";
    fprintf(stderr, "\tdestination %s\n", s);

    s = dbus_message_get_interface(message);
    if (!s) s = "(null)";
    fprintf(stderr, "\tinterface %s\n", s);

    s = dbus_message_get_member(message);
    if (!s) s = "(null)";
    fprintf(stderr, "\tmember %s\n", s);

    s = dbus_message_get_path(message);
    if (!s) s = "(null)";
    fprintf(stderr, "\tpath %s\n", s);
}
#endif
