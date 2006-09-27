# Copyright (C) 2003, 2004 Seth Nickell
# Copyright (C) 2003 David Zeuthen
# Copyright (C) 2004 Rob Taylor
# Copyright (C) 2004, 2005, 2006 Red Hat Inc. <http://www.redhat.com/>
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

import sys
import logging

import _dbus_bindings
import dbus.introspect_parser as introspect_parser
from dbus.exceptions import MissingReplyHandlerException, MissingErrorHandlerException, IntrospectionParserException, DBusException

__docformat__ = 'restructuredtext'


_logger = logging.getLogger('dbus.proxies')


class _ReplyHandler(object):
    __slots__ = ('_on_error', '_on_reply')
    def __init__(self, on_reply, on_error):
        self._on_error = on_error
        self._on_reply = on_reply

    def __call__(self, message):
        if isinstance(message, _dbus_bindings.MethodReturnMessage):
            self._on_reply(*message.get_args_list())
        elif isinstance(message, _dbus_bindings.ErrorMessage):
            args = message.get_args_list()
            if len(args) > 0:
                self._on_error(DBusException(args[0]))
            else:
                self._on_error(DBusException())
        else:
            self._on_error(DBusException('Unexpected reply message type: %s'
                                        % message))


class DeferedMethod:
    """A DeferedMethod
    
    This is returned instead of ProxyMethod when we are defering DBus calls
    while waiting for introspection data to be returned
    """
    def __init__(self, proxy_method):
        self._proxy_method = proxy_method
        self._method_name  = proxy_method._method_name
    
    def __call__(self, *args, **keywords):
        reply_handler = None
        if keywords.has_key('reply_handler'):
            reply_handler = keywords['reply_handler']

        #block for now even on async
        # FIXME: put ret in async queue in future if we have a reply handler

        self._proxy_method._proxy._pending_introspect._block()
        ret = self._proxy_method (*args, **keywords)
        
        return ret

class ProxyMethod:
    """A proxy Method.

    Typically a member of a ProxyObject. Calls to the
    method produce messages that travel over the Bus and are routed
    to a specific named Service.
    """
    def __init__(self, proxy, connection, named_service, object_path, method_name, iface):
        self._proxy          = proxy
        self._connection     = connection
        self._named_service  = named_service
        self._object_path    = object_path
        self._method_name    = method_name
        self._dbus_interface = iface

    def __call__(self, *args, **keywords):
        timeout = -1
        if keywords.has_key('timeout'):
            timeout = keywords['timeout']

        reply_handler = None
        if keywords.has_key('reply_handler'):
            reply_handler = keywords['reply_handler']

        error_handler = None
        if keywords.has_key('error_handler'):
            error_handler = keywords['error_handler']            

        ignore_reply = False
        if keywords.has_key('ignore_reply'):
            ignore_reply = keywords['ignore_reply']


        if not(reply_handler and error_handler):
            if reply_handler:
                raise MissingErrorHandlerException()
            elif error_handler:
                raise MissingReplyHandlerException()

        dbus_interface = self._dbus_interface 
        if keywords.has_key('dbus_interface'):
            dbus_interface = keywords['dbus_interface']

        tmp_iface = ''
        if dbus_interface:
	    tmp_iface = dbus_interface + '.'

        key = tmp_iface + self._method_name

        introspect_sig = None
        if self._proxy._introspect_method_map.has_key (key):
            introspect_sig = self._proxy._introspect_method_map[key]

        message = _dbus_bindings.MethodCallMessage(destination=None,
                                                   path=self._object_path,
                                                   interface=dbus_interface,
                                                   method=self._method_name)
        message.set_destination(self._named_service)
        
        # Add the arguments to the function
        try:
            message.append(signature=introspect_sig, *args)
        except Exception, e:
            _logger.error('Unable to set arguments %r according to '
                          'introspected signature %r: %s: %s',
                          args, introspect_sig, e.__class__, e)
            raise

        # FIXME: using private API on Connection while I decide whether to
        # make it public or what
        if ignore_reply:
            result = self._connection._send(message)
            return None
        elif reply_handler:
            result = self._connection._send_with_reply(message, _ReplyHandler(reply_handler, error_handler), timeout/1000.0)
            return None
        else:
            reply_message = self._connection._send_with_reply_and_block(message, timeout)
            args_list = reply_message.get_args_list()
            if len(args_list) == 0:
                return None
            elif len(args_list) == 1:
                return args_list[0]
            else:
                return tuple(args_list)


