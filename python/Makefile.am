SUBDIRS=examples

INCLUDES=-I$(top_builddir) -I$(top_builddir)/dbus $(DBUS_CLIENT_CFLAGS) $(DBUS_GLIB_CFLAGS) $(DBUS_GLIB_TOOL_CFLAGS) $(PYTHON_INCLUDES)

dbusdir = $(pythondir)
dbus_PYTHON = dbus.py

dbusbindingsdir = $(pythondir)
dbusbindings_LTLIBRARIES = dbus_bindings.la

dbus_bindings_la_LDFLAGS = -module -avoid-version -fPIC -export-symbols-regex initdbus_bindings
dbus_bindings_la_LIBADD = $(top_builddir)/dbus/libdbus-1.la $(top_builddir)/glib/libdbus-glib-1.la
nodist_dbus_bindings_la_SOURCES = dbus_bindings.c


EXTRA_DIST = 			\
	dbus_h_wrapper.h	\
	dbus_bindings.pyx.in	\
	extract.py

CLEANFILES =			\
	dbus_bindings.pyx	\
	dbus_bindings.c


dbus_bindings.pyx: dbus_bindings.pyx.in extract.py
	-$(PYTHON) extract.py dbus_bindings.pyx.in -I$(top_builddir)  > dbus_bindings.pyx

dbus_bindings.c: dbus_bindings.pyx
	-pyrexc dbus_bindings.pyx
