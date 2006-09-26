/* Main module source for the _dbus_bindings extension.
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

#include <Python.h>
#include <structmember.h>

#define INSIDE_DBUS_BINDINGS
#include "dbus_bindings.h"

PyDoc_STRVAR(module_doc, "");

#include "debug-impl.h"              /* DBG, USING_DBG, DBG_EXC */
#include "generic-impl.h"            /* Non D-Bus support code */
#include "validation-impl.h"         /* Interface name, etc., validation */
#include "exceptions-impl.h"         /* Exception base classes */
#include "signature-impl.h"          /* Signature and its custom iterator */
#include "types-impl.h"              /* IntNN, UIntNN, ObjectPath */
#include "containers-impl.h"         /* Array, Dict, Variant */
#include "bytes-impl.h"              /* Byte, ByteArray */
#include "message-impl.h"            /* Message and subclasses */
#include "pending-call-impl.h"       /* PendingCall */
#include "conn-impl.h"               /* Connection */
#include "bus-impl.h"                /* Bus */

static PyMethodDef module_functions[] = {
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
init_dbus_bindings (void)
{
    PyObject *this_module;

    if (!init_generic ()) return;
    if (!init_exception_types ()) return;
    if (!init_signature ()) return;
    if (!init_types ()) return;
    if (!init_container_types ()) return;
    if (!init_byte_types ()) return;
    if (!init_message_types ()) return;
    if (!init_pending_call ()) return;
    if (!init_conn_types ()) return;
    if (!init_bus_types ()) return;

    this_module = Py_InitModule3 ("_dbus_bindings", module_functions, module_doc);
    if (!this_module) return;

    if (!insert_exception_types (this_module)) return;
    if (!insert_signature (this_module)) return;
    if (!insert_types (this_module)) return;
    if (!insert_container_types (this_module)) return;
    if (!insert_byte_types (this_module)) return;
    if (!insert_message_types (this_module)) return;
    if (!insert_pending_call (this_module)) return;
    if (!insert_conn_types (this_module)) return;
    if (!insert_bus_types (this_module)) return;

#define ADD_CONST_VAL(x, v) \
    if (PyModule_AddIntConstant (this_module, x, v) < 0) abort();
#define ADD_CONST_PREFIXED(x) ADD_CONST_VAL(#x, DBUS_##x)
#define ADD_CONST(x) ADD_CONST_VAL(#x, x)

    ADD_CONST(DBUS_START_REPLY_SUCCESS)
    ADD_CONST(DBUS_START_REPLY_ALREADY_RUNNING)

    ADD_CONST_PREFIXED(RELEASE_NAME_REPLY_RELEASED)
    ADD_CONST_PREFIXED(RELEASE_NAME_REPLY_NON_EXISTENT)
    ADD_CONST_PREFIXED(RELEASE_NAME_REPLY_NOT_OWNER)

    ADD_CONST_PREFIXED(REQUEST_NAME_REPLY_PRIMARY_OWNER)
    ADD_CONST_PREFIXED(REQUEST_NAME_REPLY_IN_QUEUE)
    ADD_CONST_PREFIXED(REQUEST_NAME_REPLY_EXISTS)
    ADD_CONST_PREFIXED(REQUEST_NAME_REPLY_ALREADY_OWNER)

    ADD_CONST_PREFIXED(NAME_FLAG_ALLOW_REPLACEMENT)
    ADD_CONST_PREFIXED(NAME_FLAG_REPLACE_EXISTING)
    ADD_CONST_PREFIXED(NAME_FLAG_DO_NOT_QUEUE)

    ADD_CONST_PREFIXED(BUS_SESSION)
    ADD_CONST_PREFIXED(BUS_SYSTEM)
    ADD_CONST_PREFIXED(BUS_STARTER)

    ADD_CONST_PREFIXED(MESSAGE_TYPE_INVALID)
    ADD_CONST_PREFIXED(MESSAGE_TYPE_METHOD_CALL)
    ADD_CONST_PREFIXED(MESSAGE_TYPE_METHOD_RETURN)
    ADD_CONST_PREFIXED(MESSAGE_TYPE_ERROR)
    ADD_CONST_PREFIXED(MESSAGE_TYPE_SIGNAL)

    ADD_CONST_PREFIXED(MESSAGE_TYPE_SIGNAL)

    ADD_CONST_PREFIXED(TYPE_INVALID)
    ADD_CONST_PREFIXED(TYPE_BYTE)
    ADD_CONST_PREFIXED(TYPE_BOOLEAN)
    ADD_CONST_PREFIXED(TYPE_INT16)
    ADD_CONST_PREFIXED(TYPE_UINT16)
    ADD_CONST_PREFIXED(TYPE_INT32)
    ADD_CONST_PREFIXED(TYPE_UINT32)
    ADD_CONST_PREFIXED(TYPE_INT64)
    ADD_CONST_PREFIXED(TYPE_UINT64)
    ADD_CONST_PREFIXED(TYPE_DOUBLE)
    ADD_CONST_PREFIXED(TYPE_STRING)
    ADD_CONST_PREFIXED(TYPE_OBJECT_PATH)
    ADD_CONST_PREFIXED(TYPE_SIGNATURE)
    ADD_CONST_PREFIXED(TYPE_ARRAY)
    ADD_CONST_PREFIXED(TYPE_STRUCT)
    ADD_CONST_VAL("STRUCT_BEGIN", DBUS_STRUCT_BEGIN_CHAR)
    ADD_CONST_VAL("STRUCT_END", DBUS_STRUCT_END_CHAR)
    ADD_CONST_PREFIXED(TYPE_VARIANT)
    ADD_CONST_PREFIXED(TYPE_DICT_ENTRY)
    ADD_CONST_VAL("DICT_ENTRY_BEGIN", DBUS_DICT_ENTRY_BEGIN_CHAR)
    ADD_CONST_VAL("DICT_ENTRY_END", DBUS_DICT_ENTRY_END_CHAR)

    ADD_CONST_PREFIXED(HANDLER_RESULT_HANDLED)
    ADD_CONST_PREFIXED(HANDLER_RESULT_NOT_YET_HANDLED)
    ADD_CONST_PREFIXED(HANDLER_RESULT_NEED_MEMORY)
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
