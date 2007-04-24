#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

ORIGDIR=`pwd`
cd $srcdir

PROJECT=dbus-python

test -f dbus-python.pc.in || {
	echo "You must run this script in the top-level $PROJECT directory">&2
	exit 1
}

(autoreconf --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have autoconf, automake and libtoolize installed">&2
	echo "to compile $PROJECT. Download the appropriate packages for">&2
	echo "your distribution, or get the source tarball at ">&2
	echo "ftp://ftp.gnu.org/pub/gnu/">&2
	exit 1
}

if test -z "$*"; then
	echo "I am going to run ./configure with no arguments - if you wish "
        echo "to pass any to it, please specify them on the $0 command line."
fi

echo "Running autoreconf -f -i..."
autoreconf -f -i

cd $ORIGDIR

run_configure=true
for arg in $*; do
    case $arg in 
        --no-configure)
            run_configure=false
            ;;
        *)
            ;;
    esac
done

if $run_configure; then
    $srcdir/configure --enable-maintainer-mode --config-cache "$@"
    echo
    echo "Now run 'make' to compile $PROJECT."
fi
