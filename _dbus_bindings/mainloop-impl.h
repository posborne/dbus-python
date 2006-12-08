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

/* Watch ============================================================ */

PyDoc_STRVAR(Watch_tp_doc,
"Object representing a watched file descriptor.\n"
"Cannot be instantiated from Python.\n"
);

static PyTypeObject WatchType;

DEFINE_CHECK(Watch)

typedef struct {
    PyObject_HEAD
    DBusWatch *watch;
} Watch;

PyDoc_STRVAR(Watch_fileno__doc__,
"fileno() -> int\n\n"
"Return the watched file descriptor.\n");
static PyObject *
Watch_fileno(Watch *self, PyObject *unused UNUSED)
{
    int fd;

    if (!self->watch) {
        DBG("Attempted fileno() on broken Watch at %p", self);
        PyErr_SetString(PyExc_ValueError, "FD watch is no longer valid");
        return NULL;
    }
    fd = dbus_watch_get_fd(self->watch);
    DBG("Watch at %p (wrapping DBusWatch at %p) has fileno %d", self,
        self->watch, fd);
    return PyInt_FromLong(fd);
}

PyDoc_STRVAR(Watch_get_flags__doc__,
"get_flags() -> int\n\n"
"Return 0, WATCH_READABLE, WATCH_WRITABLE or WATCH_READABLE|WATCH_WRITABLE.\n");
static PyObject *
Watch_get_flags(Watch *self, PyObject *unused UNUSED)
{
    unsigned int flags;

    if (!self->watch) {
        DBG("Attempted get_flags() on broken Watch at %p", self);
        PyErr_SetString(PyExc_ValueError, "FD watch is no longer valid");
        return NULL;
    }
    flags = dbus_watch_get_flags(self->watch);
    DBG("Watch at %p (wrapping DBusWatch at %p) has flags 0x%x (%s,%s)",
        self, self->watch, flags, flags & DBUS_WATCH_READABLE ? "read" : ".",
        flags & DBUS_WATCH_WRITABLE ? "write" : ".");
    return PyInt_FromLong((long)flags);
}

PyDoc_STRVAR(Watch_get_enabled__doc__,
"get_enabled() -> bool\n\n"
"Return True if this watch is currently active.\n");
static PyObject *
Watch_get_enabled(Watch *self, PyObject *unused UNUSED)
{
    dbus_bool_t enabled;

    if (!self->watch) {
        DBG("Attempted get_enabled() on broken Watch at %p", self);
        PyErr_SetString(PyExc_ValueError, "FD watch is no longer valid");
        return NULL;
    }
    enabled = dbus_watch_get_enabled(self->watch);
    DBG("Watch at %p (wrapping DBusWatch at %p) is %s", self,
        self->watch, enabled ? "enabled" : "disabled");
    return PyBool_FromLong(enabled);
}

PyDoc_STRVAR(Watch_handle__doc__,
"handle(flags)\n\n"
"To be called when the file descriptor is closed or reports error,\n"
"or enters a readable/writable state for which notification is required\n"
"(according to get_flags()).");
static PyObject *
Watch_handle(Watch *self, PyObject *args)
{
    unsigned int flags;
    if (!self->watch) {
        DBG("Attempted handle() on broken Watch at %p", self);
        PyErr_SetString(PyExc_ValueError, "FD watch is no longer valid");
        return NULL;
    }
    if (!PyArg_ParseTuple(args, "i", &flags)) {
        DBG("Python passed junk to <Watch at %p>.handle()", self);
        return NULL;
    }
    DBG("Watch at %p (wrapping DBusWatch at %p) handle(0x%x) (%s,%s)",
        self, self->watch, flags, flags & DBUS_WATCH_READABLE ? "read" : ".",
        flags & DBUS_WATCH_WRITABLE ? "write" : ".");
    Py_BEGIN_ALLOW_THREADS
    dbus_watch_handle(self->watch, (unsigned int)flags);
    Py_END_ALLOW_THREADS
    Py_RETURN_NONE;
}

/* Arranges for the watch to be referenced by its corresponding DBusWatch.
 * Returns a borrowed reference, or NULL with a Python exception.
 */
static PyObject *
Watch_BorrowFromDBusWatch(DBusWatch *watch, PyObject *mainloop)
{
    Watch *self = (Watch *)dbus_watch_get_data(watch);

    if (self)
        return (PyObject *)self;
    if (!mainloop) {
        PyErr_SetString(PyExc_AssertionError,
                        "Attempted to use a non-added watch");
        return NULL;
    }

    self = PyObject_New(Watch, &WatchType);
    if (!self) {
        return NULL;
    }
    self->watch = watch;

    Py_INCREF(self);
    dbus_watch_set_data(watch, self,
                        (DBusFreeFunction)dbus_py_take_gil_and_xdecref);
    return (PyObject *)self;
}

