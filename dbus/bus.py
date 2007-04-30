"""Bus mixin, for use within dbus-python only. See `_BusMixin`."""

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

BUS_DAEMON_NAME = 'org.freedesktop.DBus'
BUS_DAEMON_PATH = '/org/freedesktop/DBus'
BUS_DAEMON_IFACE = BUS_DAEMON_NAME

from _dbus_bindings import validate_interface_name, validate_member_name,\
                           validate_bus_name, validate_object_path,\
                           validate_error_name


class _BusDaemonMixin(object):
    """This mixin provides simple blocking wrappers for various methods on
    the org.freedesktop.DBus bus-daemon object, to reduce the amount of C
    code we need.
    """

    def get_unix_user(self, bus_name):
        """Get the numeric uid of the process owning the given bus name.

        :Parameters:
            `bus_name` : str
                A bus name, either unique or well-known
        :Returns: a `dbus.UInt32`
        """
        validate_bus_name(bus_name)
        return self.call_blocking(BUS_DAEMON_NAME, BUS_DAEMON_PATH,
                                  BUS_DAEMON_IFACE, 'GetConnectionUnixUser',
                                  's', (bus_name,))

    def start_service_by_name(self, bus_name, flags=0):
        """Start a service which will implement the given bus name on this Bus.

        :Parameters:
            `bus_name` : str
                The well-known bus name to be activated.
            `flags` : dbus.UInt32
                Flags to pass to StartServiceByName (currently none are
                defined)

        :Returns: A tuple of 2 elements. The first is always True, the
            second is either START_REPLY_SUCCESS or
            START_REPLY_ALREADY_RUNNING.

        :Raises DBusException: if the service could not be started.
        """
        validate_bus_name(bus_name)
        return (True, self.call_blocking(BUS_DAEMON_NAME, BUS_DAEMON_PATH,
                                         BUS_DAEMON_IFACE,
                                         'StartServiceByName',
                                         'su', (bus_name, flags)))

    # XXX: it might be nice to signal IN_QUEUE, EXISTS by exception,
    # but this would not be backwards-compatible
    def request_name(self, name, flags=0):
        """Request a bus name.

        :Parameters:
            `name` : str
                The well-known name to be requested
            `flags` : dbus.UInt32
                A bitwise-OR of 0 or more of the flags
                `DBUS_NAME_FLAG_ALLOW_REPLACEMENT`,
                `DBUS_NAME_FLAG_REPLACE_EXISTING`
                and `DBUS_NAME_FLAG_DO_NOT_QUEUE`
        :Returns: `DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER`,
            `DBUS_REQUEST_NAME_REPLY_IN_QUEUE`,
            `DBUS_REQUEST_NAME_REPLY_EXISTS` or
            `DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER`
        :Raises DBusException: if the bus daemon cannot be contacted or
            returns an error.
        """
        validate_bus_name(name, allow_unique=False)
        return self.call_blocking(BUS_DAEMON_NAME, BUS_DAEMON_PATH,
                                  BUS_DAEMON_IFACE, 'RequestName',
                                  'su', (name, flags))

    def release_name(self, name):
        """Release a bus name.

        :Parameters:
            `name` : str
                The well-known name to be released
        :Returns: `DBUS_RELEASE_NAME_REPLY_RELEASED`,
            `DBUS_RELEASE_NAME_REPLY_NON_EXISTENT`
            or `DBUS_RELEASE_NAME_REPLY_NOT_OWNER`
        :Raises DBusException: if the bus daemon cannot be contacted or
            returns an error.
        """
        validate_bus_name(name, allow_unique=False)
        return self.call_blocking(BUS_DAEMON_NAME, BUS_DAEMON_PATH,
                                  BUS_DAEMON_IFACE, 'ReleaseName',
                                  's', (name,))

    def name_has_owner(self, bus_name):
        """Return True iff the given bus name has an owner on this bus.

        :Parameters:
            `name` : str
                The bus name to look up
        :Returns: a `bool`
        """
        return bool(self.call_blocking(BUS_DAEMON_NAME, BUS_DAEMON_PATH,
                                       BUS_DAEMON_IFACE, 'NameHasOwner',
                                       's', (bus_name,)))

    def add_match_string(self, rule):
        """Arrange for this application to receive messages on the bus that
        match the given rule. This version will block.

        :Parameters:
            `rule` : str
                The match rule
        :Raises: `DBusException` on error.
        """
        self.call_blocking(BUS_DAEMON_NAME, BUS_DAEMON_PATH,
                           BUS_DAEMON_IFACE, 'AddMatch', 's', (rule,))

    # FIXME: add an async success/error handler capability?
    # (and the same for remove_...)
    def add_match_string_non_blocking(self, rule):
        """Arrange for this application to receive messages on the bus that
        match the given rule. This version will not block, but any errors
        will be ignored.


        :Parameters:
            `rule` : str
                The match rule
        :Raises: `DBusException` on error.
        """
        self.call_async(BUS_DAEMON_NAME, BUS_DAEMON_PATH,
                        BUS_DAEMON_IFACE, 'AddMatch', 's', (rule,),
                        None, None)

    def remove_match_string(self, rule):
        """Arrange for this application to receive messages on the bus that
        match the given rule. This version will block.

        :Parameters:
            `rule` : str
                The match rule
        :Raises: `DBusException` on error.
        """
        self.call_blocking(BUS_DAEMON_NAME, BUS_DAEMON_PATH,
                           BUS_DAEMON_IFACE, 'RemoveMatch', 's', (rule,))

    def remove_match_string_non_blocking(self, rule):
        """Arrange for this application to receive messages on the bus that
        match the given rule. This version will not block, but any errors
        will be ignored.


        :Parameters:
            `rule` : str
                The match rule
        :Raises: `DBusException` on error.
        """
        self.call_async(BUS_DAEMON_NAME, BUS_DAEMON_PATH,
                        BUS_DAEMON_IFACE, 'RemoveMatch', 's', (rule,),
                        None, None)
