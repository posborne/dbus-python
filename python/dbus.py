
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

_threads_initialized = 0
def init_gthreads ():
    global _threads_initialized
    if not _threads_initialized:
        dbus_bindings.init_gthreads ()
        _threads_initialized = 1
    
class Bus:
    """A connection to a DBus daemon.

    One of three possible standard buses, the SESSION, SYSTEM,
    or ACTIVATION bus
    """
    TYPE_SESSION    = dbus_bindings.BUS_SESSION
    TYPE_SYSTEM     = dbus_bindings.BUS_SYSTEM
    TYPE_ACTIVATION = dbus_bindings.BUS_ACTIVATION

    """bus_type=[Bus.TYPE_SESSION | Bus.TYPE_SYSTEM | Bus.TYPE_ACTIVATION]
    """

    ACTIVATION_REPLY_ACTIVATED = dbus_bindings.ACTIVATION_REPLY_ACTIVATED
    ACTIVATION_REPLY_ALREADY_ACTIVE = dbus_bindings.ACTIVATION_REPLY_ALREADY_ACTIVE 

    def __init__(self, bus_type=TYPE_SESSION, glib_mainloop=True):
        self._connection = dbus_bindings.bus_get(bus_type)

        self._connection.add_filter(self._signal_func)
        self._match_rule_to_receivers = { }
        if (glib_mainloop):
            self._connection.setup_with_g_main()

    def get_connection(self):
        return self._connection

    def get_service(self, service_name="org.freedesktop.Broadcast"):
        """Get one of the RemoteServices connected to this Bus. service_name
        is just a string of the form 'com.widgetcorp.MyService'
        """
        return RemoteService(self, service_name)

    def add_signal_receiver(self, handler_function, signal_name=None, interface=None, service=None, path=None):
        match_rule = self._get_match_rule(signal_name, interface, service, path)
        
        if (not self._match_rule_to_receivers.has_key(match_rule)):
            self._match_rule_to_receivers[match_rule] = [ ]
        self._match_rule_to_receivers[match_rule].append(handler_function)
        
        dbus_bindings.bus_add_match(self._connection, match_rule)

    def remove_signal_receiver(self, handler_function, signal_name=None, interface=None, service=None, path=None):
        match_rule = self._get_match_rule(signal_name, interface, service, path)

        if self._match_rule_to_receivers.has_key(match_rule):
            if self._match_rule_to_receivers[match_rule].__contains__(handler_function):
                self._match_rule_to_receivers[match_rule].remove(handler_function)
                dbus_bindings.bus_remove_match(self._connection, match_rule)

    def get_connection(self):
        """Get the dbus_bindings.Connection object associated with this Bus"""
        return self._connection
    
    def get_unix_user(self, service_name):
        """Get the unix user for the given service_name on this Bus"""
        return dbus_bindings.bus_get_unix_user(self._connection, service_name)

    def _get_match_rule(self, signal_name, interface, service, path):
        match_rule = "type='signal'"
        if (interface):
            match_rule = match_rule + ",interface='%s'" % (interface)
        if (service):
            if (service[0] != ':' and service != "org.freedesktop.DBus"):
                bus_service = self.get_service("org.freedesktop.DBus")
                bus_object = bus_service.get_object('/org/freedesktop/DBus',
                                                     'org.freedesktop.DBus')
                service = bus_object.GetServiceOwner(service)

            match_rule = match_rule + ",sender='%s'" % (service)
        if (path):
            match_rule = match_rule + ",path='%s'" % (path)
        if (signal_name):
            match_rule = match_rule + ",member='%s'" % (signal_name)
        return match_rule
    
    def _signal_func(self, connection, message):
        if (message.get_type() != dbus_bindings.MESSAGE_TYPE_SIGNAL):
            return dbus_bindings.HANDLER_RESULT_NOT_YET_HANDLED
        
        interface = message.get_interface()
        service   = message.get_sender()
        path      = message.get_path()
        member    = message.get_member()

        match_rule = self._get_match_rule(member, interface, service, path)

        if (self._match_rule_to_receivers.has_key(match_rule)):
            receivers = self._match_rule_to_receivers[match_rule]
            args = [interface, member, service, path, message]
            for receiver in receivers:
                receiver(*args)

    def activate_service(self, service):
        return dbus_bindings.bus_activate_service(self._connection, service)

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

