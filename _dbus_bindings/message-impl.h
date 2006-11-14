/* Implementation of D-Bus Message and subclasses (but see message-get-args.h
 * and message-append.h for unserialization and serialization code).
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

#ifdef USING_DBG
static void _dbg_dump_message(DBusMessage *message)
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
#   define DBG_DUMP_MESSAGE(x) _dbg_dump_message(x)
#else
#   define DBG_DUMP_MESSAGE(x) do {} while(0)
#endif

static PyTypeObject MessageType, SignalMessageType, ErrorMessageType;
static PyTypeObject MethodReturnMessageType, MethodCallMessageType;

static inline int Message_Check (PyObject *o)
{
    return (o->ob_type == &MessageType)
            || PyObject_IsInstance(o, (PyObject *)&MessageType);
}

typedef struct {
    PyObject_HEAD
    DBusMessage *msg;
} Message;

PyDoc_STRVAR(Message_tp_doc,
"A message to be sent or received over a D-Bus Connection.\n");

static void Message_tp_dealloc (Message *self)
{
    if (self->msg) {
        dbus_message_unref (self->msg);
    }
    self->ob_type->tp_free ((PyObject *)self);
}

static PyObject *
Message_tp_new (PyTypeObject *type,
                PyObject *args UNUSED,
                PyObject *kwargs UNUSED)
{
    Message *self;

    self = (Message *)type->tp_alloc (type, 0);
    if (!self) return NULL;
    self->msg = NULL;
    return (PyObject *)self;
}

PyDoc_STRVAR(MethodCallMessage_tp_doc, "A method-call message.\n\n"
"MethodCallMessage(destination: str or None, path: str,\n"
"                  interface: str or None, method: str)\n");
static int
MethodCallMessage_tp_init (Message *self, PyObject *args, PyObject *kwargs)
{
    const char *destination, *path, *interface, *method;
    static char *kwlist[] = {"destination", "path", "interface", "method", NULL};

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "zszs:__init__", kwlist,
                                      &destination, &path, &interface,
                                      &method)) {
        return -1;
    }
    if (destination && !_validate_bus_name(destination, 1, 1)) return -1;
    if (!_validate_object_path(path)) return -1;
    if (interface && !_validate_interface_name(interface)) return -1;
    if (!_validate_member_name(method)) return -1;
    if (self->msg) {
        dbus_message_unref (self->msg);
        self->msg = NULL;
    }
    self->msg = dbus_message_new_method_call (destination, path, interface,
                                              method);
    if (!self->msg) {
        PyErr_NoMemory();
        return -1;
    }
    return 0;
}

PyDoc_STRVAR(MethodReturnMessage_tp_doc, "A method-return message.\n\n"
"MethodReturnMessage(method_call: MethodCallMessage)\n");
static int
MethodReturnMessage_tp_init (Message *self, PyObject *args, PyObject *kwargs)
{
    Message *other;
    static char *kwlist[] = {"method_call", NULL};

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "O!:__init__", kwlist,
                                      &MessageType, &other)) {
        return -1;
    }
    if (self->msg) {
        dbus_message_unref (self->msg);
        self->msg = NULL;
    }
    self->msg = dbus_message_new_method_return (other->msg);
    if (!self->msg) {
        PyErr_NoMemory();
        return -1;
    }
    return 0;
}

PyDoc_STRVAR(SignalMessage_tp_doc, "A signal message.\n\n"
"SignalMessage(path: str, interface: str, method: str)\n");
static int
SignalMessage_tp_init (Message *self, PyObject *args, PyObject *kwargs)
{
    const char *path, *interface, *name;
    static char *kwlist[] = {"path", "interface", "name", NULL};

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "sss:__init__", kwlist,
                                    &path, &interface, &name)) {
      return -1;
    }
    if (!_validate_object_path(path)) return -1;
    if (!_validate_interface_name(interface)) return -1;
    if (!_validate_member_name(name)) return -1;
    if (self->msg) {
        dbus_message_unref (self->msg);
        self->msg = NULL;
    }
    self->msg = dbus_message_new_signal (path, interface, name);
    if (!self->msg) {
        PyErr_NoMemory();
        return -1;
    }
    return 0;
}

PyDoc_STRVAR(ErrorMessage_tp_doc, "An error message.\n\n"
"ErrorMessage(reply_to: Message, error_name: str,\n"
"             error_message: str or None)\n");
static int
ErrorMessage_tp_init (Message *self, PyObject *args, PyObject *kwargs)
{
    Message *reply_to;
    const char *error_name, *error_message;
    static char *kwlist[] = {"reply_to", "error_name", "error_message", NULL};

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "O!sz:__init__", kwlist,
                                      &MessageType, &reply_to, &error_name,
                                      &error_message)) {
        return -1;
    }
    if (!_validate_error_name(error_name)) return -1;
    if (self->msg) {
        dbus_message_unref (self->msg);
        self->msg = NULL;
    }
    self->msg = dbus_message_new_error (reply_to->msg, error_name, error_message);
    if (!self->msg) {
        PyErr_NoMemory();
        return -1;
    }
    return 0;
}

static DBusMessage *
Message_BorrowDBusMessage (PyObject *msg)
{
    if (!Message_Check (msg)) {
        PyErr_SetString (PyExc_TypeError,
                       "A _dbus_bindings.Message instance is required");
        return NULL;
    }
    if (!((Message *)msg)->msg) {
        DBusException_UnusableMessage();
        return NULL;
    }
    return ((Message *)msg)->msg;
}

static PyObject *
Message_ConsumeDBusMessage (DBusMessage *msg)
{
    PyTypeObject *type;
    Message *self;

    switch (dbus_message_get_type (msg)) {
    case DBUS_MESSAGE_TYPE_METHOD_CALL:
        type = &MethodCallMessageType;
        break;
    case DBUS_MESSAGE_TYPE_METHOD_RETURN:
        type = &MethodReturnMessageType;
        break;
    case DBUS_MESSAGE_TYPE_ERROR:
        type = &ErrorMessageType;
        break;
    case DBUS_MESSAGE_TYPE_SIGNAL:
        type = &SignalMessageType;
        break;
    default:
        type = &MessageType;
    }

    self = (Message *)(type->tp_new) (type, empty_tuple, NULL);
    if (!self) {
        dbus_message_unref(msg);
        return NULL;
    }
    self->msg = msg;
    return (PyObject *)self;
}

PyDoc_STRVAR(Message_copy__doc__,
"message.copy() -> Message (or subclass)\n"
"Deep-copy the message, resetting the serial number to zero.\n");
static PyObject *
Message_copy (Message *self, PyObject *args UNUSED)
{
    DBusMessage *msg;
    if (!self->msg) return DBusException_UnusableMessage();
    msg = dbus_message_copy(self->msg);
    if (!msg) return PyErr_NoMemory();
    return Message_ConsumeDBusMessage(msg);
}

PyDoc_STRVAR(Message_get_auto_start__doc__,
"message.get_auto_start() -> bool\n"
"Return true if this message will cause an owner for the destination name\n"
"to be auto-started.\n");
static PyObject *
Message_get_auto_start (Message *self, PyObject *unused UNUSED)
{
    if (!self->msg) return DBusException_UnusableMessage();
    return PyBool_FromLong (dbus_message_get_auto_start (self->msg));
}

PyDoc_STRVAR(Message_set_auto_start__doc__,
"message.set_auto_start(bool) -> None\n"
"Set whether this message will cause an owner for the destination name\n"
"to be auto-started.\n");
static PyObject *
Message_set_auto_start (Message *self, PyObject *args)
{
    int value;
    if (!PyArg_ParseTuple (args, "i", &value)) return NULL;
    if (!self->msg) return DBusException_UnusableMessage();
    dbus_message_set_auto_start (self->msg, value ? TRUE : FALSE);
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(Message_get_no_reply__doc__,
"message.get_no_reply() -> bool\n"
"Return true if this message need not be replied to.\n");
static PyObject *
Message_get_no_reply (Message *self, PyObject *unused UNUSED)
{
    if (!self->msg) return DBusException_UnusableMessage();
    return PyBool_FromLong (dbus_message_get_no_reply (self->msg));
}

PyDoc_STRVAR(Message_set_no_reply__doc__,
"message.set_no_reply(bool) -> None\n"
"Set whether no reply to this message is required.\n");
static PyObject *
Message_set_no_reply (Message *self, PyObject *args)
{
    int value;
    if (!PyArg_ParseTuple (args, "i", &value)) return NULL;
    if (!self->msg) return DBusException_UnusableMessage();
    dbus_message_set_no_reply (self->msg, value ? TRUE : FALSE);
    Py_RETURN_NONE;
}

PyDoc_STRVAR(Message_get_reply_serial__doc__,
"message.get_reply_serial() -> long\n"
"Returns the serial that the message is a reply to or 0 if none.\n");
static PyObject *
Message_get_reply_serial (Message *self, PyObject *unused UNUSED)
{
    if (!self->msg) return DBusException_UnusableMessage();
    return PyLong_FromUnsignedLong (dbus_message_get_reply_serial (self->msg));
}

PyDoc_STRVAR(Message_set_reply_serial__doc__,
"message.set_reply_serial(bool) -> None\n"
"Set the serial that this message is a reply to.\n");
static PyObject *
Message_set_reply_serial (Message *self, PyObject *args)
{
    dbus_uint32_t value;

    if (!PyArg_ParseTuple (args, "k", &value)) return NULL;
    if (!self->msg) return DBusException_UnusableMessage();
    if (!dbus_message_set_reply_serial (self->msg, value)) {
        return PyErr_NoMemory();
    }
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(Message_get_type__doc__,
"message.get_type() -> int\n\n"
"Returns the type of the message.\n");
static PyObject *
Message_get_type (Message *self, PyObject *unused UNUSED)
{
    if (!self->msg) return DBusException_UnusableMessage();
    return PyInt_FromLong (dbus_message_get_type (self->msg));
}

PyDoc_STRVAR(Message_get_serial__doc__,
"message.get_serial() -> long\n"
"Returns the serial of a message or 0 if none has been specified.\n"
"\n"
"The message's serial number is provided by the application sending the\n"
"message and is used to identify replies to this message. All messages\n"
"received on a connection will have a serial, but messages you haven't\n"
"sent yet may return 0.\n");
static PyObject *
Message_get_serial (Message *self, PyObject *unused UNUSED)
{
    if (!self->msg) return DBusException_UnusableMessage();
    return PyLong_FromUnsignedLong (dbus_message_get_serial (self->msg));
}

PyDoc_STRVAR(Message_is_method_call__doc__,
"is_method_call(interface: str, member: str) -> bool");
static PyObject *
Message_is_method_call (Message *self, PyObject *args)
{
    const char *interface, *method;

    if (!PyArg_ParseTuple(args, "ss:is_method_call", &interface, &method)) {
        return NULL;
    }
    if (!self->msg) return DBusException_UnusableMessage();
    return PyBool_FromLong (dbus_message_is_method_call (self->msg, interface,
                                                         method));
}

PyDoc_STRVAR(Message_is_error__doc__,
"is_error(error: str) -> bool");
static PyObject *
Message_is_error (Message *self, PyObject *args)
{
    const char *error_name;

    if (!PyArg_ParseTuple(args, "s:is_error", &error_name)) {
        return NULL;
    }
    if (!self->msg) return DBusException_UnusableMessage();
    return PyBool_FromLong (dbus_message_is_error (self->msg, error_name));
}

PyDoc_STRVAR(Message_is_signal__doc__,
"is_signal(interface: str, member: str) -> bool");
static PyObject *
Message_is_signal (Message *self, PyObject *args)
{
    const char *interface, *signal_name;

    if (!PyArg_ParseTuple(args, "ss:is_signal", &interface, &signal_name)) {
        return NULL;
    }
    if (!self->msg) return DBusException_UnusableMessage();
    return PyBool_FromLong (dbus_message_is_signal (self->msg, interface,
                                                    signal_name));
}

PyDoc_STRVAR(Message_get_member__doc__,
"get_member() -> str or None");
static PyObject *
Message_get_member (Message *self, PyObject *unused UNUSED)
{
    const char *c_str;

    if (!self->msg) return DBusException_UnusableMessage();
    c_str = dbus_message_get_member (self->msg);
    if (!c_str) {
        Py_RETURN_NONE;
    }
    return PyString_FromString(c_str);
}

PyDoc_STRVAR(Message_has_member__doc__,
"has_member(name: str or None) -> bool");
static PyObject *
Message_has_member (Message *self, PyObject *args)
{
    const char *name;

    if (!PyArg_ParseTuple(args, "z:has_member", &name)) {
        return NULL;
    }
    if (!self->msg) return DBusException_UnusableMessage();
    return PyBool_FromLong (dbus_message_has_member(self->msg, name));
}

PyDoc_STRVAR(Message_set_member__doc__,
"set_member(unique_name: str or None)");
static PyObject *
Message_set_member (Message *self, PyObject *args)
{
    const char *name;

    if (!PyArg_ParseTuple(args, "z:set_member", &name)) {
        return NULL;
    }
    if (!self->msg) return DBusException_UnusableMessage();
    if (!_validate_member_name(name)) return NULL;
    if (!dbus_message_set_member (self->msg, name)) return PyErr_NoMemory();
    Py_RETURN_NONE;
}

PyDoc_STRVAR(Message_get_path__doc__,
"get_path() -> ObjectPath or None\n\n"
"Return the message's destination object path (if it's a method call) or\n"
"source object path (if it's a method reply or a signal) or None (if it\n"
"has no path).\n");
static PyObject *
Message_get_path (Message *self, PyObject *unused UNUSED)
{
    const char *c_str;
  
    if (!self->msg) return DBusException_UnusableMessage();
    c_str = dbus_message_get_path (self->msg);
    if (!c_str) {
        Py_RETURN_NONE;
    }
    return PyObject_CallFunction((PyObject *)&ObjectPathType, "(s)", c_str);
}

PyDoc_STRVAR(Message_get_path_decomposed__doc__,
"get_path_decomposed() -> list of str, or None\n\n"
"Return a list of path components (e.g. /foo/bar -> ['foo','bar'], / -> [])\n"
"or None if the message has no associated path.\n");
static PyObject *
Message_get_path_decomposed (Message *self, PyObject *unused UNUSED)
{
    char **paths, **ptr;
    PyObject *ret = PyList_New(0);

    if (!ret) return NULL;
    if (!self->msg) {
        Py_DECREF (ret);
        return DBusException_UnusableMessage ();
    }
    if (!dbus_message_get_path_decomposed (self->msg, &paths)) {
        Py_DECREF (ret);
        return PyErr_NoMemory ();
    }
    if (!paths) {
        Py_DECREF(ret);
        Py_RETURN_NONE;
    }
    for (ptr = paths; *ptr; ptr++) {
        PyObject *str = PyString_FromString (*ptr);

        if (!str) {
            Py_DECREF (ret);
            ret = NULL;
            break;
        }
        if (PyList_Append (ret, str) < 0) {
            Py_DECREF (ret);
            ret = NULL;
            break;
        }
        Py_DECREF (str);
        str = NULL;
    }
    dbus_free_string_array (paths);
    return ret;
}

PyDoc_STRVAR(Message_has_path__doc__,
"has_path(name: str or None) -> bool");
static PyObject *
Message_has_path (Message *self, PyObject *args)
{
    const char *name;

    if (!PyArg_ParseTuple(args, "z:has_path", &name)) {
        return NULL;
    }
    if (!self->msg) return DBusException_UnusableMessage();
    return PyBool_FromLong (dbus_message_has_path(self->msg, name));
}

PyDoc_STRVAR(Message_set_path__doc__,
"set_path(name: str or None)");
static PyObject *
Message_set_path (Message *self, PyObject *args)
{
    const char *name;

    if (!PyArg_ParseTuple(args, "z:set_path", &name)) return NULL;
    if (!self->msg) return DBusException_UnusableMessage();
    if (!dbus_message_has_path(self->msg, name)) return PyErr_NoMemory();
    Py_RETURN_NONE;
}

PyDoc_STRVAR(Message_get_signature__doc__,
"get_signature() -> Signature or None");
static PyObject *
Message_get_signature (Message *self, PyObject *unused UNUSED)
{
    const char *c_str;

    if (!self->msg) return DBusException_UnusableMessage();
    c_str = dbus_message_get_signature (self->msg);
    if (!c_str) {
        return PyObject_CallFunction((PyObject *)&SignatureType, "(s)", "");
    }
    return PyObject_CallFunction((PyObject *)&SignatureType, "(s)", c_str);
}

PyDoc_STRVAR(Message_has_signature__doc__,
"has_signature(signature: str) -> bool");
static PyObject *
Message_has_signature (Message *self, PyObject *args)
{
    const char *name;

    if (!PyArg_ParseTuple(args, "s:has_signature", &name)) {
        return NULL;
    }
    if (!self->msg) return DBusException_UnusableMessage();
    return PyBool_FromLong (dbus_message_has_signature (self->msg, name));
}

PyDoc_STRVAR(Message_get_sender__doc__,
"get_sender() -> str or None\n\n"
"Return the message's sender unique name, or None if none.\n");
static PyObject *
Message_get_sender (Message *self, PyObject *unused UNUSED)
{
    const char *c_str;

    if (!self->msg) return DBusException_UnusableMessage();
    c_str = dbus_message_get_sender (self->msg);
    if (!c_str) {
        Py_RETURN_NONE;
    }
    return PyString_FromString(c_str);
}

PyDoc_STRVAR(Message_has_sender__doc__,
"has_sender(unique_name: str) -> bool");
static PyObject *
Message_has_sender (Message *self, PyObject *args)
{
  const char *name;

  if (!PyArg_ParseTuple(args, "s:has_sender", &name)) {
      return NULL;
  }
  if (!self->msg) return DBusException_UnusableMessage();
  return PyBool_FromLong (dbus_message_has_sender (self->msg, name));
}

PyDoc_STRVAR(Message_set_sender__doc__,
"set_sender(unique_name: str or None)");
static PyObject *
Message_set_sender (Message *self, PyObject *args)
{
    const char *name;

    if (!PyArg_ParseTuple(args, "z:set_sender", &name)) {
        return NULL;
    }
    if (!self->msg) return DBusException_UnusableMessage();
    if (!_validate_bus_name(name, 1, 0)) return NULL;
    if (!dbus_message_set_sender (self->msg, name)) return PyErr_NoMemory();
    Py_RETURN_NONE;
}

PyDoc_STRVAR(Message_get_destination__doc__,
"get_destination() -> str or None\n\n"
"Return the message's destination bus name, or None if none.\n");
static PyObject *
Message_get_destination(Message *self, PyObject *unused UNUSED)
{
    const char *c_str;

    if (!self->msg) return DBusException_UnusableMessage();
    c_str = dbus_message_get_destination(self->msg);
    if (!c_str) {
        Py_RETURN_NONE;
    }
    return PyString_FromString(c_str);
}

PyDoc_STRVAR(Message_has_destination__doc__,
"has_destination(bus_name: str) -> bool");
static PyObject *
Message_has_destination (Message *self, PyObject *args)
{
    const char *name;

    if (!PyArg_ParseTuple(args, "s:has_destination", &name)) {
        return NULL;
    }
    if (!self->msg) return DBusException_UnusableMessage();
    return PyBool_FromLong (dbus_message_has_destination (self->msg, name));
}

PyDoc_STRVAR(Message_set_destination__doc__,
"set_destination(bus_name: str or None)");
static PyObject *
Message_set_destination (Message *self, PyObject *args)
{
    const char *name;

    if (!PyArg_ParseTuple(args, "z:set_destination", &name)) {
        return NULL;
    }
    if (!self->msg) return DBusException_UnusableMessage();
    if (!_validate_bus_name(name, 1, 1)) return NULL;
    if (!dbus_message_set_destination (self->msg, name)) return PyErr_NoMemory();
    Py_RETURN_NONE;
}

PyDoc_STRVAR(Message_get_interface__doc__,
"get_interface() -> str or None");
static PyObject *
Message_get_interface (Message *self, PyObject *unused UNUSED)
{
    const char *c_str;

    if (!self->msg) return DBusException_UnusableMessage();
    c_str = dbus_message_get_interface (self->msg);
    if (!c_str) {
        Py_RETURN_NONE;
    }
    return PyString_FromString(c_str);
}

PyDoc_STRVAR(Message_has_interface__doc__,
"has_interface(interface: str or None) -> bool");
static PyObject *
Message_has_interface(Message *self, PyObject *args)
{
    const char *name;

    if (!PyArg_ParseTuple(args, "z:has_interface", &name)) {
        return NULL;
    }
    if (!self->msg) return DBusException_UnusableMessage();
    return PyBool_FromLong(dbus_message_has_interface (self->msg, name));
}

PyDoc_STRVAR(Message_set_interface__doc__,
"set_interface(name: str or None)");
static PyObject *
Message_set_interface (Message *self, PyObject *args)
{
    const char *name;

    if (!PyArg_ParseTuple(args, "z:set_interface", &name)) {
        return NULL;
    }
    if (!self->msg) return DBusException_UnusableMessage();
    if (!_validate_interface_name(name)) return NULL;
    if (!dbus_message_set_interface (self->msg, name)) return PyErr_NoMemory();
    Py_RETURN_NONE;
}

PyDoc_STRVAR(Message_get_error_name__doc__,
"get_error_name() -> str or None");
static PyObject *
Message_get_error_name (Message *self, PyObject *unused UNUSED)
{
    const char *c_str;

    if (!self->msg) return DBusException_UnusableMessage();
    c_str = dbus_message_get_error_name (self->msg);
    if (!c_str) {
        Py_RETURN_NONE;
    }
    return PyString_FromString(c_str);
}

PyDoc_STRVAR(Message_set_error_name__doc__,
"set_error_name(name: str or None)");
static PyObject *
Message_set_error_name(Message *self, PyObject *args)
{
    const char *name;

    if (!PyArg_ParseTuple(args, "z:set_error_name", &name)) {
        return NULL;
    }
    if (!self->msg) return DBusException_UnusableMessage();
    if (!_validate_error_name(name)) return NULL;
    if (!dbus_message_set_error_name(self->msg, name)) return PyErr_NoMemory();
    Py_RETURN_NONE;
}

#include "message-append-impl.h"
#include "message-get-args-impl.h"

static PyMethodDef Message_tp_methods[] = {
    {"copy", (PyCFunction)Message_copy,
      METH_NOARGS, Message_copy__doc__},
    {"is_method_call", (PyCFunction)Message_is_method_call,
      METH_VARARGS, Message_is_method_call__doc__},
    {"is_signal", (PyCFunction)Message_is_signal,
      METH_VARARGS, Message_is_signal__doc__},
    {"is_error", (PyCFunction)Message_is_error,
      METH_VARARGS, Message_is_error__doc__},

    {"get_args_list", (PyCFunction)Message_get_args_list,
      METH_VARARGS|METH_KEYWORDS, Message_get_args_list__doc__},
    {"guess_signature", (PyCFunction)Message_guess_signature,
      METH_VARARGS|METH_STATIC, Message_guess_signature__doc__},
    {"append", (PyCFunction)Message_append,
      METH_VARARGS|METH_KEYWORDS, Message_append__doc__},

    {"get_auto_start", (PyCFunction)Message_get_auto_start,
      METH_NOARGS, Message_get_auto_start__doc__},
    {"set_auto_start", (PyCFunction)Message_set_auto_start,
      METH_VARARGS, Message_set_auto_start__doc__},
    {"get_destination", (PyCFunction)Message_get_destination,
      METH_NOARGS, Message_get_destination__doc__},
    {"set_destination", (PyCFunction)Message_set_destination,
      METH_VARARGS, Message_set_destination__doc__},
    {"has_destination", (PyCFunction)Message_has_destination,
      METH_VARARGS, Message_has_destination__doc__},
    {"get_error_name", (PyCFunction)Message_get_error_name,
      METH_NOARGS, Message_get_error_name__doc__},
    {"set_error_name", (PyCFunction)Message_set_error_name,
      METH_VARARGS, Message_set_error_name__doc__},
    {"get_interface", (PyCFunction)Message_get_interface,
      METH_NOARGS, Message_get_interface__doc__},
    {"set_interface", (PyCFunction)Message_set_interface,
      METH_VARARGS, Message_set_interface__doc__},
    {"has_interface", (PyCFunction)Message_has_interface,
      METH_VARARGS, Message_has_interface__doc__},
    {"get_member", (PyCFunction)Message_get_member,
      METH_NOARGS, Message_get_member__doc__},
    {"set_member", (PyCFunction)Message_set_member,
      METH_VARARGS, Message_set_member__doc__},
    {"has_member", (PyCFunction)Message_has_member,
      METH_VARARGS, Message_has_member__doc__},
    {"get_path", (PyCFunction)Message_get_path,
      METH_NOARGS, Message_get_path__doc__},
    {"get_path_decomposed", (PyCFunction)Message_get_path_decomposed,
      METH_NOARGS, Message_get_path_decomposed__doc__},
    {"set_path", (PyCFunction)Message_set_path,
      METH_VARARGS, Message_set_path__doc__},
    {"has_path", (PyCFunction)Message_has_path,
      METH_VARARGS, Message_has_path__doc__},
    {"get_no_reply", (PyCFunction)Message_get_no_reply,
      METH_NOARGS, Message_get_no_reply__doc__},
    {"set_no_reply", (PyCFunction)Message_set_no_reply,
      METH_VARARGS, Message_set_no_reply__doc__},
    {"get_reply_serial", (PyCFunction)Message_get_reply_serial,
      METH_NOARGS, Message_get_reply_serial__doc__},
    {"set_reply_serial", (PyCFunction)Message_set_reply_serial,
      METH_VARARGS, Message_set_reply_serial__doc__},
    {"get_sender", (PyCFunction)Message_get_sender,
      METH_NOARGS, Message_get_sender__doc__},
    {"set_sender", (PyCFunction)Message_set_sender,
      METH_VARARGS, Message_set_sender__doc__},
    {"has_sender", (PyCFunction)Message_has_sender,
      METH_VARARGS, Message_has_sender__doc__},
    {"get_serial", (PyCFunction)Message_get_serial,
      METH_NOARGS, Message_get_serial__doc__},
    {"get_signature", (PyCFunction)Message_get_signature,
      METH_NOARGS, Message_get_signature__doc__},
    {"has_signature", (PyCFunction)Message_has_signature,
      METH_VARARGS, Message_has_signature__doc__},
    {"get_type", (PyCFunction)Message_get_type,
      METH_NOARGS, Message_get_type__doc__},
    {NULL, NULL, 0, NULL}
};

static PyTypeObject MessageType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_dbus_bindings.Message",   /*tp_name*/
    sizeof(Message),     /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Message_tp_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    Message_tp_doc,            /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Message_tp_methods,        /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    Message_tp_new,            /* tp_new */
};

