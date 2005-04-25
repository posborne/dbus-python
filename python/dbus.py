
"""Module for high-level communication over the FreeDesktop.org Bus (DBus)

DBus allows you to share and access remote objects between processes
running on the desktop, and also to access system services (such as
the print spool).

To use DBus, first get a Bus object, which provides a connection to one
of a few standard dbus-daemon instances that might be running. From the
Bus you can get a RemoteService. A service is provided by an application or
process connected to the Bus, and represents a set of objects. Once you
have a RemoteService you can get a RemoteObject that implements a specific interface
(an interface is just a standard group of member functions). Then you can call
those member functions directly.

You can think of a complete method call as looking something like:

Bus:SESSION -> Service:org.gnome.Evolution -> Object:/org/gnome/Evolution/Inbox -> Interface: org.gnome.Evolution.MailFolder -> Method: Forward('message1', 'seth@gnome.org')

This communicates over the SESSION Bus to the org.gnome.Evolution process to call the
Forward method of the /org/gnome/Evolution/Inbox object (which provides the
org.gnome.Evolution.MailFolder interface) with two string arguments.

For example, the dbus-daemon itself provides a service and some objects:

# Get a connection to the desktop-wide SESSION bus
bus = dbus.Bus(dbus.Bus.TYPE_SESSION)

# Get the service provided by the dbus-daemon named org.freedesktop.DBus
dbus_service = bus.get_service('org.freedesktop.DBus')

# Get a reference to the desktop bus' standard object, denoted
# by the path /org/freedesktop/DBus. The object /org/freedesktop/DBus
# implements the 'org.freedesktop.DBus' interface
dbus_object = dbus_service.get_object('/org/freedesktop/DBus',
                                       'org.freedesktop.DBus')

# One of the member functions in the org.freedesktop.DBus interface
# is ListServices(), which provides a list of all the other services
# registered on this bus. Call it, and print the list.
print(dbus_object.ListServices())
"""

import dbus_bindings
import re
import inspect


version = (0, 40, 0)

_threads_initialized = 0
def init_gthreads ():
    global _threads_initialized
    if not _threads_initialized:
        dbus_bindings.init_gthreads ()
        _threads_initialized = 1

def _validate_interface_or_name(value):
    elements = value.split('.')
    if len(elements) <= 1:
        raise ValidationException("%s must contain at least two elements seperated by a period ('.')"%(value))

    validate = re.compile('[A-Za-z][\w_]*')
    for element in elements:
        if not validate.match(element):
            raise ValidationException("Element %s of %s has invalid characters"%(element ,value))


#Decorators
def method(dbus_interface):
    _validate_interface_or_name(dbus_interface)

    def decorator(func):
        func._dbus_is_method = True
        func._dbus_interface = dbus_interface
        func._dbus_args = inspect.getargspec(func)[0]
        func._dbus_args.pop(0)
        return func

    return decorator

def signal(dbus_interface):
    _validate_interface_or_name(dbus_interface)
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


