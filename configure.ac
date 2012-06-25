-*- mode: m4 -*-
AC_PREREQ(2.59c)

dnl If not 1, append datestamp to the version number
m4_define(dbus_python_released, 1)
dnl The dbus-python version number (must actually be numeric at the moment)
m4_define(dbus_python_major_version, 1)
m4_define(dbus_python_minor_version, 1)
m4_define(dbus_python_micro_version, 1)

m4_define(dbus_python_maybe_datestamp,
          m4_esyscmd([if test x]dbus_python_released[ != x1; then date +.%Y%m%d | tr -d '\n\r'; fi]))

m4_define(dbus_python_version, dbus_python_major_version.dbus_python_minor_version.dbus_python_micro_version[]dbus_python_maybe_datestamp)

dnl versions of packages we require ...
dnl m4_define(glib_required_version, 2.8.0)

AC_INIT(dbus-python, dbus_python_version,
        [http://bugs.freedesktop.org/enter_bug.cgi?product=dbus&component=python])
AC_CONFIG_MACRO_DIR([m4])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])

AC_DEFINE(DBUS_PYTHON_MAJOR_VERSION, dbus_python_major_version, [dbus-python major version])
AC_SUBST(DBUS_PYTHON_MAJOR_VERSION, dbus_python_major_version)
AC_DEFINE(DBUS_PYTHON_MINOR_VERSION, dbus_python_minor_version, [dbus-python minor version])
AC_SUBST(DBUS_PYTHON_MINOR_VERSION, dbus_python_minor_version)
AC_DEFINE(DBUS_PYTHON_MICRO_VERSION, dbus_python_micro_version, [dbus-python micro version])
AC_SUBST(DBUS_PYTHON_MICRO_VERSION, dbus_python_micro_version)

AC_CONFIG_SRCDIR([_dbus_bindings/module.c])
AM_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE([1.9 -Wno-portability])

AC_CANONICAL_BUILD
AC_CANONICAL_HOST

AC_DISABLE_STATIC

dnl XXXX hack to kill off all the libtool tags ...
dnl it isn't like we are using C++ or Fortran.
dnl (copied from libglade/configure.in)
m4_define([_LT_AC_TAGCONFIG],[])

AC_PROG_LIBTOOL
AM_PROG_CC_C_O

AM_PATH_PYTHON([2.6])

AM_CHECK_PYTHON_HEADERS(,[AC_MSG_ERROR(could not find Python headers)])

# This command exits 0 (success) if sys.version >= "3", or 1 (failure)
# if sys.version < "3" (i.e. starts with "2").
AM_CONDITIONAL([HAVE_PYTHON_3],
  [$PYTHON -c "import sys; sys.exit(sys.version < '3')"])

PLATFORM=`$PYTHON -c "from __future__ import print_function; from distutils import util; print(util.get_platform())"`
AC_SUBST(PLATFORM)

AC_ARG_VAR([PYTHON_LIBS], [LDFLAGS for Python, overriding output of python2.x-config --ldflags, e.g. "-L/opt/misc/lib -lpython2.7"])

if test "${PYTHON_LIBS+set}" = set; then
  AC_MSG_NOTICE([PYTHON_LIBS overridden to: $PYTHON_LIBS])
else
  AC_MSG_CHECKING([for Python library linker flags])
  PYTHON_LIBS=`$PYTHON-config --ldflags`
  AC_MSG_RESULT([$PYTHON_LIBS])
fi

AC_SUBST([PYTHON_LIBS])

dnl Building documentation

AC_MSG_CHECKING([whether you want to build HTML docs])
AC_ARG_ENABLE(html-docs,
AC_HELP_STRING([--enable-html-docs], [Enable HTML documentation building (requires docutils, default: auto-detect)]), enable_html_docs=$enableval, enable_html_docs="if possible")
AC_MSG_RESULT([$enable_html_docs])

AC_MSG_CHECKING([whether you want to build API docs])
AC_ARG_ENABLE(api-docs,
AC_HELP_STRING([--enable-api-docs], [Enable API documentation building (requires epydoc 3 and docutils)]), enable_api_docs=$enableval, enable_api_docs="if possible")
AC_MSG_RESULT([$enable_api_docs])


if test "$enable_api_docs" != no || test "$enable_html_docs" != no; then
  AM_CHECK_PYMOD([docutils], [__version__], [have_docutils=yes], [have_docutils=no])
  if test "$have_docutils" = no; then
    if test "$enable_api_docs" = "if possible"; then
       enable_api_docs=no
    fi
    if test "$enable_html_docs" = "if possible"; then
       enable_html_docs=no
    fi
    if test "$enable_api_docs" != no || test "$enable_html_docs" != no; then
      AC_MSG_ERROR([cannot compile HTML documentation or API documentation without python-docutils installed])
    fi
  fi
