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
import logging

builddir = os.path.normpath(os.environ["DBUS_TOP_BUILDDIR"])
pydir = os.path.normpath(os.environ["DBUS_TOP_SRCDIR"])

import dbus

if not dbus.__file__.startswith(pydir):
    raise Exception("DBus modules are not being picked up from the package")

import dbus.service
import dbus.glib
import gobject
import random


logging.basicConfig(filename=builddir + '/test/test-service.log', filemode='w')
logging.getLogger().setLevel(1)
logger = logging.getLogger('test-service')


IFACE = 'org.freedesktop.DBus.TestSuiteInterface'


class TestInterface(dbus.service.Interface):
    @dbus.service.method(IFACE, in_signature='', out_signature='b')
    def CheckInheritance(self):
        return False

class TestObject(dbus.service.Object, TestInterface):
    def __init__(self, bus_name, object_path="/org/freedesktop/DBus/TestSuitePythonObject"):
        dbus.service.Object.__init__(self, bus_name, object_path)

    """ Echo whatever is sent
    """
    @dbus.service.method(IFACE)
    def Echo(self, arg):
        return arg

    @dbus.service.method(IFACE, in_signature='s', out_signature='s')
    def AcceptUnicodeString(self, foo):
        assert isinstance(foo, unicode), (foo, foo.__class__.__mro__)
        return foo

    @dbus.service.method(IFACE, in_signature='s', out_signature='s', utf8_strings=True)
    def AcceptUTF8String(self, foo):
        assert isinstance(foo, str), (foo, foo.__class__.__mro__)
        return foo

    @dbus.service.method(IFACE, in_signature='ay', out_signature='ay')
    def AcceptListOfByte(self, foo):
        assert isinstance(foo, list), (foo, foo.__class__.__mro__)
        return foo

    @dbus.service.method(IFACE, in_signature='ay', out_signature='ay', byte_arrays=True)
    def AcceptByteArray(self, foo):
        assert isinstance(foo, str), (foo, foo.__class__.__mro__)
        return foo

    @dbus.service.method(IFACE)
    def GetComplexArray(self):
        ret = []
        for i in range(0,100):
            ret.append((random.randint(0,100), random.randint(0,100), str(random.randint(0,100))))

        return dbus.Array(ret, signature="(uus)")

    def returnValue(self, test):
        if test == 0:
            return ""
        elif test == 1:
            return "",""
        elif test == 2:
            return "","",""
        elif test == 3:
            return []
        elif test == 4:
            return {}
        elif test == 5:
            return ["",""]
        elif test == 6:
            return ["","",""]

    @dbus.service.method(IFACE, in_signature='u', out_signature='s')
    def ReturnOneString(self, test):
        return self.returnValue(test)

    @dbus.service.method(IFACE, in_signature='u', out_signature='ss')
    def ReturnTwoStrings(self, test):
        return self.returnValue(test)

    @dbus.service.method(IFACE, in_signature='u', out_signature='(ss)')
    def ReturnStruct(self, test):
        logger.info('ReturnStruct(%r) -> %r', test, self.returnValue(test))
        return self.returnValue(test)

    @dbus.service.method(IFACE, in_signature='u', out_signature='as')
    def ReturnArray(self, test):
        return self.returnValue(test)

    @dbus.service.method(IFACE, in_signature='u', out_signature='a{ss}')
    def ReturnDict(self, test):
        return self.returnValue(test)

    @dbus.service.signal(IFACE, signature='s')
    def SignalOneString(self, test):
        logger.info('SignalOneString(%r)', test)

    @dbus.service.signal(IFACE, signature='ss')
    def SignalTwoStrings(self, test, test2):
        logger.info('SignalTwoStrings(%r, %r)', test, test2)

    @dbus.service.signal(IFACE, signature='(ss)')
    def SignalStruct(self, test):
        logger.info('SignalStruct(%r)', test)

    @dbus.service.signal(IFACE, signature='as')
    def SignalArray(self, test):
        pass

    @dbus.service.signal(IFACE, signature='a{ss}')
    def SignalDict(self, test):
        pass

    @dbus.service.method(IFACE, in_signature='su', out_signature='')
    def EmitSignal(self, signal, value):
        sig = getattr(self, str(signal), None)
        assert(sig != None)

        val = self.returnValue(value)
        # make two string case work by passing arguments in by tuple
        if (signal == 'SignalTwoStrings' and (value == 1 or value == 5)):
            val = tuple(val)
        else:
            val = tuple([val])

        logger.info('Emitting %s with %r', signal, val)
        sig(*val)

    def CheckInheritance(self):
        return True

    @dbus.service.method(IFACE, in_signature='bbv', out_signature='v', async_callbacks=('return_cb', 'error_cb'))
    def AsynchronousMethod(self, async, fail, variant, return_cb, error_cb):
        try:
            if async:
                gobject.timeout_add(500, self.AsynchronousMethod, False, fail, variant, return_cb, error_cb)
                return
            else:
                if fail:
                    raise RuntimeError
                else:
                    return_cb(variant)

                return False # do not run again
        except Exception, e:
            error_cb(e)

    @dbus.service.method(IFACE, in_signature='', out_signature='s', sender_keyword='sender')
    def WhoAmI(self, sender):
        return sender

session_bus = dbus.SessionBus()
name = dbus.service.BusName("org.freedesktop.DBus.TestSuitePythonService", bus=session_bus)
object = TestObject(name)
loop = gobject.MainLoop()
loop.run()
