#!/usr/bin/env python
print "WARNING: this hasn't been updated to current API yet, and might not work"
#FIXME: doesn't work with the new bindings

# Copyright (C) 2004-2006 Red Hat Inc. <http://www.redhat.com/>
# Copyright (C) 2005-2007 Collabora Ltd. <http://www.collabora.co.uk/>
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use, copy,
# modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

import dbus

import gobject
import gconf

class GConfService(dbus.Service):

    def __init__(self):
        dbus.Service.__init__(self, "org.gnome.GConf", dbus.SessionBus())

        gconf_object_tree = self.GConfObjectTree(self)
        
    class GConfObjectTree(dbus.ObjectTree):
        def __init__(self, service):
            dbus.ObjectTree.__init__(self, "/org/gnome/GConf", service)

            self.client = gconf.client_get_default()

        def object_method_called(self, message, object_path, method_name, argument_list):
            print ("Method %s called on GConf key %s" % (method_name, object_path))

            if "getString" == method_name:
                return self.client.get_string(object_path)
            elif "setString" == method_name:
                self.client.set_int(object_path, argument_list[0])
            elif "getInt" == method_name:
                return self.client.get_int(object_path)                
            elif "setInt" == method_name:
                self.client.set_int(object_path, argument_list[0])

gconf_service = GConfService()

print ("GConf Proxy service started.")
print ("Run 'gconf-proxy-client.py' to fetch a GConf key through the proxy...")

mainloop = gobject.MainLoop()
mainloop.run()
