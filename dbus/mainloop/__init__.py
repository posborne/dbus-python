# Copyright (C) 2006 Collabora Ltd. <http://www.collabora.co.uk/>
#
# Licensed under the Academic Free License version 2.1
#
# This library is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 2.1 of the License, or
# (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public License
# along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

"""Base definitions, etc. for main loop integration.

"""

import _dbus_bindings

NativeMainLoop = _dbus_bindings.NativeMainLoop

NULL_MAIN_LOOP = _dbus_bindings.NULL_MAIN_LOOP
"""A null mainloop which doesn't actually do anything.

For advanced users who want to dispatch events by hand. This is almost
certainly a bad idea - if in doubt, use the GLib main loop found in
`dbus.mainloop.glib`.
"""

WATCH_READABLE = _dbus_bindings.WATCH_READABLE
"""Represents a file descriptor becoming readable.
Used to implement file descriptor watches."""

WATCH_WRITABLE = _dbus_bindings.WATCH_WRITABLE
"""Represents a file descriptor becoming readable.
Used to implement file descriptor watches."""

WATCH_HANGUP = _dbus_bindings.WATCH_HANGUP
"""Represents a file descriptor reaching end-of-file.
Used to implement file descriptor watches."""

WATCH_ERROR = _dbus_bindings.WATCH_ERROR
"""Represents an error condition on a file descriptor.
Used to implement file descriptor watches."""

__all__ = (
           # Imported into this module
           'NativeMainLoop', 'WATCH_READABLE', 'WATCH_WRITABLE',
           'WATCH_HANGUP', 'WATCH_ERROR', 'NULL_MAIN_LOOP',

           # Submodules
           'glib'
           )
