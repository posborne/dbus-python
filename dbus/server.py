# Copyright (C) 2003-2006 Red Hat Inc. <http://www.redhat.com/>
# Copyright (C) 2003 David Zeuthen
# Copyright (C) 2004 Rob Taylor
# Copyright (C) 2005-2006 Collabora Ltd. <http://www.collabora.co.uk/>
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use, copy,
# modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

import _dbus_bindings
import dbus.connection
import dbus.lowlevel

class Server (_dbus_bindings.Server):
	def __init__ (self, address):
		_dbus_bindings.Server.__init__ (self, address)
		self._connections = []
		self._objects = {}
		self.set_new_connection_function (self._new_connection_cb, dbus.connection.Connection)

	def register_object (self, obj, path):
		if path in self._objects:
			raise Exception ("%s has been registered", path)
		self._objects[path] = obj

		for conn in self._connections:
			obj.add_to_connection (conn, path)

	def _new_connection_cb (self, server, new_connection):
		self._connections.append (new_connection)
		new_connection.add_message_filter (self._message_filter_cb)

		# add all objects to the new connection
		for path, obj in self._objects.items ():
			obj.add_to_connection (new_connection, path)

		self.new_connection (server, new_connection)

	def _message_filter_cb (self, connection, message):
		if message.get_interface() == "org.freedesktop.DBus.Local" and \
		 	message.get_member() == "Disconnected":
			# remove all object from this connection
			for path, obj in self._objects.items ():
				obj.remove_from_connection (connection, path)
			
			self.remove_connection (connection)
			self._connections.remove (connection)
			return dbus.lowlevel.HANDLER_RESULT_HANDLED

		return dbus.lowlevel.HANDLER_RESULT_NOT_YET_HANDLED

	def new_connection (self, server, connection):
		pass

	def remove_connection (self, server, connection):
		pass


