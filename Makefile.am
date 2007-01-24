# the api, doc, include subdirs don't need their own Makefile.am
SUBDIRS = _dbus_bindings _dbus_glib_bindings dbus examples test

CLEANFILES =
EXTRA_DIST = API_CHANGES.txt dbus-python.pc.in HACKING.txt \
	     AUTHORS COPYING NEWS ChangeLog TODO \
	     doc/tutorial.txt.in
# miss out the gconf examples for now - they don't work

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dbus-python.pc

dbusincludedir = $(includedir)/dbus-1.0/dbus
dbusinclude_HEADERS = include/dbus-python.h

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

TXT_RSTDOCS = doc/tutorial.txt doc/API_CHANGES.txt doc/HACKING.txt
RSTDOCS = README NEWS TODO
dist_doc_DATA = $(TXT_RSTDOCS) $(RSTDOCS)

maintainer-update-website: _maintainer-update-apidocs \
	_maintainer-update-htmldocs

if ENABLE_DOCS
_maintainer-update-htmldocs: $(HTML_TXT_RSTDOCS) $(HTML_RSTDOCS)
	rsync -rtvzPp --chmod=Dg+s,ug+rwX,o=rX doc \
	$(RSTDOCS) $(HTML_RSTDOCS) \
	dbus.freedesktop.org:/srv/dbus.freedesktop.org/www/doc/dbus-python/

HTML_TXT_RSTDOCS = doc/tutorial.html doc/API_CHANGES.html doc/HACKING.html
HTML_RSTDOCS = README.html NEWS.html TODO.html
nodist_doc_DATA = $(HTML_TXT_RSTDOCS) $(HTML_RSTDOCS)

CLEANFILES += $(nodist_doc_DATA)

$(HTML_TXT_RSTDOCS) : %.html: %.txt
		$(RST2HTML) $(RST2HTMLFLAGS) $< $@
$(HTML_RSTDOCS) : %.html: %
		$(RST2HTML) $(RST2HTMLFLAGS) $< $@
else
_maintainer-update-htmldocs:
	@echo "*** Not updating the HTML docs on the website - install rst2html"
	@echo "*** from http://docutils.sourceforge.net/ and configure with "
	@echo "*** --enable-html-docs"
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
	PYTHONPATH=$(APIDOC_PYTHONPATH) DBUS_PYTHON_NO_DEPRECATED=1 $(PYTHON) -Wignore::DeprecationWarning $(EPYDOC) -o api --html \
		--docformat restructuredtext -v \
		`find dbus -name '*.py' | grep -v dbus_bindings \
			| sed -e 's#/__init__\.py##g' \
				-e 's/\.py\>//g' -e 's#/#.#'g` \
		|| { rm -rf api; exit 1; }

_maintainer-update-apidocs: api
	rsync -rtvzPp --chmod=Dg+s,ug+rwX,o=rX api/ \
	dbus.freedesktop.org:/srv/dbus.freedesktop.org/www/doc/dbus-python/api/
else
_maintainer-update-apidocs:
	@echo "*** Not updating the API docs on the website - install epydoc 3"
	@echo "*** alpha (or newer) and configure with --enable-api-docs"
endif

.PHONY: cross-test-compile cross-test-server cross-test-client \
	always-rebuild maintainer-update-website \
	_maintainer-update-apidocs _maintainer-update-htmldocs
