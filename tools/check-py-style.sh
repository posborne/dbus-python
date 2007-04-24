#!/bin/sh
fail=0

/bin/sh "${top_srcdir}"/tools/check-whitespace.sh "$@" || fail=$?

# at the moment we're not actually checking much Python style here...
# to be added

if test -n "$CHECK_FOR_LONG_LINES"
then
  if egrep -n '.{80,}' "$@"
  then
    echo "^^^ The above files contain long lines"
    fail=1
  fi
fi

exit $fail
