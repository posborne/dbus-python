/* Implementation of PendingCall helper type for D-Bus bindings.
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

PyDoc_STRVAR(PendingCall_tp_doc,
"Object representing a pending D-Bus call, returned by\n"
"Connection.send_message_with_reply(). Cannot be instantiated directly.\n"
);

static PyTypeObject PendingCallType;

static inline int PendingCall_Check (PyObject *o)
{
    return (o->ob_type == &PendingCallType)
            || PyObject_IsInstance(o, (PyObject *)&PendingCallType);
}

typedef struct {
    PyObject_HEAD
    DBusPendingCall *pc;
} PendingCall;

PyDoc_STRVAR(PendingCall_cancel__doc__,
"cancel()\n\n"
"Cancel this pending call. Its reply will be ignored and the associated\n"
"reply handler will never be called.\n");
static PyObject *
PendingCall_cancel(PendingCall *self, PyObject *unused UNUSED)
{
    Py_BEGIN_ALLOW_THREADS
    dbus_pending_call_cancel(self->pc);
    Py_END_ALLOW_THREADS
    Py_RETURN_NONE;
}

PyDoc_STRVAR(PendingCall_block__doc__,
"block()\n\n"
"Block until this pending call has completed and the associated\n"
"reply handler has been called.\n"
"\n"
"This can lead to a deadlock, if the called method tries to make a\n"
"synchronous call to a method in this application.\n");
static PyObject *
PendingCall_block(PendingCall *self, PyObject *unused UNUSED)
{
    Py_BEGIN_ALLOW_THREADS
    dbus_pending_call_block(self->pc);
    Py_END_ALLOW_THREADS
    Py_RETURN_NONE;
}

static void
_pending_call_notify_function(DBusPendingCall *pc,
                              PyObject *list)
{
    PyGILState_STATE gil = PyGILState_Ensure();
    /* BEGIN CRITICAL SECTION
     * While holding the GIL, make sure the callback only gets called once
     * by deleting it from the 1-item list that's held by libdbus.
     */
    PyObject *handler = PyList_GetItem(list, 0);
    DBusMessage *msg;

    if (!handler) {
        PyErr_Print();
        goto release;
    }
    if (handler == Py_None) {
        /* We've already called (and thrown away) the callback */
        goto release;
    }
    Py_INCREF(handler);     /* previously borrowed from the list, now owned */
    Py_INCREF(Py_None);     /* take a ref so SetItem can steal it */
    PyList_SetItem(list, 0, Py_None);
    /* END CRITICAL SECTION */

    msg = dbus_pending_call_steal_reply(pc);

    if (!msg) {
        /* omg, what happened here? the notify should only get called
         * when we have a reply */
        PyErr_Warn(PyExc_UserWarning, "D-Bus notify function was called "
                   "for an incomplete pending call (shouldn't happen)");
    } else {
        PyObject *msg_obj = DBusPyMessage_ConsumeDBusMessage(msg);

        if (msg_obj) {
            PyObject *ret = PyObject_CallFunctionObjArgs(handler, msg_obj, NULL);

            if (!ret) {
                PyErr_Print();
            }
            Py_XDECREF(ret);
            Py_DECREF(msg_obj);
        }
        /* else OOM has happened - not a lot we can do about that,
         * except possibly making it fatal (FIXME?) */
    }

release:
    Py_XDECREF(handler);
    PyGILState_Release(gil);
}

PyDoc_STRVAR(PendingCall_get_completed__doc__,
"get_completed() -> bool\n\n"
"Return true if this pending call has completed.\n\n"
"If so, its associated reply handler has been called and it is no\n"
"longer meaningful to cancel it.\n");
static PyObject *
PendingCall_get_completed(PendingCall *self, PyObject *unused UNUSED)
{
    dbus_bool_t ret;

    Py_BEGIN_ALLOW_THREADS
    ret = dbus_pending_call_get_completed(self->pc);
    Py_END_ALLOW_THREADS
    return PyBool_FromLong(ret);
}

