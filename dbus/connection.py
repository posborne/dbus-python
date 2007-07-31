# Copyright (C) 2007 Collabora Ltd. <http://www.collabora.co.uk/>
#
# Licensed under the Academic Free License version 2.1
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation; either version 2.1 of the License, or
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

__all__ = ('Connection', 'SignalMatch')
__docformat__ = 'reStructuredText'

import logging
try:
    import thread
except ImportError:
    import dummy_thread as thread
import weakref

from _dbus_bindings import Connection as _Connection, \
                           LOCAL_PATH, LOCAL_IFACE, PEER_IFACE, \
                           validate_interface_name, validate_member_name,\
                           validate_bus_name, validate_object_path,\
                           validate_error_name
from _dbus_bindings import DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE \
        as _INTROSPECT_DOCTYPE
from dbus.data import UTF8String, ObjectPath
from dbus.exceptions import DBusException
from dbus.lowlevel import ErrorMessage, MethodCallMessage, SignalMessage, \
                          MethodReturnMessage, \
                          HANDLER_RESULT_NOT_YET_HANDLED, \
                          HANDLER_RESULT_HANDLED
from dbus.proxies import ProxyObject


_logger = logging.getLogger('dbus.connection')


_ROOT = ObjectPath('/')


def _noop(*args, **kwargs):
    pass


class SignalMatch(object):
    __slots__ = ('_sender_name_owner', '_member', '_interface', '_sender',
                 '_path', '_handler', '_args_match', '_rule',
                 '_utf8_strings', '_byte_arrays', '_conn_weakref',
                 '_destination_keyword', '_interface_keyword',
                 '_message_keyword', '_member_keyword',
                 '_sender_keyword', '_path_keyword', '_int_args_match')

    def __init__(self, conn, sender, object_path, dbus_interface,
                 member, handler, utf8_strings=False, byte_arrays=False,
                 sender_keyword=None, path_keyword=None,
                 interface_keyword=None, member_keyword=None,
                 message_keyword=None, destination_keyword=None,
                 **kwargs):
        if member is not None:
            validate_member_name(member)
        if dbus_interface is not None:
            validate_interface_name(dbus_interface)
        if sender is not None:
            validate_bus_name(sender)
        if object_path is not None:
            validate_object_path(object_path)

        self._rule = None
        self._conn_weakref = weakref.ref(conn)
        self._sender = sender
        self._interface = dbus_interface
        self._member = member
        self._path = object_path
        self._handler = handler

        # if the connection is actually a bus, it's responsible for changing
        # this later
        self._sender_name_owner = sender

        self._utf8_strings = utf8_strings
        self._byte_arrays = byte_arrays
        self._sender_keyword = sender_keyword
        self._path_keyword = path_keyword
        self._member_keyword = member_keyword
        self._interface_keyword = interface_keyword
        self._message_keyword = message_keyword
        self._destination_keyword = destination_keyword

        self._args_match = kwargs
        if not kwargs:
            self._int_args_match = None
        else:
            self._int_args_match = {}
            for kwarg in kwargs:
                if not kwarg.startswith('arg'):
                    raise TypeError('SignalMatch: unknown keyword argument %s'
                                    % kwarg)
                try:
                    index = int(kwarg[3:])
                except ValueError:
                    raise TypeError('SignalMatch: unknown keyword argument %s'
                                    % kwarg)
                if index < 0 or index > 63:
                    raise TypeError('SignalMatch: arg match index must be in '
                                    'range(64), not %d' % index)
                self._int_args_match[index] = kwargs[kwarg]

    def __hash__(self):
        """SignalMatch objects are compared by identity."""
        return hash(id(self))

    def __eq__(self, other):
        """SignalMatch objects are compared by identity."""
        return self is other

    def __ne__(self, other):
        """SignalMatch objects are compared by identity."""
        return self is not other

    sender = property(lambda self: self._sender)

    def __str__(self):
        if self._rule is None:
            rule = ["type='signal'"]
            if self._sender is not None:
                rule.append("sender='%s'" % self._sender)
            if self._path is not None:
                rule.append("path='%s'" % self._path)
            if self._interface is not None:
                rule.append("interface='%s'" % self._interface)
            if self._member is not None:
                rule.append("member='%s'" % self._member)
            if self._int_args_match is not None:
                for index, value in self._int_args_match.iteritems():
                    rule.append("arg%d='%s'" % (index, value))

            self._rule = ','.join(rule)

        return self._rule

    def __repr__(self):
        return ('<%s at %x "%s" on conn %r>'
                % (self.__class__, id(self), self._rule, self._conn_weakref()))

    def set_sender_name_owner(self, new_name):
        self._sender_name_owner = new_name

    def matches_removal_spec(self, sender, object_path,
                             dbus_interface, member, handler, **kwargs):
        if handler not in (None, self._handler):
            return False
        if sender != self._sender:
            return False
        if object_path != self._path:
            return False
        if dbus_interface != self._interface:
            return False
        if member != self._member:
            return False
        if kwargs != self._args_match:
            return False
        return True

    def maybe_handle_message(self, message):
        args = None

        # these haven't been checked yet by the match tree
        if self._sender_name_owner not in (None, message.get_sender()):
            return False
        if self._int_args_match is not None:
            # extracting args with utf8_strings and byte_arrays is less work
            args = message.get_args_list(utf8_strings=True, byte_arrays=True)
            for index, value in self._int_args_match.iteritems():
                if (index >= len(args)
                    or not isinstance(args[index], UTF8String)
                    or args[index] != value):
                    return False

        # these have likely already been checked by the match tree
        if self._member not in (None, message.get_member()):
            return False
        if self._interface not in (None, message.get_interface()):
            return False
        if self._path not in (None, message.get_path()):
            return False

        try:
            # minor optimization: if we already extracted the args with the
            # right calling convention to do the args match, don't bother
            # doing so again
            if args is None or not self._utf8_strings or not self._byte_arrays:
                args = message.get_args_list(utf8_strings=self._utf8_strings,
                                             byte_arrays=self._byte_arrays)
            kwargs = {}
            if self._sender_keyword is not None:
                kwargs[self._sender_keyword] = message.get_sender()
            if self._destination_keyword is not None:
                kwargs[self._destination_keyword] = message.get_destination()
            if self._path_keyword is not None:
                kwargs[self._path_keyword] = message.get_path()
            if self._member_keyword is not None:
                kwargs[self._member_keyword] = message.get_member()
            if self._interface_keyword is not None:
                kwargs[self._interface_keyword] = message.get_interface()
            if self._message_keyword is not None:
                kwargs[self._message_keyword] = message
            self._handler(*args, **kwargs)
        except:
            # basicConfig is a no-op if logging is already configured
            logging.basicConfig()
            _logger.error('Exception in handler for D-Bus signal:', exc_info=1)

        return True

    def remove(self):
        conn = self._conn_weakref()
        # do nothing if the connection has already vanished
        if conn is not None:
            conn.remove_signal_receiver(self, self._member,
                                        self._interface, self._sender,
                                        self._path,
                                        **self._args_match)


