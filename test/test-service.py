#!/usr/bin/env python
import sys
import os

builddir = os.environ["DBUS_TOP_BUILDDIR"]
pydir = builddir

sys.path.insert(0, pydir)
sys.path.insert(0, pydir + 'dbus')

import dbus

if not dbus.__file__.startswith(pydir):
    os.system("echo %s> /tmp/dbus.log"%pydir)
    raise Exception("DBus modules are not being picked up from the package")

import dbus.service
import dbus.glib
import gobject
import random

class TestInterface(dbus.service.Interface):
    @dbus.service.method("org.freedesktop.DBus.TestSuiteInterface", in_signature='', out_signature='b')
    def CheckInheritance(self):
        return False

class TestObject(dbus.service.Object, TestInterface):
    def __init__(self, bus_name, object_path="/org/freedesktop/DBus/TestSuitePythonObject"):
        dbus.service.Object.__init__(self, bus_name, object_path)

    """ Echo whatever is sent
    """
    @dbus.service.method("org.freedesktop.DBus.TestSuiteInterface")
    def Echo(self, arg):
        return arg

    @dbus.service.method("org.freedesktop.DBus.TestSuiteInterface")
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

    @dbus.service.method("org.freedesktop.DBus.TestSuiteInterface", in_signature='u', out_signature='s')
    def ReturnOneString(self, test):
        return self.returnValue(test)

    @dbus.service.method("org.freedesktop.DBus.TestSuiteInterface", in_signature='u', out_signature='ss')
    def ReturnTwoStrings(self, test):
        return self.returnValue(test)

    @dbus.service.method("org.freedesktop.DBus.TestSuiteInterface", in_signature='u', out_signature='(ss)')
    def ReturnStruct(self, test):
        return self.returnValue(test)

    @dbus.service.method("org.freedesktop.DBus.TestSuiteInterface", in_signature='u', out_signature='as')
    def ReturnArray(self, test):
        return self.returnValue(test)

    @dbus.service.method("org.freedesktop.DBus.TestSuiteInterface", in_signature='u', out_signature='a{ss}')
    def ReturnDict(self, test):
        return self.returnValue(test)

    @dbus.service.signal("org.freedesktop.DBus.TestSuiteInterface", signature='s')
    def SignalOneString(self, test):
        pass

    @dbus.service.signal("org.freedesktop.DBus.TestSuiteInterface", signature='ss')
    def SignalTwoStrings(self, test, test2):
        pass

    @dbus.service.signal("org.freedesktop.DBus.TestSuiteInterface", signature='(ss)')
    def SignalStruct(self, test):
        pass

    @dbus.service.signal("org.freedesktop.DBus.TestSuiteInterface", signature='as')
    def SignalArray(self, test):
        pass

    @dbus.service.signal("org.freedesktop.DBus.TestSuiteInterface", signature='a{ss}')
    def SignalDict(self, test):
        pass

    @dbus.service.method("org.freedesktop.DBus.TestSuiteInterface", in_signature='su', out_signature='')
    def EmitSignal(self, signal, value):
        sig = getattr(self, signal, None)
        assert(sig != None)

        val = self.returnValue(value)
        # make two string case work by passing arguments in by tuple
        if (signal == 'SignalTwoStrings' and (value == 1 or value == 5)):
            val = tuple(val)
        else:
            val = tuple([val])

        sig(*val)

    def CheckInheritance(self):
        return True

    @dbus.service.method('org.freedesktop.DBus.TestSuiteInterface', in_signature='bbv', out_signature='v', async_callbacks=('return_cb', 'error_cb'))
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

    @dbus.service.method('org.freedesktop.DBus.TestSuiteInterface', in_signature='', out_signature='s', sender_keyword='sender')
    def WhoAmI(self, sender):
        return sender

session_bus = dbus.SessionBus()
name = dbus.service.BusName("org.freedesktop.DBus.TestSuitePythonService", bus=session_bus)
object = TestObject(name)
loop = gobject.MainLoop()
loop.run()
