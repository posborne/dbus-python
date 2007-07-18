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

from sys import maxint

from _dbus_bindings import Signature, \
                           Dictionary, \
                           Struct

from _dbus_bindings import validate_object_path


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

    def __new__(cls, value=0, variant_level=0):
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

        return super(Byte, cls).__new__(cls, value, variant_level)

    def __str__(self):
        return chr(self)


class String(_DBusTypeMixin, unicode):
    """A human-readable string: a subtype of `unicode`, with U+0000 disallowed.

    All strings on D-Bus are required to be valid Unicode; in the "wire
    protocol" they're transported as UTF-8.

    By default, when strings are converted from D-Bus to Python, they come
    out as this class. If you prefer to get UTF-8 strings (as instances of
    a subtype of `str`) or you want to avoid the conversion overhead of
    going from UTF-8 to Python's internal Unicode representation, see the
    documentation for `dbus.UTF8String`.
    """
    __slots__ = ('_dbus_variant_level',)

    def __new__(cls, value=u'', variant_level=0):

        if isinstance(value, str):
            try:
                value = value.decode('utf8')
            except UnicodeError, e:
                raise UnicodeError('A str argument to String.__new__ must be '
                                   'UTF-8: %s', e)
        elif not isinstance(value, unicode):
            raise TypeError('String.__new__ requires a unicode or UTF-8 str, '
                            'not %r' % value)

        if u'\0' in value:
            raise TypeError(r'D-Bus strings cannot contain u"\0", but %r does'
                            % value)

        return super(String, cls).__new__(cls, value, variant_level)


class UTF8String(_DBusTypeMixin, str):
    r"""A human-readable string represented as UTF-8; a subtype of `str`,
    with '\0' disallowed.

    By default, when byte arrays are converted from D-Bus to Python, they
    come out as a `dbus.String`, which is a subtype of `unicode`.
    If you prefer to get UTF-8 strings (as instances of this class) or you
    want to avoid the conversion overhead of going from UTF-8 to Python's
    internal Unicode representation, you can pass the ``utf8_strings=True``
    keyword argument to any of these methods:

    * any D-Bus method proxy, or ``connect_to_signal``, on the objects returned
      by `Bus.get_object`
    * any D-Bus method on a `dbus.Interface`
    * `dbus.Interface.connect_to_signal`
    * `Bus.add_signal_receiver`

    :Since: 0.80 (in older versions, use dbus.String)
    """

    def __new__(cls, value='', variant_level=0):

        if isinstance(value, str):
            try:
                # evaluating for its side-effect of performing validation
                value.decode('utf8')
            except UnicodeError, e:
                raise UnicodeError('A str argument to UTF8String.__new__ must '
                                   'be UTF-8: %s', e)
        elif isinstance(value, unicode):
            value = value.encode('utf8')
        else:
            raise TypeError('UTF8String.__new__ requires a unicode or UTF-8 '
                            'str, not %r' % value)

        if '\0' in value:
            raise TypeError(r'D-Bus strings cannot contain "\0", but %r does'
                            % value)

        return super(UTF8String, cls).__new__(cls, value, variant_level)


class ObjectPath(_DBusTypeMixin, str):
    """A D-Bus object path, such as '/com/example/MyApp/Documents/abc'.

    ObjectPath is a subtype of str, and object-paths behave like strings.
    """

    def __new__(cls, value, variant_level=0):
        if isinstance(value, unicode):
            value = value.encode('ascii')
        elif not isinstance(value, str):
            raise TypeError('ObjectPath.__new__ requires a unicode or '
                            'str instance, not %r' % value)

        validate_object_path(value)

        return super(ObjectPath, cls).__new__(cls, value, variant_level)


class Boolean(_DBusTypeMixin, int):
    """A boolean, represented as a subtype of `int` (not `bool`, because
    `bool` cannot be subclassed).
    """

    __slots__ = ('_dbus_variant_level',)

    def __new__(cls, value=False, variant_level=0):
        return super(Boolean, cls).__new__(cls, bool(value), variant_level)

    def __repr__(self):
        variant_level = self._dbus_variant_level
        bool_repr = self and 'True' or 'False'
        if variant_level:
            return '%s(%s, variant_level=%d)' % (self.__class__.__name__,
                                                 bool_repr, variant_level)
        else:
            return '%s(%s)' % (self.__class__.__name__, bool_repr)


if maxint >= 0x7FFFFFFFFFFFFFFFL:
    _I64 = int
    if maxint >= 0xFFFFFFFFFFFFFFFFL:
        _U64 = int
    else:
        _U64 = long
else:
    _I64 = long
    _U64 = long

