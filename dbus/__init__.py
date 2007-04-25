"""\
Implements the public API for a D-Bus client. See the dbus.service module
to export objects or claim well-known names.

..
  for epydoc's benefit

:NewField SupportedUsage: Supported usage
:NewField Constructor: Constructor
"""

# Copyright (C) 2003, 2004, 2005, 2006 Red Hat Inc. <http://www.redhat.com/>
# Copyright (C) 2003 David Zeuthen
# Copyright (C) 2004 Rob Taylor
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

import os

__all__ = (
           # from _dbus
           'Bus', 'SystemBus', 'SessionBus', 'StarterBus',
           'Interface',

           # from _dbus_bindings
           'get_default_main_loop', 'set_default_main_loop',

           'validate_interface_name', 'validate_member_name',
           'validate_bus_name', 'validate_object_path',
           'validate_error_name',

           'DBusException',

           'ObjectPath', 'ByteArray', 'Signature', 'Byte', 'Boolean',
           'Int16', 'UInt16', 'Int32', 'UInt32', 'Int64', 'UInt64',
           'Double', 'String', 'Array', 'Struct', 'Dictionary', 'UTF8String',

           # from exceptions
           'MissingErrorHandlerException', 'MissingReplyHandlerException',
           'ValidationException', 'IntrospectionParserException',
           'UnknownMethodException', 'NameExistsException',

           # submodules
           'service', 'mainloop', 'lowlevel'
           )
__docformat__ = 'restructuredtext'

try:
    from dbus._version import version, __version__
except ImportError:
    pass

# OLPC Sugar compatibility
import dbus.exceptions as exceptions
import dbus.types as types

from _dbus_bindings import get_default_main_loop, set_default_main_loop,\
                           validate_interface_name, validate_member_name,\
                           validate_bus_name, validate_object_path,\
                           validate_error_name
from _dbus_bindings import DBusException
from dbus.exceptions import MissingErrorHandlerException, \
                            MissingReplyHandlerException, \
                            ValidationException, \
                            IntrospectionParserException, \
                            UnknownMethodException, \
                            NameExistsException
from _dbus_bindings import ObjectPath, ByteArray, Signature, Byte, Boolean,\
                           Int16, UInt16, Int32, UInt32, Int64, UInt64,\
                           Double, String, Array, Struct, Dictionary, \
                           UTF8String
from dbus._dbus import Bus, SystemBus, SessionBus, StarterBus, Interface


if 'DBUS_PYTHON_NO_DEPRECATED' not in os.environ:
    from dbus._dbus import dbus_bindings    # for backwards compat
