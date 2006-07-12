#!/usr/bin/env python

import dbus
import dbus.decorators
import dbus.glib
import gobject

def handle_reply(msg):
    print msg

def handle_error(e):
    print str(e)

def emit_signal():
   #call the emitHelloSignal method 
   object.emitHelloSignal(dbus_interface="org.designfu.TestService")
                          #reply_handler = handle_reply, error_handler = handle_error)
   return True

bus = dbus.SessionBus()
object  = bus.get_object("org.designfu.TestService","/org/designfu/TestService/object")

def hello_signal_handler(hello_string):
    print ("Received signal and it says: " + hello_string)

@dbus.decorators.explicitly_pass_message
def catchall_signal_handler(*args, **keywords):
    #The dbus.handler directive passes in the special __dbus_message__ variable
    dbus_message = keywords["dbus_message"]
    print "Caught signal " + dbus_message.get_member()
    for arg in args:
        print "        " + str(arg)

def catchall_hello_signals_handler(hello_string):
    print ("Received a hello signal and it says ") + hello_string
    
@dbus.decorators.explicitly_pass_message
def catchall_testservice_interface_handler(hello_string, dbus_message):
    print "org.designfu.TestService interface says " + hello_string + " when it sent signal " + dbus_message.get_member()

object.connect_to_signal("HelloSignal", hello_signal_handler, dbus_interface="org.designfu.TestService", arg0="Hello")

#lets make a catchall
bus.add_signal_receiver(catchall_signal_handler)
bus.add_signal_receiver(catchall_hello_signals_handler, dbus_interface = "org.designfu.TestService", signal_name = "HelloSignal")
bus.add_signal_receiver(catchall_testservice_interface_handler, dbus_interface = "org.designfu.TestService")


gobject.timeout_add(2000, emit_signal)

# Tell the remote object to emit the signal

loop = gobject.MainLoop()
loop.run()
