"""Implementation for dbus.Bus. Not to be imported directly."""

# Copyright (C) 2003, 2004, 2005, 2006 Red Hat Inc. <http://www.redhat.com/>
# Copyright (C) 2003 David Zeuthen
# Copyright (C) 2004 Rob Taylor
# Copyright (C) 2005, 2006 Collabora Ltd. <http://www.collabora.co.uk/>
#
# Licensed under the Academic Free License version 2.1
#
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

from __future__ import generators

__all__ = ('Bus', 'SystemBus', 'SessionBus', 'StarterBus')
__docformat__ = 'reStructuredText'

import os
import logging
import sys
import weakref
from traceback import print_exc

from _dbus_bindings import BUS_DAEMON_NAME, BUS_DAEMON_PATH,\
                           BUS_DAEMON_IFACE, DBusException, UTF8String,\
                           validate_member_name, validate_interface_name,\
                           validate_bus_name, validate_object_path,\
                           BUS_SESSION, BUS_SYSTEM, BUS_STARTER,\
                           DBUS_START_REPLY_SUCCESS, \
                           DBUS_START_REPLY_ALREADY_RUNNING, \
                           SignalMessage,\
                           HANDLER_RESULT_NOT_YET_HANDLED,\
                           HANDLER_RESULT_HANDLED
from dbus.bus import BusConnection
from dbus.proxies import ProxyObject

try:
    import thread
except ImportError:
    import dummy_thread as thread

logger = logging.getLogger('dbus._dbus')

_NAME_OWNER_CHANGE_MATCH = ("type='signal',sender='%s',"
                            "interface='%s',member='NameOwnerChanged',"
                            "path='%s',arg0='%%s'"
                            % (BUS_DAEMON_NAME, BUS_DAEMON_IFACE,
                               BUS_DAEMON_PATH))
"""(_NAME_OWNER_CHANGE_MATCH % sender) matches relevant NameOwnerChange
messages"""