class ActivationBus(Bus):
    """The bus that activated this process (if
    this process was launched by DBus activation)
    """
    def __init__(self):
        Bus.__init__(self, Bus.TYPE_ACTIVATION)


class RemoteObject:
    """A remote Object.

    A RemoteObject is provided by a RemoteService on a particular Bus. RemoteObjects
    have member functions, and can be called like normal Python objects.
    """
    def __init__(self, service, object_path, interface):
        self._service      = service
        self._object_path  = object_path
        self._interface    = interface

    def connect_to_signal(self, signal_name, handler_function):
        self._service.get_bus().add_signal_receiver(handler_function,
                                                    signal_name=signal_name,
                                                    interface=self._interface,
                                                    service=self._service.get_service_name(),
                                                    path=self._object_path)

    def __getattr__(self, member):
        if member == '__call__':
            return object.__call__
        else:
            return RemoteMethod(self._service.get_bus().get_connection(),
                                self._service.get_service_name(),
                                self._object_path, self._interface, member)


class RemoteMethod:
    """A remote Method.

    Typically a member of a RemoteObject. Calls to the
    method produce messages that travel over the Bus and are routed
    to a specific Service.
    """
    def __init__(self, connection, service_name, object_path, interface, method_name):
        self._connection   = connection
        self._service_name = service_name
        self._object_path  = object_path
        self._interface    = interface
        self._method_name  = method_name

    def __call__(self, *args):
        message = dbus_bindings.MethodCall(self._object_path, self._interface, self._method_name)
        message.set_destination(self._service_name)
        
        # Add the arguments to the function
        iter = message.get_iter()
        for arg in args:
            iter.append(arg)

        reply_message = self._connection.send_with_reply_and_block(message, 5000)

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
    def __init__(self, service_name, bus=None):
        self._service_name = service_name
                             
        if bus == None:
            # Get the default bus
            self._bus = Bus()
        else:
            self._bus = bus

        dbus_bindings.bus_acquire_service(self._bus.get_connection(), service_name)

    def get_bus(self):
        """Get the Bus this Service is on"""
        return self._bus

    def get_service_name(self):
        """Get the name of this service"""
        return self._service_name

def _dispatch_dbus_method_call(target_method, argument_list, message):
    """Calls method_to_call using argument_list, but handles
    exceptions, etc, and generates a reply to the DBus Message message
    """
    try:
        retval = target_method(message, *argument_list)
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
            iter = reply.get_iter()
            iter.append(retval)

    return reply

def _build_method_dictionary(methods):
    method_dict = {}
    for method in methods:
        if method_dict.has_key(method.__name__):
            print ('WARNING: registering DBus Object methods, already have a method named %s' % (method.__name__))
        method_dict[method.__name__] = method
    return method_dict

class Object:
    """A base class for exporting your own Objects across the Bus.

    Just inherit from Object and provide a list of methods to share
    across the Bus. These will appear as member functions of your
    ServiceObject.
    """
    def __init__(self, object_path, service, dbus_methods=[]):
        # Reversed constructor argument order. Add a temporary
        # check to help people get things straightened out with minimal pain.
        if type(service) == list:
            raise TypeError, "dbus.Object.__init__(): the order of the 'service' and 'dbus_methods' arguments has been reversed (for consistency with dbus.ObjectTree)."
        
        self._object_path = object_path
        self._service = service
        self._bus = service.get_bus()
        self._connection = self._bus.get_connection()

        self._method_name_to_method = _build_method_dictionary(dbus_methods)
        
        self._connection.register_object_path(object_path, self._unregister_cb, self._message_cb)

    def emit_signal(self, interface, signal_name, *args):
        message = dbus_bindings.Signal(self._object_path, interface, signal_name)
        iter = message.get_iter()
        for arg in args:
            iter.append(arg)
        
        self._connection.send(message)

    def _unregister_cb(self, connection):
        print ("Unregister")

    def _message_cb(self, connection, message):
        target_method_name = message.get_member()
        target_method = self._method_name_to_method[target_method_name]
        args = message.get_args_list()

        reply = _dispatch_dbus_method_call(target_method, args, message)
        
        self._connection.send(reply)



