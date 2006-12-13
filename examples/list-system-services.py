#!/usr/bin/env python

"""Usage: python list-system-services.py [--session|--system]
List services on the system bus (default) or the session bus."""

import sys

import dbus
from dbus.mainloop import NULL_MAIN_LOOP

def main(argv):
    factory = dbus.SystemBus

    if len(argv) > 2:
        sys.exit(__doc__)
    elif len(argv) == 2:
        if argv[1] == '--session':
            factory = dbus.SessionBus
        elif argv[1] != 'system':
            sys.exit(__doc__)

    # Get a connection to the system or session bus as appropriate
    # We're only using blocking calls, so don't actually need a main loop here
    bus = factory(mainloop=NULL_MAIN_LOOP)

    # Get a reference to the desktop bus' standard object, denoted
    # by the path /org/freedesktop/DBus. 
    dbus_object = bus.get_object('org.freedesktop.DBus', '/org/freedesktop/DBus')

    # The object /org/freedesktop/DBus
    # implements the 'org.freedesktop.DBus' interface
    dbus_iface = dbus.Interface(dbus_object, 'org.freedesktop.DBus')

    # One of the member functions in the org.freedesktop.DBus interface
    # is ListServices(), which provides a list of all the other services
    # registered on this bus. Call it, and print the list.
    services = dbus_object.ListNames()
    services.sort()
    for service in services:
        print service

if __name__ == '__main__':
    main(sys.argv)
