import dbus
import dbus_bindings


def TestEcho(value, should_be_equal = True):
    global remote_object
    echoed = remote_object.Echo(value)
    if type(echoed) != type(value):
        raise Exception ("Sending %s, expected echo of type %s, but got %s" % (value, type(value), type(echoed)))

    if echoed.__class__ != value.__class__:
        raise Exception ("Sending %s, expected echo to be of class %s, but got %s" % (value, value.__class__, echoed.__class__))

    if should_be_equal:
        if echoed != value:
            raise Exception("Sending %s, expected echo to be the same, but was %s" % (value, echoed))

session_bus = dbus.SessionBus()

remote_service = session_bus.get_service("org.designfu.Test")
remote_object = remote_service.get_object("/TestObject", "org.designfu.Test")

TestEcho(chr(120))
TestEcho(10)
TestEcho(39.5)
TestEcho("HelloWorld")
TestEcho(dbus_bindings.ObjectPath("/test/path"))

