import _util
import inspect
import dbus_bindings

def method(dbus_interface, in_signature=None, out_signature=None, async_callbacks=None, sender_keyword=None):
    _util._validate_interface_or_name(dbus_interface)

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
            in_sig = tuple(dbus_bindings.Signature(in_signature))

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
        return func

    return decorator

def signal(dbus_interface, signature=None):
    _util._validate_interface_or_name(dbus_interface)
    def decorator(func):
        def emit_signal(self, *args, **keywords):
            func(self, *args, **keywords)
            message = dbus_bindings.Signal(self._object_path, dbus_interface, func.__name__)
            iter = message.get_iter(True)

            if emit_signal._dbus_signature:
                signature = tuple(dbus_bindings.Signature(emit_signal._dbus_signature))
                for (arg, sig) in zip(args, signature):
                    iter.append_strict(arg, sig)
            else:
                for arg in args:
                    iter.append(arg)

            self._connection.send(message)

        args = inspect.getargspec(func)[0]
        args.pop(0)

        if signature:
            sig = tuple(dbus_bindings.Signature(signature))

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
    func._dbus_pass_message = True
    return func
