
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
    def __init__(self, bus_type=TYPE_SESSION, glib_mainloop=True):
        self._connection = dbus_bindings.bus_get(bus_type)

        self._connection.add_filter(self._signal_func)
        self._match_rule_to_receivers = { }
        if (glib_mainloop):
            self._connection.setup_with_g_main()

    def get_service(self, service_name="org.freedesktop.Broadcast"):
        """Get one of the RemoteServices connected to this Bus. service_name
        is just a string of the form 'com.widgetcorp.MyService'
        """
        return RemoteService(self._connection, service_name)

    def add_signal_receiver(self, receiver, interface=None, service=None, path=None):
        match_rule = self._get_match_rule(interface, service, path)
        
        if (not self._match_rule_to_receivers.has_key(match_rule)):
            self._match_rule_to_receivers[match_rule] = [ ]
        self._match_rule_to_receivers[match_rule].append(receiver)
        
        dbus_bindings.bus_add_match(self._connection, match_rule)

    def remove_signal_receiver(self, receiver, interface=None, service=None, path=None):
        match_rule = self._get_match_rule(interface, service, path)

        if self._match_rule_to_receivers.has_key(match_rule):
            if self._match_rule_to_receivers[match_rule].__contains__(receiver):
                self._match_rule_to_receivers[match_rule].remove(receiver)
                dbus_bindings.bus_remove_match(self._connection, match_rule)

    def get_connection(self):
        """Get the dbus_bindings.Connection object associated with this Bus"""
        return self._connection

    def _get_match_rule(self, interface, service, path):
##        if (interface):
##            match_rule = match_rule + ",interface='%s'" % (interface)
##        if (service):
##            match_rule = match_rule + ",service='%s'" % (service)
##        if (path):
##            match_rule = match_rule + ",path='%s'" % (path)
        # FIXME: use the service here too!!!
        return "type='signal',interface='%s',path='%s'" % (interface, path)
    
    def _signal_func(self, connection, message):
        if (message.get_type() != dbus_bindings.MESSAGE_TYPE_SIGNAL):
            return dbus_bindings.HANDLER_RESULT_NOT_YET_HANDLED
        
        interface = message.get_interface()
        service   = message.get_sender()
        path      = message.get_path()
        member    = message.get_member()

        match_rule = self._get_match_rule(interface, service, path)

        if (self._match_rule_to_receivers.has_key(match_rule)):
            receivers = self._match_rule_to_receivers[match_rule]
            args = [interface, member, service, path, message]
            for receiver in receivers:
                receiver(*args)

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
    def __init__(self, connection, service_name, object_path, interface):
        self._connection   = connection
        self._service_name = service_name
        self._object_path  = object_path
        self._interface    = interface

    def __getattr__(self, member):
        if member == '__call__':
            return object.__call__
        else:
            return RemoteMethod(self._connection, self._service_name,
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

class Object:
    """A base class for exporting your own Objects across the Bus.

    Just inherit from Object and provide a list of methods to share
    across the Bus. These will appear as member functions of your
    ServiceObject.
    """
    def __init__(self, object_path, methods_to_share, service):
        self._object_path = object_path
        self._service = service
        self._bus = service.get_bus()
        self._connection = self._bus.get_connection()

        self._method_name_to_method = self._build_method_dictionary(methods_to_share)
        
        self._connection.register_object_path(object_path, self._unregister_cb, self._message_cb)

    def broadcast_signal(self, interface, signal_name):
        message = dbus_bindings.Signal(self._object_path, interface, signal_name)
        #FIXME: need to set_sender, but it always disconnects when we do this
        #message.set_sender(self._service.get_service_name())
        self._connection.send(message)

    def _unregister_cb(self, connection):
        print ("Unregister")
        
    def _message_cb(self, connection, message):
        target_method_name = message.get_member()
        target_method = self._method_name_to_method[target_method_name]
        args = message.get_args_list()
        
        try:
            retval = target_method(*args)
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
                
        self._connection.send(reply)

    def _build_method_dictionary(self, methods):
        method_dict = {}
        for method in methods:
            if method_dict.has_key(method.__name__):
                print ('WARNING: registering DBus Object methods, already have a method named %s' % (method.__name__))
            method_dict[method.__name__] = method
        return method_dict

class RemoteService:
    """A remote service providing objects.

    A service is typically a process or application that provides
    remote objects, but can also be the broadcast service that
    receives signals from all applications on the Bus.
    """
    
    def __init__(self, connection, service_name):
        self._connection     = connection
        self._service_name   = service_name

    def get_object(self, object_path, interface):
        """Get an object provided by this Service that implements a
        particular interface. object_path is a string of the form
        '/com/widgetcorp/MyService/MyObject1'. interface looks a lot
        like a service_name (they're often the same) and is of the form,
        'com.widgetcorp.MyInterface', and mostly just defines the
        set of member functions that will be present in the object.
        """
        return RemoteObject(self._connection, self._service_name, object_path, interface)
                             
