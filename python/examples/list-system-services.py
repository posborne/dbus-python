#!/usr/bin/env python

"""Lists services on the system bus
"""

import dbus

# Get a connection to the SYSTEM bus
bus = dbus.SystemBus()

# Get a reference to the desktop bus' standard object, denoted
# by the path /org/freedesktop/DBus. 
dbus_object = bus.get_object('org.freedesktop.DBus', '/org/freedesktop/DBus')

# The object /org/freedesktop/DBus
# implements the 'org.freedesktop.DBus' interface
dbus_iface = dbus.Interface(dbus_object, 'org.freedesktop.DBus')

# One of the member functions in the org.freedesktop.DBus interface
# is ListServices(), which provides a list of all the other services
# registered on this bus. Call it, and print the list.
print dbus_object.ListNames()
