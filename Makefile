#!/usr/bin/make -f
# Convenience Makefile for development - distributors and users should use
# setup.py as usual.

default: build

warning:
	@echo "### If you're not a dbus-python developer, please ignore this"
	@echo "### Makefile and use setup.py as usual."

PYTHON ?= python
EPYDOC ?= epydoc
CFLAGS ?= -Wall -Wextra -Werror -Wno-missing-field-initializers \
	 -Wdeclaration-after-statement

build: warning
	rm -rf build/lib.*/*.so build/temp.*
	CFLAGS="$(CFLAGS)" $(PYTHON) setup.py build --debug

quick: warning
	CFLAGS="$(CFLAGS)" $(PYTHON) setup.py build --debug

check: warning
	CFLAGS="$(CFLAGS)" $(PYTHON) setup.py check

# This assumes you've only built for one architecture.
docs: build
	cd $(shell echo build/lib.*) && PYTHONPATH=. \
	$(EPYDOC) -o ../epydoc --html --docformat restructuredtext -v \
		dbus _dbus_bindings _dbus_glib_bindings

cross-test-compile:
	$(PYTHON) setup.py build --debug

cross-test-server:
	cd $(shell echo build/lib.*) && PYTHONPATH=. \
	PYTHONUNBUFFERED=1 \
	$(PYTHON) ../../test/cross-test-server.py

cross-test-client:
	cd $(shell echo build/lib.*) && PYTHONPATH=. \
	PYTHONUNBUFFERED=1 \
	$(PYTHON) ../../test/cross-test-client.py

.PHONY: default docs warning
.PHONY: cross-test-compile cross-test-server cross-test-client
