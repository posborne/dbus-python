# === Miscellaneous ===

CLEANFILES =
EXTRA_DIST = API_CHANGES.txt dbus-python.pc.in HACKING.txt \
	     AUTHORS COPYING ChangeLog NEWS TODO \
	     examples/example-client.py \
	     examples/example-service.py \
	     examples/example-signal-emitter.py \
	     examples/example-signal-recipient.py \
	     examples/list-system-services.py \
	     test/cross-test-client.py \
	     test/cross-test-server.py \
	     test/crosstest.py \
	     test/run-test.sh \
	     test/run-with-tmp-session-bus.sh \
	     test/test-client.py \
	     test/test-service.py \
	     test/test-signals.py \
	     test/test-standalone.py \
	     test/TestSuitePythonService.service.in \
	     test/tmp-session-bus.conf.in
# miss out the gconf examples for now - they don't work

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dbus-python.pc

include_HEADERS = include/dbus-python.h

# === Pure Python ===

nobase_python_PYTHON = dbus_bindings.py \
		       dbus/dbus_bindings.py \
		       dbus/_dbus.py \
		       dbus/decorators.py \
		       dbus/exceptions.py \
		       dbus/_expat_introspect_parser.py \
		       dbus/glib.py \
		       dbus/__init__.py \
		       dbus/introspect_parser.py \
		       dbus/lowlevel.py \
		       dbus/mainloop/__init__.py \
		       dbus/mainloop/glib.py \
		       dbus/proxies.py \
		       dbus/service.py \
		       dbus/types.py

# === Python C extensions ===

pyexec_LTLIBRARIES = _dbus_bindings.la _dbus_glib_bindings.la
_dbus_bindings_la_CPPFLAGS = -I$(srcdir)/include \
			     $(DBUS_CFLAGS) \
			     $(PYTHON_INCLUDES)
_dbus_bindings_la_LDFLAGS = -module -avoid-version \
			 -export-symbols-regex init_dbus_bindings \
			 $(DBUS_LIBS)
_dbus_bindings_la_SOURCES = \
			    _dbus_bindings/abstract.c \
			    _dbus_bindings/bus.c \
			    _dbus_bindings/bytes.c \
			    _dbus_bindings/conn.c \
			    _dbus_bindings/conn-internal.h \
			    _dbus_bindings/conn-methods.c \
			    _dbus_bindings/containers.c \
			    _dbus_bindings/dbus_bindings-internal.h \
			    _dbus_bindings/debug.c \
			    _dbus_bindings/exceptions.c \
			    _dbus_bindings/float.c \
			    _dbus_bindings/generic.c \
			    _dbus_bindings/int.c \
			    _dbus_bindings/mainloop.c \
			    _dbus_bindings/message-append.c \
			    _dbus_bindings/message.c \
			    _dbus_bindings/message-get-args.c \
			    _dbus_bindings/message-internal.h \
			    _dbus_bindings/module.c \
			    _dbus_bindings/pending-call.c \
			    _dbus_bindings/signature.c \
			    _dbus_bindings/string.c \
			    _dbus_bindings/types-internal.h \
			    _dbus_bindings/validation.c \
			    include/dbus-python.h

_dbus_glib_bindings_la_CPPFLAGS = -I$(top_srcdir)/include \
				  $(DBUS_CFLAGS) \
				  $(DBUS_GLIB_CFLAGS) \
				  $(PYTHON_INCLUDES)
_dbus_glib_bindings_la_LDFLAGS = -module -avoid-version \
			 -export-symbols-regex init_dbus_glib_bindings \
			 $(DBUS_LIBS) $(DBUS_GLIB_LIBS)
_dbus_glib_bindings_la_SOURCES = _dbus_glib_bindings/module.c \
				 include/dbus-python.h

# === Tests ===

PWD = `pwd`
TESTS_ENVIRONMENT = DBUS_TOP_SRCDIR="$(PWD)/$(top_srcdir)" \
		    DBUS_TOP_BUILDDIR="$(PWD)" \
		    DBUS_PYTHON_VERSION='$(PACKAGE_VERSION)' \
		    PYTHONPATH="$(PWD)/.libs:$(PWD)/$(top_srcdir):$(PWD)/$(top_srcdir)/test" \
		    PYTHON='$(PYTHON)'
TESTS = test/run-test.sh

cross-test-compile: all
cross-test-server:
	$(TESTS_ENVIRONMENT) $(PYTHON) $(top_srcdir)/test/cross-test-server.py
cross-test-client:
	$(TESTS_ENVIRONMENT) $(PYTHON) $(top_srcdir)/test/cross-test-client.py

# === Documentation ===

if ENABLE_API_DOCS
all: api/index.html

api api/index.html: $(nobase_python_PYTHON) _dbus_bindings.la _dbus_glib_bindings.la
	rm -rf api
	mkdir api
	$(TESTS_ENVIRONMENT) $(EPYDOC) -o api --html \
		--docformat restructuredtext -v \
		`echo $(nobase_python_PYTHON) \
			| sed -e 's#/__init__\.py##g' \
				-e 's/\.py\>//g' -e 's#/#.#'g` \
	|| { rm -rf api; exit 1; }
endif

.PHONY: cross-test-compile cross-test-server cross-test-client