static PyMethodDef Watch_tp_methods[] = {
    {"fileno", (PyCFunction)Watch_fileno, METH_NOARGS,
     Watch_fileno__doc__},
    {"get_flags", (PyCFunction)Watch_get_flags, METH_NOARGS,
     Watch_get_flags__doc__},
    {"handle", (PyCFunction)Watch_handle, METH_VARARGS,
     Watch_handle__doc__},
    {"get_enabled", (PyCFunction)Watch_get_enabled, METH_NOARGS,
     Watch_get_enabled__doc__},
    {NULL, NULL, 0, NULL}
};

static void Watch_tp_dealloc(PyObject *self)
{
    PyObject_Del(self);
}

static PyTypeObject WatchType = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
    "_dbus_bindings.Watch",
    sizeof(Watch),
    0,
    Watch_tp_dealloc,                       /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_compare */
    0,                                      /* tp_repr */
    0,                                      /* tp_as_number */
    0,                                      /* tp_as_sequence */
    0,                                      /* tp_as_mapping */
    Glue_tp_hash_by_pointer,                /* tp_hash */
    0,                                      /* tp_call */
    0,                                      /* tp_str */
    0,                                      /* tp_getattro */
    0,                                      /* tp_setattro */
    0,                                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                     /* tp_flags */
    Watch_tp_doc,                           /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    Glue_tp_richcompare_by_pointer,         /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    0,                                      /* tp_iter */
    0,                                      /* tp_iternext */
    Watch_tp_methods,                       /* tp_methods */
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

/* Timeout ========================================================== */

PyDoc_STRVAR(Timeout_tp_doc,
"Object representing a watched file descriptor.\n"
"Cannot be instantiated from Python.\n"
);

static PyTypeObject TimeoutType;

DEFINE_CHECK(Timeout)

typedef struct {
    PyObject_HEAD
    DBusTimeout *timeout;
} Timeout;

PyDoc_STRVAR(Timeout_get_interval__doc__,
"get_interval() -> float\n\n"
"Return the interval in seconds.\n");
static PyObject *
Timeout_get_interval(Timeout *self, PyObject *unused UNUSED)
{
    double interval;
    if (!self->timeout) {
        DBG("Attempted get_interval() on broken Timeout at %p", self);
        PyErr_SetString(PyExc_ValueError, "Timeout object is no longer valid");
        return NULL;
    }
    interval = ((double)dbus_timeout_get_interval(self->timeout)) / 1000.0;
    DBG("Timeout at %p (wrapping DBusTimeout at %p) has interval %f s", self,
        self->timeout, interval);
    return PyFloat_FromDouble(interval);
}

PyDoc_STRVAR(Timeout_get_enabled__doc__,
"get_enabled() -> bool\n\n"
"Return True if this timeout is currently active.\n");
static PyObject *
Timeout_get_enabled(Timeout *self, PyObject *unused UNUSED)
{
    dbus_bool_t enabled;

    if (!self->timeout) {
        DBG("Attempted get_enabled() on broken Timeout at %p", self);
        PyErr_SetString(PyExc_ValueError, "Timeout object is no longer valid");
        return NULL;
    }
    enabled = dbus_timeout_get_enabled(self->timeout);
    DBG("Timeout at %p (wrapping DBusTimeout at %p) is %s", self,
        self->timeout, enabled ? "enabled" : "disabled");
    return PyBool_FromLong(enabled);
}

PyDoc_STRVAR(Timeout_handle__doc__,
"handle()\n\n"
"To be called when timeout occurs.\n");
static PyObject *
Timeout_handle(Timeout *self, PyObject *unused UNUSED)
{
    if (!self->timeout) {
        DBG("Attempted handle() on broken Timeout at %p", self);
        PyErr_SetString(PyExc_ValueError, "Timeout object is no longer valid");
        return NULL;
    }
    DBG("Timeout_handle() with self->timeout=%p", self->timeout);
    Py_BEGIN_ALLOW_THREADS
    dbus_timeout_handle(self->timeout);
    Py_END_ALLOW_THREADS
    Py_RETURN_NONE;
}

/* Returns a borrowed reference */
static PyObject *
Timeout_BorrowFromDBusTimeout(DBusTimeout *timeout, PyObject *mainloop)
{
    Timeout *self = (Timeout *)dbus_timeout_get_data(timeout);
    if (self)
        return (PyObject *)self;
    if (!mainloop) {
        PyErr_SetString(PyExc_AssertionError,
                        "Attempted to use a non-added timeout");
        return NULL;
    }

    self = PyObject_New(Timeout, &TimeoutType);
    if (!self) {
        return NULL;
    }
    self->timeout = timeout;

    Py_INCREF(self);
    dbus_timeout_set_data(timeout, self,
                          (DBusFreeFunction)dbus_py_take_gil_and_xdecref);
    return (PyObject *)self;
}

static PyMethodDef Timeout_tp_methods[] = {
    {"get_interval", (PyCFunction)Timeout_get_interval, METH_NOARGS,
     Timeout_get_interval__doc__},
    {"handle", (PyCFunction)Timeout_handle, METH_NOARGS,
     Timeout_handle__doc__},
    {"get_enabled", (PyCFunction)Timeout_get_enabled, METH_NOARGS,
     Timeout_get_enabled__doc__},
    {NULL, NULL, 0, NULL}
};