fi

if test "${enable_api_docs}" != no; then
  AC_PATH_PROG([EPYDOC], [epydoc])
  AC_MSG_CHECKING([epydoc 3])
  if test -n "$EPYDOC"; then
    EPYDOC_VERSION=`$EPYDOC --version`
    case "$EPYDOC_VERSION" in
      *ersion?3*)
        AC_MSG_RESULT([yes, $EPYDOC_VERSION])
        ;;
      *)
        AC_MSG_RESULT([no, $EPYDOC_VERSION])
        EPYDOC=
        ;;
    esac
  fi
  if test -z "$EPYDOC"; then
    case "$enable_api_docs" in
      if*possible)
        enable_api_docs=no
        ;;
      *)
        AC_MSG_ERROR([cannot compile API documentation without epydoc 3.0beta1 or newer installed])
        ;;
    esac
  fi
fi

RST2HTMLFLAGS=
if test "${enable_html_docs}" != no; then
  AC_CHECK_PROGS([RST2HTML], [rst2html rst2html.py], [])
  if test -z "$RST2HTML"; then
    case "$enable_html_docs" in
      if*possible)
        enable_html_docs=no
      ;;
      *)
        AC_MSG_ERROR([cannot compile HTML documentation without rst2html installed])
      ;;
    esac
  else
    DBUS_PY_ADD_RST2HTMLFLAG([--generator])
    DBUS_PY_ADD_RST2HTMLFLAG([--date])
    DBUS_PY_ADD_RST2HTMLFLAG([--time])
    DBUS_PY_ADD_RST2HTMLFLAG([--exit-status=2])
    DBUS_PY_ADD_RST2HTMLFLAG([--no-raw])
    DBUS_PY_ADD_RST2HTMLFLAG([--no-file-insertion])
    DBUS_PY_ADD_RST2HTMLFLAG([--cloak-email-addresses])
    DBUS_PY_ADD_RST2HTMLFLAG([--input-encoding=UTF-8])
  fi
fi
AC_SUBST([RST2HTML])
AC_SUBST([RST2HTMLFLAGS])

AM_CONDITIONAL([ENABLE_API_DOCS], [test "$enable_api_docs" != no])
AM_CONDITIONAL([ENABLE_DOCS], [test "$enable_html_docs" != no])

PKG_CHECK_MODULES(DBUS, [dbus-1 >= 1.4])
PKG_CHECK_MODULES(DBUS_GLIB, [dbus-glib-1 >= 0.70])

dbuspy_save_CFLAGS="$CFLAGS"
dbuspy_save_LIBS="$LIBS"
CFLAGS="$CFLAGS $DBUS_CFLAGS"
LIBS="$CFLAGS $DBUS_LIBS"
AC_CHECK_TYPES([DBusBasicValue], [], [], [#include <dbus/dbus.h>])
AC_CHECK_FUNCS([dbus_validate_utf8])
CFLAGS="$dbuspy_save_CFLAGS"
LIBS="$dbuspy_save_LIBS"

TP_COMPILER_WARNINGS([CFLAGS_WARNINGS], [test] dbus_python_released [= 0],
  [all \
   extra \
   declaration-after-statement \
   deprecated-declarations \
   shadow \
   strict-prototypes \
   missing-prototypes \
   missing-declarations \
   sign-compare \
   nested-externs \
   pointer-arith \
   format-security \
   init-self],
  [missing-field-initializers \
   unused-parameter])

JH_ADD_CFLAG([-std=c9x])
JH_ADD_CFLAG([-fno-strict-aliasing])

CFLAGS="$CFLAGS $CFLAGS_WARNINGS"

ifelse(dbus_python_released, 1,
    [ # version x.y.z - disable coding style checks by default
AC_ARG_ENABLE(coding-style-checks,
  AC_HELP_STRING([--enable-coding-style-checks],
                 [check coding style using grep]),
    [ENABLE_CODING_STYLE_CHECKS=$enableval], [ENABLE_CODING_STYLE_CHECKS=no] )
    ],
    [ # version x.y.z.200xyyzz - enable coding style checks by default
AC_ARG_ENABLE(coding-style-checks,
  AC_HELP_STRING([--disable-coding-style-checks],
                 [don't check coding style using grep]),
    [ENABLE_CODING_STYLE_CHECKS=$enableval], [ENABLE_CODING_STYLE_CHECKS=yes])
    ])

AC_CONFIG_FILES(
  Makefile
  _dbus_bindings/Makefile
  _dbus_glib_bindings/Makefile
  dbus-python.pc
  dbus/_version.py
  examples/Makefile
  m4/Makefile
  test/Makefile
  test/tmp-session-bus.conf
  test/TestSuitePythonService.service
  tools/Makefile)
AC_OUTPUT
