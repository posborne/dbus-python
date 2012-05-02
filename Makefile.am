abs_top_srcdir = @abs_top_srcdir@
abs_top_builddir = @abs_top_builddir@

ACLOCAL_AMFLAGS = -I m4

# the api, doc, include subdirs don't need their own Makefile.am
# the dbus subdir is handled with non-recursive make because of #12741
SUBDIRS = _dbus_bindings _dbus_glib_bindings examples m4 test tools .

CLEANFILES =
EXTRA_DIST = \
	dbus-python.pc.in \
	AUTHORS \
	COPYING \
	ChangeLog
# miss out the gconf examples for now - they don't work

# === dbus package ===

nobase_python_PYTHON = \
    dbus/bus.py \
    dbus/connection.py \
    dbus/_compat.py \
    dbus/_dbus.py \
    dbus/_version.py \
    dbus/decorators.py \
    dbus/exceptions.py \
    dbus/_expat_introspect_parser.py \
    dbus/gi_service.py \
    dbus/glib.py \
    dbus/__init__.py \
    dbus/lowlevel.py \
    dbus/mainloop/__init__.py \
    dbus/mainloop/glib.py \
    dbus/proxies.py \
    dbus/server.py \
    dbus/service.py \
    dbus/types.py

if !HAVE_PYTHON_3
nobase_python_PYTHON += \
    dbus/gobject_service.py \
    $(NULL)
endif

check_py_sources = $(nobase_python_PYTHON)
include $(top_srcdir)/tools/check-coding-style.mk

# === Devel stuff ===

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dbus-python.pc

dbusincludedir = $(includedir)/dbus-1.0/dbus
dbusinclude_HEADERS = include/dbus-python.h

# === Tests ===

cross-test-compile: all
cross-test-server:
	@$(MAKE) -C test cross-test-server
cross-test-client:
	@$(MAKE) -C test cross-test-client

# === Documentation ===

dist-hook:
	chmod u+w $(distdir)/ChangeLog
	if test -d $(top_srcdir)/.git; then \
		if GIT_DIR=$(top_srcdir)/.git git log --stat > $(distdir)/ChangeLog; then \
			: ; \
		else \
			GIT_DIR=$(top_srcdir)/.git git log > $(distdir)/ChangeLog; \
		fi; \
	fi

TXT_RSTDOCS = \
	doc/tutorial.txt \
	doc/API_CHANGES.txt \
	doc/HACKING.txt \
	doc/PY3PORT.txt \
	$(NULL)
RSTDOCS = \
	README \
	NEWS \
	$(NULL)
dist_doc_DATA = $(TXT_RSTDOCS) $(RSTDOCS)

maintainer-upload:
	rsync -tvpP --chmod=ugo=r $(DIST_ARCHIVES) $(DIST_ARCHIVES:%=%.asc) \
	dbus.freedesktop.org:/srv/dbus.freedesktop.org/www/releases/dbus-python/

maintainer-update-website: _maintainer-update-apidocs \
	_maintainer-update-htmldocs

if ENABLE_DOCS
_maintainer-update-htmldocs: $(HTML_TXT_RSTDOCS) $(HTML_RSTDOCS)
	rsync -rtvzPp --chmod=Dg+s,ug+rwX,o=rX doc \
	$(patsubst %,$(srcdir)/%,$(RSTDOCS)) $(HTML_RSTDOCS) \
	dbus.freedesktop.org:/srv/dbus.freedesktop.org/www/doc/dbus-python/

HTML_TXT_RSTDOCS = $(patsubst %.txt,%.html,$(TXT_RSTDOCS))
HTML_RSTDOCS = $(patsubst %,%.html,$(RSTDOCS))
nodist_doc_DATA = $(HTML_TXT_RSTDOCS) $(HTML_RSTDOCS)

CLEANFILES += $(nodist_doc_DATA)

$(HTML_TXT_RSTDOCS) : %.html: %.txt
	$(INSTALL) -d doc
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

_dbus_bindings/_dbus_bindings.la:
	$(MAKE) -C _dbus_bindings
_dbus_glib_bindings/_dbus_glib_bindings.la:
	$(MAKE) -C _dbus_glib_bindings

APIDOC_PYTHONPATH = $(abs_top_srcdir):$(abs_top_builddir)/_dbus_bindings/.libs:$(abs_top_builddir)/_dbus_glib_bindings/.libs

api api/index.html: $(nobase_python_PYTHON) \
		    _dbus_bindings/_dbus_bindings.la \
		    _dbus_glib_bindings/_dbus_glib_bindings.la
	rm -rf api
	mkdir api
	cd $(abs_top_srcdir) && \
	PYTHONPATH=$(APIDOC_PYTHONPATH) DBUS_PYTHON_NO_DEPRECATED=1 \
	$(PYTHON) -Wignore::DeprecationWarning \
	$(EPYDOC) -o $(abs_top_builddir)/api --html \
		--docformat restructuredtext -v \
		`find dbus -name '*.py' \
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
	_maintainer-update-apidocs _maintainer-update-htmldocs \
	maintainer-upload
