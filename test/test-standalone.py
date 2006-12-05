#!/usr/bin/env python

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

import sys
import os
import unittest
import time
from traceback import print_exc

builddir = os.environ["DBUS_TOP_BUILDDIR"]
pydir = builddir

sys.path.insert(0, pydir)
sys.path.insert(0, pydir + 'dbus')

import _dbus_bindings
import dbus
import dbus.types as types

pkg = dbus.__file__
if not pkg.startswith(pydir):
    raise Exception("DBus modules (%s) are not being picked up from the package"%pkg)

if not _dbus_bindings.__file__.startswith(pydir):
    raise Exception("DBus modules (%s) are not being picked up from the package"%_dbus_bindings.__file__)

class TestTypes(unittest.TestCase):

    def test_Dictionary(self):
        self.assertEquals(types.Dictionary({'foo':'bar'}), {'foo':'bar'})
        self.assertEquals(types.Dictionary({}, variant_level=2), {})
        self.assertEquals(types.Dictionary({}, variant_level=2).variant_level, 2)

    def test_Array(self):
        self.assertEquals(types.Array(['foo','bar']), ['foo','bar'])
        self.assertEquals(types.Array([], variant_level=2), [])
        self.assertEquals(types.Array([], variant_level=2).variant_level, 2)

    def test_Double(self):
        self.assertEquals(types.Double(0.0), 0.0)
        self.assertEquals(types.Double(0.125, variant_level=2), 0.125)
        self.assertEquals(types.Double(0.125, variant_level=2).variant_level, 2)

    def test_Struct(self):
        x = types.Struct(('',))
        self.assertEquals(x.variant_level, 0)
        self.assertEquals(x, ('',))
        x = types.Struct('abc', variant_level=42)
        self.assertEquals(x.variant_level, 42)
        self.assertEquals(x, ('a','b','c'))

    def test_Byte(self):
        self.assertEquals(types.Byte('x', variant_level=2), types.Byte(ord('x')))

    def test_integers(self):
        # This is an API guarantee. Note that exactly which of these types
        # are ints and which of them are longs is *not* guaranteed.
        for cls in (types.Int16, types.UInt16, types.Int32, types.UInt32,
            types.Int64, types.UInt64):
            self.assert_(issubclass(cls, (int, long)))
            self.assert_(isinstance(cls(0), (int, long)))
            self.assertEquals(cls(0), 0)
            self.assertEquals(cls(23, variant_level=1), 23)
            self.assertEquals(cls(23, variant_level=1).variant_level, 1)

    def test_integer_limits_16(self):
        self.assertEquals(types.Int16(0x7fff), 0x7fff)
        self.assertEquals(types.Int16(-0x8000), -0x8000)
        self.assertEquals(types.UInt16(0xffff), 0xffff)
        self.assertRaises(Exception, types.Int16, 0x8000)
        self.assertRaises(Exception, types.Int16, -0x8001)
        self.assertRaises(Exception, types.UInt16, 0x10000)

    def test_integer_limits_32(self):
        self.assertEquals(types.Int32(0x7fffffff), 0x7fffffff)
        self.assertEquals(types.Int32(-0x80000000L), -0x80000000L)
        self.assertEquals(types.UInt32(0xffffffffL), 0xffffffffL)
        self.assertRaises(Exception, types.Int32, 0x80000000L)
        self.assertRaises(Exception, types.Int32, -0x80000001L)
        self.assertRaises(Exception, types.UInt32, 0x100000000L)

    def test_integer_limits_64(self):
        self.assertEquals(types.Int64(0x7fffffffffffffffL), 0x7fffffffffffffffL)
        self.assertEquals(types.Int64(-0x8000000000000000L), -0x8000000000000000L)
        self.assertEquals(types.UInt64(0xffffffffffffffffL), 0xffffffffffffffffL)
        self.assertRaises(Exception, types.Int16, 0x8000000000000000L)
        self.assertRaises(Exception, types.Int16, -0x8000000000000001L)
        self.assertRaises(Exception, types.UInt16, 0x10000000000000000L)

    def test_Signature(self):
        self.assertRaises(Exception, types.Signature, 'a')
        self.assertEquals(types.Signature('ab', variant_level=23), 'ab')
        self.assert_(isinstance(types.Signature('ab'), str))
        self.assertEquals(tuple(types.Signature('ab(xt)a{sv}')),
                          ('ab', '(xt)', 'a{sv}'))
        self.assert_(isinstance(tuple(types.Signature('ab'))[0],
                                types.Signature))


