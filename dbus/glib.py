# Copyright (C) 2004 Anders Carlsson
# Copyright (C) 2004, 2005, 2006 Red Hat Inc. <http://www.redhat.com/>
# Copyright (C) 2005, 2006 Collabora Ltd. <http://www.collabora.co.uk/>
#
# Licensed under the Academic Free License version 2.1
#
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

"""Deprecated module which sets the default GLib main context as the mainloop
implementation within D-Bus, as a side-effect of being imported!

This API is highly non-obvious, so instead of importing this module,
new programs which don't need pre-0.80 compatibility should use this
equivalent code::

    from dbus.mainloop.glib import DBusGMainLoop
    DBusGMainLoop(set_as_default=True)
"""
__docformat__ = 'restructuredtext'

from dbus.mainloop.glib import DBusGMainLoop, threads_init
import _dbus_glib_bindings
import _dbus_bindings

init_threads = threads_init

DBusGMainLoop(set_as_default=True)
