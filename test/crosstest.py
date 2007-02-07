# Shared code for the cross-test.

# Copyright (C) 2006 Collabora Ltd. <http://www.collabora.co.uk/>
#
# Licensed under the Academic Free License version 2.1
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 2.1 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

import dbus.service

INTERFACE_SINGLE_TESTS = 'org.freedesktop.DBus.Binding.SingleTests'
INTERFACE_TESTS = 'org.freedesktop.DBus.Binding.Tests'
INTERFACE_SIGNAL_TESTS = 'org.freedesktop.DBus.Binding.TestSignals'
INTERFACE_CALLBACK_TESTS = 'org.freedesktop.DBus.Binding.TestCallbacks'

CROSS_TEST_PATH = '/Test'
CROSS_TEST_BUS_NAME = 'org.freedesktop.DBus.Binding.TestServer'


# Exported by both the client and the server
class SignalTestsImpl(dbus.service.Object):
    @dbus.service.signal(INTERFACE_SIGNAL_TESTS, 't')
    def Triggered(self, parameter):
        pass

    @dbus.service.signal(INTERFACE_SIGNAL_TESTS, 'qd')
    def Trigger(self, parameter1, parameter2):
        pass
