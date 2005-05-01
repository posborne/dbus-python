import _util 
import inspect


def method(dbus_interface):
    _util._validate_interface_or_name(dbus_interface)

    def decorator(func):
        func._dbus_is_method = True
        func._dbus_interface = dbus_interface
        func._dbus_args = inspect.getargspec(func)[0]
        func._dbus_args.pop(0)
        return func

    return decorator

def signal(dbus_interface):
    _util._validate_interface_or_name(dbus_interface)
    def decorator(func):
        def emit_signal(self, *args, **keywords):
	    func(self, *args, **keywords)
	    message = dbus_bindings.Signal(self._object_path, dbus_interface, func.__name__)
            iter = message.get_iter(True)
            for arg in args:
                iter.append(arg)
        
            self._connection.send(message)
	
        emit_signal._dbus_is_signal = True
        emit_signal._dbus_interface = dbus_interface
        emit_signal.__name__ = func.__name__
        emit_signal._dbus_args = inspect.getargspec(func)[0]
        emit_signal._dbus_args.pop(0)
        return emit_signal

    return decorator

