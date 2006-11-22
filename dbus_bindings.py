from warnings import warn as _warn
from dbus._dbus import _dbus_bindings_warning
_warn(_dbus_bindings_warning, DeprecationWarning, stacklevel=2)

from dbus.dbus_bindings import *