class Bus:
    """A connection to a DBus daemon.

    One of three possible standard buses, the SESSION, SYSTEM,
    or STARTER bus
    """
    TYPE_SESSION    = dbus_bindings.BUS_SESSION
    TYPE_SYSTEM     = dbus_bindings.BUS_SYSTEM
    TYPE_STARTER = dbus_bindings.BUS_STARTER

    """bus_type=[Bus.TYPE_SESSION | Bus.TYPE_SYSTEM | Bus.TYPE_STARTER]
    """

    START_REPLY_SUCCESS = dbus_bindings.DBUS_START_REPLY_SUCCESS
    START_REPLY_ALREADY_RUNNING = dbus_bindings.DBUS_START_REPLY_ALREADY_RUNNING 

    def __init__(self, bus_type=TYPE_SESSION, glib_mainloop=True):
        self._connection = dbus_bindings.bus_get(bus_type)

        self._connection.add_filter(self._signal_func)
        self._match_rule_to_receivers = { }
        if (glib_mainloop):
            self._connection.setup_with_g_main()

    def get_connection(self):
        return self._connection

    def get_session():
        """Static method that returns the session bus"""
        return SessionBus()

    get_session = staticmethod(get_session)

    def get_system():
        """Static method that returns the system bus"""
        return SystemBus()

    get_system = staticmethod(get_system)


    def get_starter():
        """Static method that returns the starter bus"""
        return StarterBus()

    get_starter = staticmethod(get_starter)


    def get_object(self, named_service, object_path):
        """Get a proxy object to call over the bus"""
        return ProxyObject(self, named_service, object_path)

    def add_signal_receiver(self, handler_function, signal_name=None, dbus_interface=None, named_service=None, path=None):
        match_rule = self._get_match_rule(signal_name, dbus_interface, named_service, path)

        if (not self._match_rule_to_receivers.has_key(match_rule)):
            self._match_rule_to_receivers[match_rule] = [handler_function]
	else:
	    self._match_rule_to_receivers[match_rule].append(handler_function)

        dbus_bindings.bus_add_match(self._connection, match_rule)

    def remove_signal_receiver(self, handler_function, signal_name=None, dbus_interface=None, named_service=None, path=None):
        match_rule = self._get_match_rule(signal_name, dbus_interface, named_service, path)

        if self._match_rule_to_receivers.has_key(match_rule):
            if self._match_rule_to_receivers[match_rule].__contains__(handler_function):
                self._match_rule_to_receivers[match_rule].pop(handler_function)
                dbus_bindings.bus_remove_match(self._connection, match_rule)

    def get_unix_user(self, named_service):
        """Get the unix user for the given named_service on this Bus"""
        return dbus_bindings.bus_get_unix_user(self._connection, named_service)

    def _get_match_rule(self, signal_name, dbus_interface, named_service, path):
        match_rule = "type='signal'"
        if (dbus_interface):
            match_rule = match_rule + ",interface='%s'" % (dbus_interface)
        if (named_service):
            if (named_service[0] != ':' and named_service != "org.freedesktop.DBus"):
                bus_object = self.get_object('org.freedesktop.DBus', '/org/freedesktop/DBus')
                named_service = bus_object.GetNameOwner(named_service, dbus_interface='org.freedesktop.DBus')

            match_rule = match_rule + ",sender='%s'" % (named_service)
        if (path):
            match_rule = match_rule + ",path='%s'" % (path)
        if (signal_name):
            match_rule = match_rule + ",member='%s'" % (signal_name)
        return match_rule
    
    def _signal_func(self, connection, message):
        if (message.get_type() != dbus_bindings.MESSAGE_TYPE_SIGNAL):
            return dbus_bindings.HANDLER_RESULT_NOT_YET_HANDLED
        
        dbus_interface      = message.get_interface()
        named_service  = message.get_sender()
        path           = message.get_path()
        signal_name    = message.get_member()

        match_rule = self._get_match_rule(signal_name, dbus_interface, named_service, path)

        if (self._match_rule_to_receivers.has_key(match_rule)):
            receivers = self._match_rule_to_receivers[match_rule]

            for receiver in receivers:
	        args = message.get_args_list()
		receiver(*args)

    def start_service_by_name(self, named_service):
        return dbus_bindings.bus_start_service_by_name(self._connection, named_service)

class SystemBus(Bus):
    """The system-wide message bus
    """
    def __init__(self):
        Bus.__init__(self, Bus.TYPE_SYSTEM)

class SessionBus(Bus):
    """The session (current login) message bus
    """
    def __init__(self):
        Bus.__init__(self, Bus.TYPE_SESSION)

class StarterBus(Bus):
    """The bus that activated this process (if
    this process was launched by DBus activation)
    """
    def __init__(self):
        Bus.__init__(self, Bus.TYPE_STARTER)


class Interface:
    """An inteface into a remote object

    An Interface can be used to wrap ProxyObjects
    so that calls can be routed to their correct
    dbus interface
    """

    def __init__(self, object, dbus_interface):
        self._obj = object
        self._dbus_interface = dbus_interface

    def connect_to_signal(self, signal_name, handler_function, dbus_interface = None):
        if not dbus_interface:
	    dbus_interface = self._dbus_interface
		
        self._obj.connect_to_signal(signal_name, handler_function, dbus_interface)

    def __getattr__(self, member, **keywords):
        if (keywords.has_key('dbus_interface')):
            _dbus_interface = keywords['dbus_interface']
        else:
            _dbus_interface = self._dbus_interface

        if member == '__call__':
            return object.__call__
        else:
            return self._obj.__getattr__(member, dbus_interface=_dbus_interface)

