/* D-Bus types: implementation internals
 *
 * Copyright (C) 2006, 2007 Collabora Ltd.
 *
 * Licensed under the Academic Free License version 2.1
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <Python.h>
#include <stdint.h>

#include "dbus_bindings-internal.h"

#ifndef DBUS_BINDINGS_TYPES_INTERNAL_H
#define DBUS_BINDINGS_TYPES_INTERNAL_H

extern PyTypeObject DBusPyStrBase_Type;
DEFINE_CHECK(DBusPyStrBase)

dbus_int16_t dbus_py_int16_range_check(PyObject *);
dbus_uint16_t dbus_py_uint16_range_check(PyObject *);
dbus_int32_t dbus_py_int32_range_check(PyObject *);
dbus_uint32_t dbus_py_uint32_range_check(PyObject *);

#if defined(DBUS_HAVE_INT64) && defined(HAVE_LONG_LONG)
#   define DBUS_PYTHON_64_BIT_WORKS 1
dbus_int64_t dbus_py_int64_range_check(PyObject *);
dbus_uint64_t dbus_py_uint64_range_check(PyObject *);
#else
#   undef DBUS_PYTHON_64_BIT_WORKS
#endif /* defined(DBUS_HAVE_INT64) && defined(HAVE_LONG_LONG) */

extern PyObject *dbus_py_variant_level_const;
extern PyObject *dbus_py_signature_const;
extern PyObject *dbus_py__dbus_object_path__const;

typedef struct {
    PyDictObject super;
    PyObject *signature;
    long variant_level;
} DBusPyDictionary;

PyObject *dbus_py_variant_level_getattro(PyObject *obj, PyObject *name);
dbus_bool_t dbus_py_variant_level_set(PyObject *obj, long variant_level);
void dbus_py_variant_level_clear(PyObject *obj);
long dbus_py_variant_level_get(PyObject *obj);

#endif
