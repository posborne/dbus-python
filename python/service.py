import dbus_bindings
import _dbus
import operator
import traceback

from exceptions import NameExistsException
from exceptions import UnknownMethodException
from decorators import method
from decorators import signal

class BusName(object):
    """A base class for exporting your own Named Services across the Bus
    """
    def __new__(cls, name, bus=None):
        # get default bus
        if bus == None:
            bus = _dbus.Bus()

        # see if this name is already defined, return it if so
        if name in bus._bus_names:
            return bus._bus_names[name]

        # otherwise register the name
        retval = dbus_bindings.bus_request_name(bus.get_connection(), name)

        # TODO: more intelligent tracking of bus name states?
        if retval == dbus_bindings.REQUEST_NAME_REPLY_PRIMARY_OWNER:
            pass
        elif retval == dbus_bindings.REQUEST_NAME_REPLY_IN_QUEUE:
            # queueing can happen by default, maybe we should
            # track this better or let the user know if they're
            # queued or not?
            pass
        elif retval == dbus_bindings.REQUEST_NAME_REPLY_EXISTS:
            raise NameExistsException(name)
        elif retval == dbus_bindings.REQUEST_NAME_REPLY_ALREADY_OWNER:
            # if this is a shared bus which is being used by someone
            # else in this process, this can happen legitimately
            pass
        else:
            raise RuntimeError('requesting bus name %s returned unexpected value %s' % (name, retval))

        # and create the object
        bus_name = object.__new__(cls)
        bus_name._bus = bus
        bus_name._name = name

        # cache instance
        bus._bus_names[name] = bus_name

        return bus_name

    # do nothing because this is called whether or not the bus name
    # object was retrieved from the cache or created new
    def __init__(self, *args, **keywords):
        pass

    # we can delete the low-level name here because these objects
    # are guaranteed to exist only once for each bus name
    def __del__(self):
        dbus_bindings.bus_release_name(self._bus.get_connection(), self._name)
        pass

    def get_bus(self):
        """Get the Bus this Service is on"""
        return self._bus

    def get_name(self):
        """Get the name of this service"""
        return self._name

    def __repr__(self):
        return '<dbus.service.BusName %s on %r at %#x>' % (self._name, self._bus, id(self))
    __str__ = __repr__


def _method_lookup(self, method_name, dbus_interface):
    """Walks the Python MRO of the given class to find the method to invoke.

    Returns two methods, the one to call, and the one it inherits from which
    defines its D-Bus interface name, signature, and attributes.
    """
    parent_method = None
    candidate_class = None
    successful = False

    # split up the cases when we do and don't have an interface because the
    # latter is much simpler
    if dbus_interface:
        # search through the class hierarchy in python MRO order
        for cls in self.__class__.__mro__:
            # if we haven't got a candidate class yet, and we find a class with a
            # suitably named member, save this as a candidate class
            if (not candidate_class and method_name in cls.__dict__):
                if ("_dbus_is_method" in cls.__dict__[method_name].__dict__
                    and "_dbus_interface" in cls.__dict__[method_name].__dict__):
                    # however if it is annotated for a different interface
                    # than we are looking for, it cannot be a candidate
                    if cls.__dict__[method_name]._dbus_interface == dbus_interface:
                        candidate_class = cls
                        parent_method = cls.__dict__[method_name]
                        sucessful = True
                        break
                    else:
                        pass
                else:
                    candidate_class = cls

            # if we have a candidate class, carry on checking this and all
            # superclasses for a method annoated as a dbus method
            # on the correct interface
            if (candidate_class and method_name in cls.__dict__
                and "_dbus_is_method" in cls.__dict__[method_name].__dict__
                and "_dbus_interface" in cls.__dict__[method_name].__dict__
                and cls.__dict__[method_name]._dbus_interface == dbus_interface):
                # the candidate class has a dbus method on the correct interface,
                # or overrides a method that is, success!
                parent_method = cls.__dict__[method_name]
                sucessful = True
                break

    else:
        # simpler version of above
        for cls in self.__class__.__mro__:
            if (not candidate_class and method_name in cls.__dict__):
                candidate_class = cls

            if (candidate_class and method_name in cls.__dict__
                and "_dbus_is_method" in cls.__dict__[method_name].__dict__):
                parent_method = cls.__dict__[method_name]
                sucessful = True
                break

    if sucessful:
        return (candidate_class.__dict__[method_name], parent_method)
    else:
        if dbus_interface:
            raise UnknownMethodException('%s is not a valid method of interface %s' % (method_name, dbus_interface))
        else:
            raise UnknownMethodException('%s is not a valid method' % method_name)


