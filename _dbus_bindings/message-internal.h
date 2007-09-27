/* D-Bus message: implementation internals
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

#include <Python.h>

#include "dbus_bindings-internal.h"

#ifndef DBUS_BINDINGS_MESSAGE_INTERNAL_H
#define DBUS_BINDINGS_MESSAGE_INTERNAL_H

typedef struct {
    PyObject_HEAD
    DBusMessage *msg;
} Message;

extern char dbus_py_Message_append__doc__[];
extern PyObject *dbus_py_Message_append(Message *, PyObject *, PyObject *);
extern char dbus_py_Message_guess_signature__doc__[];
extern PyObject *dbus_py_Message_guess_signature(PyObject *, PyObject *);
extern char dbus_py_Message_get_args_list__doc__[];
extern PyObject *dbus_py_Message_get_args_list(Message *,
                                               PyObject *,
                                               PyObject *);

extern PyObject *DBusPy_RaiseUnusableMessage(void);

#endif
