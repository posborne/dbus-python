# the api, doc, include subdirs don't need their own Makefile.am
SUBDIRS = _dbus_bindings _dbus_glib_bindings dbus examples test

CLEANFILES =
EXTRA_DIST = API_CHANGES.txt dbus-python.pc.in HACKING.txt \
	     AUTHORS COPYING NEWS ChangeLog TODO \
	     doc/tutorial.txt.in
# miss out the gconf examples for now - they don't work

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dbus-python.pc

include_HEADERS = include/dbus-python.h

python_PYTHON = dbus_bindings.py

# === Tests ===

cross-test-compile: all
cross-test-server:
	@$(MAKE) -C test cross-test-server
cross-test-client:
	@$(MAKE) -C test cross-test-client

# === Documentation ===

ChangeLog: always-rebuild
	@if test -d $(top_srcdir)/.git; then \
		if git-log --stat > ChangeLog; then \
			: ; \
		else \
			git-log > Changelog || exit 1; \
		fi; \
	fi

TXT_RSTDOCS = doc/tutorial.txt API_CHANGES.txt HACKING.txt
RSTDOCS = README NEWS TODO
dist_doc_DATA = $(TXT_RSTDOCS) $(RSTDOCS)

if ENABLE_DOCS
HTML_TXT_RSTDOCS = doc/tutorial.html API_CHANGES.html HACKING.html
HTML_RSTDOCS = README.html NEWS.html TODO.html
nodist_doc_DATA = $(HTML_TXT_RSTDOCS) $(HTML_RSTDOCS)

CLEANFILES += $(nodist_doc_DATA)

$(HTML_TXT_RSTDOCS) : %.html: %.txt
		$(RST2HTML) $(RST2HTMLFLAGS) $< $@
$(HTML_RSTDOCS) : %.html: %
		$(RST2HTML) $(RST2HTMLFLAGS) $< $@
endif

if ENABLE_API_DOCS
all: api/index.html

clean-local:
	rm -rf api

dbus/.doc-needs-rebuild-stamp:
	$(MAKE) -C dbus
_dbus_bindings/_dbus_bindings.la:
	$(MAKE) -C _dbus_bindings
_dbus_glib_bindings/_dbus_glib_bindings.la:
	$(MAKE) -C _dbus_glib_bindings

PWD = `pwd`
APIDOC_PYTHONPATH = $(PWD)/$(top_srcdir):$(PWD)/_dbus_bindings/.libs:$(PWD)/_dbus_glib_bindings/.libs

api api/index.html: $(python_PYTHON) dbus/.doc-needs-rebuild-stamp \
		    _dbus_bindings/_dbus_bindings.la \
		    _dbus_glib_bindings/_dbus_glib_bindings.la
	rm -rf api
	mkdir api
	PYTHONPATH=$(APIDOC_PYTHONPATH) $(EPYDOC) -o api --html \
		--docformat restructuredtext -v \
		`find dbus -name '*.py' \
			| sed -e 's#/__init__\.py##g' \
				-e 's/\.py\>//g' -e 's#/#.#'g` \
	|| { rm -rf api; exit 1; }
endif

.PHONY: cross-test-compile cross-test-server cross-test-client always-rebuild
