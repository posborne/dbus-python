# Shared code for the cross-test.

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
