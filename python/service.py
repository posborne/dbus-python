from decorators import *

import dbus_bindings 

class BusName:
    """A base class for exporting your own Named Services across the Bus
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
                reflection_data = reflection_data + cls._reflect_on_method(func)

            if reflection_interface_signal_hash.has_key(interface):
                for func in reflection_interface_signal_hash[interface]:
                    reflection_data = reflection_data + cls._reflect_on_signal(func)

                del reflection_interface_signal_hash[interface]
                
            reflection_data = reflection_data + '  </interface>\n'

	for interface in reflection_interface_signal_hash.keys():
            reflection_data = reflection_data + '  <interface name="%s">\n'%(interface)
            
            for func in reflection_interface_signal_hash[interface]:
                reflection_data = reflection_data + cls._reflect_on_signal(func)

            reflection_data = reflection_data + '  </interface>\n'

        cls._dbus_reflection_data = reflection_data  
	cls._dbus_method_vtable = method_vtable
        
        super(ObjectType, cls).__init__(name, bases, dct)

    #reflections on methods and signals may look like similar code but may in fact
    #diverge in the future so keep them seperate
    def _reflect_on_method(cls, func):
        reflection_data = '    <method name="%s">\n'%(func.__name__)
        for arg in func._dbus_args:
            reflection_data = reflection_data + '      <arg name="%s" type="v" />\n'%(arg)

            #reclaim some memory
            func._dbus_args = None
            reflection_data = reflection_data + '    </method>\n'

        return reflection_data  
             
    def _reflect_on_signal(cls, func):
        reflection_data = '    <signal name="%s">\n'%(func.__name__)
        for arg in func._dbus_args:
            reflection_data = reflection_data + '      <arg name="%s" type="v" />\n'%(arg)
            #reclaim some memory
            func._dbus_args = None
            reflection_data = reflection_data + '    </signal>\n'

        return reflection_data

class Object:
    """A base class for exporting your own Objects across the Bus.

    Just inherit from Object and provide a list of methods to share
    across the Bus
    """
    __metaclass__ = ObjectType
    
    def __init__(self, bus_name, object_path):
        self._object_path = object_path
        self._name = bus_name 
        self._bus = bus_name.get_bus()
            
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

