/* _dbus_bindings internal API. For use within _dbus_bindings only.
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

#ifndef DBUS_BINDINGS_INTERNAL_H
#define DBUS_BINDINGS_INTERNAL_H

#define PY_SSIZE_T_CLEAN 1

#include <Python.h>

#define INSIDE_DBUS_PYTHON_BINDINGS
#include "dbus-python.h"

/* no need for extern "C", this is only for internal use */

/* on/off switch for debugging support (see below) */
#undef USING_DBG
#if 0 && !defined(DBG_IS_TOO_VERBOSE)
#   define USING_DBG 1
#endif

#define DEFINE_CHECK(type) \
static inline int type##_Check (PyObject *o) \
{ \
    return (PyObject_TypeCheck (o, &type##_Type)); \
} \
static inline int type##_CheckExact (PyObject *o) \
{ \
    return (Py_TYPE(o) == &type##_Type); \
}

/* This is a clever little trick to make writing the various object reprs
 * easier.  It relies on Python's %V format option which consumes two
 * arguments.  The first is a unicode object which may be NULL, and the second
 * is a char* which will be used if the first parameter is NULL.
 *
 * The issue is that we don't know whether the `parent_repr` at the call site
 * is a unicode or a bytes (a.k.a. 8-bit string).  Under Python 3, it will
 * always be a unicode.  Under Python 2 it will *probably* be a bytes/str, but
 * could potentially be a unicode.  So, we check the type, and if it's a
 * unicode, we pass that as the first argument, leaving NULL as the second
 * argument (since it will never be checked).  However, if the object is not a
 * unicode, it better be a bytes.  In that case, we'll pass NULL as the first
 * argument so that the second one gets used, and we'll dig the char* out of
 * the bytes object for that purpose.
 *
 * You might think that this would crash if obj is neither a bytes/str or
 * unicode, and you'd be right *except* that Python doesn't allow any other
 * types to be returned in the reprs.  Also, since obj will always be the repr
 * of a built-in type, it will never be anything other than a bytes or a
 * unicode in any version of Python.  So in practice, this is safe.
 */
#define REPRV(obj) \
    (PyUnicode_Check(obj) ? (obj) : NULL), \
    (PyUnicode_Check(obj) ? NULL : PyBytes_AS_STRING(obj))

#ifdef PY3
#define NATIVEINT_TYPE (PyLong_Type)
#define NATIVEINT_FROMLONG(x) (PyLong_FromLong(x))
#define NATIVEINT_ASLONG(x) (PyLong_AsLong(x))
#define INTORLONG_CHECK(obj) (PyLong_Check(obj))
#define NATIVESTR_TYPE (PyUnicode_Type)
#define NATIVESTR_CHECK(obj) (PyUnicode_Check(obj))
#define NATIVESTR_FROMSTR(obj) (PyUnicode_FromString(obj))
#else
#define NATIVEINT_TYPE (PyInt_Type)
#define NATIVEINT_FROMLONG(x) (PyInt_FromLong(x))
#define NATIVEINT_ASLONG(x) (PyInt_AsLong(x))
#define INTORLONG_CHECK(obj) (PyLong_Check(obj) || PyInt_Check(obj))
#define NATIVESTR_TYPE (PyBytes_Type)
#define NATIVESTR_CHECK(obj) (PyBytes_Check(obj))
#define NATIVESTR_FROMSTR(obj) (PyBytes_FromString(obj))
#endif

#ifdef PY3
PyMODINIT_FUNC PyInit__dbus_bindings(void);
#else
PyMODINIT_FUNC init_dbus_bindings(void);
#endif

/* conn.c */
extern PyTypeObject DBusPyConnection_Type;
DEFINE_CHECK(DBusPyConnection)
extern dbus_bool_t dbus_py_init_conn_types(void);
extern dbus_bool_t dbus_py_insert_conn_types(PyObject *this_module);

/* libdbusconn.c */
extern PyTypeObject DBusPyLibDBusConnection_Type;
DEFINE_CHECK(DBusPyLibDBusConnection)
PyObject *DBusPyLibDBusConnection_New(DBusConnection *conn);
extern dbus_bool_t dbus_py_init_libdbus_conn_types(void);
extern dbus_bool_t dbus_py_insert_libdbus_conn_types(PyObject *this_module);

/* bus.c */
extern dbus_bool_t dbus_py_init_bus_types(void);
extern dbus_bool_t dbus_py_insert_bus_types(PyObject *this_module);

/* exceptions.c */
extern PyObject *DBusPyException_SetString(const char *msg);
extern PyObject *DBusPyException_ConsumeError(DBusError *error);
extern dbus_bool_t dbus_py_init_exception_types(void);
extern dbus_bool_t dbus_py_insert_exception_types(PyObject *this_module);

/* types */
extern PyTypeObject DBusPyBoolean_Type;
DEFINE_CHECK(DBusPyBoolean)
extern PyTypeObject DBusPyObjectPath_Type, DBusPySignature_Type;
DEFINE_CHECK(DBusPyObjectPath)
DEFINE_CHECK(DBusPySignature)
extern PyTypeObject DBusPyArray_Type, DBusPyDict_Type, DBusPyStruct_Type;
DEFINE_CHECK(DBusPyArray)
DEFINE_CHECK(DBusPyDict)
DEFINE_CHECK(DBusPyStruct)
extern PyTypeObject DBusPyByte_Type, DBusPyByteArray_Type;
DEFINE_CHECK(DBusPyByteArray)
DEFINE_CHECK(DBusPyByte)
extern PyTypeObject DBusPyString_Type;
DEFINE_CHECK(DBusPyString)
#ifndef PY3
extern PyTypeObject DBusPyUTF8String_Type;
DEFINE_CHECK(DBusPyUTF8String)
#endif
extern PyTypeObject DBusPyDouble_Type;
DEFINE_CHECK(DBusPyDouble)
extern PyTypeObject DBusPyInt16_Type, DBusPyUInt16_Type;
DEFINE_CHECK(DBusPyInt16)
DEFINE_CHECK(DBusPyUInt16)
extern PyTypeObject DBusPyInt32_Type, DBusPyUInt32_Type;
DEFINE_CHECK(DBusPyInt32)
DEFINE_CHECK(DBusPyUInt32)
extern PyTypeObject DBusPyUnixFd_Type;
DEFINE_CHECK(DBusPyUnixFd)
extern PyTypeObject DBusPyInt64_Type, DBusPyUInt64_Type;
DEFINE_CHECK(DBusPyInt64)
DEFINE_CHECK(DBusPyUInt64)
extern dbus_bool_t dbus_py_init_abstract(void);
extern dbus_bool_t dbus_py_init_signature(void);
extern dbus_bool_t dbus_py_init_int_types(void);
extern dbus_bool_t dbus_py_init_unixfd_type(void);
extern dbus_bool_t dbus_py_init_string_types(void);
extern dbus_bool_t dbus_py_init_float_types(void);
extern dbus_bool_t dbus_py_init_container_types(void);
extern dbus_bool_t dbus_py_init_byte_types(void);
extern dbus_bool_t dbus_py_insert_abstract_types(PyObject *this_module);
extern dbus_bool_t dbus_py_insert_signature(PyObject *this_module);
extern dbus_bool_t dbus_py_insert_int_types(PyObject *this_module);
extern dbus_bool_t dbus_py_insert_unixfd_type(PyObject *this_module);
extern dbus_bool_t dbus_py_insert_string_types(PyObject *this_module);
extern dbus_bool_t dbus_py_insert_float_types(PyObject *this_module);
extern dbus_bool_t dbus_py_insert_container_types(PyObject *this_module);
extern dbus_bool_t dbus_py_insert_byte_types(PyObject *this_module);

int dbus_py_unix_fd_get_fd(PyObject *self);

/* generic */
extern void dbus_py_take_gil_and_xdecref(PyObject *);
extern int dbus_py_immutable_setattro(PyObject *, PyObject *, PyObject *);
extern PyObject *dbus_py_empty_tuple;
extern dbus_bool_t dbus_py_init_generic(void);

/* message.c */
extern DBusMessage *DBusPyMessage_BorrowDBusMessage(PyObject *msg);
extern PyObject *DBusPyMessage_ConsumeDBusMessage(DBusMessage *);
extern dbus_bool_t dbus_py_init_message_types(void);
extern dbus_bool_t dbus_py_insert_message_types(PyObject *this_module);

/* pending-call.c */
extern PyObject *DBusPyPendingCall_ConsumeDBusPendingCall(DBusPendingCall *,
                                                          PyObject *);
extern dbus_bool_t dbus_py_init_pending_call(void);
extern dbus_bool_t dbus_py_insert_pending_call(PyObject *this_module);

/* mainloop.c */
extern dbus_bool_t dbus_py_set_up_connection(PyObject *conn,
                                             PyObject *mainloop);
extern dbus_bool_t dbus_py_set_up_server(PyObject *server,
                                         PyObject *mainloop);
extern PyObject *dbus_py_get_default_main_loop(void);
extern dbus_bool_t dbus_py_check_mainloop_sanity(PyObject *);
extern dbus_bool_t dbus_py_init_mainloop(void);
extern dbus_bool_t dbus_py_insert_mainloop_types(PyObject *);

/* server.c */
extern PyTypeObject DBusPyServer_Type;
DEFINE_CHECK(DBusPyServer)
extern dbus_bool_t dbus_py_init_server_types(void);
extern dbus_bool_t dbus_py_insert_server_types(PyObject *this_module);
extern DBusServer *DBusPyServer_BorrowDBusServer(PyObject *self);

/* validation.c */
dbus_bool_t dbus_py_validate_bus_name(const char *name,
                                      dbus_bool_t may_be_unique,
                                      dbus_bool_t may_be_not_unique);
dbus_bool_t dbus_py_validate_member_name(const char *name);
dbus_bool_t dbus_py_validate_interface_name(const char *name);
dbus_bool_t dbus_py_validate_object_path(const char *path);
#define dbus_py_validate_error_name dbus_py_validate_interface_name

/* debugging support */
void _dbus_py_assertion_failed(const char *);
#define DBUS_PY_RAISE_VIA_NULL_IF_FAIL(assertion) \
    do { if (!(assertion)) { \
            _dbus_py_assertion_failed(#assertion); \
            return NULL; \
        } \
    } while (0)

#define DBUS_PY_RAISE_VIA_GOTO_IF_FAIL(assertion, label) \
    do { if (!(assertion)) { \
            _dbus_py_assertion_failed(#assertion); \
            goto label; \
        } \
    } while (0)

#define DBUS_PY_RAISE_VIA_RETURN_IF_FAIL(assertion, value) \
    do { if (!(assertion)) { \
            _dbus_py_assertion_failed(#assertion); \
            return value; \
        } \
    } while (0)

/* verbose debugging support */
#ifdef USING_DBG

#   include <sys/types.h>
#   include <unistd.h>

void _dbus_py_dbg_exc(void);
void _dbus_py_whereami(void);
void _dbus_py_dbg_dump_message(DBusMessage *);

#   define TRACE(self) do { \
    fprintf(stderr, "TRACE: <%s at %p> in %s, "                 \
            "%d refs\n",                                        \
            self ? Py_TYPE(self)->tp_name : NULL,               \
            self, __func__,                                     \
            self ? (int)Py_REFCNT(self) : 0);                   \
    } while (0)
#   define DBG(format, ...) fprintf(stderr, "DEBUG: " format "\n",\
                                    __VA_ARGS__)
#   define DBG_EXC(format, ...) do {DBG(format, __VA_ARGS__); \
                                    _dbus_py_dbg_exc();} while (0)
#   define DBG_DUMP_MESSAGE(x) _dbus_py_dbg_dump_message(x)
#   define DBG_WHEREAMI _dbus_py_whereami()

#else /* !defined(USING_DBG) */

#   define TRACE(self) do {} while (0)
#   define DBG(format, ...) do {} while (0)
#   define DBG_EXC(format, ...) do {} while (0)
#   define DBG_DUMP_MESSAGE(x) do {} while (0)
#   define DBG_WHEREAMI do {} while (0)

#endif /* !defined(USING_DBG) */

/* General-purpose Python glue */

#define DEFERRED_ADDRESS(ADDR) 0

#if defined(__GNUC__)
#   if __GNUC__ >= 3
#       define UNUSED __attribute__((__unused__))
#   else
#       define UNUSED /*nothing*/
#   endif
#else
#   define UNUSED /*nothing*/
#endif

#endif