class SignalMatch(object):
    __slots__ = ('sender_unique', '_member', '_interface', '_sender',
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

        self._conn_weakref = weakref.ref(conn)
        self._sender = sender
        self._interface = dbus_interface
        self._member = member
        self._path = object_path
        self._handler = handler
        if (sender is not None and sender[:1] != ':'
            and sender != BUS_DAEMON_NAME):
            self.sender_unique = conn.get_name_owner(sender)
        else:
            self.sender_unique = sender
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

        # we're always going to have to calculate the match rule for
        # the Bus's benefit, so this constructor might as well do the work
        rule = ["type='signal'"]
        if self._sender is not None:
            rule.append("sender='%s'" % self._sender)
        if self._path is not None:
            rule.append("path='%s'" % self._path)
        if self._interface is not None:
            rule.append("interface='%s'" % self._interface)
        if self._member is not None:
            rule.append("member='%s'" % self._member)
        for kwarg, value in kwargs.iteritems():
            rule.append("%s='%s'" % (kwarg, value))

        self._rule = ','.join(rule)

    def __str__(self):
        return self._rule

    def __repr__(self):
        return ('<%s at %x "%s" on conn %r>'
                % (self.__class__, id(self), self._rule, self._conn_weakref()))

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
        if self.sender_unique not in (None, message.get_sender()):
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
            # FIXME: need to decide whether dbus-python uses logging, or
            # stderr, or what, and make it consistent
            sys.stderr.write('Exception in handler for D-Bus signal:\n')
            print_exc()

        return True

    def remove(self):
        #logger.debug('%r: removing', self)
        conn = self._conn_weakref()
        # do nothing if the connection has already vanished
        if conn is not None:
            #logger.debug('%r: removing from connection %r', self, conn)
            conn.remove_signal_receiver(self, self._member,
                                        self._interface, self._sender,
                                        self._path,
                                        **self._args_match)


class Bus(BusConnection):
    """A connection to a DBus daemon.

    One of three possible standard buses, the SESSION, SYSTEM,
    or STARTER bus
    """

    TYPE_SESSION    = BUS_SESSION
    """Represents a session bus (same as the global dbus.BUS_SESSION)"""

    TYPE_SYSTEM     = BUS_SYSTEM
    """Represents the system bus (same as the global dbus.BUS_SYSTEM)"""

    TYPE_STARTER = BUS_STARTER
    """Represents the bus that started this service by activation (same as
    the global dbus.BUS_STARTER)"""

    ProxyObjectClass = ProxyObject

    START_REPLY_SUCCESS = DBUS_START_REPLY_SUCCESS
    START_REPLY_ALREADY_RUNNING = DBUS_START_REPLY_ALREADY_RUNNING

    _shared_instances = {}

    def __new__(cls, bus_type=TYPE_SESSION, private=False, mainloop=None):
        """Constructor, returning an existing instance where appropriate.

        The returned instance is actually always an instance of `SessionBus`,
        `SystemBus` or `StarterBus`.

        :Parameters:
            `bus_type` : cls.TYPE_SESSION, cls.TYPE_SYSTEM or cls.TYPE_STARTER
                Connect to the appropriate bus
            `private` : bool
                If true, never return an existing shared instance, but instead
                return a private connection
            `mainloop` : dbus.mainloop.NativeMainLoop
                The main loop to use. The default is to use the default
                main loop if one has been set up, or raise an exception
                if none has been.
        :ToDo:
            - There is currently no way to connect this class to a custom
              address.
            - Some of this functionality should be available on
              peer-to-peer D-Bus connections too.
        :Changed: in dbus-python 0.80:
            converted from a wrapper around a Connection to a Connection
            subclass.
        """
        if (not private and bus_type in cls._shared_instances):
            return cls._shared_instances[bus_type]

        # this is a bit odd, but we create instances of the subtypes
        # so we can return the shared instances if someone tries to
        # construct one of them (otherwise we'd eg try and return an
        # instance of Bus from __new__ in SessionBus). why are there
        # three ways to construct this class? we just don't know.
        if bus_type == BUS_SESSION:
            subclass = SessionBus
        elif bus_type == BUS_SYSTEM:
            subclass = SystemBus
        elif bus_type == BUS_STARTER:
            subclass = StarterBus
        else:
            raise ValueError('invalid bus_type %s' % bus_type)

        bus = subclass._new_for_bus(bus_type, mainloop=mainloop)

        bus._bus_type = bus_type
        # _bus_names is used by dbus.service.BusName!
        bus._bus_names = weakref.WeakValueDictionary()

        bus._signal_recipients_by_object_path = {}
        """Map from object path to dict mapping dbus_interface to dict
        mapping member to list of SignalMatch objects."""

        bus._signal_sender_matches = {}
        """Map from sender well-known name to list of match rules for all
        signal handlers that match on sender well-known name."""

        bus._signals_lock = thread.allocate_lock()
        """Lock used to protect signal data structures if doing two
        removals at the same time (everything else is atomic, thanks to
        the GIL)"""

        bus.add_message_filter(bus.__class__._signal_func)

        if not private:
            cls._shared_instances[bus_type] = bus

        return bus

    def close(self):
        t = self._bus_type
        if self.__class__._shared_instances[t] is self:
            del self.__class__._shared_instances[t]
        super(BusConnection, self).close()

    def get_connection(self):
        """(Deprecated - in new code, just use self)

        Return self, for backwards compatibility with earlier dbus-python
        versions where Bus was not a subclass of Connection.
        """
        return self
    _connection = property(get_connection, None, None,
                           """self._connection == self, for backwards
                           compatibility with earlier dbus-python versions
                           where Bus was not a subclass of Connection.""")

    def get_session(private=False):
        """Static method that returns a connection to the session bus.

        :Parameters:
            `private` : bool
                If true, do not return a shared connection.
        """
        return SessionBus(private=private)

    get_session = staticmethod(get_session)

    def get_system(private=False):
        """Static method that returns a connection to the system bus.

        :Parameters:
            `private` : bool
                If true, do not return a shared connection.
        """
        return SystemBus(private=private)

    get_system = staticmethod(get_system)


    def get_starter(private=False):
        """Static method that returns a connection to the starter bus.

        :Parameters:
            `private` : bool
                If true, do not return a shared connection.
        """
        return StarterBus(private=private)

    get_starter = staticmethod(get_starter)

    def get_object(self, named_service, object_path, introspect=True,
                   follow_name_owner_changes=False):
        """Return a local proxy for the given remote object.

        Method calls on the proxy are translated into method calls on the
        remote object.

        :Parameters:
            `named_service` : str
                A bus name (either the unique name or a well-known name)
                of the application owning the object
            `object_path` : str
                The object path of the desired object
            `introspect` : bool
                If true (default), attempt to introspect the remote
                object to find out supported methods and their signatures
            `follow_name_owner_changes` : bool
                If the object path is a well-known name and this parameter
                is false (default), resolve the well-known name to the unique
                name of its current owner and bind to that instead; if the
                ownership of the well-known name changes in future,
                keep communicating with the original owner.
                This is necessary if the D-Bus API used is stateful.

                If the object path is a well-known name and this parameter
                is true, whenever the well-known name changes ownership in
                future, bind to the new owner, if any.

                If the given object path is a unique name, this parameter
                has no effect.

        :Returns: a `dbus.proxies.ProxyObject`
        :Raises `DBusException`: if resolving the well-known name to a
            unique name fails
        """
        if follow_name_owner_changes:
            self._require_main_loop()   # we don't get the signals otherwise
        return self.ProxyObjectClass(self, named_service, object_path,
                                     introspect=introspect,
                                     follow_name_owner_changes=follow_name_owner_changes)

    def add_signal_receiver(self, handler_function,
                                  signal_name=None,
                                  dbus_interface=None,
                                  named_service=None,
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
            `named_service` : str
                A bus name for the sender, which will be resolved to a
                unique name if it is not already; None (the default) matches
                any sender
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
        """
        self._require_main_loop()

        match = SignalMatch(self, named_service, path, dbus_interface,
                            signal_name, handler_function, **keywords)
        by_interface = self._signal_recipients_by_object_path.setdefault(path,
                                                                         {})
        by_member = by_interface.setdefault(dbus_interface, {})
        matches = by_member.setdefault(signal_name, [])
        # The bus daemon is special - its unique-name is org.freedesktop.DBus
        # rather than starting with :
        if (named_service is not None and named_service[:1] != ':'
            and named_service != BUS_DAEMON_NAME):
            notification = self._signal_sender_matches.setdefault(named_service,
                                                                  [])
            if not notification:
                self.add_match_string(_NAME_OWNER_CHANGE_MATCH % named_service)
            notification.append(match)
        # make sure nobody is currently manipulating the list
        self._signals_lock.acquire()
        try:
            matches.append(match)
        finally:
            self._signals_lock.release()
        self.add_match_string(str(match))
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

    def _remove_name_owner_changed_for_match(self, named_service, match):
        # The signals lock must be held.
        notification = self._signal_sender_matches.get(named_service, False)
        if notification:
            try:
                notification.remove(match)
            except LookupError:
                pass
            if not notification:
                self.remove_match_string(_NAME_OWNER_CHANGE_MATCH
                                         % named_service)

    def remove_signal_receiver(self, handler_or_match,
                               signal_name=None,
                               dbus_interface=None,
                               named_service=None,
                               path=None,
                               **keywords):
        #logger.debug('%r: removing signal receiver %r: member=%s, '
                     #'iface=%s, sender=%s, path=%s, kwargs=%r',
                     #self, handler_or_match, signal_name,
                     #dbus_interface, named_service, path, keywords)
        #logger.debug('%r', self._signal_recipients_by_object_path)
        by_interface = self._signal_recipients_by_object_path.get(path, None)
        if by_interface is None:
            return
        by_member = by_interface.get(dbus_interface, None)
        if by_member is None:
            return
        matches = by_member.get(signal_name, None)
        if matches is None:
            return
        self._signals_lock.acquire()
        #logger.debug(matches)
        try:
            new = []
            for match in matches:
                if (handler_or_match is match
                    or match.matches_removal_spec(named_service,
                                                  path,
                                                  dbus_interface,
                                                  signal_name,
                                                  handler_or_match,
                                                  **keywords)):
                    #logger.debug('Removing match string: %s', match)
                    self.remove_match_string(str(match))
                    self._remove_name_owner_changed_for_match(named_service,
                                                              match)
                else:
                    new.append(match)
            by_member[signal_name] = new
        finally:
            self._signals_lock.release()

    def _signal_func(self, message):
        """D-Bus filter function. Handle signals by dispatching to Python
        callbacks kept in the match-rule tree.
        """

        #logger.debug('Incoming message %r with args %r', message,
                     #message.get_args_list())

        if not isinstance(message, SignalMessage):
            return HANDLER_RESULT_NOT_YET_HANDLED

        # If it's NameOwnerChanged, we'll need to update our
        # sender well-known name -> sender unique name mappings
        if (message.is_signal(BUS_DAEMON_IFACE, 'NameOwnerChanged')
            and message.has_sender(BUS_DAEMON_NAME)
            and message.has_path(BUS_DAEMON_PATH)):
                name, unused, new = message.get_args_list()
                for match in self._signal_sender_matches.get(name, (None,))[1:]:
                    match.sender_unique = new

        # See if anyone else wants to know
        dbus_interface = message.get_interface()
        path = message.get_path()
        signal_name = message.get_member()

        ret = HANDLER_RESULT_NOT_YET_HANDLED
        for match in self._iter_easy_matches(path, dbus_interface,
                                             signal_name):
            if match.maybe_handle_message(message):
                ret = HANDLER_RESULT_HANDLED
        return ret

    def __repr__(self):
        if self._bus_type == BUS_SESSION:
            name = 'SESSION'
        elif self._bus_type == BUS_SYSTEM:
            name = 'SYSTEM'
        elif self._bus_type == BUS_STARTER:
            name = 'STARTER'
        else:
            raise AssertionError('Unable to represent unknown bus type.')

        return '<dbus.Bus on %s at %#x>' % (name, id(self))
    __str__ = __repr__


# FIXME: Drop the subclasses here? I can't think why we'd ever want
# polymorphism
class SystemBus(Bus):
    """The system-wide message bus."""
    def __new__(cls, private=False, mainloop=None):
        """Return a connection to the system bus.

        :Parameters:
            `private` : bool
                If true, never return an existing shared instance, but instead
                return a private connection.
            `mainloop` : dbus.mainloop.NativeMainLoop
                The main loop to use. The default is to use the default
                main loop if one has been set up, or raise an exception
                if none has been.
        """
        return Bus.__new__(cls, Bus.TYPE_SYSTEM, mainloop=mainloop,
                           private=private)

class SessionBus(Bus):
    """The session (current login) message bus."""
    def __new__(cls, private=False, mainloop=None):
        """Return a connection to the session bus.

        :Parameters:
            `private` : bool
                If true, never return an existing shared instance, but instead
                return a private connection.
            `mainloop` : dbus.mainloop.NativeMainLoop
                The main loop to use. The default is to use the default
                main loop if one has been set up, or raise an exception
                if none has been.
        """
        return Bus.__new__(cls, Bus.TYPE_SESSION, private=private,
                           mainloop=mainloop)

class StarterBus(Bus):
    """The bus that activated this process (only valid if
    this process was launched by DBus activation).
    """
    def __new__(cls, private=False, mainloop=None):
        """Return a connection to the bus that activated this process.

        :Parameters:
            `private` : bool
                If true, never return an existing shared instance, but instead
                return a private connection.
            `mainloop` : dbus.mainloop.NativeMainLoop
                The main loop to use. The default is to use the default
                main loop if one has been set up, or raise an exception
                if none has been.
        """
        return Bus.__new__(cls, Bus.TYPE_STARTER, private=private,
                           mainloop=mainloop)


_dbus_bindings_warning = DeprecationWarning("""\
The dbus_bindings module is deprecated and will go away soon.

dbus-python 0.80 provides only a partial emulation of the old
dbus_bindings, which was never meant to be public API.

Most uses of dbus_bindings are applications catching the exception
dbus.dbus_bindings.DBusException. You should use dbus.DBusException
instead (this is compatible with all dbus-python versions since 0.40.2).

If you need additional public API, please contact the maintainers via
<dbus@lists.freedesktop.org>.
""")

if 'DBUS_PYTHON_NO_DEPRECATED' not in os.environ:

    class _DBusBindingsEmulation:
        """A partial emulation of the dbus_bindings module."""
        _module = None
        def __str__(self):
            return '_DBusBindingsEmulation()'
        def __repr__(self):
            return '_DBusBindingsEmulation()'
        def __getattr__(self, attr):
            if self._module is None:
                from warnings import warn as _warn
                _warn(_dbus_bindings_warning, DeprecationWarning, stacklevel=2)

                import dbus.dbus_bindings as m
                self._module = m
            return getattr(self._module, attr)

    dbus_bindings = _DBusBindingsEmulation()
    """Deprecated, don't use."""
