/* Implementation of main-loop integration for dbus-python.
 *
 * Copyright (C) 2006 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008 Huang Peng <phuang@redhat.com>
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

#include "config.h"

#include "dbus_bindings-internal.h"

/* Native mainloop wrapper ========================================= */

PyDoc_STRVAR(NativeMainLoop_tp_doc,
"Object representing D-Bus main loop integration done in native code.\n"
"Cannot be instantiated directly.\n"
);

static PyTypeObject NativeMainLoop_Type;

DEFINE_CHECK(NativeMainLoop)

typedef struct {
    PyObject_HEAD
    /* Called with the GIL held, should set a Python exception on error */
    dbus_bool_t (*set_up_connection_cb)(DBusConnection *, void *);
    dbus_bool_t (*set_up_server_cb)(DBusServer *, void *);
    /* Called in a destructor. Must not touch the exception state (use
     * PyErr_Fetch and PyErr_Restore if necessary). */
    void (*free_cb)(void *);
    void *data;
} NativeMainLoop;

static void NativeMainLoop_tp_dealloc(NativeMainLoop *self)
{
    if (self->data && self->free_cb) {
        (self->free_cb)(self->data);
    }
    PyObject_Del((PyObject *)self);
}

static PyTypeObject NativeMainLoop_Type = {
    PyVarObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type), 0)
    "dbus.mainloop.NativeMainLoop",
    sizeof(NativeMainLoop),
    0,
    (destructor)NativeMainLoop_tp_dealloc,  /* tp_dealloc */
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
    NativeMainLoop_tp_doc,                  /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    0,                                      /* tp_iter */
    0,                                      /* tp_iternext */
    0,                                      /* tp_methods */
    0,                                      /* tp_members */
    0,                                      /* tp_getset */
    0,                                      /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    /* deliberately not callable! */
    0,                                      /* tp_new */
};

/* Internal C API for Connection, Bus, Server ======================= */

dbus_bool_t
dbus_py_check_mainloop_sanity(PyObject *mainloop)
{
    if (NativeMainLoop_Check(mainloop)) {
        return TRUE;
    }
    PyErr_SetString(PyExc_TypeError,
                    "A dbus.mainloop.NativeMainLoop instance is required");
    return FALSE;
}

dbus_bool_t
dbus_py_set_up_connection(PyObject *conn, PyObject *mainloop)
{
    if (NativeMainLoop_Check(mainloop)) {
        /* Native mainloops are allowed to do arbitrary strange things */
        NativeMainLoop *nml = (NativeMainLoop *)mainloop;
        DBusConnection *dbc = DBusPyConnection_BorrowDBusConnection(conn);

        if (!dbc) {
            return FALSE;
        }
        return (nml->set_up_connection_cb)(dbc, nml->data);
    }
    PyErr_SetString(PyExc_TypeError,
                    "A dbus.mainloop.NativeMainLoop instance is required");
    return FALSE;
}

dbus_bool_t
dbus_py_set_up_server(PyObject *server, PyObject *mainloop)
{
    if (NativeMainLoop_Check(mainloop)) {
        /* Native mainloops are allowed to do arbitrary strange things */
        NativeMainLoop *nml = (NativeMainLoop *)mainloop;
        DBusServer *dbs = DBusPyServer_BorrowDBusServer(server);

        if (!dbs) {
            return FALSE;
        }
        return (nml->set_up_server_cb)(dbs, nml->data);
    }
    PyErr_SetString(PyExc_TypeError,
                    "A dbus.mainloop.NativeMainLoop instance is required");
    return FALSE;
}

/* C API ============================================================ */

PyObject *
DBusPyNativeMainLoop_New4(dbus_bool_t (*conn_cb)(DBusConnection *, void *),
                          dbus_bool_t (*server_cb)(DBusServer *, void *),
                          void (*free_cb)(void *),
                          void *data)
{
    NativeMainLoop *self = PyObject_New(NativeMainLoop, &NativeMainLoop_Type);
    if (self) {
        self->data = data;
        self->free_cb = free_cb;
        self->set_up_connection_cb = conn_cb;
        self->set_up_server_cb = server_cb;
    }
    return (PyObject *)self;
}

/* Null mainloop implementation ===================================== */

static dbus_bool_t
noop_main_loop_cb(void *conn_or_server UNUSED, void *data UNUSED)
{
    return TRUE;
}

#define noop_conn_cb ((dbus_bool_t (*)(DBusConnection *, void *))(noop_main_loop_cb))
#define noop_server_cb ((dbus_bool_t (*)(DBusServer *, void *))(noop_main_loop_cb))

/* Initialization =================================================== */

dbus_bool_t
dbus_py_init_mainloop(void)
{
    if (PyType_Ready (&NativeMainLoop_Type) < 0) return 0;

    return 1;
}

dbus_bool_t
dbus_py_insert_mainloop_types(PyObject *this_module)
{
    PyObject *null_main_loop = DBusPyNativeMainLoop_New4(noop_conn_cb,
                                                         noop_server_cb,
                                                         NULL,
                                                         NULL);
    if (!null_main_loop) return 0;

    /* PyModule_AddObject steals a ref */
    Py_INCREF (&NativeMainLoop_Type);
    if (PyModule_AddObject (this_module, "NativeMainLoop",
                            (PyObject *)&NativeMainLoop_Type) < 0) return 0;
    if (PyModule_AddObject (this_module, "NULL_MAIN_LOOP",
                            null_main_loop) < 0) return 0;
    return 1;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
