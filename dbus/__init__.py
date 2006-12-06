"""\
Implements the public API for a D-Bus client. See the dbus.service module
to export objects or claim well-known names.

The first thing you need to do is connect dbus-python to a main loop
implementation. Currently, the only working implementation is (still)
to use pygobject and libdbus-glib:

    from gobject import MainLoop, idle_add
    from dbus.mainloop.glib import DBusGMainLoop

    def main_function():
        "your code here"

    if __name__ == '__main__':
        idle_add(main_function)
        dbus_mainloop_wrapper = DBusGMainLoop(set_as_default=True)
        mainloop = MainLoop()
        mainloop.run()

In particular, receiving signals and making asynchronous method calls
can only work while the main loop is running.

Calling methods on remote objects
=================================

To access remote objects over the D-Bus you need to get a Bus object,
providing a connection to the bus daemon. You then obtain a remote
object proxy (ProxyObject) from the Bus, by providing the bus name
(either a well-known name or a unique name) and the object path.

Using the ProxyObject you can call methods in a fairly obvious
way, using the dbus_interface keyword argument to choose which interface's
method to call. You can also make an Interface object which wraps the
remote object, and call methods on that.

>>> from dbus import Bus, Interface
>>> bus = Bus(Bus.TYPE_SESSION)
>>> #
>>> # the bus daemon itself provides a service, so use it
>>> #
>>> bus_object = bus.get_object('org.freedesktop.DBus', '/org/freedesktop/DBus')
>>> bus_object
<ProxyObject wrapping <dbus.Bus on SESSION at 0x300843c0> org.freedesktop.DBus /org/freedesktop/DBus at 0x30302080>
>>> for x in bus_object.ListNames(dbus_interface='org.freedesktop.DBus'):
...    print repr(x)
...
dbus.String(u'org.freedesktop.DBus')
dbus.String(u'org.freedesktop.Notifications')
dbus.String(u':1.0')
dbus.String(u'org.gnome.ScreenSaver')
dbus.String(u':1.1')
dbus.String(u':1.2')
>>> # a different way to do the same thing
>>> bus_iface = Interface(bus_object, 'org.freedesktop.DBus')
>>> for x in bus_iface.ListNames():
...    print repr(x)
...
dbus.String(u'org.freedesktop.DBus')
dbus.String(u'org.freedesktop.Notifications')
dbus.String(u':1.0')
dbus.String(u'org.gnome.ScreenSaver')
dbus.String(u':1.1')
dbus.String(u':1.2')
>>> #
>>> # use one of the other services on the session bus
>>> #
>>> notify_object = bus.get_object('org.freedesktop.Notifications', '/org/freedesktop/Notifications')
>>> notify_iface = Interface(notify_object, 'org.freedesktop.Notifications')
>>> notify_iface.notify('a dbus-python script', 1, '', 'Hello, world', 'Hello from dbus-python', [], {}, 0)
>>> #
>>> # objects can support multiple interfaces
>>> #
>>> peer_iface = Interface(notify_object, 'org.freedesktop.DBus.Peer')
>>> # org.freedesktop.DBus.Peer.Ping is a "do-nothing" method
>>> peer_iface.Ping()
>>>

Asynchronous method calls
=========================

You can call methods asynchronously by passing the ``reply_handler``
and ``error_handler`` keyword arguments. The initial call immediately
returns `None`. The actual result will arrive when it becomes available,
as long as the main loop is running.

If the method succeeds, the ``reply_handler`` will be called with the
return values as arguments. If it fails or times out, the ``error_handler``
will be called with a `dbus.DBusException` instance as its argument.

Receiving signals
=================

To receive signals, get a `dbus.proxies.ProxyObject` or `dbus.Interface`
in the same way as above, and call its ``connect_to_signal`` method.

You can also connect to signals in a generic way using the
`Bus.add_signal_receiver` method.

Either way, a `SignalMatch` object is returned - this object has a `remove`
method which you can call to stop receiving that signal.

Receiving signals happens asynchronously, so it only works while the
main loop is running.

Obtaining a well-known name
===========================

See `dbus.service.BusName`.

Exporting objects
=================

See `dbus.service.Object`.

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
# version is really 0.80rc1, but you can't represent that in this form
version = (0, 79, 91)
__version__ = '.'.join(map(str, version))

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
from dbus._dbus import dbus_bindings    # for backwards compat
