#!/usr/bin/make -f
# Convenience Makefile.

# This assumes you've only built for one architecture.
docs:
	python setup.py build
	cd $(shell echo build/lib.*) && PYTHONPATH=. \
	epydoc -o ../epydoc --html --docformat plaintext -v \
		dbus _dbus_bindings _dbus_glib_bindings
