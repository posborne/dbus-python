# Copyright (C) 2004 Anders Carlsson
# Copyright (C) 2004, 2005, 2006 Red Hat Inc. <http://www.redhat.com/>
# Copyright (C) 2005, 2006 Collabora Ltd. <http://www.collabora.co.uk/>
#
# Licensed under the Academic Free License version 2.1
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

import dbus
import _dbus_glib_bindings

def _setup_with_g_main(conn):
    _dbus_glib_bindings.setup_with_g_main(conn._connection)

_dbus_gthreads_initialized = False
def threads_init():
    global _dbus_gthreads_initialized
    if not _dbus_gthreads_initialized:
        _dbus_glib_bindings.gthreads_init()
        _dbus_gthreads_initialized = True

def init_threads():
    threads_init()

setattr(dbus, "_dbus_mainloop_setup_function", _setup_with_g_main)
