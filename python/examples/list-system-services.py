"""Lists services on the system bus
"""
import dbus

# Get a connection to the SYSTEM bus
bus = dbus.SystemBus()

# Get the service provided by the dbus-daemon named org.freedesktop.DBus
dbus_service = bus.get_service('org.freedesktop.DBus')
    
# Get a reference to the desktop bus' standard object, denoted
# by the path /org/freedesktop/DBus. The object /org/freedesktop/DBus
# implements the 'org.freedesktop.DBus' interface
dbus_object = dbus_service.get_object('/org/freedesktop/DBus',
                                      'org.freedesktop.DBus')
            
# One of the member functions in the org.freedesktop.DBus interface
# is ListServices(), which provides a list of all the other services
# registered on this bus. Call it, and print the list.
system_service_list = dbus_object.ListServices()

for service in system_service_list:
    if service[0] != ':':
        print (service)
