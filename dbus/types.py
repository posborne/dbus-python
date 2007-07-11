"""dbus.types is deprecated due to a name clash with the types module in the
standard library, which causes problems in Scratchbox.

For maximum compatibility, import these types from the top-level dbus package
instead.

:Deprecated: since <unreleased 'purity' branch>."""

__all__ = ('ObjectPath', 'ByteArray', 'Signature', 'Byte', 'Boolean',
           'Int16', 'UInt16', 'Int32', 'UInt32', 'Int64', 'UInt64',
           'Double', 'String', 'Array', 'Struct', 'Dictionary',
           'UTF8String')

from warnings import warn as _warn

from dbus.data import *

_warn(__doc__, DeprecationWarning, stacklevel=2)
