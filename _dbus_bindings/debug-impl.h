/* Debug code for _dbus_bindings.
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

#if 0
#   include <sys/types.h>
#   include <unistd.h>

#   define USING_DBG
#   define DBG(format, ...) fprintf(stderr, "DEBUG: " format "\n",\
                                 __VA_ARGS__)
static void _dbg_exc(void)
{
    PyObject *c, *v, *t;
    /* This is a little mad. We want to get the traceback without
    clearing the error indicator. */
    PyErr_Fetch(&c, &v, &t); /* 3 new refs */
    Py_XINCREF(c); Py_XINCREF(v); Py_XINCREF(t); /* now we own 6 refs */
    PyErr_Restore(c, v, t); /* steals 3 refs */
    PyErr_Print();
    PyErr_Restore(c, v, t); /* steals another 3 refs */
}
#   define DBG_EXC(format, ...) do {DBG(format, __VA_ARGS__); \
                                    _dbg_exc();} while (0)
#else
#   undef USING_DBG
#   define DBG(format, ...) do {} while (0)
#   define DBG_EXC(format, ...) do {} while (0)
#endif
