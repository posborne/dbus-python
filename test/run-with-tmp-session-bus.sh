#! /bin/bash

# Copyright (C) 2003-2005 Red Hat Inc. <http://www.redhat.com/>
# Copyright (C) 2005 Colin Walters
# Copyright (C) 2007 Collabora Ltd. <http://www.collabora.co.uk/>

SCRIPTNAME=$0
WRAPPED_SCRIPT=$1
shift

function die()
{
    if ! test -z "$DBUS_SESSION_BUS_PID" ; then
        echo "killing message bus $DBUS_SESSION_BUS_PID" >&2
        kill -9 "$DBUS_SESSION_BUS_PID"
    fi
    echo "$SCRIPTNAME: $*" >&2
    exit 1
}

if test -z "$DBUS_TOP_BUILDDIR" ; then
    die "Must set DBUS_TOP_BUILDDIR"
fi

## convenient to be able to ctrl+C without leaking the message bus process
trap 'die "Received SIGINT"' SIGINT

CONFIG_FILE="$DBUS_TOP_BUILDDIR"/test/tmp-session-bus.conf

unset DBUS_SESSION_BUS_ADDRESS
unset DBUS_SESSION_BUS_PID

echo "Running dbus-launch --sh-syntax --config-file=$CONFIG_FILE" >&2
eval `dbus-launch --sh-syntax --config-file=$CONFIG_FILE`

if test -z "$DBUS_SESSION_BUS_PID" ; then
    die "Failed to launch message bus for introspection generation to run"
fi

echo "Started bus pid $DBUS_SESSION_BUS_PID at $DBUS_SESSION_BUS_ADDRESS" >&2

# Execute wrapped script
echo "Running: $WRAPPED_SCRIPT $*" >&2
"$WRAPPED_SCRIPT" "$@" || die "script \"$WRAPPED_SCRIPT\" failed"

kill -TERM "$DBUS_SESSION_BUS_PID" \
    || die "Message bus vanished! should not have happened" \
    && echo "Killed daemon $DBUS_SESSION_BUS_PID" >&2

sleep 2

## be sure it really died
kill -9 $DBUS_SESSION_BUS_PID > /dev/null 2>&1 || true

exit 0
