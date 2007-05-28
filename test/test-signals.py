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

builddir = os.path.normpath(os.environ["DBUS_TOP_BUILDDIR"])
pydir = os.path.normpath(os.environ["DBUS_TOP_SRCDIR"])

import dbus
import _dbus_bindings
import gobject
import dbus.glib
import dbus.service


logging.basicConfig()
logging.getLogger().setLevel(1)
logger = logging.getLogger('test-signals')


pkg = dbus.__file__
if not pkg.startswith(pydir):
    raise Exception("DBus modules (%s) are not being picked up from the package"%pkg)

if not _dbus_bindings.__file__.startswith(builddir):
    raise Exception("DBus modules (%s) are not being picked up from the package"%_dbus_bindings.__file__)

class TestSignals(unittest.TestCase):
    def setUp(self):
        logger.info('setUp()')
        self.bus = dbus.SessionBus()
        self.remote_object = self.bus.get_object("org.freedesktop.DBus.TestSuitePythonService", "/org/freedesktop/DBus/TestSuitePythonObject")
        self.remote_object_follow = self.bus.get_object("org.freedesktop.DBus.TestSuitePythonService", "/org/freedesktop/DBus/TestSuitePythonObject", follow_name_owner_changes=True)
        self.iface = dbus.Interface(self.remote_object, "org.freedesktop.DBus.TestSuiteInterface")
        self.iface_follow = dbus.Interface(self.remote_object_follow, "org.freedesktop.DBus.TestSuiteInterface")
        self.in_test = None

    def signal_test_impl(self, iface, name, test_removal=False):
        self.in_test = name
        # using append rather than assignment here to avoid scoping issues
        result = []

        def _timeout_handler():
            logger.debug('_timeout_handler for %s: current state %s', name, self.in_test)
            if self.in_test == name:
                main_loop.quit()
        def _signal_handler(s, sender, path):
            logger.debug('_signal_handler for %s: current state %s', name, self.in_test)
            if self.in_test not in (name, name + '+removed'):
                return
            logger.info('Received signal from %s:%s, argument is %r',
                        sender, path, s)
            result.append('received')
            main_loop.quit()
        def _rm_timeout_handler():
            logger.debug('_timeout_handler for %s: current state %s', name, self.in_test)
            if self.in_test == name + '+removed':
                main_loop.quit()

        logger.info('Testing %s', name)
        match = iface.connect_to_signal('SignalOneString', _signal_handler,
                                        sender_keyword='sender',
                                        path_keyword='path')
        logger.info('Waiting for signal...')
        iface.EmitSignal('SignalOneString', 0)
        source_id = gobject.timeout_add(1000, _timeout_handler)
        main_loop.run()
        if not result:
            raise AssertionError('Signal did not arrive within 1 second')
        logger.debug('Removing match')
        match.remove()
        gobject.source_remove(source_id)
        if test_removal:
            self.in_test = name + '+removed'
            logger.info('Testing %s', name)
            result = []
            iface.EmitSignal('SignalOneString', 0)
            source_id = gobject.timeout_add(1000, _rm_timeout_handler)
            main_loop.run()
            if result:
                raise AssertionError('Signal should not have arrived, but did')
            gobject.source_remove(source_id)

    def testSignal(self):
        self.signal_test_impl(self.iface, 'Signal')

    def testRemoval(self):
        self.signal_test_impl(self.iface, 'Removal', True)

    def testSignalAgain(self):
        self.signal_test_impl(self.iface, 'SignalAgain')

    def testRemovalAgain(self):
        self.signal_test_impl(self.iface, 'RemovalAgain', True)

    def testSignalF(self):
        self.signal_test_impl(self.iface_follow, 'Signal')

    def testRemovalF(self):
        self.signal_test_impl(self.iface_follow, 'Removal', True)

    def testSignalAgainF(self):
        self.signal_test_impl(self.iface_follow, 'SignalAgain')

    def testRemovalAgainF(self):
        self.signal_test_impl(self.iface_follow, 'RemovalAgain', True)

if __name__ == '__main__':
    main_loop = gobject.MainLoop()
    gobject.threads_init()
    dbus.glib.init_threads()

    logger.info('Starting unit test')
    unittest.main()
