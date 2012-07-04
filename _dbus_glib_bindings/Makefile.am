pyexec_LTLIBRARIES = _dbus_glib_bindings.la

AM_CPPFLAGS = -I$(top_srcdir)/include $(DBUS_CFLAGS) $(DBUS_GLIB_CFLAGS) \
	      $(PYTHON_INCLUDES)
AM_LDFLAGS = -module -avoid-version \
	     -export-symbols-regex \(PyInit__\|init_\)dbus_glib_bindings \
	     $(NULL)

libadd = \
	$(DBUS_LIBS) \
	$(DBUS_GLIB_LIBS) \
	$(NULL)

if WINDOWS
# Win32 DLLs can't have undefined symbols (so this needs explicit linking
# against the Python DLL), and Python expects extensions to be *.pyd
# instead of *.dll
AM_LDFLAGS += \
	-no-undefined \
	-shrext ".pyd" \
	$(NULL)

libadd += \
	$(PYTHON_LIBS) \
	$(NULL)
endif

_dbus_glib_bindings_la_LIBADD = $(libadd)
_dbus_glib_bindings_la_SOURCES = module.c
