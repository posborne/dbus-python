import pygtk
import gtk

import dbus

class SignalFrom(dbus.Object):
    def __init__(self, service):
        dbus.Object.__init__(self, "/", [], service)

def signal_to(interface, signal_name, service, path):
    print ("Received signal '%s.%s' from '%s%s'" % (interface, signal_name, service, path))

bus = dbus.Bus()
bus.add_signal_receiver(signal_to,
                        "org.designfu.SignalInterface",
                        "org.designfu.SignalService",
                        "/")
                                  

service = dbus.Service("org.designfu.SignalService", bus)
signal_from = SignalFrom(service)

signal_from.broadcast_signal("org.designfu.SignalInterface", "HelloWorld")

gtk.main()