class Int16(_DBusTypeMixin, int):
    """A signed 16-bit integer between -0x8000 and +0x7FFF, represented as a
    subtype of `int`.
    """

    __slots__ = ('_dbus_variant_level',)

    def __new__(cls, value=0, variant_level=0):
        self = super(Int16, cls).__new__(cls, value, variant_level)
        if self < -0x8000 or self > 0x7FFF:
            raise OverflowError('Value %r out of range for Int16'
                                % value)
        return self

class UInt16(_DBusTypeMixin, int):
    """An unsigned 16-bit integer between 0 and +0xFFFF, represented as a
    subtype of `int`.
    """

    __slots__ = ('_dbus_variant_level',)

    def __new__(cls, value=0, variant_level=0):
        self = super(UInt16, cls).__new__(cls, value, variant_level)
        if self < 0 or self > 0xFFFF:
            raise OverflowError('Value %r out of range for UInt16'
                                % value)
        return self

class Int32(_DBusTypeMixin, int):
    """A signed 32-bit integer between -0x8000 0000 and +0x7FFF FFFF,
    represented as a subtype of `int`.
    """

    __slots__ = ('_dbus_variant_level',)

    def __new__(cls, value=0, variant_level=0):
        self = super(Int32, cls).__new__(cls, value, variant_level)
        if self < -0x80000000 or self > 0x7FFFFFFF:
            raise OverflowError('Value %r out of range for Int32'
                                % value)
        return self

class UInt32(_DBusTypeMixin, _I64):
    """An unsigned 32-bit integer between 0 and +0xFFFF FFFF, represented as a
    subtype of either `int` or `long` (platform-dependent and subject to
    change).
    """

    if _I64 is int:
        __slots__ = ('_dbus_variant_level',)

    def __new__(cls, value=0, variant_level=0):
        self = super(UInt32, cls).__new__(cls, value, variant_level)
        if self < 0 or self > 0xFFFFFFFFL:
            raise OverflowError('Value %r out of range for UInt32'
                                % value)
        return self

class Int64(_DBusTypeMixin, _I64):
    """A signed 64-bit integer between -0x8000 0000 0000 0000 and
    +0x7FFF FFFF FFFF FFFF,
    represented as a subtype of `int`.
    """

    if _I64 is int:
        __slots__ = ('_dbus_variant_level',)

    def __new__(cls, value=0, variant_level=0):
        self = super(Int64, cls).__new__(cls, value, variant_level)
        if self < -0x8000000000000000L or self > 0x7FFFFFFFFFFFFFFFL:
            raise OverflowError('Value %r out of range for Int64'
                                % value)
        return self

class UInt64(_DBusTypeMixin, _U64):
    """An unsigned 64-bit integer between 0 and +0xFFFF FFFF FFFF FFFF,
    represented as a subtype of either `int` or `long` (platform-dependent and
    subject to change).
    """

    if _U64 is int:
        __slots__ = ('_dbus_variant_level',)

    def __new__(cls, value=0, variant_level=0):
        self = super(UInt64, cls).__new__(cls, value, variant_level)
        if self < 0 or self > 0xFFFFFFFFFFFFFFFFL:
            raise OverflowError('Value %r out of range for UInt64'
                                % value)
        return self

class Double(_DBusTypeMixin, float):
    """A double-precision floating-point number, represented as a subtype
    of `float`."""

    __slots__ = ('_dbus_variant_level',)

    def __new__(cls, value=0.0, variant_level=0):
        return super(Double, cls).__new__(cls, value, variant_level)

class Array(_DBusTypeMixin, list):
    """An array of items of the same type, implemented as a subtype of
    list.

    As currently implemented, an Array behaves just like a list, but with
    the addition of a ``signature`` property set by the constructor;
    conversion of its items to D-Bus types is only done when it's sent
    in a Message. This might change in future so validation is done earlier.
    """

    __slots__ = ('_dbus_variant_level', '_signature')

    @property
    def signature(self):
        """The D-Bus signature of each element of this Array (a Signature
        instance), or None if unspecified. Read-only.

        The signature of an Array ``arr`` is given by ``'a' + arr.signature``.

        If None, when the Array is sent over D-Bus, the signature will be
        guessed from the first element. Try to avoid this if possible.
        """
        return self._signature

    def __new__(cls, iterable=(), signature=None, variant_level=0):
        """"""

        if signature is not None:
            signature = Signature(signature)

            if len(tuple(signature)) != 1:
                raise ValueError("There must be exactly one complete type "
                                 "in an Array's signature parameter")

        self = super(Array, cls).__new__(cls, iterable,
                                         variant_level=variant_level)
        self._signature = signature
        return self

    def __init__(self, iterable=(), signature=None, variant_level=0):
        super(Array, self).__init__(iterable)
