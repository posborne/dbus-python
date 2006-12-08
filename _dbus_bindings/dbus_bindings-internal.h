/* _dbus_bindings internal API. For use within _dbus_bindings only.
 *
 * Copyright (C) 2006 Collabora Ltd. <http://www.collabora.co.uk/>
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

#ifndef DBUS_BINDINGS_INTERNAL_H
#define DBUS_BINDINGS_INTERNAL_H

#include <Python.h>
#define INSIDE_DBUS_BINDINGS
#include "dbus_bindings.h"

/* no need for extern "C", this is only for internal use */

/* on/off switch for debugging support (see below) */
#undef USING_DBG
/* #define USING_DBG */

/* conn.c */
extern PyTypeObject DBusPyConnectionType;
#define DBusPyConnection_Check(o) PyObject_TypeCheck(o, &DBusPyConnectionType)
extern PyObject *DBusPyConnection_NewConsumingDBusConnection(PyTypeObject *,
                                                             DBusConnection *,
                                                             PyObject *);
extern dbus_bool_t dbus_py_init_conn_types(void);
extern dbus_bool_t dbus_py_insert_conn_types(PyObject *this_module);

/* exceptions.c */
extern PyObject *DBusPyException;
extern PyObject *DBusPyException_ConsumeError(DBusError *error);

/* generic */
extern void dbus_py_take_gil_and_xdecref(PyObject *);

/* message.c */
extern DBusMessage *DBusPyMessage_BorrowDBusMessage(PyObject *msg);
extern PyObject *DBusPyMessage_ConsumeDBusMessage(DBusMessage *);

/* pending-call.c */
extern PyObject *DBusPyPendingCall_ConsumeDBusPendingCall(DBusPendingCall *,
                                                          PyObject *);

/* mainloop.c */
extern dbus_bool_t dbus_py_set_up_connection(PyObject *conn,
                                             PyObject *mainloop);
extern PyObject *dbus_py_get_default_main_loop(void);

/* validation.c */
dbus_bool_t dbus_py_validate_bus_name(const char *name,
                                      dbus_bool_t may_be_unique,
                                      dbus_bool_t may_be_not_unique);
dbus_bool_t dbus_py_validate_member_name(const char *name);
dbus_bool_t dbus_py_validate_interface_name(const char *name);
dbus_bool_t dbus_py_validate_object_path(const char *path);
#define dbus_py_validate_error_name dbus_py_validate_interface_name

/* debugging support */
#ifdef USING_DBG

#   include <sys/types.h>
#   include <unistd.h>

void _dbus_py_dbg_exc(void);
void _dbus_py_dbg_dump_message(DBusMessage *);

#   define DBG(format, ...) fprintf(stderr, "DEBUG: " format "\n",\
                                    __VA_ARGS__)
#   define DBG_EXC(format, ...) do {DBG(format, __VA_ARGS__); \
                                    _dbus_py_dbg_exc();} while (0)
#   define DBG_DUMP_MESSAGE(x) _dbg_dump_message(x)

#else /* !defined(USING_DBG) */

#   define DBG(format, ...) do {} while (0)
#   define DBG_EXC(format, ...) do {} while (0)
#   define DBG_DUMP_MESSAGE(x) do {} while(0)

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
