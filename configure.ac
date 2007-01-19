-*- mode: m4 -*-
AC_PREREQ(2.52)

dnl If 1, append datestamp to the version number
m4_define(dbus_python_released, 0)
dnl The dbus-python version number (must actually be numeric at the moment)
m4_define(dbus_python_major_version, 0)
m4_define(dbus_python_minor_version, 79)
m4_define(dbus_python_micro_version, 93)

m4_define(dbus_python_maybe_datestamp,
	  m4_esyscmd([if test x]dbus_python_released[ != x1; then date +.%Y%m%d | tr -d '\n\r'; fi]))

m4_define(dbus_python_version, dbus_python_major_version.dbus_python_minor_version.dbus_python_micro_version[]dbus_python_maybe_datestamp)

dnl versions of packages we require ...
dnl m4_define(glib_required_version, 2.8.0)

AC_INIT(dbus-python, dbus_python_version,
        [http://bugzilla.gnome.org/enter_bug.cgi?product=dbus&component=python])

AC_DEFINE(DBUS_PYTHON_MAJOR_VERSION, dbus_python_major_version, [dbus-python major version])
AC_SUBST(DBUS_PYTHON_MAJOR_VERSION, dbus_python_major_version)
AC_DEFINE(DBUS_PYTHON_MINOR_VERSION, dbus_python_minor_version, [dbus-python minor version])
AC_SUBST(DBUS_PYTHON_MINOR_VERSION, dbus_python_minor_version)
AC_DEFINE(DBUS_PYTHON_MICRO_VERSION, dbus_python_micro_version, [dbus-python micro version])
AC_SUBST(DBUS_PYTHON_MICRO_VERSION, dbus_python_micro_version)

AC_CONFIG_SRCDIR([_dbus_bindings/module.c])
AM_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE

dnl put the ACLOCAL flags in the makefile
ACLOCAL="$ACLOCAL $ACLOCAL_FLAGS"

AC_CANONICAL_BUILD
AC_CANONICAL_HOST

AC_DISABLE_STATIC

dnl XXXX hack to kill off all the libtool tags ...
dnl it isn't like we are using C++ or Fortran.
dnl (copied from libglade/configure.in)
m4_define([_LT_AC_TAGCONFIG],[])

AC_PROG_LIBTOOL

AM_PATH_PYTHON(2.4.0)

AM_CHECK_PYTHON_HEADERS(,[AC_MSG_ERROR(could not find Python headers)])

PLATFORM=`$PYTHON -c "from distutils import util; print util.get_platform()"`
AC_SUBST(PLATFORM)

dnl Building documentation
AC_ARG_ENABLE(api-docs,
AC_HELP_STRING([--enable-api-docs], [Enable documentation building (requires epydoc 3 and docutils)]),, enable_api_docs=no)
AM_CHECK_PYMOD([docutils], [__version__], [:], [enable_api_docs=no])
if test "${enable_api_docs}" != no; then
  AC_PATH_PROG([EPYDOC], [epydoc])
  if test -z "$EPYDOC"; then
    enable_api_docs=no
  fi
fi

AM_CONDITIONAL([ENABLE_API_DOCS], [test x$enable_api_docs != xno])

AC_CHECK_PROGS([RST2HTML], [rst2html rst2html.py], [])
AC_SUBST([RST2HTML])
AM_CONDITIONAL([ENABLE_DOCS], [test x$RST2HTML != x])

PKG_CHECK_MODULES(DBUS, [dbus-1 >= 1.0 dbus-glib-1 >= 0.70])
PKG_CHECK_MODULES(DBUS_GLIB, [dbus-1 >= 1.0 dbus-glib-1 >= 0.70])

dnl add required cflags ...
JH_ADD_CFLAG([-Wall])
JH_ADD_CFLAG([-Wextra])
JH_ADD_CFLAG([-Wno-missing-field-initializers])
JH_ADD_CFLAG([-Wdeclaration-after-statement])
JH_ADD_CFLAG([-std=c9x])
JH_ADD_CFLAG([-fno-strict-aliasing])

AC_CONFIG_FILES(
  Makefile
  dbus-python.pc
  dbus/_version.py
  doc/tutorial.txt
  test/tmp-session-bus.conf
  test/TestSuitePythonService.service)
AC_OUTPUT
