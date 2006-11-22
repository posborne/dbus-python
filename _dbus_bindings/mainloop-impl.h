/* Implementation of main-loop integration for dbus-python.
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

/* Native mainloop wrapper ========================================= */

PyDoc_STRVAR(NativeMainLoop_tp_doc,
"Object representing D-Bus main loop integration done in native code.\n"
"Cannot be instantiated directly.\n"
);

static PyTypeObject NativeMainLoopType;

DEFINE_CHECK(NativeMainLoop)

typedef struct {
    PyObject_HEAD
    /* Called with the GIL held, should set a Python exception on error */
    dbus_bool_t (*set_up_connection_cb)(DBusConnection *, void *);
    dbus_bool_t (*set_up_server_cb)(DBusServer *, void *);
    /* Called in a destructor. */
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

static PyTypeObject NativeMainLoopType = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
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

static dbus_bool_t
check_mainloop_sanity(PyObject *mainloop)
{
    if (NativeMainLoop_Check(mainloop)) {
        return TRUE;
    }
    PyErr_SetString(PyExc_TypeError,
                    "A dbus.mainloop.NativeMainLoop instance is required");
    return FALSE;
}

static dbus_bool_t
dbus_python_set_up_connection(Connection *conn, PyObject *mainloop)
{
    if (NativeMainLoop_Check(mainloop)) {
        /* Native mainloops are allowed to do arbitrary strange things */
        NativeMainLoop *nml = (NativeMainLoop *)mainloop;
        return (nml->set_up_connection_cb)(conn->conn, nml->data);
    }
    PyErr_SetString(PyExc_TypeError,
                    "A dbus.mainloop.NativeMainLoop instance is required");
    return FALSE;
}

/* C API ============================================================ */

static PyObject *
NativeMainLoop_New4(dbus_bool_t (*conn_cb)(DBusConnection *, void *),
                    dbus_bool_t (*server_cb)(DBusServer *, void *),
                    void (*free_cb)(void *),
                    void *data)
{
    NativeMainLoop *self = PyObject_New(NativeMainLoop, &NativeMainLoopType);
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

static inline int
init_mainloop (void)
{
    if (PyType_Ready (&NativeMainLoopType) < 0) return 0;
    return 1;
}

static inline int
insert_mainloop_types (PyObject *this_module)
{
    PyObject *null_main_loop = NativeMainLoop_New4(noop_conn_cb,
                                                   noop_server_cb,
                                                   NULL,
                                                   NULL);
    if (!null_main_loop) return 0;

    if (PyModule_AddObject (this_module, "NativeMainLoop",
                            (PyObject *)&NativeMainLoopType) < 0) return 0;
    if (PyModule_AddObject (this_module, "NULL_MAIN_LOOP",
                            null_main_loop) < 0) return 0;
    return 1;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
