#!/usr/bin/env python

import dbus

from dbus_py_test import UnusableMainLoop

def main():
    UnusableMainLoop(set_as_default=True)
    try:
        bus = dbus.SessionBus()
    except ValueError, e:
        print "Correctly got ValueError from UnusableMainLoop"
    else:
        raise AssertionError("Expected ValueError from UnusableMainLoop")

if __name__ == '__main__':
    main()
