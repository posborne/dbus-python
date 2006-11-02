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

    def test_integers(self):
        # This is an API guarantee. Note that exactly which of these types
        # are ints and which of them are longs is *not* guaranteed.
        for cls in (types.Int16, types.UInt16, types.Int32, types.UInt32,
            types.Int64, types.UInt64):
            assert issubclass(cls, (int, long))
            assert isinstance(cls(0), (int, long))

    def test_Variant(self):
        Variant = types.Variant
        a = self.assert_
        a(Variant(1, 'i') == Variant(1, 'i'))
        a(not (Variant(1, 'i') == Variant(1, 'u')))
        a(not (Variant(1, 'i') == Variant(2, 'i')))
        a(not (Variant(1, 'i') == Variant(2, 'u')))
        a(not (Variant(1, 'i') != Variant(1, 'i')))
        a(Variant(1, 'i') != Variant(1, 'u'))
        a(Variant(1, 'i') != Variant(2, 'i'))
        a(Variant(1, 'i') != Variant(2, 'u'))

    def test_Signature(self):
        self.assertRaises(Exception, types.Signature, 'a')
        self.assertEquals(types.Signature('ab'), 'ab')
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
        aeq(s.get_args_list(), [[types.Variant(types.Byte('a')),
                                 types.Variant(types.Byte('b'))]])

    def test_append_Variant(self):
        a = self.assert_
        aeq = self.assertEquals
        from _dbus_bindings import SignalMessage
        s = SignalMessage('/', 'foo.bar', 'baz')
        s.append(types.Variant(1, signature='i'),
                 types.Variant('a', signature='s'),
                 types.Variant([(types.Variant('a', signature='y'), 'b'),
                                (types.Variant(123, signature='u'), 1)],
                               signature='a(vy)'))
        aeq(s.get_signature(), 'vvv')
        args = s.get_args_list()
        aeq(args[0].__class__, types.Variant)
        aeq(args[0].signature, 'i')
        aeq(args[0].object.__class__, types.Int32)
        aeq(args[0].object, 1)
        aeq(args[1].__class__, types.Variant)
        aeq(args[1].signature, 's')
        a(isinstance(args[1].object, unicode))
        aeq(args[2].__class__, types.Variant)
        aeq(args[1].object, 'a')
        aeq(args[2].signature, 'a(vy)')
        avy = args[2].object
        aeq(avy.__class__, types.Array)
        aeq(len(avy), 2)
        aeq(avy[0].__class__, tuple)
        aeq(len(avy[0]), 2)
        aeq(avy[0][0].__class__, types.Variant)
        aeq(avy[0][0].signature, 'y')
        aeq(avy[0][0].object.__class__, types.Byte)
        aeq(avy[0][0].object, types.Byte('a'))
        aeq(avy[0][1].__class__, types.Byte)
        aeq(avy[0][1], types.Byte('b'))
        aeq(avy[1].__class__, tuple)
        aeq(len(avy[1]), 2)
        aeq(avy[1][0].__class__, types.Variant)
        aeq(avy[1][0].signature, 'u')
        aeq(avy[1][0].object.__class__, types.UInt32)
        aeq(avy[1][0].object, 123)
        aeq(avy[1][1].__class__, types.Byte)
        aeq(avy[1][1], types.Byte(1))

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

    def test_get_args_options(self):
        aeq = self.assertEquals
        s = _dbus_bindings.SignalMessage('/', 'foo.bar', 'baz')
        s.append('b', 'bytes', -1, 1, 'str', 'var', signature='yayiusv')
        aeq(s.get_args_list(), [ord('b'),
                                [ord('b'),ord('y'),ord('t'),ord('e'), ord('s')],
                                -1, 1, u'str', types.Variant(u'var', signature='s')])
        byte, bytes, int32, uint32, string, variant = s.get_args_list()
        aeq(byte.__class__, types.Byte)
        aeq(bytes.__class__, types.Array)
        aeq(bytes[0].__class__, types.Byte)
        aeq(int32.__class__, types.Int32)
        aeq(uint32.__class__, types.UInt32)
        aeq(string.__class__, unicode)
        aeq(variant.__class__, types.Variant)
        aeq(variant.signature, 's')

        byte, bytes, int32, uint32, string, variant = s.get_args_list(
                integer_bytes=True)
        aeq(byte.__class__, int)
        aeq(byte, ord('b'))
        aeq(bytes.__class__, types.Array)
        aeq(bytes[0].__class__, int)
        aeq(bytes, [ord('b'), ord('y'), ord('t'), ord('e'), ord('s')])
        aeq(int32.__class__, types.Int32)
        aeq(uint32.__class__, types.UInt32)
        aeq(string.__class__, unicode)
        aeq(variant.__class__, types.Variant)
        aeq(variant.signature, 's')

        byte, bytes, int32, uint32, string, variant = s.get_args_list(
                byte_arrays=True)
        aeq(byte.__class__, types.Byte)
        aeq(bytes.__class__, types.ByteArray)
        aeq(bytes, 'bytes')
        aeq(bytes[0].__class__, types.Byte)
        aeq(int32.__class__, types.Int32)
        aeq(uint32.__class__, types.UInt32)
        aeq(string.__class__, unicode)
        aeq(variant.__class__, types.Variant)
        aeq(variant.signature, 's')

        byte, bytes, int32, uint32, string, variant = s.get_args_list(
            utf8_strings=True)
        aeq(byte.__class__, types.Byte)
        aeq(bytes.__class__, types.Array)
        aeq(bytes[0].__class__, types.Byte)
        aeq(int32.__class__, types.Int32)
        aeq(uint32.__class__, types.UInt32)
        aeq(string.__class__, str)
        aeq(string, 'str')
        aeq(variant.__class__, types.Variant)
        aeq(variant.signature, 's')
        aeq(variant.object.__class__, str)
        aeq(variant.object, 'var')

        byte, bytes, int32, uint32, string, variant = s.get_args_list(
            variant_unpack_level=1)
        aeq(byte.__class__, types.Byte)
        aeq(bytes.__class__, types.Array)
        aeq(bytes[0].__class__, types.Byte)
        aeq(int32.__class__, types.Int32)
        aeq(uint32.__class__, types.UInt32)
        aeq(string.__class__, unicode)
        aeq(variant.__class__, unicode)
        aeq(variant, 'var')


if __name__ == '__main__':
    unittest.main()
