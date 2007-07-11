"""D-Bus data types.

The module isn't called "types" due to the name clash with the top-level
types module, which is problematic in Scratchbox.
"""

# Copyright (C) 2007 Collabora Ltd. <http://www.collabora.co.uk/>
#
# Licensed under the Academic Free License version 2.1
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation; either version 2.1 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

__all__ = ('ObjectPath', 'ByteArray', 'Signature', 'Byte', 'Boolean',
           'Int16', 'UInt16', 'Int32', 'UInt32', 'Int64', 'UInt64',
           'Double', 'String', 'Array', 'Struct', 'Dictionary',
           'UTF8String')


from weakref import ref

from _dbus_bindings import ObjectPath, Signature, \
                           Int16, UInt16, Int32, UInt32,\
                           Int64, UInt64, Dictionary, Array, \
                           String, Boolean, Double, Struct, UTF8String


class _DBusTypeMixin(object):
    # Base class for D-Bus data types. Must be mixed-in with a class that
    # allows a _dbus_variant_level attribute.
    #
    # Do not use this class, or assume that it exists, outside dbus-python.

    # Slightly odd implementation which deserves a comment:
    #
    # Some of the types we want to subtype have a layout which
    # means we can't use __slots__ and must give them a __dict__. Others
    # are fine for __slots__. So, we have to make the decision per-subclass.
    #
    # Omitting __slots__ here would give all subtypes a __dict__, which would
    # be a waste of memory for the ones that don't need it (all mutable types,
    # plus unicode). So we give it no slots, and add either a slot or a
    # __dict__ (as appropriate) in the subclasses.

    __slots__ = ()

    def __new__(cls, value, variant_level=0):
        if variant_level < 0:
            raise ValueError('variant_level must be non-negative')
        self = super(_DBusTypeMixin, cls).__new__(cls, value)
        self._dbus_variant_level = variant_level
        return self

    @property
    def variant_level(self):
        """Indicates how many nested Variant containers this object is
        contained in: if a message's wire format has a variant containing
        a variant containing an `Int32`, this is represented in Python by an
        `Int32` object with ``variant_level == 2``.
        """
        return self._dbus_variant_level

    def __repr__(self):
        variant_level = self._dbus_variant_level
        parent_repr = super(_DBusTypeMixin, self).__repr__()
        if variant_level:
            return '%s(%s, variant_level=%d)' % (self.__class__.__name__,
                                                 parent_repr, variant_level)
        else:
            return '%s(%s)' % (self.__class__.__name__, parent_repr)


class ByteArray(_DBusTypeMixin, str):
    """ByteArray is a subtype of str which can be used when you want an
    efficient immutable representation of a D-Bus byte array (signature 'ay').

    By default, when byte arrays are converted from D-Bus to Python, they
    come out as a `dbus.Array` of `dbus.Byte`. This is just for symmetry with
    the other D-Bus types - in practice, what you usually want is the byte
    array represented as a string, using this class. To get this, pass the
    ``byte_arrays=True`` keyword argument to any of these methods:

    * any D-Bus method proxy, or ``connect_to_signal``, on the objects returned
      by `Bus.get_object`
    * any D-Bus method on a `dbus.Interface`
    * `dbus.Interface.connect_to_signal`
    * `Bus.add_signal_receiver`

    Import via::

       from dbus import ByteArray

    Constructor::

       ByteArray(value: str)
    """


class Byte(_DBusTypeMixin, int):
    """An unsigned byte: a subtype of int, with range restricted to [0, 255].

    A Byte b may be converted to a str of length 1 via str(b) == chr(b).

    Most of the time you don't want to use this class - it mainly exists
    for symmetry with the other D-Bus types. See `dbus.ByteArray` for a
    better way to handle arrays of Byte.

    Constructor::

       dbus.Byte(value: integer or str of length 1[, variant_level: integer])
    """

    def __new__(cls, value, variant_level=0):
        if isinstance(value, str):
            if len(value) != 1:
                raise TypeError('A string argument to Byte.__new__() must '
                                'be a single byte, not %r' % value)
            value = ord(value)
        elif not isinstance(value, (int, long)):
            raise TypeError('Argument to Byte.__new__() must be a str, int '
                            'or long, not %r' % value)

        if value < 0 or value > 255:
            raise TypeError('Argument %r to Byte.__new__() not in range(256)'
                            % value)

        return super(Byte, cls).__new__(cls, value)

    def __str__(self):
        return chr(self)
