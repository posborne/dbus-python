#!/usr/bin/make -f
# Convenience Makefile.

PYTHON ?= python
EPYDOC ?= epydoc

# This assumes you've only built for one architecture.
docs:
	$(PYTHON) setup.py build
	cd $(shell echo build/lib.*) && PYTHONPATH=. \
	$(EPYDOC) -o ../epydoc --html --docformat plaintext -v \
		dbus _dbus_bindings _dbus_glib_bindings

cross-test-compile:
	$(PYTHON) setup.py build

cross-test-server:
	cd $(shell echo build/lib.*) && PYTHONPATH=. \
	PYTHONUNBUFFERED=1 \
	$(PYTHON) ../../test/cross-test-server.py

cross-test-client:
	cd $(shell echo build/lib.*) && PYTHONPATH=. \
	PYTHONUNBUFFERED=1 \
	$(PYTHON) ../../test/cross-test-client.py
