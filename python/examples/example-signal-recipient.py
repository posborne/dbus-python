import gtk
import dbus

bus = dbus.SessionBus()
service = bus.get_service("org.designfu.TestService")
object  = service.get_object("/org/designfu/TestService/object", "org.designfu.TestService")

def hello_signal_handler(interface, signal_name, service, path, message):
        print ("Received signal '%s.%s' from object '%s%s'"
               % (interface, signal_name, service, path))


object.connect_to_signal("hello", hello_signal_handler)

# Tell the remote object to emit the signal
object.emitHelloSignal()

gtk.main()

