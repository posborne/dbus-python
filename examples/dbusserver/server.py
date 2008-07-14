#!/usr/bin/env python

import dbus
import dbus.server
import dbus.lowlevel
import dbus.service
import dbus.mainloop.glib
import gobject

class DBusObject (dbus.service.Object):
	SUPPORTS_MULTIPLE_CONNECTIONS = True
	def __init__ (self):
		dbus.service.Object.__init__ (self)
		self._max_id = 1

	@dbus.service.method (dbus_interface="org.freedesktop.DBus", out_signature="s")
	def Hello (self):
		print "Hello is called"
		name = "ibus.%d" % self._max_id
		self._max_id = self._max_id +1
		return name
	
	@dbus.service.method (dbus_interface="org.freedesktop.DBus", in_signature="s", out_signature="s")
	def HelloWorld (self, s):
		print "HelloWorld [%s]" %s
		return "Return HelloWorld [%s]" %s

class IBusManager (dbus.server.Server):
	def __init__ (self):
		dbus.server.Server.__init__ (self, "unix:abstract=/tmp/ibus")

	def new_connection (self, server, connection):
		connection.add_message_filter (self.message_filter_cb)
		print "got a new connection"

	def remove_connection (self, connection):
		print "A connection is disconnected"

	def message_filter_cb (self, connection, message):
		if message.get_interface() == "org.freedesktop.DBus.Local" and \
			message.get_member() == "Disconnected":
			print "Disconnected"
			return dbus.lowlevel.HANDLER_RESULT_NOT_YET_HANDLED

		print "Got a Message (%s) : " % message.__class__.__name__
		print "\t From:      %s" % message.get_sender ()
		print "\t To:        %s" % message.get_destination ()
		print "\t Interface: %s" % message.get_interface ()
		print "\t Path:      %s" % message.get_path ()
		print "\t Member:    %s" % message.get_member ()
		print "\t Arguments:"
		i = 0
		for arg in message.get_args_list():
			print "\t\t Arg[%d] : %s" % (i, arg)
			i = i + 1

		return dbus.lowlevel.HANDLER_RESULT_NOT_YET_HANDLED

if __name__ == "__main__":
	dbus.mainloop.glib.DBusGMainLoop (set_as_default = True)
	loop = gobject.MainLoop ()
	bus = IBusManager ()
	bus.register_object (DBusObject (), "/org/freedesktop/DBus")
	print "DBUS_SESSION_BUS_ADDRESS=\"%s\"" % bus.get_address ()
	loop.run ()