class ProxyObject:
    """A proxy to the remote Object.

    A ProxyObject is provided by the Bus. ProxyObjects
    have member functions, and can be called like normal Python objects.
    """
    ProxyMethodClass = ProxyMethod
    DeferedMethodClass = DeferedMethod

    INTROSPECT_STATE_DONT_INTROSPECT = 0
    INTROSPECT_STATE_INTROSPECT_IN_PROGRESS = 1
    INTROSPECT_STATE_INTROSPECT_DONE = 2

    def __init__(self, bus, named_service, object_path, introspect=True):
        """Initialize the proxy object.

        :Parameters:
            `bus` : `dbus.Bus`
                The bus on which to find this object
            `named_service` : str
                A bus name for the endpoint owning the object (need not
                actually be a service name)
            `object_path` : str
                The object path at which the endpoint exports the object
            `introspect` : bool
                If true (default), attempt to introspect the remote
                object to find out supported methods and their signatures
        """
        self._bus           = bus
        self._named_service = named_service
        self._object_path   = object_path

        #PendingCall object for Introspect call
        self._pending_introspect = None
        #queue of async calls waiting on the Introspect to return 
        self._pending_introspect_queue = []
        #dictionary mapping method names to their input signatures
        self._introspect_method_map = {}

        if not introspect:
            self._introspect_state = self.INTROSPECT_STATE_DONT_INTROSPECT
        else:
            self._introspect_state = self.INTROSPECT_STATE_INTROSPECT_IN_PROGRESS
            
            self._pending_introspect = self._Introspect()
            

    def connect_to_signal(self, signal_name, handler_function, dbus_interface=None, **keywords):
        """Arrange for the given function to be called when the given signal
        is received.

        :Parameters:
            `signal_name` : str
                The name of the signal
            `handler_function` : callable
                A function to be called (FIXME arguments?) when the signal
                is emitted by the remote object.
            `dbus_interface` : str
                Optional interface with which to qualify the signal name.
                The default is to use the interface this Interface represents.
            `sender_keyword` : str
                If not None (the default), the handler function will receive
                the unique name of the sending endpoint as a keyword
                argument with this name
            `path_keyword` : str
                If not None (the default), the handler function will receive
                the object-path of the sending object as a keyword argument
                with this name
            `keywords`
                If there are additional keyword parameters of the form
                ``arg``\ *n*, match only signals where the *n*\ th argument
                is the value given for that keyword parameter
        """
        self._bus.add_signal_receiver(handler_function,
                                      signal_name=signal_name,
                                      dbus_interface=dbus_interface,
                                      named_service=self._named_service,
                                      path=self._object_path,
                                      **keywords)

    def _Introspect(self):
        message = _dbus_bindings.MethodCallMessage(None, self._object_path, 'org.freedesktop.DBus.Introspectable', 'Introspect')
        message.set_destination(self._named_service)
        
        result = self._bus.get_connection()._send_with_reply(message, _ReplyHandler(self._introspect_reply_handler, self._introspect_error_handler), -1)
        return result   
    
    def _introspect_execute_queue(self): 
        for call in self._pending_introspect_queue:
            (member, iface, args, keywords) = call

            introspect_sig = None

            tmp_iface = ''
            if iface:
                tmp_iface = iface + '.'
                    
            key = tmp_iface + '.' + member
            if self._introspect_method_map.has_key (key):
                introspect_sig = self._introspect_method_map[key]

            
            call_object = self.ProxyMethodClass(self._bus.get_connection(),
                                                self._named_service,
                                                self._object_path, 
                                                iface, 
                                                member,
                                                introspect_sig)
                                                                       
            call_object(args, keywords)

    def _introspect_reply_handler(self, data):
        try:
            self._introspect_method_map = introspect_parser.process_introspection_data(data)
        except IntrospectionParserException, e:
            self._introspect_error_handler(e)
            return
        
        self._introspect_state = self.INTROSPECT_STATE_INTROSPECT_DONE
        #self._introspect_execute_queue()

    def _introspect_error_handler(self, error):
        self._introspect_state = self.INTROSPECT_STATE_DONT_INTROSPECT
        self._introspect_execute_queue()
        sys.stderr.write("Introspect error: " + str(error) + "\n")

    def __getattr__(self, member, dbus_interface=None):
        if member == '__call__':
            return object.__call__
        elif member.startswith('__') and member.endswith('__'):
            raise AttributeError(member)
        else:
            ret = self.ProxyMethodClass(self, self._bus.get_connection(),
                                        self._named_service,
                                        self._object_path, member, 
                                        dbus_interface)
        
            if self._introspect_state == self.INTROSPECT_STATE_INTROSPECT_IN_PROGRESS:
                ret = self.DeferedMethodClass(ret)
                    
            return ret

    def __repr__(self):
        return '<ProxyObject wrapping %s %s %s at %#x>'%(
            self._bus, self._named_service, self._object_path , id(self))
    __str__ = __repr__

