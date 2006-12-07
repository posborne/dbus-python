#!/usr/bin/env python

usage = """Usage:
python example-signal-emitter.py &
python example-signal-recipient.py
python example-signal-recipient.py --exit-service
"""

import gobject

import dbus
import dbus.service
import dbus.mainloop.glib

class TestObject(dbus.service.Object):
    def __init__(self, conn, object_path='/com/example/TestService/object'):
        dbus.service.Object.__init__(self, conn, object_path)

    @dbus.service.signal('com.example.TestService')
    def HelloSignal(self, message):
        # The signal is emitted when this method exits
        # You can have code here if you wish
        pass

    @dbus.service.method('com.example.TestService')
    def emitHelloSignal(self):
        #you emit signals by calling the signal's skeleton method
        self.HelloSignal('Hello')
        return 'Signal emitted'

    @dbus.service.method("com.example.TestService",
                         in_signature='', out_signature='')
    def Exit(self):
        loop.quit()

if __name__ == '__main__':
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

    session_bus = dbus.SessionBus()
    name = dbus.service.BusName('com.example.TestService', session_bus)
    object = TestObject(session_bus)

    loop = gobject.MainLoop()
    print "Running example signal emitter service."
    print usage
    loop.run()
