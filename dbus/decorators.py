"""Service-side D-Bus decorators."""

# Copyright (C) 2003, 2004, 2005, 2006 Red Hat Inc. <http://www.redhat.com/>
# Copyright (C) 2003 David Zeuthen
# Copyright (C) 2004 Rob Taylor
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

__all__ = ('method', 'signal')
__docformat__ = 'restructuredtext'

import inspect

import _dbus_bindings


def method(dbus_interface, in_signature=None, out_signature=None,
        async_callbacks=None,
        sender_keyword=None, path_keyword=None, destination_keyword=None,
        message_keyword=None,
        utf8_strings=False, byte_arrays=False):
    """Factory for decorators used to mark methods of a `dbus.service.Object`
    to be exported on the D-Bus.

    The decorated method will be exported over D-Bus as the method of the
    same name on the given D-Bus interface.

    :Parameters:
        `dbus_interface` : str
            Name of a D-Bus interface
        `in_signature` : str or None
            If not None, the signature of the method parameters in the usual
            D-Bus notation
        `out_signature` : str or None
            If not None, the signature of the return value in the usual
            D-Bus notation
        `async_callbacks` : tuple containing (str,str), or None
            If None (default) the decorated method is expected to return
            values matching the `out_signature` as usual, or raise
            an exception on error. If not None, the following applies:

            `async_callbacks` contains the names of two keyword arguments to
            the decorated function, which will be used to provide a success
            callback and an error callback (in that order).

            When the decorated method is called via the D-Bus, its normal
            return value will be ignored; instead, a pair of callbacks are
            passed as keyword arguments, and the decorated method is
            expected to arrange for one of them to be called.

            On success the success callback must be called, passing the
            results of this method as positional parameters in the format
            given by the `out_signature`.

            On error the decorated method may either raise an exception
            before it returns, or arrange for the error callback to be
            called with an Exception instance as parameter.

        `sender_keyword` : str or None
            If not None, contains the name of a keyword argument to the
            decorated function, conventionally ``'sender'``. When the
            method is called, the sender's unique name will be passed as
            this keyword argument.

        `path_keyword` : str or None
            If not None (the default), the decorated method will receive
            the destination object path as a keyword argument with this
            name. Normally you already know the object path, but in the
            case of "fallback paths" you'll usually want to use the object
            path in the method's implementation.

        `destination_keyword` : str or None
            If not None (the default), the decorated method will receive
            the destination bus name as a keyword argument with this name.
            Included for completeness - you shouldn't need this.

        `message_keyword` : str or None
            If not None (the default), the decorated method will receive
            the `dbus.lowlevel.MethodCallMessage` as a keyword argument
            with this name.

        `utf8_strings` : bool
            If False (default), D-Bus strings are passed to the decorated
            method as objects of class dbus.String, a unicode subclass.

            If True, D-Bus strings are passed to the decorated method
            as objects of class dbus.UTF8String, a str subclass guaranteed
            to be encoded in UTF-8.

            This option does not affect object-paths and signatures, which
            are always 8-bit strings (str subclass) encoded in ASCII.

        `byte_arrays` : bool
            If False (default), a byte array will be passed to the decorated
            method as an `Array` (a list subclass) of `Byte` objects.

            If True, a byte array will be passed to the decorated method as
            a `ByteArray`, a str subclass. This is usually what you want,
            but is switched off by default to keep dbus-python's API
            consistent.
    """
    _dbus_bindings.validate_interface_name(dbus_interface)

    def decorator(func):
        args = inspect.getargspec(func)[0]
        args.pop(0)

        if async_callbacks:
            if type(async_callbacks) != tuple:
                raise TypeError('async_callbacks must be a tuple of (keyword for return callback, keyword for error callback)')
            if len(async_callbacks) != 2:
                raise ValueError('async_callbacks must be a tuple of (keyword for return callback, keyword for error callback)')
            args.remove(async_callbacks[0])
            args.remove(async_callbacks[1])

        if sender_keyword:
            args.remove(sender_keyword)
        if path_keyword:
            args.remove(path_keyword)
        if destination_keyword:
            args.remove(destination_keyword)
        if message_keyword:
            args.remove(message_keyword)

        if in_signature:
            in_sig = tuple(_dbus_bindings.Signature(in_signature))

            if len(in_sig) > len(args):
                raise ValueError, 'input signature is longer than the number of arguments taken'
            elif len(in_sig) < len(args):
                raise ValueError, 'input signature is shorter than the number of arguments taken'

        func._dbus_is_method = True
        func._dbus_async_callbacks = async_callbacks
        func._dbus_interface = dbus_interface
        func._dbus_in_signature = in_signature
        func._dbus_out_signature = out_signature
        func._dbus_sender_keyword = sender_keyword
        func._dbus_path_keyword = path_keyword
        func._dbus_destination_keyword = destination_keyword
        func._dbus_message_keyword = message_keyword
        func._dbus_args = args
        func._dbus_get_args_options = {'byte_arrays': byte_arrays,
                                       'utf8_strings': utf8_strings}
        return func

    return decorator


def signal(dbus_interface, signature=None, path_keyword=None):
    """Factory for decorators used to mark methods of a `dbus.service.Object`
    to emit signals on the D-Bus.

    Whenever the decorated method is called in Python, after the method
    body is executed, a signal with the same name as the decorated method,
    with the given D-Bus interface, will be emitted from this object.

    :Parameters:
        `dbus_interface` : str
            The D-Bus interface whose signal is emitted
        `signature` : str
            The signature of the signal in the usual D-Bus notation

        `path_keyword` : str or None
            A keyword argument to the decorated method. If not None,
            that argument will not be emitted as an argument of
            the signal, and when the signal is emitted, it will appear
            to come from the object path given by the keyword argument.

            Note that when calling the decorated method, you must always
            pass in the object path as a keyword argument, not as a
            positional argument.
    """
    _dbus_bindings.validate_interface_name(dbus_interface)
    def decorator(func):
        member_name = func.__name__
        _dbus_bindings.validate_member_name(member_name)

        def emit_signal(self, *args, **keywords):
            func(self, *args, **keywords)
            object_path = self.__dbus_object_path__
            if path_keyword:
                kw = keywords.pop(path_keyword, None)
                if kw is not None:
                    if not (kw == object_path
                            or object_path == '/'
                            or kw.startswith(object_path + '/')):
                        raise DBusException('Object path %s is not in the '
                                            'subtree starting at %s'
                                            % (kw, object_path))
                    object_path = kw

            message = _dbus_bindings.SignalMessage(object_path,
                                                   dbus_interface,
                                                   member_name)

            if signature is not None:
                message.append(signature=signature, *args)
            else:
                message.append(*args)

            self._connection.send_message(message)

        args = inspect.getargspec(func)[0]
        args.pop(0)

        if signature:
            sig = tuple(_dbus_bindings.Signature(signature))

            if len(sig) > len(args):
                raise ValueError, 'signal signature is longer than the number of arguments provided'
            elif len(sig) < len(args):
                raise ValueError, 'signal signature is shorter than the number of arguments provided'

        emit_signal.__name__ = func.__name__
        emit_signal.__doc__ = func.__doc__
        emit_signal._dbus_is_signal = True
        emit_signal._dbus_interface = dbus_interface
        emit_signal._dbus_signature = signature
        emit_signal._dbus_args = args
        return emit_signal

    return decorator