class ProxyObject:
    """A proxy to the remote Object.

    A ProxyObject is provided by the Bus. ProxyObjects
    have member functions, and can be called like normal Python objects.
    """
    def __init__(self, bus, named_service, object_path):
        self._bus          = bus
        self._named_service = named_service
        self._object_path  = object_path

    def connect_to_signal(self, signal_name, handler_function, dbus_interface=None):
        self._bus.add_signal_receiver(handler_function,
                                      signal_name=signal_name,
                                      dbus_interface=dbus_interface,
                                      named_service=self._named_service,
                                      path=self._object_path)



    def __getattr__(self, member, **keywords):
        if member == '__call__':
            return object.__call__
        else:
            iface = None
            if (keywords.has_key('dbus_interface')):
                iface = keywords['dbus_interface']

            return ProxyMethod(self._bus.get_connection(),
                                self._named_service,
                                self._object_path, iface, member)


class ProxyMethod:
    """A proxy Method.

    Typically a member of a ProxyObject. Calls to the
    method produce messages that travel over the Bus and are routed
    to a specific named Service.
    """
    def __init__(self, connection, named_service, object_path, dbus_interface, method_name):
        self._connection   = connection
        self._named_service = named_service
        self._object_path  = object_path
        self._method_name  = method_name
        self._dbus_interface = dbus_interface

    def __call__(self, *args, **keywords):
        dbus_interface = self._dbus_interface
        if (keywords.has_key('dbus_interface')):
            dbus_interface = keywords['dbus_interface']

        reply_handler = None
        if (keywords.has_key('reply_handler')):
            reply_handler = keywords['reply_handler']

        error_handler = None
        if (keywords.has_key('error_handler')):
            error_handler = keywords['error_handler']            

        if not(reply_handler and error_handler):
            if reply_handler:
                raise MissingErrorself, HandlerException()
            elif error_handler:
                raise MissingReplyHandlerException()

        message = dbus_bindings.MethodCall(self._object_path, dbus_interface, self._method_name)
        message.set_destination(self._named_service)
        
        # Add the arguments to the function
        iter = message.get_iter(True)
        for arg in args:
            iter.append(arg)

        if reply_handler:
            result = self._connection.send_with_reply_handlers(message, -1, reply_handler, error_handler)
            args_tuple = (result,)
        else:
            reply_message = self._connection.send_with_reply_and_block(message, -1)
            args_tuple = reply_message.get_args_list()
            
        if len(args_tuple) == 0:
            return
        elif len(args_tuple) == 1:
            return args_tuple[0]
        else:
            return args_tuple

class Service:
    """A base class for exporting your own Services across the Bus

    Just inherit from Service, providing the name of your service
    (e.g. org.designfu.SampleService).
    """
    def __init__(self, named_service, bus=None):
        self._named_service = named_service
                             
        if bus == None:
            # Get the default bus
            self._bus = Bus()
        else:
            self._bus = bus

        dbus_bindings.bus_request_name(self._bus.get_connection(), named_service)

    def get_bus(self):
        """Get the Bus this Service is on"""
        return self._bus

    def get_name(self):
        """Get the name of this service"""
        return self._named_service

def _dispatch_dbus_method_call(target_methods, self, argument_list, message):
    """Calls method_to_call using argument_list, but handles
    exceptions, etc, and generates a reply to the DBus Message message
    """
    try:
        target_method = None
        
        dbus_interface = message.get_interface()
        if dbus_interface == None:
            if target_methods:
                target_method = target_methods[0]
        else:
            for dbus_method in target_methods:
                if dbus_method._dbus_interface == dbus_interface:
                    target_method = dbus_method
                    break
        
        if target_method:
            retval = target_method(self, *argument_list)
        else:
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
        if retval != None:
            iter = reply.get_iter(append=True)
            iter.append(retval)
	    
    return reply

