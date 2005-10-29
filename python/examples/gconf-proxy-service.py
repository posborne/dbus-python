#!/usr/bin/env python
#FIXME: Doesn't work with the new bindings
import dbus

import gobject
import gconf

class GConfService(dbus.Service):

    def __init__(self):
        dbus.Service.__init__(self, "org.gnome.GConf", dbus.SessionBus())

        gconf_object_tree = self.GConfObjectTree(self)
        
    class GConfObjectTree(dbus.ObjectTree):
        def __init__(self, service):
            dbus.ObjectTree.__init__(self, "/org/gnome/GConf", service, dbus_methods=[ self.getString, self.setString, self.getInt, self.setInt ])

            self.client = gconf.client_get_default()

        def getString(self, message, object_path):
            print ("getString called on GConf key %s" % (object_path))
            return self.client.get_string(object_path)

        def setString(self, message, object_path, new_value):
            print ("setString called on GConf key %s" % (object_path))            
            self.client.set_string(object_path, new_value)

        def getInt(self, message, object_path):
            print ("getInt called on GConf key %s" % (object_path))
            return self.client.get_int(object_path)

        def setInt(self, message, object_path, new_value):
            print ("setInt called on GConf key %s" % (object_path))
            self.client.set_int(object_path, new_value)
            
gconf_service = GConfService()

print ("GConf Proxy service started.")
print ("Run 'gconf-proxy-client.py' to fetch a GConf key through the proxy...")

mainloop = gobject.MainLoop()
mainloop.run()
