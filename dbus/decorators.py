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

__all__ = ('explicitly_pass_message', 'method', 'signal')
__docformat__ = 'restructuredtext'

import inspect

import _dbus_bindings


def method(dbus_interface, in_signature=None, out_signature=None, async_callbacks=None, sender_keyword=None, utf8_strings=False, byte_arrays=False):
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
        func._dbus_args = args
        func._dbus_get_args_options = {'byte_arrays': byte_arrays,
                                       'utf8_strings': utf8_strings}
        return func

    return decorator


def signal(dbus_interface, signature=None):
    """Factory for decorators used to mark methods of a `dbus.service.Object`
    to emit signals on the D-Bus.

    Whenever the decorated method is called in Python, after the method
    body is executed, a signal with the same name as the decorated method,
    from the given D-Bus interface, will be emitted.

    :Parameters:
        `dbus_interface` : str
            The D-Bus interface whose signal is emitted
        `signature` : str
            The signature of the signal in the usual D-Bus notation
    """
    _dbus_bindings.validate_interface_name(dbus_interface)
    def decorator(func):
        def emit_signal(self, *args, **keywords):
            func(self, *args, **keywords)
            message = _dbus_bindings.SignalMessage(self._object_path, dbus_interface, func.__name__)

            if emit_signal._dbus_signature:
                message.append(signature=emit_signal._dbus_signature,
                               *args)
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


def explicitly_pass_message(func):
    """Decorator which marks the given function such that, if it is called
    as a D-Bus signal recipient, then the Signal message will be passed
    to it as a keyword parameter named ``dbus_message``.

    :Deprecated:
        This is mostly a workaround for the lack of ``sender_keyword``
        and ``path_keyword`` in older dbus-python versions. If this
        functionality is still needed, it should be a keyword
        parameter to the signal connection function like the rest of the
        calling-convention modifiers.
    :Bug:
        This decorator alters the namespace of the decorated function
        without using the ``__magic__`` naming convention.
    """
    func._dbus_pass_message = True
    return func