static void Timeout_tp_dealloc(PyObject *self)
{
    PyObject_Del(self);
}

static PyTypeObject TimeoutType = {
    PyObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type))
    0,
    "_dbus_bindings.Timeout",
    sizeof(Timeout),
    0,
    Timeout_tp_dealloc,                     /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_compare */
    0,                                      /* tp_repr */
    0,                                      /* tp_as_number */
    0,                                      /* tp_as_sequence */
    0,                                      /* tp_as_mapping */
    Glue_tp_hash_by_pointer,                /* tp_hash */
    0,                                      /* tp_call */
    0,                                      /* tp_str */
    0,                                      /* tp_getattro */
    0,                                      /* tp_setattro */
    0,                                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                     /* tp_flags */
    Timeout_tp_doc,                         /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    Glue_tp_richcompare_by_pointer,         /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    0,                                      /* tp_iter */
    0,                                      /* tp_iternext */
    Timeout_tp_methods,                     /* tp_methods */
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

/* Internal C API for Connection, Bus, Server ======================= */

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

/* The main loop if none is passed to the constructor */
static PyObject *default_main_loop;

/* Return a new reference to the default main loop */
PyObject *
dbus_py_get_default_main_loop(void)
{
    if (!default_main_loop) {
        Py_RETURN_NONE;
    }
    Py_INCREF(default_main_loop);
    return default_main_loop;
}

/* Python API ======================================================= */

PyDoc_STRVAR(get_default_main_loop__doc__,
"get_default_main_loop() -> object\n\n"
"Return the global default dbus-python main loop wrapper, which is used\n"
"when no main loop wrapper is passed to the Connection constructor.\n"
"\n"
"If None, there is no default and you must always pass the mainloop\n"
"parameter to the constructor. This will be the case until\n"
"set_default_main_loop is called.\n");
static PyObject *
get_default_main_loop(PyObject *always_null UNUSED,
                      PyObject *no_args UNUSED)
{
    return dbus_py_get_default_main_loop();
}

PyDoc_STRVAR(set_default_main_loop__doc__,
"set_default_main_loop(object)\n\n"
"Change the global default dbus-python main loop wrapper, which is used\n"
"when no main loop wrapper is passed to the Connection constructor.\n"
"\n"
"If None, return to the initial situation: there is no default, and you\n"
"must always pass the mainloop parameter to the constructor.\n"
"\n"
"Two types of main loop wrapper are planned in dbus-python.\n"
"Native main-loop wrappers are instances of dbus.mainloop.NativeMainLoop\n"
"supplied by extension modules like `dbus.mainloop.glib`: they have no\n"
"Python API, but connect themselves to ``libdbus`` using native code.\n"

"Python main-loop wrappers are not yet implemented. They will be objects\n"
"supporting the interface defined by `dbus.mainloop.MainLoop`, with an\n"
"API entirely based on Python methods.\n"
"\n"
);
static PyObject *
set_default_main_loop(PyObject *always_null UNUSED,
                      PyObject *args)
{
    PyObject *new_loop, *old_loop;

    if (!PyArg_ParseTuple(args, "O", &new_loop)) {
        return NULL;
    }
    if (!check_mainloop_sanity(new_loop)) {
        return NULL;
    }
    old_loop = default_main_loop;
    Py_INCREF(new_loop);
    default_main_loop = new_loop;
    Py_XDECREF(old_loop);
    Py_RETURN_NONE;
}

/* C API ============================================================ */

PyObject *
DBusPyNativeMainLoop_New4(dbus_bool_t (*conn_cb)(DBusConnection *, void *),
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
    default_main_loop = NULL;

    if (PyType_Ready (&WatchType) < 0) return 0;
    if (PyType_Ready (&TimeoutType) < 0) return 0;
    if (PyType_Ready (&NativeMainLoopType) < 0) return 0;

    /* placate -Wunused */
    (void)&Watch_BorrowFromDBusWatch;
    (void)&Timeout_BorrowFromDBusTimeout;

    return 1;
}

static inline int
insert_mainloop_types (PyObject *this_module)
{
    PyObject *null_main_loop = DBusPyNativeMainLoop_New4(noop_conn_cb,
                                                         noop_server_cb,
                                                         NULL,
                                                         NULL);
    if (!null_main_loop) return 0;

    if (PyModule_AddObject (this_module, "Watch",
                            (PyObject *)&WatchType) < 0) return 0;
    if (PyModule_AddObject (this_module, "Timeout",
                            (PyObject *)&TimeoutType) < 0) return 0;
    if (PyModule_AddObject (this_module, "NativeMainLoop",
                            (PyObject *)&NativeMainLoopType) < 0) return 0;
    if (PyModule_AddObject (this_module, "NULL_MAIN_LOOP",
                            null_main_loop) < 0) return 0;
    return 1;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
