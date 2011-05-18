/* Simple D-Bus types: Unix FD type.
 *
 * Copyright (C) 2006 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2010 Signove  <http://www.signove.com>
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

#include "types-internal.h"

PyDoc_STRVAR(UnixFd_tp_doc,
"An Unix Fd.\n"
"\n"
"Constructor::\n"
"\n"
"    dbus.UnixFd(value: int or file object[, variant_level: int]) -> UnixFd\n"
"\n"
"``value`` must be the integer value of a file descriptor, or an object that\n"
"implements the fileno() method. Otherwise, `ValueError` will be\n"
"raised.\n"
"\n"
"UnixFd keeps a dup() (duplicate) of the supplied file descriptor. The\n"
"caller remains responsible for closing the original fd.\n"
"\n"
"``variant_level`` must be non-negative; the default is 0.\n"
"\n"
":IVariables:\n"
"  `variant_level` : int\n"
"    Indicates how many nested Variant containers this object\n"
"    is contained in: if a message's wire format has a variant containing a\n"
"    variant containing an Unix Fd, this is represented in Python by an\n"
"    Unix Fd with variant_level==2.\n"
);

typedef struct {
    PyObject_HEAD
    int fd;
} UnixFdObject;

static PyObject *
UnixFd_tp_new(PyTypeObject *cls, PyObject *args, PyObject *kwargs UNUSED)
{
    UnixFdObject *self = NULL;
    PyObject *arg;
    PyObject *fdnumber;
    int fd_original, fd;

    if (! PyArg_ParseTuple(args, "O", &arg, NULL)) {
        return NULL;
    }

    if (PyInt_Check(arg)) {
        fd_original = PyInt_AsLong(arg);
        fd = dup(fd_original);
        if (fd < 0) {
            PyErr_Format(PyExc_ValueError, "Invalid file descriptor");
            return NULL;
        }

    } else if (PyObject_HasAttrString(arg, "fileno")) {
        fdnumber = PyObject_CallMethod(arg, "fileno", NULL);
        if (! fdnumber) {
            PyErr_Format(PyExc_ValueError, "Argument's fileno() method "
                                            "is not callable");
            return NULL;
        }
        if (! PyInt_Check(fdnumber)) {
            PyErr_Format(PyExc_ValueError, "Argument's fileno() method "
                                            "returned a non-int value");
            return NULL;
        }
        fd_original = PyInt_AsLong(fdnumber);
        Py_DECREF(fdnumber);
        fd = dup(fd_original);
        if (fd < 0) {
            PyErr_Format(PyExc_ValueError, "Invalid file descriptor from fileno()");
            return NULL;
        }

    } else {
        PyErr_Format(PyExc_ValueError, "Argument is not int and does not "
                                       "implement fileno() method");
        return NULL;
    }

    self = (UnixFdObject *) cls->tp_alloc(cls, 0);
    if (!self)
        return NULL;

    self->fd = fd;

    return (PyObject *) self;
}

static void
UnixFd_dealloc(UnixFdObject *self)
{
    if (self->fd >= 0) {
        close(self->fd);
        self->fd = -1;
    }
}

PyDoc_STRVAR(UnixFd_take__doc__,
"take() -> int\n"
"\n"
"This method returns the file descriptor owned by UnixFd object.\n"
"Note that, once this method is called, closing the file descriptor is\n"
"the caller's responsibility.\n"
"\n"
"This method may be called at most once; UnixFd 'forgets' the file\n"
"descriptor after it is taken.\n"
"\n"
":Raises ValueError: if this method has already been called\n"
);
static PyObject *
UnixFd_take(UnixFdObject *self)
{
    PyObject *fdnumber;

    if (self->fd < 0) {
        PyErr_SetString(PyExc_ValueError, "File descriptor already taken");
        return NULL;
    }

    fdnumber = Py_BuildValue("i", self->fd);
    self->fd = -1;

    return fdnumber;
}

int
dbus_py_unix_fd_get_fd(PyObject *self)
{
    return ((UnixFdObject *) self)->fd;
}

static PyMethodDef UnixFd_methods[] = {
    {"take", (PyCFunction) UnixFd_take, METH_NOARGS, UnixFd_take__doc__ },
    {NULL}
};

PyTypeObject DBusPyUnixFd_Type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "dbus.UnixFd",
    sizeof(UnixFdObject),
    0,
    (destructor) UnixFd_dealloc,            /* tp_dealloc */
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
    UnixFd_tp_doc,                          /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    0,                                      /* tp_iter */
    0,                                      /* tp_iternext */
    UnixFd_methods,                         /* tp_methods */
    0,                                      /* tp_members */
    0,                                      /* tp_getset */
    0,                                      /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    UnixFd_tp_new,                          /* tp_new */
};

dbus_bool_t
dbus_py_init_unixfd_type(void)
{
    if (PyType_Ready(&DBusPyUnixFd_Type) < 0) return 0;

    return 1;
}

dbus_bool_t
dbus_py_insert_unixfd_type(PyObject *this_module)
{
    Py_INCREF(&DBusPyUnixFd_Type);
    if (PyModule_AddObject(this_module, "UnixFd",
                           (PyObject *)&DBusPyUnixFd_Type) < 0) return 0;
    return 1;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