def _method_reply_return(connection, message, method_name, signature, *retval):
    reply = dbus_bindings.MethodReturn(message)
    iter = reply.get_iter(append=True)

    # do strict adding if an output signature was provided
    if signature:
        if len(signature) > len(retval):
            raise TypeError('output signature %s is longer than the number of values returned by %s' %
                (signature, method_name))
        elif len(retval) > len(signature):
            raise TypeError('output signature %s is shorter than the number of values returned by %s' %
                (signature, method_name))
        else:
            for (value, sig) in zip(retval, signature):
                iter.append_strict(value, sig)

    # no signature, try and guess the return type by inspection
    else:
        for value in retval:
            iter.append(value)

    connection.send(reply)


def _method_reply_error(connection, message, exception):
    if '_dbus_error_name' in exception.__dict__:
        name = exception._dbus_error_name
    elif exception.__module__ == '__main__':
        name = 'org.freedesktop.DBus.Python.%s' % exception.__class__.__name__
    else:
        name = 'org.freedesktop.DBus.Python.%s.%s' % (exception.__module__, exception.__class__.__name__)

    contents = traceback.format_exc()
    reply = dbus_bindings.Error(message, name, contents)

    connection.send(reply)


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
            # types, not number of characters. the length is checked by
            # the decorator to make sure it matches the length of args.
            in_sig = tuple(dbus_bindings.Signature(func._dbus_in_signature))
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
            # lookup candidate method and parent method
            method_name = message.get_member()
            interface_name = message.get_interface()
            (candidate_method, parent_method) = _method_lookup(self, method_name, interface_name)

            # set up method call parameters
            args = message.get_args_list()
            keywords = {}

            # iterate signature into list of complete types
            if parent_method._dbus_out_signature:
                signature = tuple(dbus_bindings.Signature(parent_method._dbus_out_signature))
            else:
                signature = None

            # set up async callback functions
            if parent_method._dbus_async_callbacks:
                (return_callback, error_callback) = parent_method._dbus_async_callbacks
                keywords[return_callback] = lambda *retval: _method_reply_return(connection, message, method_name, signature, *retval)
                keywords[error_callback] = lambda exception: _method_reply_error(connection, message, exception)

            # include the sender if desired
            if parent_method._dbus_sender_keyword:
                keywords[parent_method._dbus_sender_keyword] = message.get_sender()

            # call method
            retval = candidate_method(self, *args, **keywords)

            # we're done - the method has got callback functions to reply with
            if parent_method._dbus_async_callbacks:
                return

            # otherwise we send the return values in a reply. if we have a
            # signature, use it to turn the return value into a tuple as
            # appropriate
            if parent_method._dbus_out_signature:
                # if we have zero or one return values we want make a tuple
                # for the _method_reply_return function, otherwise we need
                # to check we're passing it a sequence
                if len(signature) == 0:
                    if retval == None:
                        retval = ()
                    else:
                        raise TypeError('%s has an empty output signature but did not return None' %
                            method_name)
                elif len(signature) == 1:
                    retval = (retval,)
                else:
                    if operator.isSequenceType(retval):
                        # multi-value signature, multi-value return... proceed unchanged
                        pass
                    else:
                        raise TypeError('%s has multiple output values in signature %s but did not return a sequence' %
                            (method_name, signature))

            # no signature, so just turn the return into a tuple and send it as normal
            else:
                signature = None
                if retval == None:
                    retval = ()
                else:
                    retval = (retval,)

            _method_reply_return(connection, message, method_name, signature, *retval)
        except Exception, exception:
            # send error reply
            _method_reply_error(connection, message, exception)

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

    def __repr__(self):
        return '<dbus.service.Object %s on %r at %#x>' % (self._object_path, self._name, id(self))
    __str__ = __repr__

