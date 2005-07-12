#!/usr/bin/env python

import dbus
import dbus.service
import gtk

class TestObject(dbus.service.Object):
    def __init__(self, name):
        dbus.service.Object.__init__(self, "/org/designfu/TestService/object", name)

    @dbus.service.signal('org.designfu.TestService')
    def HelloSignal(self, message):
        # The signal is emitted when this method exits
        # You can have code here if you wish
        pass

    @dbus.service.method('org.designfu.TestService')
    def emitHelloSignal(self):
        #you emit signals by calling the signal's skeleton method
        self.HelloSignal("Hello")
        return "Signal emitted"

session_bus = dbus.SessionBus()
name = dbus.service.Name("org.designfu.TestService", bus=session_bus)
object = TestObject(name)

gtk.main()
