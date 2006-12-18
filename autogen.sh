#!/bin/sh
if test -z "$ACLOCAL"; then
    ACLOCAL="aclocal"
fi
ACLOCAL="$ACLOCAL -I autotools" autoreconf -f -i
