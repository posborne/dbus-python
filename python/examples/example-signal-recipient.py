import gtk
import dbus

bus = dbus.SessionBus()
service = bus.get_service("org.designfu.TestService")
object  = service.get_object("/org/designfu/TestService/object", "org.designfu.TestService")

def hello_signal_handler(sender):
        print ("Received signal '%s.%s' from object '%s%s'"
               % (sender.interface, sender.signal_name, sender.service, sender.path))


object.connect_to_signal("hello", hello_signal_handler)

# Tell the remote object to emit the signal
object.emitHelloSignal()

gtk.main()

