#! /bin/bash

function die()
{
    if ! test -z "$DBUS_SESSION_BUS_PID" ; then
        echo "killing message bus "$DBUS_SESSION_BUS_PID >&2
        kill -9 $DBUS_SESSION_BUS_PID
    fi
    echo $SCRIPTNAME: $* >&2

    exit 1
}


SCRIPTNAME=$0
MODE=$1

#create service file
SERVICE_DIR=test/data/valid-service-files/
SERVICE_FILE=$SERVICE_DIR/test-python.service
mkdir -p $SERVICE_DIR
echo "[D-BUS Service]" > $SERVICE_FILE
echo "Name=org.freedesktop.DBus.TestSuitePythonService" >> $SERVICE_FILE
echo "Exec=`pwd`/test/test-service.py" >> $SERVICE_FILE

## so the tests can complain if you fail to use the script to launch them
export DBUS_TEST_PYTHON_RUN_TEST_SCRIPT=1
export LD_LIBRARY_PATH="$DBUS_TOP_BUILDDIR/dbus"
# Rerun ourselves with tmp session bus if we're not already
if test -z "$DBUS_TEST_PYTHON_IN_RUN_TEST"; then
  DBUS_TEST_PYTHON_IN_RUN_TEST=1
  export DBUS_TEST_PYTHON_IN_RUN_TEST
  exec tools/run-with-tmp-session-bus.sh $SCRIPTNAME $MODE
fi  
echo "running test-client.py"
test/test-client.py || die "test-client.py failed"

