import dbus

gconf_key = "/desktop/gnome/file_views/icon_theme"

bus = dbus.SessionBus()
gconf_service = bus.get_service("org.gnome.GConf")
gconf_key_object = gconf_service.get_object("/org/gnome/GConf" + gconf_key, "org.gnome.GConf")

value = gconf_key_object.getString()

print ("Value of GConf key %s is %s" % (gconf_key, value))
