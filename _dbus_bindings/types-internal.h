/* D-Bus types: implementation internals
 *
 * Copyright (C) 2006-2007 Collabora Ltd. <http://www.collabora.co.uk/>
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

#include <Python.h>
#include <stdint.h>

#include "dbus_bindings-internal.h"

#ifndef DBUS_BINDINGS_TYPES_INTERNAL_H
#define DBUS_BINDINGS_TYPES_INTERNAL_H

extern PyTypeObject DBusPyIntBase_Type;
DEFINE_CHECK(DBusPyIntBase)

typedef struct {
    PyIntObject base;
    long variant_level;
} DBusPyIntBase;

extern PyTypeObject DBusPyLongBase_Type;
DEFINE_CHECK(DBusPyLongBase)

extern PyTypeObject DBusPyFloatBase_Type;
DEFINE_CHECK(DBusPyFloatBase)

typedef struct {
    PyFloatObject base;
    long variant_level;
} DBusPyFloatBase;

typedef struct {
    PyUnicodeObject unicode;
    long variant_level;
} DBusPyString;

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
    PyListObject super;
    PyObject *signature;
    long variant_level;
} DBusPyArray;

typedef struct {
    PyDictObject super;
    PyObject *signature;
    long variant_level;
} DBusPyDict;

PyObject *dbus_py_variant_level_getattro(PyObject *obj, PyObject *name);
dbus_bool_t dbus_py_variant_level_set(PyObject *obj, long variant_level);
void dbus_py_variant_level_clear(PyObject *obj);
long dbus_py_variant_level_get(PyObject *obj);

#endif
