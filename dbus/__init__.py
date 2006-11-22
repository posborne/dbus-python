"""\
Implements the public API for a D-Bus client. See the dbus.service module
to implement services.

FIXME
-----
Which of the imported symbols constitute public API?
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

__all__ = (
           # from _dbus
           'Bus', 'SystemBus', 'SessionBus', 'StarterBus',
           'Interface',
           # from _dbus_bindings via exceptions
           'DBusException', 'ConnectionError',
           # from exceptions
           'MissingErrorHandlerException', 'MissingReplyHandlerException',
           'ValidationException', 'IntrospectionParserException',
           'UnknownMethodException', 'NameExistsException',
           # from types
           'ObjectPath', 'ByteArray', 'Signature', 'Byte', 'Boolean',
           'Int16', 'UInt16', 'Int32', 'UInt32', 'Int64', 'UInt64',
           'Double', 'String', 'Array', 'Struct', 'Dictionary', 'UTF8String',
           )
__docformat__ = 'restructuredtext'
# version is really 0.80pre1, but you can't represent that in this form
version = (0, 79, 1)
__version__ = '.'.join(map(str, version))

from dbus._dbus import *
from dbus.types import *