class ObjectType(type):
    def __init__(cls, name, bases, dct):

        #generate out vtable
        method_vtable = getattr(cls, '_dbus_method_vtable', {})
        reflection_data = getattr(cls, '_dbus_reflection_data', "")

        reflection_interface_method_hash = {}
        reflection_interface_signal_hash = {}

        for func in dct.values():
            if getattr(func, '_dbus_is_method', False):
                if method_vtable.has_key(func.__name__):
                    method_vtable[func.__name__].append(func)
                else:
	            method_vtable[func.__name__] = [func]
                
                #generate a hash of interfaces so we can group
                #methods in the xml data
                if reflection_interface_method_hash.has_key(func._dbus_interface):
                    reflection_interface_method_hash[func._dbus_interface].append(func)
                else:
                    reflection_interface_method_hash[func._dbus_interface] = [func]

            elif getattr(func, '_dbus_is_signal', False):
                if reflection_interface_signal_hash.has_key(func._dbus_interface):
                    reflection_interface_signal_hash[func._dbus_interface].append(func)
                else:
                    reflection_interface_signal_hash[func._dbus_interface] = [func]

	for interface in reflection_interface_method_hash.keys():
            reflection_data = reflection_data + '  <interface name="%s">\n'%(interface)
            for func in reflection_interface_method_hash[interface]:
                reflection_data = reflection_data + '    <method name="%s">\n'%(func.__name__)
                for arg in func._dbus_args:
                    reflection_data = reflection_data + '      <arg name="%s" type="v" />\n'%(arg)

                #reclaim some memory
                func._dbus_args = None
                reflection_data = reflection_data + '    </method>\n'
            if reflection_interface_signal_hash.has_key(interface):
                for func in reflection_interface_signal_hash[interface]:
                    reflection_data = reflection_data + '    <signal name="%s">\n'%(func.__name__)
                    for arg in func._dbus_args:
                        reflection_data = reflection_data + '      <arg name="%s" type="v" />\n'%(arg)
                    #reclaim some memory
                    func._dbus_args = None
                    reflection_data = reflection_data + '    </signal>\n'

                del reflection_interface_signal_hash[interface]
            reflection_data = reflection_data + '  </interface>\n'

	for interface in reflection_interface_signal_hash.keys():
            reflection_data = reflection_data + '  <interface name="%s">\n'%(interface)
            
            for func in reflection_interface_signal_hash[interface]:
                reflection_data = reflection_data + '    <signal name="%s">\n'%(func.__name__)
                for arg in func._dbus_args:
                    reflection_data = reflection_data + '      <arg name="%s" type="v" />\n'%(arg)

                #reclaim some memory
                func._dbus_args = None
                reflection_data = reflection_data + '    </signal>\n'

            reflection_data = reflection_data + '  </interface>\n'

        cls._dbus_reflection_data = reflection_data  
	cls._dbus_method_vtable = method_vtable
        
        super(ObjectType, cls).__init__(name, bases, dct)


class Object:
    """A base class for exporting your own Objects across the Bus.

    Just inherit from Object and provide a list of methods to share
    across the Bus. These will appear as member functions of your
    ServiceObject.
    """
    __metaclass__ = ObjectType
    
    def __init__(self, object_path, service):
        self._object_path = object_path
        self._service = service
        self._bus = service.get_bus()
            
        self._connection = self._bus.get_connection()

        self._connection.register_object_path(object_path, self._unregister_cb, self._message_cb)

    def _unregister_cb(self, connection):
        print ("Unregister")

    def _message_cb(self, connection, message):
        target_method_name = message.get_member()
        target_methods = self._dbus_method_vtable[target_method_name]
        args = message.get_args_list()
        
        reply = _dispatch_dbus_method_call(target_methods, self, args, message)

        self._connection.send(reply)

    @method('org.freedesktop.DBus.Introspectable')
    def Introspect(self):
        reflection_data = '<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN" "http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">\n'
        reflection_data = reflection_data + '<node name="%s">\n'%(self._object_path)
        reflection_data = reflection_data + self._dbus_reflection_data
        reflection_data = reflection_data + '</node>\n'

        return reflection_data

#Exceptions
class MissingErrorHandlerException(Exception):
    def __init__(self):
        Exception.__init__(self)


    def __str__(self):
        return "error_handler not defined: if you define a reply_handler you must also define an error_handler"


class MissingReplyHandlerException(Exception):
    def __init__(self):
        Exception.__init__(self)

    def __str__(self):
        return "reply_handler not defined: if you define an error_handler you must also define a reply_handler"

class ValidationException(Exception):
    def __init__(self, msg=''):
        self.msg = msg
        Exception.__init__(self)

    def __str__(self):
        return "Error validating string: %s" % self.msg

class UnknownMethodException(Exception):
    def __init__(self, msg=''):
        self.msg = msg
        Exception.__init__(self)

    def __str__(self):
        return "Unknown method: %s" % self.msg

ObjectPath = dbus_bindings.ObjectPath
ByteArray = dbus_bindings.ByteArray

