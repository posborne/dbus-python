pythondbusdir = $(pythondir)/dbus

nobase_pythondbus_PYTHON = dbus_bindings.py \
			   _dbus.py \
			   _version.py \
			   decorators.py \
			   exceptions.py \
			   _expat_introspect_parser.py \
			   glib.py \
			   __init__.py \
			   lowlevel.py \
			   mainloop/__init__.py \
			   mainloop/glib.py \
			   proxies.py \
			   service.py \
			   types.py

CLEANFILES = .doc-needs-rebuild-stamp
BUILT_SOURCES = .doc-needs-rebuild-stamp

.doc-needs-rebuild-stamp: $(nobase_pythondbus_PYTHON)
	touch $@
