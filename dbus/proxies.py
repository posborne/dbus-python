# Copyright (C) 2003, 2004, 2005, 2006, 2007 Red Hat Inc. <http://www.redhat.com/>
# Copyright (C) 2003 David Zeuthen
# Copyright (C) 2004 Rob Taylor
# Copyright (C) 2005, 2006, 2007 Collabora Ltd. <http://www.collabora.co.uk/>
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

try:
    from threading import RLock
except ImportError:
    from dummy_threading import RLock

import _dbus_bindings
from dbus._expat_introspect_parser import process_introspection_data
from dbus.exceptions import MissingReplyHandlerException, MissingErrorHandlerException, IntrospectionParserException, DBusException

__docformat__ = 'restructuredtext'


_logger = logging.getLogger('dbus.proxies')


BUS_DAEMON_NAME = 'org.freedesktop.DBus'
BUS_DAEMON_PATH = '/org/freedesktop/DBus'
BUS_DAEMON_IFACE = BUS_DAEMON_NAME


class _ReplyHandler(object):
    __slots__ = ('_on_error', '_on_reply', '_get_args_options')
    def __init__(self, on_reply, on_error, **get_args_options):
        self._on_error = on_error
        self._on_reply = on_reply
        self._get_args_options = get_args_options

    def __call__(self, message):
        if isinstance(message, _dbus_bindings.MethodReturnMessage):
            self._on_reply(*message.get_args_list(**self._get_args_options))
        elif isinstance(message, _dbus_bindings.ErrorMessage):
            args = message.get_args_list()
            if len(args) > 0:
                self._on_error(DBusException(args[0]))
            else:
                self._on_error(DBusException())
        else:
            self._on_error(DBusException('Unexpected reply message type: %s'
                                        % message))


class _DeferredMethod:
    """A proxy method which will only get called once we have its
    introspection reply.
    """
    def __init__(self, proxy_method, append, block):
        self._proxy_method = proxy_method
        # the test suite relies on the existence of this property
        self._method_name = proxy_method._method_name
        self._append = append
        self._block = block

    def __call__(self, *args, **keywords):
        if keywords.has_key('reply_handler'):
            # defer the async call til introspection finishes
            self._append(self._proxy_method, args, keywords)
            return None
        else:
            # we're being synchronous, so block
            self._block()
            return self._proxy_method(*args, **keywords)