/* Steals the reference to the pending call. */
PyObject *
DBusPyPendingCall_ConsumeDBusPendingCall(DBusPendingCall *pc,
                                         PyObject *callable)
{
    dbus_bool_t ret;
    PyObject *list = PyList_New(1);
    PendingCall *self = PyObject_New(PendingCall, &PendingCallType);

    if (!list || !self) {
        Py_XDECREF(list);
        Py_XDECREF(self);
        Py_BEGIN_ALLOW_THREADS
        dbus_pending_call_cancel(pc);
        dbus_pending_call_unref(pc);
        Py_END_ALLOW_THREADS
        return NULL;
    }

    /* INCREF because SET_ITEM steals a ref */
    Py_INCREF(callable);
    PyList_SET_ITEM(list, 0, callable);

    /* INCREF so we can give a ref to set_notify and still have one */
    Py_INCREF(list);

    Py_BEGIN_ALLOW_THREADS
    ret = dbus_pending_call_set_notify(pc,
        (DBusPendingCallNotifyFunction)_pending_call_notify_function,
        (void *)list, (DBusFreeFunction)dbus_py_take_gil_and_xdecref);
    Py_END_ALLOW_THREADS

    if (!ret) {
        PyErr_NoMemory();
        /* DECREF twice - one for the INCREF and one for the allocation */
        Py_DECREF(list);
        Py_DECREF(list);
        Py_DECREF(self);
        Py_BEGIN_ALLOW_THREADS
        dbus_pending_call_cancel(pc);
        dbus_pending_call_unref(pc);
        Py_END_ALLOW_THREADS
        return NULL;
    }

    /* As Alexander Larsson pointed out on dbus@lists.fd.o on 2006-11-30,
     * the API has a race condition if set_notify runs in one thread and a
     * mail loop runs in another - if the reply gets in before set_notify
     * runs, the notify isn't called and there is no indication of error.
     *
     * The workaround is to check for completion immediately, but this also
     * has a race which might lead to getting the notify called twice if
     * we're unlucky. So I use the list to arrange for the notify to be
     * deleted before it's called for the second time. The GIL protects
     * the critical section in which I delete the callback from the list.
     */
    if (dbus_pending_call_get_completed(pc)) {
        /* the first race condition happened, so call the callable here.
         * FIXME: we ought to arrange for the callable to run from the
         * mainloop thread, like it would if the race hadn't happened...
         * this needs a better mainloop abstraction, though.
         */
        _pending_call_notify_function(pc, list);
    }

    Py_DECREF(list);
    self->pc = pc;
    return (PyObject *)self;
}

static void
PendingCall_tp_dealloc (PendingCall *self)
{
    if (self->pc) {
        Py_BEGIN_ALLOW_THREADS
        dbus_pending_call_unref(self->pc);
        Py_END_ALLOW_THREADS
    }
    PyObject_Del (self);
}

static PyMethodDef PendingCall_tp_methods[] = {
    {"block", (PyCFunction)PendingCall_block, METH_NOARGS,
     PendingCall_block__doc__},
    {"cancel", (PyCFunction)PendingCall_cancel, METH_NOARGS,
     PendingCall_cancel__doc__},
    {"get_completed", (PyCFunction)PendingCall_get_completed, METH_NOARGS,
     PendingCall_get_completed__doc__},
    {NULL, NULL, 0, NULL}
};

static PyTypeObject PendingCallType = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
    "dbus.lowlevel.PendingCall",
    sizeof(PendingCall),
    0,
    (destructor)PendingCall_tp_dealloc,     /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_compare */
    0,                                      /* tp_repr */
    0,                                      /* tp_as_number */
    0,                                      /* tp_as_sequence */
    0,                                      /* tp_as_mapping */
    0,                                      /* tp_hash */
    0,                                      /* tp_call */
    0,                                      /* tp_str */
    0,                                      /* tp_getattro */
    0,                                      /* tp_setattro */
    0,                                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                     /* tp_flags */
    PendingCall_tp_doc,                     /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    0,                                      /* tp_iter */
    0,                                      /* tp_iternext */
    PendingCall_tp_methods,                 /* tp_methods */
    0,                                      /* tp_members */
    0,                                      /* tp_getset */
    0,                                      /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    /* deliberately not callable! Use PendingCall_ConsumeDBusPendingCall */
    0,                                      /* tp_new */
};

dbus_bool_t
dbus_py_init_pending_call (void)
{
    if (PyType_Ready (&PendingCallType) < 0) return 0;
    return 1;
}

dbus_bool_t
dbus_py_insert_pending_call (PyObject *this_module)
{
    if (PyModule_AddObject (this_module, "PendingCall",
                            (PyObject *)&PendingCallType) < 0) return 0;
    return 1;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
