dnl DBUS_PY_ADD_RST2HTMLFLAG(FLAG)
dnl checks whether rst2html supports the given flag, and if so, adds
dnl it to $RST2HTMLFLAGS. Same as JH_ADD_CFLAG, really.
AC_DEFUN([DBUS_PY_ADD_RST2HTMLFLAG],
[
case " $RST2HTMLFLAGS " in
*@<:@\	\ @:>@$1@<:@\	\ @:>@*)
  ;;
*)
  save_RST2HTMLFLAGS="$RST2HTMLFLAGS"
  RST2HTMLFLAGS="$RST2HTMLFLAGS $1"
  AC_MSG_CHECKING([whether [$]RST2HTML understands $1])
  if $RST2HTML --strict $RST2HTMLFLAGS /dev/null > /dev/null 2>/dev/null; then
    dbuspy_has_option=yes
  else
    dbuspy_has_option=no
  fi
  AC_MSG_RESULT($dbuspy_has_option)
  if test $dbuspy_has_option = no; then
    RST2HTMLFLAGS="$save_RST2HTMLFLAGS"
  fi
  ;;
esac])