static PyTypeObject MethodCallMessageType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_dbus_bindings.MethodCallMessage",  /*tp_name*/
    0,                         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    0,                         /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    MethodCallMessage_tp_doc,  /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    0,                         /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    DEFERRED_ADDRESS(&MessageType),   /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)MethodCallMessage_tp_init,    /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
};

static PyTypeObject MethodReturnMessageType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_dbus_bindings.MethodReturnMessage",  /*tp_name*/
    0,                         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    0,                         /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    MethodReturnMessage_tp_doc,  /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    0,                         /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    DEFERRED_ADDRESS(&MessageType),   /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)MethodReturnMessage_tp_init,    /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
};

static PyTypeObject SignalMessageType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_dbus_bindings.SignalMessage",  /*tp_name*/
    0,                         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    0,                         /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    SignalMessage_tp_doc,  /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    0,                         /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    DEFERRED_ADDRESS(&MessageType),   /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)SignalMessage_tp_init,    /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
};

static PyTypeObject ErrorMessageType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_dbus_bindings.ErrorMessage",  /*tp_name*/
    0,                         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    0,                         /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    ErrorMessage_tp_doc,  /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    0,                         /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    DEFERRED_ADDRESS(&MessageType),   /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)ErrorMessage_tp_init,    /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
};

