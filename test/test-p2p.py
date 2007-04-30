#!/usr/bin/env python

# Copyright (C) 2004 Red Hat Inc. <http://www.redhat.com/>
# Copyright (C) 2005, 2006 Collabora Ltd. <http://www.collabora.co.uk/>
#
# Licensed under the Academic Free License version 2.1
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


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
