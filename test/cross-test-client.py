# Copyright (C) 2006 Collabora Ltd. <http://www.collabora.co.uk/>
#
# Licensed under the Academic Free License version 2.1
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
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

from sets import Set
from time import sleep
import logging

import gobject

from dbus import SessionBus, Interface, Array, Byte, Double, Variant, Boolean, ByteArray, Int16, Int32, Int64, UInt16, UInt32, UInt64
from dbus.service import BusName
import dbus.glib

from crosstest import CROSS_TEST_PATH, CROSS_TEST_BUS_NAME,\
                      INTERFACE_SINGLE_TESTS, INTERFACE_TESTS,\
                      INTERFACE_SIGNAL_TESTS, INTERFACE_CALLBACK_TESTS,\
                      SignalTestsImpl


logging.basicConfig()
logger = logging.getLogger('cross-test-client')


class ListInAnyOrder(list):
    def __eq__(self, other):
        return sorted(other) == sorted(self)
    def __ne__(self, other):
        return sorted(other) != sorted(self)


class Client(SignalTestsImpl):
    fail_id = 0
    expected = Set()

    def quit(self):
        for x in self.expected:
            self.fail_id += 1
            print "%s fail %d" % (x, self.fail_id)
            s = "report %d: reply to %s didn't arrive" % (self.fail_id, x)
            print s
            logger.error(s)
        logger.info("asking server to Exit")
        Interface(self.obj, INTERFACE_TESTS).Exit(reply_handler=self.quit_reply_handler, error_handler=self.quit_error_handler)
        # if the server doesn't reply we'll just exit anyway
        gobject.timeout_add(1000, lambda: (loop.quit(), False)[1])

    def quit_reply_handler(self):
        logger.info("server says it will exit")
        loop.quit()

    def quit_error_handler(self, e):
        logger.error("error telling server to quit: %s %s",
                     e.__class__, e)
        loop.quit()

    @dbus.service.method(INTERFACE_CALLBACK_TESTS, 'qd')
    def Response(self, input1, input2):
        logger.info("signal/callback: Response received (%r,%r)",
                    input1, input2)
        self.expected.discard('%s.Trigger' % INTERFACE_SIGNAL_TESTS)
        if (input1, input2) != (42, 23):
            self.fail_id += 1
            print "%s.Trigger fail %d" % (INTERFACE_SIGNAL_TESTS, self.fail_id)
            s = ("report %d: expected (42,23), got %r"
                 % (self.fail_id, (input1, input2)))
            logger.error(s)
            print s
        else:
            print "%s.Trigger pass" % INTERFACE_SIGNAL_TESTS
        self.quit()

    def assert_method_eq(self, interface, ret, member, *args):
        if_obj = Interface(self.obj, interface)
        method = getattr(if_obj, member)
        try:
            real_ret = method(*args)
        except Exception, e:
            self.fail_id += 1
            print "%s.%s fail %d" % (interface, member, self.fail_id)
            s = ("report %d: %s.%s%r: expected %r, raised %r \"%s\""
                 % (self.fail_id, interface, member, args, ret, e, e))
            print s
            logger.error(s)
            __import__('traceback').print_exc()
            return
        if real_ret != ret:
            self.fail_id += 1
            print "%s.%s fail %d" % (interface, member, self.fail_id)
            s = ("report %d: %s.%s%r: expected %r, got %r"
                 % (self.fail_id, interface, member, args, ret, real_ret))
            print s
            logger.error(s)
            return
        print "%s.%s pass" % (interface, member)


    def triggered_cb(self, param, sender_path):
        logger.info("method/signal: Triggered(%r) by %r",
                    param, sender_path)
        self.expected.discard('%s.Trigger' % INTERFACE_TESTS)
        if sender_path != '/Where/Ever':
            self.fail_id += 1
            print "%s.Trigger fail %d" % (INTERFACE_TESTS, self.fail_id)
            s = ("report %d: expected signal from /Where/Ever, got %r"
                 % (self.fail_id, sender_path))
            print s
            logger.error(s)
        elif param != 42:
            self.fail_id += 1
            print "%s.Trigger fail %d" % (INTERFACE_TESTS, self.fail_id)
            s = ("report %d: expected signal param 42, got %r"
                 % (self.fail_id, parameter))
            print s
            logger.error(s)
        else:
            print "%s.Trigger pass" % INTERFACE_TESTS

    def trigger_returned_cb(self):
        logger.info('method/signal: Trigger() returned')
        # Callback tests
        logger.info("signal/callback: Emitting signal to trigger callback")
        self.expected.add('%s.Trigger' % INTERFACE_SIGNAL_TESTS)
        self.Trigger(42, 23)
        logger.info("signal/callback: Emitting signal returned")

    def run_client(self):
        bus = SessionBus()
        obj = bus.get_object(CROSS_TEST_BUS_NAME, CROSS_TEST_PATH)
        self.obj = obj

        self.run_synchronous_tests(obj)

        # Signal tests
        logger.info("Binding signal handler for Triggered")
        # FIXME: doesn't seem to work when going via the Interface method
        # FIXME: should be possible to ask the proxy object for its
        # bus name
        bus.add_signal_receiver(self.triggered_cb, 'Triggered',
                                INTERFACE_SIGNAL_TESTS,
                                CROSS_TEST_BUS_NAME,
                                path_keyword='sender_path')
        logger.info("method/signal: Triggering signal")
        self.expected.add('%s.Trigger' % INTERFACE_TESTS)
        Interface(obj, INTERFACE_TESTS).Trigger('/Where/Ever', 42, reply_handler=self.trigger_returned_cb, error_handler=self.trigger_error_handler)

    def trigger_error_handler(self, e):
        logger.error("method/signal: %s %s", e.__class__, e)
        Interface(self.obj, INTERFACE_TESTS).Exit()
        self.quit()

    def run_synchronous_tests(self, obj):
        # We can't test that coercion works correctly unless the server has
        # sent us introspection data. Java doesn't :-/
        have_signatures = False

        # "Single tests"
        if have_signatures:
            self.assert_method_eq(INTERFACE_SINGLE_TESTS, 6, 'Sum', [1, 2, 3])
            self.assert_method_eq(INTERFACE_SINGLE_TESTS, 6, 'Sum', ['\x01', '\x02', '\x03'])
        self.assert_method_eq(INTERFACE_SINGLE_TESTS, 6, 'Sum', [Byte(1), Byte(2), Byte(3)])
        self.assert_method_eq(INTERFACE_SINGLE_TESTS, 6, 'Sum', ByteArray('\x01\x02\x03'))

        # Main tests
        self.assert_method_eq(INTERFACE_TESTS, Variant(u'foo', 's'), 'Identity', Variant('foo'))
        self.assert_method_eq(INTERFACE_TESTS, Variant(Byte(42)), 'Identity', Variant(Byte(42)))
        self.assert_method_eq(INTERFACE_TESTS, Variant(Variant(Variant(Variant(Variant(Variant(Variant(Byte(42)))))))), 'Identity', Variant(Variant(Variant(Variant(Variant(Variant(Variant(Byte(42)))))))))
        self.assert_method_eq(INTERFACE_TESTS, Variant(42.5), 'Identity', Variant(Double(42.5)))
        self.assert_method_eq(INTERFACE_TESTS, Variant(-42.5), 'Identity', Variant(-42.5))

        if have_signatures:
            self.assert_method_eq(INTERFACE_TESTS, Variant(u'foo', 's'), 'Identity', 'foo')
            self.assert_method_eq(INTERFACE_TESTS, Variant(Byte(42)), 'Identity', Byte(42))
            self.assert_method_eq(INTERFACE_TESTS, Variant(42.5), 'Identity', Double(42.5))
            self.assert_method_eq(INTERFACE_TESTS, Variant(-42.5), 'Identity', -42.5)

        self.assert_method_eq(INTERFACE_TESTS, Byte(42), 'IdentityByte', Byte(42))
        self.assert_method_eq(INTERFACE_TESTS, True, 'IdentityBool', Boolean(42))

        self.assert_method_eq(INTERFACE_TESTS, 42, 'IdentityInt16', Int16(42))
        self.assert_method_eq(INTERFACE_TESTS, 42, 'IdentityUInt16', UInt16(42))
        self.assert_method_eq(INTERFACE_TESTS, 42, 'IdentityInt32', Int32(42))
        self.assert_method_eq(INTERFACE_TESTS, 42, 'IdentityUInt32', UInt32(42))
        self.assert_method_eq(INTERFACE_TESTS, 42, 'IdentityInt64', Int64(42))
        self.assert_method_eq(INTERFACE_TESTS, 42, 'IdentityUInt64', UInt64(42))
        self.assert_method_eq(INTERFACE_TESTS, 42.3, 'IdentityDouble', 42.3)
        self.assert_method_eq(INTERFACE_TESTS, u'\xa9', 'IdentityString', u'\xa9')
        self.assert_method_eq(INTERFACE_TESTS, u'foo', 'IdentityString', 'foo')

        if have_signatures:
            self.assert_method_eq(INTERFACE_TESTS, Byte(0x42), 'IdentityByte', '\x42')
            self.assert_method_eq(INTERFACE_TESTS, True, 'IdentityBool', 42)
            self.assert_method_eq(INTERFACE_TESTS, 42, 'IdentityInt16', 42)
            self.assert_method_eq(INTERFACE_TESTS, 42, 'IdentityUInt16', 42)
            self.assert_method_eq(INTERFACE_TESTS, 42, 'IdentityInt32', 42)
            self.assert_method_eq(INTERFACE_TESTS, 42, 'IdentityUInt32', 42)
            self.assert_method_eq(INTERFACE_TESTS, 42, 'IdentityInt64', 42)
            self.assert_method_eq(INTERFACE_TESTS, 42, 'IdentityUInt64', 42)
            self.assert_method_eq(INTERFACE_TESTS, 42.0, 'IdentityDouble', 42)

        self.assert_method_eq(INTERFACE_TESTS, [Variant(Byte('\x01')),Variant(Byte('\x02')),Variant(Byte('\x03'))], 'IdentityArray', [Variant(Byte('\x01')), Variant(Byte('\x02')), Variant(Byte('\x03'))])
        self.assert_method_eq(INTERFACE_TESTS, [Variant(Int32(1)),Variant(Int32(2)),Variant(Int32(3))], 'IdentityArray', [Variant(Int32(1)),Variant(Int32(2)),Variant(Int32(3))])
        self.assert_method_eq(INTERFACE_TESTS, [Variant(u'a'),Variant(u'b'),Variant(u'c')], 'IdentityArray', [Variant('a'),Variant('b'),Variant('c')])

        if have_signatures:
            self.assert_method_eq(INTERFACE_TESTS, [Variant(Byte('\x01')),Variant(Byte('\x02')),Variant(Byte('\x03'))], 'IdentityArray', ByteArray('\x01\x02\x03'))
            self.assert_method_eq(INTERFACE_TESTS, [Variant(Int32(1)),Variant(Int32(2)),Variant(Int32(3))], 'IdentityArray', [Int32(1),Int32(2),Int32(3)])
            self.assert_method_eq(INTERFACE_TESTS, [Variant(u'a'),Variant(u'b'),Variant(u'c')], 'IdentityArray', ['a','b','c'])

        self.assert_method_eq(INTERFACE_TESTS, ['\x01','\x02','\x03'], 'IdentityByteArray', ByteArray('\x01\x02\x03'))
        if have_signatures:
            self.assert_method_eq(INTERFACE_TESTS, ['\x01','\x02','\x03'], 'IdentityByteArray', ['\x01', '\x02', '\x03'])
        self.assert_method_eq(INTERFACE_TESTS, [False,True], 'IdentityBoolArray', [False,True])
        if have_signatures:
            self.assert_method_eq(INTERFACE_TESTS, [False,True,True], 'IdentityBoolArray', [0,1,2])

        self.assert_method_eq(INTERFACE_TESTS, [1,2,3], 'IdentityInt16Array', [Int16(1),Int16(2),Int16(3)])
        self.assert_method_eq(INTERFACE_TESTS, [1,2,3], 'IdentityUInt16Array', [UInt16(1),UInt16(2),UInt16(3)])
        self.assert_method_eq(INTERFACE_TESTS, [1,2,3], 'IdentityInt32Array', [Int32(1),Int32(2),Int32(3)])
        self.assert_method_eq(INTERFACE_TESTS, [1,2,3], 'IdentityUInt32Array', [UInt32(1),UInt32(2),UInt32(3)])
        self.assert_method_eq(INTERFACE_TESTS, [1,2,3], 'IdentityInt64Array', [Int64(1),Int64(2),Int64(3)])
        self.assert_method_eq(INTERFACE_TESTS, [1,2,3], 'IdentityUInt64Array', [UInt64(1),UInt64(2),UInt64(3)])

        if have_signatures:
            self.assert_method_eq(INTERFACE_TESTS, [1,2,3], 'IdentityInt16Array', [1,2,3])
            self.assert_method_eq(INTERFACE_TESTS, [1,2,3], 'IdentityUInt16Array', [1,2,3])
            self.assert_method_eq(INTERFACE_TESTS, [1,2,3], 'IdentityInt32Array', [1,2,3])
            self.assert_method_eq(INTERFACE_TESTS, [1,2,3], 'IdentityUInt32Array', [1,2,3])
            self.assert_method_eq(INTERFACE_TESTS, [1,2,3], 'IdentityInt64Array', [1,2,3])
            self.assert_method_eq(INTERFACE_TESTS, [1,2,3], 'IdentityUInt64Array', [1,2,3])

        self.assert_method_eq(INTERFACE_TESTS, [1.0,2.5,3.1], 'IdentityDoubleArray', [1.0,2.5,3.1])
        if have_signatures:
            self.assert_method_eq(INTERFACE_TESTS, [1.0,2.5,3.1], 'IdentityDoubleArray', [1,2.5,3.1])
        self.assert_method_eq(INTERFACE_TESTS, ['a','b','c'], 'IdentityStringArray', ['a','b','c'])
        self.assert_method_eq(INTERFACE_TESTS, 6, 'Sum', [Int32(1),Int32(2),Int32(3)])
        if have_signatures:
            self.assert_method_eq(INTERFACE_TESTS, 6, 'Sum', [1,2,3])
        self.assert_method_eq(INTERFACE_TESTS, {'fps': ListInAnyOrder(['unreal', 'quake']), 'rts': ['warcraft']}, 'InvertMapping', {'unreal': 'fps', 'quake': 'fps', 'warcraft': 'rts'})

        self.assert_method_eq(INTERFACE_TESTS, ('a', 1, 2), 'DeStruct', ('a', UInt32(1), Int16(2)))
        self.assert_method_eq(INTERFACE_TESTS, [Variant('x')], 'Primitize', [Variant(Variant(['x']))])
        self.assert_method_eq(INTERFACE_TESTS, [Variant('x'), Variant(Byte(1)), Variant(Byte(2))], 'Primitize', [Variant([Variant(['x']), Array([Byte(1), Byte(2)])])])
        self.assert_method_eq(INTERFACE_TESTS, False, 'Invert', 42)
        self.assert_method_eq(INTERFACE_TESTS, True, 'Invert', 0)


if __name__ == '__main__':
    # FIXME: should be possible to export objects without a bus name
    bus_name = BusName('com.example.Argh')
    client = Client(bus_name, '/Client')
    gobject.idle_add(client.run_client)

    loop = gobject.MainLoop()
    logger.info("running...")
    loop.run()
    logger.info("main loop exited.")
