/* D-Bus exception base classes.
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

static PyObject *imported_dbus_exception = NULL;

static dbus_bool_t
import_exception(void)
{
    PyObject *name;
    PyObject *exceptions;

    if (imported_dbus_exception != NULL) {
        return TRUE;
    }

    name = NATIVESTR_FROMSTR("dbus.exceptions");
    if (name == NULL) {
        return FALSE;
    }
    exceptions = PyImport_Import(name);
    Py_CLEAR(name);
    if (exceptions == NULL) {
        return FALSE;
    }
    imported_dbus_exception = PyObject_GetAttrString(exceptions,
                                                     "DBusException");
    Py_CLEAR(exceptions);

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

    if (!exc_value) {
        goto finally;
    }

    if (error->name) {
        PyObject *name = NATIVESTR_FROMSTR(error->name);
        int ret;

        if (!name)
            goto finally;
        ret = PyObject_SetAttrString(exc_value, "_dbus_error_name", name);
        Py_CLEAR(name);
        if (ret < 0) {
            goto finally;
        }
    }

    PyErr_SetObject(imported_dbus_exception, exc_value);

finally:
    Py_CLEAR(exc_value);
    dbus_error_free(error);
    return NULL;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
