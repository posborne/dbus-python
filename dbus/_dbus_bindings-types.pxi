# vim:set sw=4 sts=4 ft=pyrex:
# Types defined in _dbus_bindings and imported into dbus.types.

__docformat__ = 'restructuredtext'

class ObjectPath(str):
    """A D-Bus object path, e.g. ``/org/example/foo/FooBar``."""

class ByteArray(str):
    """A byte array represented as an 8-bit string.

    Used to avoid having to construct a list of integers when passing
    byte-array (``ay``) parameters to D-Bus methods.
    """

class SignatureIter(object):
    """An iterator over complete types in a D-Bus signature."""

    def __init__(self, string):
        """Constructor.

        :Parameters:
             `string` : str
                A D-Bus signature
        """
        object.__init__(self)
        self.remaining = string

    def next(self):
        """Return the next complete type, e.g. ``b``, ``ab`` or ``a{sv}``."""
        if self.remaining == '':
            raise StopIteration

        signature = self.remaining
        block_depth = 0
        block_type = None
        end = len(signature)

        for marker in range(0, end):
            cur_sig = ord(signature[marker])

            if cur_sig == TYPE_ARRAY:
                pass
            elif cur_sig == DICT_ENTRY_BEGIN or cur_sig == STRUCT_BEGIN:
                if block_type == None:
                    block_type = cur_sig

                if block_type == cur_sig:
                    block_depth = block_depth + 1

            elif cur_sig == DICT_ENTRY_END:
                if block_type == DICT_ENTRY_BEGIN:
                    block_depth = block_depth - 1

                if block_depth == 0:
                    end = marker
                    break

            elif cur_sig == STRUCT_END:
                if block_type == STRUCT_BEGIN:
                    block_depth = block_depth - 1

                if block_depth == 0:
                    end = marker
                    break

            else:
                if block_depth == 0:
                    end = marker
                    break

        end = end + 1
        self.remaining = signature[end:]
        return Signature(signature[0:end])

class Signature(str):
    """An iterable method signature. Iterating gives the signature of each
    argument in turn."""

    def __iter__(self):
        """Return an iterator over complete types in the signature."""
        return SignatureIter(self)

class Byte(int):
    """An unsigned byte"""
    # FIXME: this should subclass str and force length 1, judging by the rest
    # of the API!

class Boolean(int):
    """A Boolean value"""

class Int16(int):
    """A signed 16-bit integer"""

class UInt16(int):
    """An unsigned 16-bit integer"""
    def __init__(self, value):
        if value < 0:
            raise TypeError('Unsigned integers must not have a negative value')
        int.__init__(self, value)

class Int32(int):
    """An signed 32-bit integer"""

class UInt32(long):
    """An unsigned 32-bit integer"""
    def __init__(self, value):
        if value < 0:
            raise TypeError('Unsigned integers must not have a negative value')
        long.__init__(self, value)

class Int64(long):
    """A signed 64-bit integer"""

class UInt64(long):
    """An unsigned 64-bit integer"""
    def __init__(self, value):
        if value < 0:
            raise TypeError('Unsigned integers must not have a negative value')
        long.__init__(self, value)

class Double(float):
    """A double-precision floating point number"""

class String(unicode):
    """A Unicode string"""

class Array(list):
    """An array of values of the same type"""
    def __init__(self, value, type=None, signature=None):
        if signature and type:
            raise TypeError('Can not mix type and signature arguments in a D-BUS Array')
    
        self.type = type
        self.signature = signature
        list.__init__(self, value)

class Variant:
    """A generic wrapper for values of any other basic D-Bus type"""
    def __init__(self, value, type=None, signature=None):
        self.value = value
        if signature and type:
            raise TypeError('Can not mix type and signature arguments in a D-BUS Variant')

        self.type = type
        self.signature = signature

    def __repr__(self):
        return repr(self.value)

    def __str__(self):
        return str(self.value)

class Struct(tuple):
    """An immutable structure containing D-Bus values, possibly of
    different types.
    """

class Dictionary(dict):
    """A mapping from distinct keys (all of the same type) to values (all
    of the same type, which need not be the same type as the values).
    """
    def __init__(self, value, key_type=None, value_type=None, signature=None):
        if key_type and not value_type:
             raise TypeError('When specifying a key_type you must also have a value_type in a D-BUS Dictionary')
        elif value_type and not key_type:
             raise TypeError('When specifying a value_type you must also have a key_type in a D-BUS Dictionary')
        elif key_type and signature:
              raise TypeError('Can not mix type arguments with signature arguments in a D-BUS Dictionary')
              
        self.key_type = key_type
        self.value_type = value_type
        self.signature = signature
        dict.__init__(self, value)
