#!/usr/bin/env python

from dbus.mainloop.glib import DBusGMainLoop

import dbus
import dbus.bus
import dbus.service
import dbus.server

import gobject
import os, sys

class TestService(dbus.service.Object):
    NAME = 'org.freedesktop.dbus.TestService'
    PATH = '/org/freedesktop/dbus/TestService'

    def __init__(self, conn, path=PATH):
        super(TestService, self).__init__(conn, path)

    @dbus.service.method(dbus_interface=NAME,
                         out_signature='s',
                         in_signature='s')
    def reverse(self, text):
        text = list(text)
        text.reverse()

        return ''.join(text)

class TestServer(dbus.server.Server):
    def __init__(self, *args, **kwargs):
        super(TestServer, self).__init__(*args, **kwargs)
        self.__connections = list()

    def _on_new_connection(self, conn):
        print 'new connection: %r' % conn
        self.__connections.append(conn)
        TestService(conn)

pin, pout = os.pipe()
child = os.fork()

if 0 == child:
    DBusGMainLoop(set_as_default=True)
    server = TestServer('unix:tmpdir=/tmp')

    os.write(pout, server.address)
    os.close(pout)
    os.close(pin)

    print 'server running: %s' % server.address
    gobject.MainLoop().run()

    print 'server quit'

    print 'done?'

else:
    os.waitpid(child, os.WNOHANG)
    os.close(pout)

    address = os.read(pin, 128)
    os.close(pin)

    client = dbus.bus.Connection(address)
    proxy = client.get_object(TestService.NAME, TestService.PATH)
    object = dbus.Interface(proxy, TestService.NAME)

    while True:
        line = sys.stdin.readline()
        if not line: break

        text = line.strip()
        print 'reverse(%s): %s' % (text, object.reverse(text))

