"""\
Implements the public API for a D-Bus client. See the dbus.service module
to implement services.

FIXME
-----
Which of the imported symbols constitute public API?
"""

__all__ = ('Bus', 'SystemBus', 'SessionBus', 'StarterBus',
           'Interface',
           'DBusException', 'ConnectionError',
           'MissingErrorHandlerException', 'MissingReplyHandlerException',
           'ValidationException', 'IntrospectionParserException',
           'UnknownMethodException', 'NameExistsException',
           'ObjectPath', 'ByteArray', 'Signature', 'Byte', 'Boolean',
           'Int16', 'UInt16', 'Int32', 'UInt32', 'Int64', 'UInt64',
           'Double', 'String', 'Array', 'Struct', 'Dictionary', 'Variant'
           )
__docformat__ = 'restructuredtext'
version = (0, 70, 0)
__version__ = '.'.join(map(str, version))

from dbus._dbus import *
from dbus.types import *

_dbus_main_loop_setup_function = None
