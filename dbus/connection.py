"""Method-call mixin for use within dbus-python only.
See `_MethodCallMixin`.
"""

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

import logging

from _dbus_bindings import Connection, ErrorMessage, \
                           MethodCallMessage, MethodReturnMessage, \
                           DBusException


# This is special in libdbus - the bus daemon will kick us off if we try to
# send any message to it :-/
LOCAL_PATH = '/org/freedesktop/DBus/Local'


_logger = logging.getLogger('dbus.methods')


def _noop(*args, **kwargs):
    pass


class _MethodCallMixin(object):

    def call_async(self, bus_name, object_path, dbus_interface, method,
                   signature, args, reply_handler, error_handler,
                   timeout=-1.0, utf8_strings=False, byte_arrays=False,
                   require_main_loop=True):
        """Call the given method, asynchronously.

        If the reply_handler is None, successful replies will be ignored.
        If the error_handler is None, failures will be ignored. If both
        are None, the implementation may request that no reply is sent.

        :Returns: The dbus.lowlevel.PendingCall.
        """
        if object_path == LOCAL_PATH:
            raise DBusException('Methods may not be called on the reserved '
                                'path %s' % LOCAL_PATH)
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
                args = message.get_args_list()
                # FIXME: should we do something with the rest?
                if len(args) > 0:
                    error_handler(DBusException(args[0]))
                else:
                    error_handler(DBusException())
            else:
                error_handler(TypeError('Unexpected type for reply '
                                        'message: %r' % message))
        return self.send_message_with_reply(message, msg_reply_handler,
                                        timeout/1000.0,
                                        require_main_loop=require_main_loop)

    def call_blocking(self, bus_name, object_path, dbus_interface, method,
                      signature, args, timeout=-1.0, utf8_strings=False,
                      byte_arrays=False):
        """Call the given method, synchronously.
        """
        if object_path == LOCAL_PATH:
            raise DBusException('Methods may not be called on the reserved '
                                'path %s' % LOCAL_PATH)
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
