import dbus

import gtk
import gconf

class GConfService(dbus.Service):

    def __init__(self):
        dbus.Service.__init__(self, "org.gnome.GConf", dbus.SessionBus())

        gconf_object_tree = self.GConfObjectTree(self)
        
    class GConfObjectTree(dbus.ObjectTree):

        def __init__(self, service):
            dbus.ObjectTree.__init__(self, "/org/gnome/GConf", service)

            self.client = gconf.client_get_default()
            
        def object_method_called(self, object_path, method_name, argument_list):
            print ("Method %s called on GConf key %s" % (method_name, object_path))

            return_value = None

            if "getString" == method_name:
                assert(len(argument_list) == 0)
                return_value = self.client.get_string (object_path)
                
            elif "setString" == method_name:
                assert(len(argument_list) == 1)
                self.client.set_string(object_path, argument_list[0])
                
            elif "getInt" == method_name:
                assert(len(argument_list) == 0)
                return_value = self.client.get_int(object_path)
                
            elif "setInt" == method_name:
                assert(len(argument_list) == 1)
                self.client.set_int(object_path, argument_list[0])

            return return_value

gconf_service = GConfService()

print ("GConf Proxy service started.")
print ("Run 'gconf-proxy-client.py' to fetch a GConf key through the proxy...")

gtk.main()
