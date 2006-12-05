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