class Connection(_Connection):
    """A connection to another application. In this base class there is
    assumed to be no bus daemon.

    :Since: 0.81.0
    """

    ProxyObjectClass = ProxyObject

    def __init__(self, *args, **kwargs):
        super(Connection, self).__init__(*args, **kwargs)

        # this if-block is needed because shared bus connections can be
        # __init__'ed more than once
        if not hasattr(self, '_dbus_Connection_initialized'):
            self._dbus_Connection_initialized = 1

            self._signal_recipients_by_object_path = {}
            """Map from object path to dict mapping dbus_interface to dict
            mapping member to list of SignalMatch objects."""

            self._signals_lock = thread.allocate_lock()
            """Lock used to protect signal data structures"""

            self._object_tree_lock = thread.allocate_lock()
            """Lock used to protect _object_paths and _object_tree"""

            self._object_paths = {}
            """Dict mapping object path to one of:

            - a tuple (on_message, on_unregister, fallback)
            - a tuple (None, None, False), meaning this is a "synthetic"
              object created as a parent or ancestor for a real object
            """

            self._object_children = {}
            """Dict mapping object paths to sets of children"""

            self._filters = []

            super(Connection, self).add_message_filter(self.__class__._filter)

    def activate_name_owner(self, bus_name):
        """Return the unique name for the given bus name, activating it
        if necessary and possible.

        If the name is already unique or this connection is not to a
        bus daemon, just return it.

        :Returns: a bus name. If the given `bus_name` exists, the returned
            name identifies its current owner; otherwise the returned name
            does not exist.
        :Raises DBusException: if the implementation has failed
            to activate the given bus name.
        :Since: 0.81.0
        """
        return bus_name

    def get_object(self, bus_name=None, object_path=None, introspect=True,
                   **kwargs):
        """Return a local proxy for the given remote object.

        Method calls on the proxy are translated into method calls on the
        remote object.

        :Parameters:
            `bus_name` : str
                A bus name (either the unique name or a well-known name)
                of the application owning the object. The keyword argument
                named_service is a deprecated alias for this.
            `object_path` : str
                The object path of the desired object
            `introspect` : bool
                If true (default), attempt to introspect the remote
                object to find out supported methods and their signatures

        :Returns: a `dbus.proxies.ProxyObject`
        """
        named_service = kwargs.pop('named_service', None)
        if named_service is not None:
            if bus_name is not None:
                raise TypeError('bus_name and named_service cannot both '
                                'be specified')
            from warnings import warn
            warn('Passing the named_service parameter to get_object by name '
                 'is deprecated: please use positional parameters',
                 DeprecationWarning, stacklevel=2)
            bus_name = named_service
        if kwargs:
            raise TypeError('get_object does not take these keyword '
                            'arguments: %s' % ', '.join(kwargs.iterkeys()))

        return self.ProxyObjectClass(self, bus_name, object_path,
                                     introspect=introspect)

    def add_signal_receiver(self, handler_function,
                                  signal_name=None,
                                  dbus_interface=None,
                                  bus_name=None,
                                  path=None,
                                  **keywords):
        """Arrange for the given function to be called when a signal matching
        the parameters is received.

        :Parameters:
            `handler_function` : callable
                The function to be called. Its positional arguments will
                be the arguments of the signal. By default it will receive
                no keyword arguments, but see the description of
                the optional keyword arguments below.
            `signal_name` : str
                The signal name; None (the default) matches all names
            `dbus_interface` : str
                The D-Bus interface name with which to qualify the signal;
                None (the default) matches all interface names
            `bus_name` : str
                A bus name for the sender, which will be resolved to a
                unique name if it is not already; None (the default) matches
                any sender.
            `path` : str
                The object path of the object which must have emitted the
                signal; None (the default) matches any object path
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
                argument with this name.
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
                with this name.
            `message_keyword` : str
                If not None (the default), the handler function will receive
                the `dbus.lowlevel.SignalMessage` as a keyword argument with
                this name.
            `arg...` : unicode or UTF-8 str
                If there are additional keyword parameters of the form
                ``arg``\ *n*, match only signals where the *n*\ th argument
                is the value given for that keyword parameter. As of this
                time only string arguments can be matched (in particular,
                object paths and signatures can't).
            `named_service` : str
                A deprecated alias for `bus_name`.
        """
        self._require_main_loop()

        named_service = keywords.pop('named_service', None)
        if named_service is not None:
            if bus_name is not None:
                raise TypeError('bus_name and named_service cannot both be '
                                'specified')
            bus_name = named_service
            from warnings import warn
            warn('Passing the named_service parameter to add_signal_receiver '
                 'by name is deprecated: please use positional parameters',
                 DeprecationWarning, stacklevel=2)

        match = SignalMatch(self, bus_name, path, dbus_interface,
                            signal_name, handler_function, **keywords)

        self._signals_lock.acquire()
        try:
            by_interface = self._signal_recipients_by_object_path.setdefault(
                    path, {})
            by_member = by_interface.setdefault(dbus_interface, {})
            matches = by_member.setdefault(signal_name, [])

            matches.append(match)
        finally:
            self._signals_lock.release()

        return match

    def _iter_easy_matches(self, path, dbus_interface, member):
        if path is not None:
            path_keys = (None, path)
        else:
            path_keys = (None,)
        if dbus_interface is not None:
            interface_keys = (None, dbus_interface)
        else:
            interface_keys = (None,)
        if member is not None:
            member_keys = (None, member)
        else:
            member_keys = (None,)

        for path in path_keys:
            by_interface = self._signal_recipients_by_object_path.get(path,
                                                                      None)
            if by_interface is None:
                continue
            for dbus_interface in interface_keys:
                by_member = by_interface.get(dbus_interface, None)
                if by_member is None:
                    continue
                for member in member_keys:
                    matches = by_member.get(member, None)
                    if matches is None:
                        continue
                    for m in matches:
                        yield m

    def remove_signal_receiver(self, handler_or_match,
                               signal_name=None,
                               dbus_interface=None,
                               bus_name=None,
                               path=None,
                               **keywords):
        named_service = keywords.pop('named_service', None)
        if named_service is not None:
            if bus_name is not None:
                raise TypeError('bus_name and named_service cannot both be '
                                'specified')
            bus_name = named_service
            from warnings import warn
            warn('Passing the named_service parameter to '
                 'remove_signal_receiver by name is deprecated: please use '
                 'positional parameters',
                 DeprecationWarning, stacklevel=2)

        new = []
        deletions = []
        self._signals_lock.acquire()
        try:
            by_interface = self._signal_recipients_by_object_path.get(path,
                                                                      None)
            if by_interface is None:
                return
            by_member = by_interface.get(dbus_interface, None)
            if by_member is None:
                return
            matches = by_member.get(signal_name, None)
            if matches is None:
                return

            for match in matches:
                if (handler_or_match is match
                    or match.matches_removal_spec(bus_name,
                                                  path,
                                                  dbus_interface,
                                                  signal_name,
                                                  handler_or_match,
                                                  **keywords)):
                    deletions.append(match)
                else:
                    new.append(match)
            by_member[signal_name] = new
        finally:
            self._signals_lock.release()

        for match in deletions:
            self._clean_up_signal_match(match)

    def _clean_up_signal_match(self, match):
        # Now called without the signals lock held (it was held in <= 0.81.0)
        pass

    def call_async(self, bus_name, object_path, dbus_interface, method,
                   signature, args, reply_handler, error_handler,
                   timeout=-1.0, utf8_strings=False, byte_arrays=False,
                   require_main_loop=True):
        """Call the given method, asynchronously.

        If the reply_handler is None, successful replies will be ignored.
        If the error_handler is None, failures will be ignored. If both
        are None, the implementation may request that no reply is sent.

        :Returns: The dbus.lowlevel.PendingCall.
        :Since: 0.81.0
        """
        if object_path == LOCAL_PATH:
            raise DBusException('Methods may not be called on the reserved '
                                'path %s' % LOCAL_PATH)
        if dbus_interface == LOCAL_IFACE:
            raise DBusException('Methods may not be called on the reserved '
                                'interface %s' % LOCAL_IFACE)
        # no need to validate other args - MethodCallMessage ctor will do

        get_args_opts = {'utf8_strings': utf8_strings,
                         'byte_arrays': byte_arrays}

        message = MethodCallMessage(destination=bus_name,
                                    path=object_path,
                                    interface=dbus_interface,
                                    method=method)
        # Add the arguments to the function
        try:
            message.append(signature=signature, *args)
        except Exception, e:
            logging.basicConfig()
            _logger.error('Unable to set arguments %r according to '
                          'signature %r: %s: %s',
                          args, signature, e.__class__, e)
            raise

        if reply_handler is None and error_handler is None:
            # we don't care what happens, so just send it
            self.send_message(message)
            return

        if reply_handler is None:
            reply_handler = _noop
        if error_handler is None:
            error_handler = _noop

        def msg_reply_handler(message):
            if isinstance(message, MethodReturnMessage):
                reply_handler(*message.get_args_list(**get_args_opts))
            elif isinstance(message, ErrorMessage):
                error_handler(DBusException(name=message.get_error_name(),
                                            *message.get_args_list()))
            else:
                error_handler(TypeError('Unexpected type for reply '
                                        'message: %r' % message))
        return self.send_message_with_reply(message, msg_reply_handler,
                                        timeout,
                                        require_main_loop=require_main_loop)

    def call_blocking(self, bus_name, object_path, dbus_interface, method,
                      signature, args, timeout=-1.0, utf8_strings=False,
                      byte_arrays=False):
        """Call the given method, synchronously.
        :Since: 0.81.0
        """
        if object_path == LOCAL_PATH:
            raise DBusException('Methods may not be called on the reserved '
                                'path %s' % LOCAL_PATH)
        if dbus_interface == LOCAL_IFACE:
            raise DBusException('Methods may not be called on the reserved '
                                'interface %s' % LOCAL_IFACE)
        # no need to validate other args - MethodCallMessage ctor will do

        get_args_opts = {'utf8_strings': utf8_strings,
                         'byte_arrays': byte_arrays}

        message = MethodCallMessage(destination=bus_name,
                                    path=object_path,
                                    interface=dbus_interface,
                                    method=method)
        # Add the arguments to the function
        try:
            message.append(signature=signature, *args)
        except Exception, e:
            logging.basicConfig()
            _logger.error('Unable to set arguments %r according to '
                          'signature %r: %s: %s',
                          args, signature, e.__class__, e)
            raise

        # make a blocking call
        reply_message = self.send_message_with_reply_and_block(
            message, timeout)
        args_list = reply_message.get_args_list(**get_args_opts)
        if len(args_list) == 0:
            return None
        elif len(args_list) == 1:
            return args_list[0]
        else:
            return tuple(args_list)

    # Export functionality

    def _register_object_path(self, path, on_message, on_unregister=None,
                              fallback=False):
        """Register a callback to be called when messages arrive at the given
        object-path. Used to export objects' methods on the bus in a low-level
        way. Use `dbus.service.Object` instead.

        :Parameters:
            `path` : dbus.ObjectPath or other str
                Object path to be acted on
            `on_message` : callable
                Called when a message arrives at the given object-path, with
                two positional parameters: the first is this Connection,
                the second is the incoming `dbus.lowlevel.Message`.
            `on_unregister` : callable or None
                If not None, called when the callback is unregistered, with
                two positional parameters: the first is this Connection,
                the second is the path at which the object is no longer
                registered.
            `fallback` : bool
                If True (the default is False), when a message arrives for a
                'subdirectory' of the given path and there is no more specific
                handler, use this handler. Normally this handler is only run if
                the paths match exactly.
        """
        self._require_main_loop()
        path = ObjectPath(path)
        parent, tail = path.rsplit('/', 1)
        if on_message is None:
            raise TypeError('None may not be registered as an object-path '
                            'handler')

        self._object_tree_lock.acquire()
        paths = self._object_paths
        children = self._object_children
        try:
            if path in paths:
                old_on_message, unused, unused = paths[path]
                if old_on_message:
                    raise KeyError("Can't register the object-path handler "
                                   "for %r: there is already a handler" % path)

            while tail:
                parent = ObjectPath(parent or _ROOT)
                # make sure there's at least a synthetic parent
                paths.setdefault(parent, (None, None, False))
                # give the parent an appropriate child
                children.setdefault(parent, set()).add(tail)
                # walk upwards
                parent, tail = parent.rsplit('/', 1)

            paths[path] = (on_message, on_unregister, fallback)
        finally:
            self._object_tree_lock.release()

    def _unregister_object_path(self, path):
        """Remove a previously registered handler for the given object path.

        :Parameters:
           `path` : dbus.ObjectPath or other str
               The object path whose handler is to be removed
        :Raises KeyError: if there is no handler registered for exactly that
           object path.
        """
        path = ObjectPath(path)
        parent, tail = path.rsplit('/', 1)

        self._object_tree_lock.acquire()
        paths = self._object_paths
        children = self._object_children
        try:
            unused, on_unregister, unused = paths.pop(path)

            while tail:
                parent = ObjectPath(parent or _ROOT)
                kids = children.get(parent)
                if kids is not None:
                    kids.discard(tail)
                    if kids:
                        # parent still has other children, don't delete it
                        break
                    else:
                        del children[parent]
                parent_on_message, unused, unused = paths[parent]
                if parent_on_message is not None:
                    # parent is a real object, don't delete it
                    break
                del paths[parent]
                # walk upwards
                parent, tail = parent.rsplit('/', 1)
        finally:
            self._object_tree_lock.release()

        if on_unregister is not None:
            on_unregister(self, path)

    def list_exported_child_objects(self, path):
        """Return a list of the names of objects exported on this Connection
        as direct children of the given object path.

        Each name ``n`` returned may be converted to a valid object path using
        ``dbus.ObjectPath('%s%s%s' % (path, (path != '/' and '/' or ''), n))``.
        For the purposes of this function, every parent or ancestor of an
        exported object is considered to be an exported object, even if it's
        only an object synthesized by the library to support introspection.
        """
        return list(self._object_children.get(path, ()))

    def _handle_method_call(self, msg):
        path = msg.get_path()
        on_message = None
        absent = True
        self._object_tree_lock.acquire()
        try:
            try:
                on_message, on_unregister, fallback = self._object_paths[path]
            except KeyError:
                parent, tail = path.rsplit('/', 1)
                while parent:
                    on_message, on_unregister, fallback \
                        = self._object_paths.get(parent, (None, None, False))
                    if fallback:
                        print "Found fallback handler at %s", parent
                        absent = False
                        break
                    parent, tail = parent.rsplit('/', 1)
            else:
                print "Found handler at %s", path
                absent = False
        finally:
            self._object_tree_lock.release()

        if absent:
            # There's nothing there, and it's not an ancestor of any object
            # FIXME: Raise UnknownObject once it's specified
            reply = ErrorMessage(msg,
                                 'org.freedesktop.DBus.Error.UnknownMethod',
                                 'There is no object at %s' % path)
            self.send_message(reply)
        elif on_message is None:
            # It's a synthetic object that's an ancestor of a real object.
            # It supports Introspect and nothing else
            reflection_data = _INTROSPECT_DOCTYPE
            reflection_data += '<node name="%s">\n' % path
            for kid in self._object_children.get(path, ()):
                reflection_data += '  <node name="%s"/>\n' % kid
            reflection_data += '</node>\n'
            reply = MethodReturnMessage(msg)
            reply.append(reflection_data, signature='s')
            self.send_message(reply)
        else:
            # It's a real object and can return whatever it likes
            try:
                on_message(self, msg)
            except:
                logging.basicConfig()
                _logger.error('Error in message handler:',
                              exc_info=1)

    def _filter(self, message):
        try:
            self.__filter(message)
        except:
            logging.basicConfig()
            _logger.error('Internal error while filtering message:',
                          exc_info=1)

    def __filter(self, message):
        # Step 1. Pass any method replies to DBusPendingCall or
        # send_with_reply_and_block if they're for us.
        # - libdbus still does this for us

        # Step 1 1/2. Process the standardized org.freedesktop.DBus.Peer
        # interface. libdbus does this before any filters, so so will we.
        # FIXME: while we're using libdbus, this code won't ever run

        if (isinstance(message, MethodCallMessage) and
            message.get_interface() == PEER_IFACE):
            member = message.get_member()
            if member == 'Ping':
                # send an empty reply
                reply = MethodReturnMessage(message)
            elif member == 'GetMachineID':
                reply = ErrorMessage(message,
                        'org.freedesktop.DBus.Error.UnknownMethod',
                        'dbus-python does not yet implement Peer.GetMachineID')
            else:
                reply = ErrorMessage(message,
                        'org.freedesktop.DBus.Error.UnknownMethod',
                        'Unrecognised Peer method %s' % member)
            self.send_message(reply)
            return HANDLER_RESULT_HANDLED


        # Step 2a. Check for signals - this filter always ran first
        # in older dbus-pythons, so it runs first here

        if isinstance(message, SignalMessage):
            dbus_interface = message.get_interface()
            path = message.get_path()
            signal_name = message.get_member()

            for match in self._iter_easy_matches(path, dbus_interface,
                                                 signal_name):
                try:
                    match.maybe_handle_message(message)
                except:
                    logging.basicConfig()
                    _logger.error('Internal error handling D-Bus signal:',
                                  exc_info=1)

        # Step 2b. Run user-defined filters
        for filter in self._filters:
            try:
                ret = filter(self, message)
            except:
                logging.basicConfig()
                _logger.error('User-defined message filter raised exception:',
                              exc_info=1)
                ret = NotImplemented
            if ret is NotImplemented or ret == HANDLER_RESULT_NOT_YET_HANDLED:
                continue
            elif ret is None or ret == HANDLER_RESULT_HANDLED:
                break
            elif isinstance(ret, (int, long)):
                _logger.error('Return from message filter should be None, '
                              'NotImplemented, '
                              'dbus.lowlevel.HANDLER_RESULT_HANDLED or '
                              'dbus.lowlevel.HANDLER_RESULT_NOT_YET_HANDLED, '
                              'not %r', ret)

        # Step 3. Pass method calls to registered objects at that path
        if isinstance(message, MethodCallMessage):
            try:
                self._handle_method_call(message)
            except:
                logging.basicConfig()
                _logger.error('Internal error handling method call:',
                              exc_info=1)

        # We should be the only ones doing anything with messages now, so...
        return HANDLER_RESULT_HANDLED

    def add_message_filter(self):
        self._filters.append(filter)
