# Backwards-compatibility with the old dbus_bindings.

from warnings import warn as _warn
from dbus._dbus import _dbus_bindings_warning
_warn(_dbus_bindings_warning, DeprecationWarning, stacklevel=2)

# Exceptions
from _dbus_bindings import DBusException
class ConnectionError(Exception): pass

# Types
from dbus.types import *

# Messages
from _dbus_bindings import Message, SignalMessage as Signal,\
                           MethodCallMessage as MethodCall,\
                           MethodReturnMessage as MethodReturn,\
                           ErrorMessage as Error
# MessageIter has gone away, thankfully

# Connection
from _dbus_bindings import Connection

from dbus import Bus
bus_request_name = Bus.request_name
bus_release_name = Bus.release_name
