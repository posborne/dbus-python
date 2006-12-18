#! /bin/bash

function die() 
{
    if ! test -z "$DBUS_SESSION_BUS_PID" ; then
        echo "killing message bus $DBUS_SESSION_BUS_PID" >&2
        kill -9 "$DBUS_SESSION_BUS_PID"
    fi
    echo "$SCRIPTNAME: $*" >&2
    exit 1
}

if test -z "$PYTHON"; then
    echo "Warning: \$PYTHON not set, assuming 'python'" >&2
    export PYTHON=python
fi

if test -z "$DBUS_TOP_SRCDIR" ; then
    die "Must set DBUS_TOP_SRCDIR"
fi

if test -z "$DBUS_TOP_BUILDDIR" ; then
    die "Must set DBUS_TOP_BUILDDIR"
fi

SCRIPTNAME=$0

## so the tests can complain if you fail to use the script to launch them
export DBUS_TEST_PYTHON_RUN_TEST_SCRIPT=1
# Rerun ourselves with tmp session bus if we're not already
if test -z "$DBUS_TEST_PYTHON_IN_RUN_TEST"; then
  DBUS_TEST_PYTHON_IN_RUN_TEST=1
  export DBUS_TEST_PYTHON_IN_RUN_TEST
  exec "$DBUS_TOP_SRCDIR"/test/run-with-tmp-session-bus.sh $SCRIPTNAME
fi  
echo "running test-standalone.py"
$PYTHON "$DBUS_TOP_SRCDIR"/test/test-standalone.py || die "test-standalone.py failed"
echo "running test-client.py"
$PYTHON "$DBUS_TOP_SRCDIR"/test/test-client.py || die "test-client.py failed"
echo "running test-signals.py"
$PYTHON "$DBUS_TOP_SRCDIR"/test/test-signals.py || die "test-signals.py failed"

rm -f "$DBUS_TOP_BUILDDIR"/test/test-service.log
exit 0
