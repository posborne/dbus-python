pythondbusdir = $(pythondir)/dbus

nobase_pythondbus_PYTHON = bus.py \
			   connection.py \
			   dbus_bindings.py \
			   _dbus.py \
			   _version.py \
			   decorators.py \
			   exceptions.py \
			   _expat_introspect_parser.py \
			   glib.py \
			   gobject_service.py \
			   __init__.py \
			   lowlevel.py \
			   mainloop/__init__.py \
			   mainloop/glib.py \
			   proxies.py \
			   service.py \
			   types.py

check_py_sources = $(nobase_pythondbus_PYTHON)
include $(top_srcdir)/tools/check-coding-style.mk

CLEANFILES = .doc-needs-rebuild-stamp
BUILT_SOURCES = .doc-needs-rebuild-stamp

.doc-needs-rebuild-stamp: $(nobase_pythondbus_PYTHON)
	touch $@
