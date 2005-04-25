import dbus
import gtk

class TestObject(dbus.Object):
    def __init__(self, service):
        dbus.Object.__init__(self, "/org/designfu/TestService/object", service)

    @dbus.signal('org.designfu.TestService')
    def HelloSignal(self, message):
        # The signal is emitted when this method exits
	# You can have code here if you wish
        pass

    @dbus.method('org.designfu.TestService')
    def emitHelloSignal(self):
        #you emit signals by calling the signal's skeleton method
	self.HelloSignal("Hello")
	return "Signal emitted"
	
session_bus = dbus.SessionBus()
service = dbus.Service("org.designfu.TestService", bus=session_bus)
object = TestObject(service)

gtk.main()
