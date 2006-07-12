#!/usr/bin/env python

import dbus
import dbus.service
import dbus.glib
import gobject

class TestObject(dbus.service.Object):
    def __init__(self, bus_name, object_path='/org/designfu/TestService/object'):
        dbus.service.Object.__init__(self, bus_name, object_path)

    @dbus.service.signal('org.designfu.TestService')
    def HelloSignal(self, message):
        # The signal is emitted when this method exits
        # You can have code here if you wish
        pass

    @dbus.service.method('org.designfu.TestService')
    def emitHelloSignal(self):
        #you emit signals by calling the signal's skeleton method
        self.HelloSignal('Hello')
        return 'Signal emitted'

session_bus = dbus.SessionBus()
name = dbus.service.BusName('org.designfu.TestService', bus=session_bus)
object = TestObject(name)

loop = gobject.MainLoop()
loop.run()