class _ProxyMethod:
    """A proxy method.

    Typically a member of a ProxyObject. Calls to the
    method produce messages that travel over the Bus and are routed
    to a specific named Service.
    """
    def __init__(self, proxy, connection, named_service, object_path, method_name, iface):
        self._proxy          = proxy
        self._connection     = connection
        self._named_service  = named_service
        self._object_path    = object_path
        # the test suite relies on the existence of this property
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

        get_args_options = {}
        if keywords.has_key('utf8_strings'):
            get_args_options['utf8_strings'] = keywords['utf8_strings']
        if keywords.has_key('byte_arrays'):
            get_args_options['byte_arrays'] = keywords['byte_arrays']

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

        if ignore_reply:
            self._connection.send_message(message)
            return None
        elif reply_handler:
            self._connection.send_message_with_reply(message, _ReplyHandler(reply_handler, error_handler, **get_args_options), timeout/1000.0, require_main_loop=1)
            return None
        else:
            reply_message = self._connection.send_message_with_reply_and_block(message, timeout)
            args_list = reply_message.get_args_list(**get_args_options)
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
    ProxyMethodClass = _ProxyMethod
    DeferredMethodClass = _DeferredMethod

    INTROSPECT_STATE_DONT_INTROSPECT = 0
    INTROSPECT_STATE_INTROSPECT_IN_PROGRESS = 1
    INTROSPECT_STATE_INTROSPECT_DONE = 2

    def __init__(self, bus, named_service, object_path, introspect=True,
                 follow_name_owner_changes=False):
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
            `follow_name_owner_changes` : bool
                If true (default is false) and the `named_service` is a
                well-known name, follow ownership changes for that name
        """
        if follow_name_owner_changes:
            bus._require_main_loop()   # we don't get the signals otherwise

        self._bus           = bus
        self._named_service = named_service
        self.__dbus_object_path__ = object_path

        if (named_service[:1] != ':' and named_service != BUS_DAEMON_NAME
            and not follow_name_owner_changes):
            bus_object = bus.get_object(BUS_DAEMON_NAME, BUS_DAEMON_PATH)
            try:
                self._named_service = bus_object.GetNameOwner(named_service,
                        dbus_interface=BUS_DAEMON_IFACE)
            except DBusException, e:
                # FIXME: detect whether it's NameHasNoOwner, but properly
                #if not str(e).startswith('org.freedesktop.DBus.Error.NameHasNoOwner:'):
                #    raise
                # it might not exist: try to start it
                bus_object.StartServiceByName(named_service,
                                              _dbus_bindings.UInt32(0))
                self._named_service = bus_object.GetNameOwner(named_service,
                        dbus_interface=BUS_DAEMON_IFACE)

        #PendingCall object for Introspect call
        self._pending_introspect = None
        #queue of async calls waiting on the Introspect to return 
        self._pending_introspect_queue = []
        #dictionary mapping method names to their input signatures
        self._introspect_method_map = {}

        # must be a recursive lock because block() is called while locked,
        # and calls the callback which re-takes the lock
        self._introspect_lock = RLock()

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
                A function to be called when the signal is emitted by
                the remote object. Its positional arguments will be the
                arguments of the signal; optionally, it may be given
                keyword arguments as described below.
            `dbus_interface` : str
                Optional interface with which to qualify the signal name.
                If None (the default) the handler will be called whenever a
                signal of the given member name is received, whatever
                its interface.
        :Keywords:
            `utf8_strings` : bool
                If True, the handler function will receive any string
                arguments as dbus.UTF8String objects (a subclass of str
                guaranteed to be UTF-8). If False (default) it will receive
                any string arguments as dbus.String objects (a subclass of
                unicode).
            `byte_arrays` : bool
                If True, the handler function will receive any byte-array
                arguments as dbus.ByteArray objects (a subclass of str).
                If False (default) it will receive any byte-array
                arguments as a dbus.Array of dbus.Byte (subclasses of:
                a list of ints).
            `sender_keyword` : str
                If not None (the default), the handler function will receive
                the unique name of the sending endpoint as a keyword
                argument with this name
            `destination_keyword` : str
                If not None (the default), the handler function will receive
                the bus name of the destination (or None if the signal is a
                broadcast, as is usual) as a keyword argument with this name.
            `interface_keyword` : str
                If not None (the default), the handler function will receive
                the signal interface as a keyword argument with this name.
            `member_keyword` : str
                If not None (the default), the handler function will receive
                the signal name as a keyword argument with this name.
            `path_keyword` : str
                If not None (the default), the handler function will receive
                the object-path of the sending object as a keyword argument
                with this name
            `message_keyword` : str
                If not None (the default), the handler function will receive
                the `dbus.lowlevel.SignalMessage` as a keyword argument with
                this name.
            `arg...` : unicode or UTF-8 str
                If there are additional keyword parameters of the form
                ``arg``\ *n*, match only signals where the *n*\ th argument
                is the value given for that keyword parameter. As of this time
                only string arguments can be matched (in particular,
                object paths and signatures can't).
        """
        return \
        self._bus.add_signal_receiver(handler_function,
                                      signal_name=signal_name,
                                      dbus_interface=dbus_interface,
                                      named_service=self._named_service,
                                      path=self.__dbus_object_path__,
                                      **keywords)

    def _Introspect(self):
        message = _dbus_bindings.MethodCallMessage(None, self.__dbus_object_path__, 'org.freedesktop.DBus.Introspectable', 'Introspect')
        message.set_destination(self._named_service)
        
        result = self._bus.get_connection().send_message_with_reply(message, _ReplyHandler(self._introspect_reply_handler, self._introspect_error_handler, utf8_strings=True), -1)
        return result
    
    def _introspect_execute_queue(self):
        # FIXME: potential to flood the bus
        # We should make sure mainloops all have idle handlers
        # and do one message per idle
        for (proxy_method, args, keywords) in self._pending_introspect_queue:
            proxy_method(*args, **keywords)

    def _introspect_reply_handler(self, data):
        self._introspect_lock.acquire()
        try:
            try:
                self._introspect_method_map = process_introspection_data(data)
            except IntrospectionParserException, e:
                self._introspect_error_handler(e)
                return

            self._introspect_state = self.INTROSPECT_STATE_INTROSPECT_DONE
            self._pending_introspect = None
            self._introspect_execute_queue()
        finally:
            self._introspect_lock.release()

    def _introspect_error_handler(self, error):
        self._introspect_lock.acquire()
        try:
            self._introspect_state = self.INTROSPECT_STATE_DONT_INTROSPECT
            self._pending_introspect = None
            self._introspect_execute_queue()
            sys.stderr.write("Introspect error: " + str(error) + "\n")
        finally:
            self._introspect_lock.release()

    def _introspect_block(self):
        self._introspect_lock.acquire()
        try:
            if self._pending_introspect is not None:
                self._pending_introspect.block()
            # else someone still has a _DeferredMethod from before we
            # finished introspection: no need to do anything special any more
        finally:
            self._introspect_lock.release()

    def _introspect_add_to_queue(self, callback, args, kwargs):
        self._introspect_lock.acquire()
        try:
            if self._introspect_state == self.INTROSPECT_STATE_INTROSPECT_IN_PROGRESS:
                self._pending_introspect_queue.append((callback, args, kwargs))
            else:
                # someone still has a _DeferredMethod from before we
                # finished introspection
                callback(*args, **kwargs)
        finally:
            self._introspect_lock.release()

    def __getattr__(self, member, dbus_interface=None):
        if member == '__call__':
            return object.__call__
        elif member.startswith('__') and member.endswith('__'):
            raise AttributeError(member)
        else:
            return self.get_dbus_method(member, dbus_interface)

    def get_dbus_method(self, member, dbus_interface=None):
        """Return a proxy method representing the given D-Bus method. The
        returned proxy method can be called in the usual way. For instance, ::

            proxy.get_dbus_method("Foo", dbus_interface='com.example.Bar')(123)

        is equivalent to::

            proxy.Foo(123, dbus_interface='com.example.Bar')

        or even::

            getattr(proxy, "Foo")(123, dbus_interface='com.example.Bar')

        However, using `get_dbus_method` is the only way to call D-Bus
        methods with certain awkward names - if the author of a service
        implements a method called ``connect_to_signal`` or even
        ``__getattr__``, you'll need to use `get_dbus_method` to call them.

        For services which follow the D-Bus convention of CamelCaseMethodNames
        this won't be a problem.
        """

        ret = self.ProxyMethodClass(self, self._bus.get_connection(),
                                    self._named_service,
                                    self.__dbus_object_path__, member,
                                    dbus_interface)

        # this can be done without taking the lock - the worst that can
        # happen is that we accidentally return a _DeferredMethod just after
        # finishing introspection, in which case _introspect_add_to_queue and
        # _introspect_block will do the right thing anyway
        if self._introspect_state == self.INTROSPECT_STATE_INTROSPECT_IN_PROGRESS:
            ret = self.DeferredMethodClass(ret, self._introspect_add_to_queue,
                                           self._introspect_block)

        return ret

    def __repr__(self):
        return '<ProxyObject wrapping %s %s %s at %#x>'%(
            self._bus, self._named_service, self.__dbus_object_path__, id(self))
    __str__ = __repr__

