/* Debug code for _dbus_bindings.
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
