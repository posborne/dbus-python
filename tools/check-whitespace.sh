#!/bin/sh

fail=0

if grep -n ' $' "$@"
then
  echo "^^^ The above files contain unwanted trailing spaces"
  fail=1
fi

if grep -n '	' "$@"
then
  echo "^^^ The above files contain tabs"
  fail=1
fi

exit $fail
