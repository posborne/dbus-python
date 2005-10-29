
import dbus_bindings 
import _dbus
import operator
from exceptions import UnknownMethodException
from decorators import method
from decorators import signal

class BusName:
    """A base class for exporting your own Named Services across the Bus
    """
    def __init__(self, named_service, bus=None):
        self._named_service = named_service
                             
        if bus == None:
            # Get the default bus
            self._bus = _dbus.Bus()
        else:
            self._bus = bus

        dbus_bindings.bus_request_name(self._bus.get_connection(), named_service)

    def get_bus(self):
        """Get the Bus this Service is on"""
        return self._bus

    def get_name(self):
        """Get the name of this service"""
        return self._named_service

def _dispatch_dbus_method_call(self, argument_list, message):
    """Calls method_to_call using argument_list, but handles
    exceptions, etc, and generates a reply to the DBus Message message
    """
    try:
        method_name = message.get_member()
        dbus_interface = message.get_interface()
        candidate = None
        successful = False

        # split up the cases when we do and don't have an interface because the
        # latter is much simpler
        if dbus_interface:
            # search through the class hierarchy in python MRO order
            for cls in self.__class__.__mro__:
                # if we haven't got a candidate yet, and we find a class with a
                # suitably named member, save this as a candidate
                if (not candidate and method_name in cls.__dict__):
                    if ("_dbus_is_method" in cls.__dict__[method_name].__dict__
                        and "_dbus_interface" in cls.__dict__[method_name].__dict__):
                        # however if it is annotated for a different interface
                        # than we are looking for, it cannot be a candidate
                        if cls.__dict__[method_name]._dbus_interface == dbus_interface:
                            candidate = cls
                            sucessful = True
                            target_parent = cls.__dict__[method_name]
                            break
                        else:
                            pass
                    else:
                        candidate = cls

                # if we have a candidate, carry on checking this and all
                # superclasses for a method annoated as a dbus method
                # on the correct interface
                if (candidate and method_name in cls.__dict__
                    and "_dbus_is_method" in cls.__dict__[method_name].__dict__
                    and "_dbus_interface" in cls.__dict__[method_name].__dict__
                    and cls.__dict__[method_name]._dbus_interface == dbus_interface):
                    # the candidate is a dbus method on the correct interface,
                    # or overrides a method that is, success!
                    target_parent = cls.__dict__[method_name]
                    sucessful = True
                    break

        else:
            # simpler version of above
            for cls in self.__class__.__mro__:
                if (not candidate and method_name in cls.__dict__):
                    candidate = cls

                if (candidate and method_name in cls.__dict__
                    and "_dbus_is_method" in cls.__dict__[method_name].__dict__):
                    target_parent = cls.__dict__[method_name]
                    sucessful = True
                    break

        retval = candidate.__dict__[method_name](self, *argument_list)
        target_name = str(candidate.__module__) + '.' + candidate.__name__ + '.' + method_name

        if not sucessful:
            if not dbus_interface:
                raise UnknownMethodException('%s is not a valid method'%(message.get_member()))
            else:
                raise UnknownMethodException('%s is not a valid method of interface %s'%(message.get_member(), dbus_interface))

    except Exception, e:
        if e.__module__ == '__main__':
            # FIXME: is it right to use .__name__ here?
            error_name = e.__class__.__name__
        else:
            error_name = e.__module__ + '.' + str(e.__class__.__name__)

        error_contents = str(e)
        reply = dbus_bindings.Error(message, error_name, error_contents)
    else:
        reply = dbus_bindings.MethodReturn(message)

        # do strict adding if an output signature was provided
        if target_parent._dbus_out_signature != None:
            # iterate signature into list of complete types
            signature = tuple(dbus_bindings.Signature(target_parent._dbus_out_signature))

            if retval == None:
                if len(signature) != 0:
                    raise TypeError('%s returned nothing but output signature is %s' %
                        (target_name, target_parent._dbus_out_signature))
            elif len(signature) == 1:
                iter = reply.get_iter(append=True)
                iter.append_strict(retval, signature[0])
            elif len(signature) > 1:
                if operator.isSequenceType(retval):
                    if len(signature) > len(retval):
                        raise TypeError('output signature %s is longer than the number of values returned by %s' %
                            (target_parent._dbus_out_signature, target_name))
                    elif len(retval) > len(signature):
                        raise TypeError('output signature %s is shorter than the number of values returned by %s' %
                            (target_parent._dbus_out_signature, target_name))
                    else:
                        iter = reply.get_iter(append=True)
                        for (value, sig) in zip(retval, signature):
                            iter.append_strict(value, sig)
                else:
                    raise TypeError('output signature %s has multiple values but %s didn\'t return a sequence type' %
                        (target_parent._dbus_out_signature, target_name))

        # try and guess the return type
        elif retval != None:
            iter = reply.get_iter(append=True)
            iter.append(retval)

    return reply

