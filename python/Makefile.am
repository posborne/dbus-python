SUBDIRS=examples

INCLUDES=-I$(srcdir)/$(top_builddir) -I$(srcdir)/$(top_builddir)/dbus $(DBUS_CLIENT_CFLAGS) $(PYTHON_INCLUDES) -DDBUS_COMPILATION=1

dbusmoduledir = $(pyexecdir)
dbusmodule_PYTHON = dbus.pth

dbusdir = $(pyexecdir)/dbus
dbus_PYTHON =		\
	__init__.py 	\
	_dbus.py 	\
	decorators.py 	\
	exceptions.py 	\
	service.py 	\
	proxies.py 	\
	_util.py 	\
	types.py 	\
	matchrules.py 	\
	glib.py 	\
	introspect_parser.py

dbusbindingsdir = $(pyexecdir)/dbus
dbusbindings_LTLIBRARIES = dbus_bindings.la dbus_glib_bindings.la

dbus_bindings_la_LDFLAGS = -module -avoid-version -fPIC -export-symbols-regex initdbus_bindings
dbus_bindings_la_LIBADD = $(top_builddir)/dbus/libdbus-1.la
nodist_dbus_bindings_la_SOURCES = dbus_bindings.c

dbus_glib_bindings_la_LDFLAGS = -module -avoid-version -fPIC -export-symbols-regex initdbus_glib_bindings 
dbus_glib_bindings_la_LIBADD = $(top_builddir)/dbus/libdbus-1.la $(top_builddir)/glib/libdbus-glib-1.la
dbus_glib_bindings_la_CFLAGS = $(DBUS_GLIB_CFLAGS) $(DBUS_GLIB_TOOL_CFLAGS)
nodist_dbus_glib_bindings_la_SOURCES = dbus_glib_bindings.c


EXTRA_DIST = 			\
	dbus_h_wrapper.h	\
	dbus_bindings.pyx	\
	dbus_glib_bindings.pyx  \
	dbus_bindings.pxd.in	\
	extract.py

CLEANFILES =			\
	dbus_bindings.pxd	\
	dbus_bindings.c		\
	dbus_glib_bindings.c	


dbus_bindings.pxd: $(srcdir)/dbus_bindings.pxd.in $(srcdir)/extract.py
	-$(PYTHON) $(srcdir)/extract.py $(srcdir)/dbus_bindings.pxd.in -I$(srcdir)/$(top_builddir)  -I$(srcdir) > $@.tmp && mv $@.tmp $@

dbus_bindings.c: $(srcdir)/dbus_bindings.pyx dbus_bindings.pxd 
	pyrexc $(srcdir)/dbus_bindings.pyx -I. -o ./dbus_bindings.c

dbus_glib_bindings.c: $(srcdir)/dbus_glib_bindings.pyx dbus_bindings.pxd 
	pyrexc $(srcdir)/dbus_glib_bindings.pyx  -I. -o ./dbus_glib_bindings.c

