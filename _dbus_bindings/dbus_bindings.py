# Backwards-compatibility with the old dbus_bindings.

# Exceptions
from _dbus_bindings import DBusException
class ConnectionError(Exception): pass

# Types
from _dbus_bindings import Int16, UInt16, Int32, UInt32, Int64, UInt64,\
                           Variant, ObjectPath, Signature, Byte, ByteArray
# Don't bother importing SignatureIter, it can't be instantiated
# These used to be specialized subclasses, but these are unambiguous
String = unicode
Boolean = bool
Array = list            # FIXME: c'tor params
Double = float
Struct = tuple
Dictionary = dict       # FIXME: c'tor params

# Messages
from _dbus_bindings import Message, SignalMessage as Signal,\
                           MethodCallMessage as MethodCall,\
                           MethodReturnMessage as MethodReturn,\
                           ErrorMessage as Error
# MessageIter has gone away, thankfully

# Connection
from _dbus_bindings import Connection