class InterfaceType(type):
    def __init__(cls, name, bases, dct):
        # these attributes are shared between all instances of the Interface
        # object, so this has to be a dictionary that maps class names to
        # the per-class introspection/interface data
        class_table = getattr(cls, '_dbus_class_table', {})
        cls._dbus_class_table = class_table
        interface_table = class_table[cls.__module__ + '.' + name] = {}

        # merge all the name -> method tables for all the interfaces
        # implemented by our base classes into our own
        for b in bases:
            base_name = b.__module__ + '.' + b.__name__
            if getattr(b, '_dbus_class_table', False):
                for (interface, method_table) in class_table[base_name].iteritems():
                    our_method_table = interface_table.setdefault(interface, {})
                    our_method_table.update(method_table)

        # add in all the name -> method entries for our own methods/signals
        for func in dct.values():
            if getattr(func, '_dbus_interface', False):
                method_table = interface_table.setdefault(func._dbus_interface, {})
                method_table[func.__name__] = func

        super(InterfaceType, cls).__init__(name, bases, dct)

    # methods are different to signals, so we have two functions... :)
    def _reflect_on_method(cls, func):
        args = func._dbus_args

        if func._dbus_in_signature:
            # convert signature into a tuple so length refers to number of
            # types, not number of characters
            in_sig = tuple(dbus_bindings.Signature(func._dbus_in_signature))

            if len(in_sig) > len(args):
                raise ValueError, 'input signature is longer than the number of arguments taken'
            elif len(in_sig) < len(args):
                raise ValueError, 'input signature is shorter than the number of arguments taken'
        else:
            # magic iterator which returns as many v's as we need
            in_sig = dbus_bindings.VariantSignature()

        if func._dbus_out_signature:
            out_sig = dbus_bindings.Signature(func._dbus_out_signature)
        else:
            # its tempting to default to dbus_bindings.Signature('v'), but
            # for methods that return nothing, providing incorrect
            # introspection data is worse than providing none at all
            out_sig = []

        reflection_data = '    <method name="%s">\n' % (func.__name__)
        for pair in zip(in_sig, args):
            reflection_data += '      <arg direction="in"  type="%s" name="%s" />\n' % pair
        for type in out_sig:
            reflection_data += '      <arg direction="out" type="%s" />\n' % type
        reflection_data += '    </method>\n'

        return reflection_data

    def _reflect_on_signal(cls, func):
        args = func._dbus_args

        if func._dbus_signature:
            # convert signature into a tuple so length refers to number of
            # types, not number of characters
            sig = tuple(dbus_bindings.Signature(func._dbus_signature))

            if len(sig) > len(args):
                raise ValueError, 'signal signature is longer than the number of arguments provided'
            elif len(sig) < len(args):
                raise ValueError, 'signal signature is shorter than the number of arguments provided'
        else:
            # magic iterator which returns as many v's as we need
            sig = dbus_bindings.VariantSignature()

        reflection_data = '    <signal name="%s">\n' % (func.__name__)
        for pair in zip(sig, args):
            reflection_data = reflection_data + '      <arg type="%s" name="%s" />\n' % pair
        reflection_data = reflection_data + '    </signal>\n'

        return reflection_data

class Interface(object):
    __metaclass__ = InterfaceType

class Object(Interface):
    """A base class for exporting your own Objects across the Bus.

    Just inherit from Object and provide a list of methods to share
    across the Bus
    """
    def __init__(self, bus_name, object_path):
        self._object_path = object_path
        self._name = bus_name 
        self._bus = bus_name.get_bus()
            
        self._connection = self._bus.get_connection()

        self._connection.register_object_path(object_path, self._unregister_cb, self._message_cb)

    def _unregister_cb(self, connection):
        print ("Unregister")

    def _message_cb(self, connection, message):
        try:
            target_method_name = message.get_member()
            args = message.get_args_list()
        
            reply = _dispatch_dbus_method_call(self, args, message)

            self._connection.send(reply)
        except Exception, e:
            error_reply = dbus_bindings.Error(message, 
	                                      "org.freedesktop.DBus.Python.%s" % e.__class__.__name__, 
                                              str(e))
            self._connection.send(error_reply)

    @method('org.freedesktop.DBus.Introspectable', in_signature='', out_signature='s')
    def Introspect(self):
        reflection_data = '<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN" "http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">\n'
        reflection_data += '<node name="%s">\n' % (self._object_path)

        interfaces = self._dbus_class_table[self.__class__.__module__ + '.' + self.__class__.__name__]
        for (name, funcs) in interfaces.iteritems():
            reflection_data += '  <interface name="%s">\n' % (name)

            for func in funcs.values():
                if getattr(func, '_dbus_is_method', False):
                    reflection_data += self.__class__._reflect_on_method(func)
                elif getattr(func, '_dbus_is_signal', False):
                    reflection_data += self.__class__._reflect_on_signal(func)

            reflection_data += '  </interface>\n'

        reflection_data += '</node>\n'

        return reflection_data

