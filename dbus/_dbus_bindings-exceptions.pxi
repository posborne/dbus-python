# vim:set sw=4 sts=4 ft=pyrex:
# Exceptions defined in _dbus_bindings and imported into dbus.exceptions.

__docformat__ = 'restructuredtext'

class DBusException(Exception):
    """Represents any D-Bus-related error."""

class ConnectionError(Exception):
    """FIXME: Appears to be unused?"""
