import dbus
import gtk

class TestObject(dbus.Object):
    def __init__(self, service):
        method_list = [ self.Echo ]
        dbus.Object.__init__(self, "/TestObject", method_list, service)

    def Echo(self, variable):
        return variable
            
session_bus = dbus.SessionBus()

local_service = dbus.Service("org.designfu.Test", bus=session_bus)
local_object = TestObject(local_service)

gtk.main()
