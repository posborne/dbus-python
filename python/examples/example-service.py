import dbus

import pygtk
import gtk

class SomeObject(dbus.Object):
    def __init__(self, service):
        export = [self.HelloWorld, self.GetTuple, self.GetDict]
        dbus.Object.__init__(self, "/SomeObject", service, export)

    def HelloWorld(self, message, hello_message):
        print (str(hello_message))
        return ["Hello", " from example-service.py"]

    def GetTuple(self, message):
        return ("Hello Tuple", " from example-service.py")

    def GetDict(self, message):
        return {"first": "Hello Dict", "second": " from example-service.py"}

session_bus = dbus.SessionBus()
service = dbus.Service("org.designfu.SampleService", bus=session_bus)
object = SomeObject(service)

gtk.main()
