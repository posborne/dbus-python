#!/usr/bin/env python

import dbus

bus = dbus.SessionBus()
remote_service = bus.get_service("org.designfu.SampleService")
remote_object = remote_service.get_object("/SomeObject",
                                          "org.designfu.SampleInterface")

hello_reply_list = remote_object.HelloWorld("Hello from example-client.py!")

print (hello_reply_list)
