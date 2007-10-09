#!/usr/bin/env python

# Copyright (C) 2004 Red Hat Inc. <http://www.redhat.com/>
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
import os
import unittest
import time
import logging
import weakref

builddir = os.path.normpath(os.environ["DBUS_TOP_BUILDDIR"])
pydir = os.path.normpath(os.environ["DBUS_TOP_SRCDIR"])

import dbus
import _dbus_bindings
import gobject
import dbus.glib
import dbus.service


logging.basicConfig()
logging.getLogger().setLevel(1)


NAME = "org.freedesktop.DBus.TestSuitePythonService"
IFACE = "org.freedesktop.DBus.TestSuiteInterface"
OBJECT = "/org/freedesktop/DBus/TestSuitePythonObject"

class TestDBusBindings(unittest.TestCase):
    # This test case relies on the test service already having been activated.

    def get_conn_and_unique(self):
        # since I haven't implemented servers yet, I'll use the bus daemon
        # as our peer - note that there's no name tracking because we're not
        # using dbus.bus.BusConnection!
        conn = dbus.connection.Connection(
                os.environ['DBUS_SESSION_BUS_ADDRESS'])
        unique = conn.call_blocking('org.freedesktop.DBus',
                                    '/org/freedesktop/DBus',
                                    'org.freedesktop.DBus', 'Hello',
                                    '', (), utf8_strings=True)
        self.assert_(unique.__class__ == dbus.UTF8String, repr(unique))
        self.assert_(unique.startswith(':'), unique)
        conn.set_unique_name(unique)
        return conn, unique

    def testCall(self):
        conn, unique = self.get_conn_and_unique()
        ret = conn.call_blocking(NAME, OBJECT, IFACE, 'Echo', 'v', ('V',))
        self.assertEquals(ret, 'V')

    def testCallThroughProxy(self):
        conn, unique = self.get_conn_and_unique()
        proxy = conn.get_object(NAME, OBJECT)
        iface = dbus.Interface(proxy, IFACE)
        ret = iface.Echo('V')
        self.assertEquals(ret, 'V')

    def testSetUniqueName(self):
        conn, unique = self.get_conn_and_unique()
        ret = conn.call_blocking(NAME, OBJECT, IFACE,
                                 'MethodExtraInfoKeywords', '', (),
                                 utf8_strings=True)
        self.assertEquals(ret, (unique, OBJECT, NAME,
                                'dbus.lowlevel.MethodCallMessage'))


if __name__ == '__main__':
    gobject.threads_init()
    dbus.glib.init_threads()

    unittest.main()