static inline int
init_message_types(void)
{
    if (PyType_Ready(&MessageType) < 0) return 0;

    MethodCallMessageType.tp_base = &MessageType;
    if (PyType_Ready(&MethodCallMessageType) < 0) return 0;

    MethodReturnMessageType.tp_base = &MessageType;
    if (PyType_Ready(&MethodReturnMessageType) < 0) return 0;

    SignalMessageType.tp_base = &MessageType;
    if (PyType_Ready(&SignalMessageType) < 0) return 0;

    ErrorMessageType.tp_base = &MessageType;
    if (PyType_Ready(&ErrorMessageType) < 0) return 0;

    return 1;
}

static inline int
insert_message_types(PyObject *this_module)
{
    if (PyModule_AddObject(this_module, "Message",
                         (PyObject *)&MessageType) < 0) return 0;

    if (PyModule_AddObject(this_module, "MethodCallMessage",
                         (PyObject *)&MethodCallMessageType) < 0) return 0;

    if (PyModule_AddObject(this_module, "MethodReturnMessage",
                         (PyObject *)&MethodReturnMessageType) < 0) return 0;

    if (PyModule_AddObject(this_module, "ErrorMessage",
                         (PyObject *)&ErrorMessageType) < 0) return 0;

    if (PyModule_AddObject(this_module, "SignalMessage",
                         (PyObject *)&SignalMessageType) < 0) return 0;

    return 1;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
