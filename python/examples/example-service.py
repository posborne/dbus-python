import dbus

import pygtk
import gtk

class SomeObject(dbus.Object):
    def __init__(self, service):
        dbus.Object.__init__(self, "/SomeObject", service)

    @dbus.method("org.designfu.SampleInterface")
    def HelloWorld(self, hello_message):
        print (str(hello_message))
        return ["Hello", " from example-service.py"]

    @dbus.method("org.designfu.SampleInterface")
    def GetTuple(self):
        return ("Hello Tuple", " from example-service.py")

    @dbus.method("org.designfu.SampleInterface")
    def GetDict(self):
        return {"first": "Hello Dict", "second": " from example-service.py"}

session_bus = dbus.SessionBus()
service = dbus.Service("org.designfu.SampleService", bus=session_bus)
object = SomeObject(service)

gtk.main()
