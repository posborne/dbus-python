pyexec_LTLIBRARIES = _dbus_glib_bindings.la

AM_CPPFLAGS = -I$(top_srcdir)/include $(DBUS_CFLAGS) $(DBUS_GLIB_CFLAGS) \
	      $(PYTHON_INCLUDES)
AM_LDFLAGS = -module -avoid-version \
	     -export-symbols-regex init_dbus_glib_bindings \
	     $(DBUS_LIBS) $(DBUS_GLIB_LIBS)

_dbus_glib_bindings_la_SOURCES = module.c
