#!/usr/bin/env python

usage = """Usage:
python example-service.py &
python example-async-client.py
python example-client.py --exit-service
"""

# Copyright (C) 2004-2006 Red Hat Inc. <http://www.redhat.com/>
# Copyright (C) 2005-2007 Collabora Ltd. <http://www.collabora.co.uk/>
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use, copy,
# modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

import sys
import traceback

import gobject

import dbus
import dbus.mainloop.glib

# Callbacks for asynchronous calls

def handle_hello_reply(r):
    global hello_replied
    hello_replied = True

    print str(r)

    if hello_replied and raise_replied:
        loop.quit()

def handle_hello_error(e):
    global failed
    global hello_replied
    hello_replied = True
    failed = True

    print "HelloWorld raised an exception! That's not meant to happen..."
    print "\t", str(e)

    if hello_replied and raise_replied:
        loop.quit()

def handle_raise_reply():
    global failed
    global raise_replied
    raise_replied = True
    failed = True

    print "RaiseException returned normally! That's not meant to happen..."

    if hello_replied and raise_replied:
        loop.quit()

def handle_raise_error(e):
    global raise_replied
    raise_replied = True

    print "RaiseException raised an exception as expected:"
    print "\t", str(e)

    if hello_replied and raise_replied:
        loop.quit()

def make_calls():
    # To make an async call, use the reply_handler and error_handler kwargs
    remote_object.HelloWorld("Hello from example-async-client.py!",
                             dbus_interface='com.example.SampleInterface',
                             reply_handler=handle_hello_reply,
                             error_handler=handle_hello_error)

    # Interface objects also support async calls
    iface = dbus.Interface(remote_object, 'com.example.SampleInterface')
    
    iface.RaiseException(reply_handler=handle_raise_reply,
                         error_handler=handle_raise_error)

    return False

if __name__ == '__main__':
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

    bus = dbus.SessionBus()
    try:
        remote_object = bus.get_object("com.example.SampleService","/SomeObject")
    except dbus.DBusException:
        traceback.print_exc()
        print usage
        sys.exit(1)

    # Make the method call after a short delay
    gobject.timeout_add(1000, make_calls)

    failed = False
    hello_replied = False
    raise_replied = False

    loop = gobject.MainLoop()
    loop.run()
    if failed:
        raise SystemExit("Example async client failed!")
