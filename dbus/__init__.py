"""\
Implements the public API for a D-Bus client. See the dbus.service module
to implement services.

FIXME
-----
Which of the imported symbols constitute public API?
"""

from _dbus import *
from types import *

version = (0, 70, 0)
_dbus_main_loop_setup_function = None