class ObjectTree:
    """An object tree allows you to register a handler for a tree of object paths.
    This means that literal Python objects do not need to be created for each object
    over the bus, but you can have a virtual tree of objects handled by a single
    Python object. There are two ways to handle method calls on virtual objects:

    1) Pass a list of dbus_methods in to __init__. This works just like dbus.Object,
    except an object_path is passed as the first argument to each method, denoting which
    virtual object the call was made on. If all the objects in the tree support the same
    methods, this is the best approach.

    2) Override object_method_called. This allows you to define the valid methods dynamically
    on an object by object basis. For example, if providing an object tree that represented
    a filesystem heirarchy, you'd only want an ls method on directory objects, not file objects.
    """

    def __init__(self, base_path, service, dbus_methods=[]):
        self._base_path = base_path
        self._service = service
        self._bus = service.get_bus()
        self._connection = self._bus.get_connection()

        self._method_name_to_method = _build_method_dictionary(dbus_methods)
        
        self._connection.register_fallback(base_path, self._unregister_cb, self._message_cb)

    def relative_path_to_object_path(self, relative_path):
        return ObjectPath(self._base_path + relative_path)
        
    def broadcast_signal(self, interface, signal_name, relative_path):
        object_path = self.relative_path_to_object_path(relative_path)
        message = dbus_bindings.Signal(object_path, interface, signal_name)
        self._connection.send(message)
        
    def object_method_called(self, message, relative_path, method_name, argument_list):
        """Override this method. Called with, object_path, the relative path of the object
        under the base_path, the name of the method invoked, and a list of arguments
        """
        raise NotImplementedException, "object_method_called() must be overriden"

    def _unregister_cb(self, connection):
        print ("Unregister")

    def _message_cb(self, connection, message):
        target_object_full_path = message.get_path()
        assert(self._base_path == target_object_full_path[:len(self._base_path)])
        target_object_path = target_object_full_path[len(self._base_path):]
        target_method_name = message.get_member()        
        message_args = message.get_args_list()

        try:
            target_method = self._method_name_to_method[target_method_name]
            args = [target_object_path] + message_args
        except KeyError:
            target_method = self.object_method_called
            args = [target_object_path, target_method_name, message_args]

        reply = _dispatch_dbus_method_call(target_method, args, message)

        self._connection.send(reply)
        
class RemoteService:
    """A remote service providing objects.

    A service is typically a process or application that provides
    remote objects, but can also be the broadcast service that
    receives signals from all applications on the Bus.
    """
    
    def __init__(self, bus, service_name):
        self._bus            = bus
        self._service_name   = service_name

    def get_bus(self):
        return self._bus

    def get_service_name(self):
        return self._service_name

    def get_object(self, object_path, interface):
        """Get an object provided by this Service that implements a
        particular interface. object_path is a string of the form
        '/com/widgetcorp/MyService/MyObject1'. interface looks a lot
        like a service_name (they're often the same) and is of the form,
        'com.widgetcorp.MyInterface', and mostly just defines the
        set of member functions that will be present in the object.
        """
        return RemoteObject(self, object_path, interface)
                             
ObjectPath = dbus_bindings.ObjectPath
ByteArray = dbus_bindings.ByteArray
