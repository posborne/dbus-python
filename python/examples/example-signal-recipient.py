#!/usr/bin/env python

import gtk
import dbus
import gobject

def handle_reply(msg):
    print msg

def handle_error(e):
    print str(e)

def emit_signal():
   #call the emitHelloSignal method async
   object.emitHelloSignal(dbus_interface="org.designfu.TestService", 
                          reply_handler = handle_reply, error_handler = handle_error)
   return True

bus = dbus.SessionBus()
object  = bus.get_object("org.designfu.TestService","/org/designfu/TestService/object")

def hello_signal_handler(hello_string):
        print ("Received signal and it says: " + hello_string)

object.connect_to_signal("HelloSignal", hello_signal_handler, dbus_interface="org.designfu.TestService")

gobject.timeout_add(2000, emit_signal)

# Tell the remote object to emit the signal

gtk.main()

