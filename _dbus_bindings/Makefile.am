pyexec_LTLIBRARIES = _dbus_bindings.la

AM_CPPFLAGS = -I$(top_srcdir)/include $(DBUS_CFLAGS) $(PYTHON_INCLUDES)
AM_LDFLAGS = -module -avoid-version -export-symbols-regex init_dbus_bindings \
	     $(DBUS_LIBS)
_dbus_bindings_la_SOURCES = \
			    abstract.c \
			    bus.c \
			    bytes.c \
			    conn.c \
			    conn-internal.h \
			    conn-methods.c \
			    containers.c \
			    dbus_bindings-internal.h \
			    debug.c \
			    exceptions.c \
			    float.c \
			    generic.c \
			    int.c \
			    libdbusconn.c \
			    mainloop.c \
			    message-append.c \
			    message.c \
			    message-get-args.c \
			    message-internal.h \
			    module.c \
			    pending-call.c \
			    server.c \
			    signature.c \
			    string.c \
			    types-internal.h \
			    validation.c

check_c_sources = $(_dbus_bindings_la_SOURCES)
include $(top_srcdir)/tools/check-coding-style.mk
