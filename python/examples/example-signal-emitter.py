import dbus
import gtk

class TestObject(dbus.Object):
    def __init__(self, service):
        dbus.Object.__init__(self, "/object", service, [self.HelloWorld])

    def emitHelloSignal(self, message):
        # Emit the signal
        self.emit_signal(interface="org.designfu.TestService",
                              signal_name="hello")

session_bus = dbus.SessionBus()
service = dbus.Service("org.designfu.TestService", bus=session_bus)
object = TestObject(service)

gtk.main()
A