class TestMessageMarshalling(unittest.TestCase):

    def test_append(self):
        aeq = self.assertEquals
        from _dbus_bindings import SignalMessage
        s = SignalMessage('/', 'foo.bar', 'baz')
        s.append([types.Byte(1)], signature='ay')
        aeq(s.get_signature(), 'ay')
        aeq(s.get_args_list(), [[types.Byte(1)]])

        s = SignalMessage('/', 'foo.bar', 'baz')
        s.append([], signature='ay')
        aeq(s.get_args_list(), [[]])

    def test_append_ByteArray(self):
        aeq = self.assertEquals
        from _dbus_bindings import SignalMessage
        s = SignalMessage('/', 'foo.bar', 'baz')
        s.append(types.ByteArray('ab'), signature='ay')
        aeq(s.get_args_list(), [[types.Byte('a'), types.Byte('b')]])
        s = SignalMessage('/', 'foo.bar', 'baz')
        s.append(types.ByteArray('ab'), signature='av')
        aeq(s.get_args_list(), [[types.Byte('a'), types.Byte('b')]])

    def test_append_Variant(self):
        a = self.assert_
        aeq = self.assertEquals
        from _dbus_bindings import SignalMessage
        s = SignalMessage('/', 'foo.bar', 'baz')
        s.append(types.Int32(1, variant_level=0),
                 types.String('a', variant_level=42),
                 types.Array([types.Byte('a', variant_level=1),
                              types.UInt32(123, variant_level=1)],
                             signature='v'),
                 signature='vvv')
        aeq(s.get_signature(), 'vvv')
        args = s.get_args_list()
        aeq(args[0].__class__, types.Int32)
        aeq(args[0].variant_level, 1)
        aeq(args[1].__class__, types.String)
        aeq(args[1].variant_level, 42)
        aeq(args[2].__class__, types.Array)
        aeq(args[2].variant_level, 1)
        aeq(args[2].signature, 'v')

    def test_guess_signature(self):
        aeq = self.assertEquals
        from _dbus_bindings import Message
        aeq(Message.guess_signature(('a','b')), '(ss)')
        aeq(Message.guess_signature('a','b'), 'ss')
        aeq(Message.guess_signature(['a','b']), 'as')
        aeq(Message.guess_signature(('a',)), '(s)')
        aeq(Message.guess_signature('abc'), 's')
        aeq(Message.guess_signature(types.Int32(123)), 'i')
        aeq(Message.guess_signature(('a',)), '(s)')
        aeq(Message.guess_signature(['a']), 'as')
        aeq(Message.guess_signature({'a':'b'}), 'a{ss}')

    def test_guess_signature_dbus_types(self):
        aeq = self.assertEquals
        from _dbus_bindings import Message
        gs = Message.guess_signature
        aeq(gs(types.Dictionary({'a':'b'})), 'a{ss}')
        aeq(gs(types.Dictionary({'a':'b'}, signature='sv')), 'a{sv}')
        aeq(gs(types.Dictionary({}, signature='iu')), 'a{iu}')
        aeq(gs(types.Array([types.Int32(1)])), 'ai')
        aeq(gs(types.Array([types.Int32(1)], signature='u')), 'au')

    def test_get_args_options(self):
        aeq = self.assertEquals
        s = _dbus_bindings.SignalMessage('/', 'foo.bar', 'baz')
        s.append('b', 'bytes', -1, 1, 'str', 'var', signature='yayiusv')
        aeq(s.get_args_list(), [ord('b'),
                                [ord('b'),ord('y'),ord('t'),ord('e'), ord('s')],
                                -1, 1, u'str', u'var'])
        byte, bytes, int32, uint32, string, variant = s.get_args_list()
        aeq(byte.__class__, types.Byte)
        aeq(bytes.__class__, types.Array)
        aeq(bytes[0].__class__, types.Byte)
        aeq(int32.__class__, types.Int32)
        aeq(uint32.__class__, types.UInt32)
        aeq(string.__class__, types.String)
        aeq(string.variant_level, 0)
        aeq(variant.__class__, types.String)
        aeq(variant.variant_level, 1)

        byte, bytes, int32, uint32, string, variant = s.get_args_list(
                byte_arrays=True)
        aeq(byte.__class__, types.Byte)
        aeq(bytes.__class__, types.ByteArray)
        aeq(bytes, 'bytes')
        aeq(bytes[0].__class__, types.Byte)
        aeq(int32.__class__, types.Int32)
        aeq(uint32.__class__, types.UInt32)
        aeq(string.__class__, types.String)
        aeq(variant.__class__, types.String)
        aeq(variant.variant_level, 1)

        byte, bytes, int32, uint32, string, variant = s.get_args_list(
            utf8_strings=True)
        aeq(byte.__class__, types.Byte)
        aeq(bytes.__class__, types.Array)
        aeq(bytes[0].__class__, types.Byte)
        aeq(int32.__class__, types.Int32)
        aeq(uint32.__class__, types.UInt32)
        aeq(string.__class__, types.UTF8String)
        aeq(string, 'str')
        aeq(variant.__class__, types.UTF8String)
        aeq(variant.variant_level, 1)
        aeq(variant, 'var')


if __name__ == '__main__':
    unittest.main()
