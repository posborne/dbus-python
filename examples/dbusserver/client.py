#!/usr/bin/env python
import dbus
import dbus.mainloop.glib
import os
import gobject
import time


def main():
	loop = gobject.MainLoop ()
	bus = dbus.SessionBus ()
	obj = bus.get_object ("org.freedesktop.DBus", "/org/freedesktop/DBus")
	while True:
		print obj.HelloWorld ("[%s][%s]" % (time.time(), bus.get_unique_name()))
		time.sleep (1)
	loop.run ()

if __name__ == "__main__":
	os.environ["DBUS_SESSION_BUS_ADDRESS"]="unix:abstract=/tmp/ibus"
	dbus.mainloop.glib.DBusGMainLoop (set_as_default=True)
	main ()
