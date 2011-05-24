#!/usr/bin/env python

usage = """Usage:
python unix-fd-service.py <file name> &
python unix-fd-client.py
"""

# Copyright (C) 2004-2006 Red Hat Inc. <http://www.redhat.com/>
# Copyright (C) 2005-2007 Collabora Ltd. <http://www.collabora.co.uk/>
# Copyright (C) 2010 Signove <http://www.signove.com>
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

import gobject

import dbus
import dbus.service
import dbus.mainloop.glib
import sys
import random

class SomeObject(dbus.service.Object):

    counter = 0

    @dbus.service.method("com.example.SampleInterface",
                         in_signature='', out_signature='h')
    def GetFd(self):
        self.counter = (self.counter + 1) % 3

        if self.counter == 0:
            print "sending UnixFd(filelike)"
            return dbus.types.UnixFd(f)
        elif self.counter == 1:
            print "sending int"
            return f.fileno()
        else:
            print "sending UnixFd(int)"
            return dbus.types.UnixFd(f.fileno())

if len(sys.argv) < 2:
    print usage
    sys.exit(1)

f = file(sys.argv[1], "r")

if __name__ == '__main__':
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

    session_bus = dbus.SessionBus()
    name = dbus.service.BusName("com.example.SampleService", session_bus)
    object = SomeObject(session_bus, '/SomeObject')

    mainloop = gobject.MainLoop()
    print "Running fd service."
    print usage
    mainloop.run()
