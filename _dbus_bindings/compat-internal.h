/* Old D-Bus compatibility: implementation internals
 *
 * Copyright © 2006-2011 Collabora Ltd.
 * Copyright © 2011 Nokia Corporation
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

#ifndef DBUS_BINDINGS_COMPAT_INTERNAL_H
#define DBUS_BINDINGS_COMPAT_INTERNAL_H

#include "config.h"
#include "dbus_bindings-internal.h"

#ifndef HAVE_DBUSBASICVALUE
typedef union {
    dbus_bool_t bool_val;
    double dbl;
    dbus_uint16_t u16;
    dbus_int16_t i16;
    dbus_uint32_t u32;
    dbus_int32_t i32;
#if defined(DBUS_HAVE_INT64) && defined(HAVE_LONG_LONG)
    dbus_uint64_t u64;
    dbus_int64_t i64;
#endif
    const char *str;
    unsigned char byt;
    float f;
    int fd;
} DBusBasicValue;
#endif

#endif
