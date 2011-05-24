#!/usr/bin/env python

import time

usage = """Usage:
python unix-fd-service.py <file name> &
python unix-fd-client.py
"""

# Copyright (C) 2004-2006 Red Hat Inc. <http://www.redhat.com/>
# Copyright (C) 2005-2007 Collabora Ltd. <http://www.collabora.co.uk/>
# Copyright (C) 2010 Signove <http://www.signove.com>
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use, copy,
# modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

import sys
from traceback import print_exc
import os

import dbus

def main():
    bus = dbus.SessionBus()

    try:
        remote_object = bus.get_object("com.example.SampleService",
                                       "/SomeObject")

    except dbus.DBusException:
        print_exc()
        print usage
        sys.exit(1)

    iface = dbus.Interface(remote_object, "com.example.SampleInterface")

    # UnixFd is an opaque object that takes care of received fd
    fd_object = iface.GetFd()
    print fd_object

    # Once we take the fd number, we are in charge of closing it!
    fd = fd_object.take()
    print fd

    # We want to encapsulate the integer fd into a Python file or socket object
    f = os.fdopen(fd, "r")

    # If it were an UNIX socket we would do
    # sk = socket.fromfd(fd, socket.AF_UNIX, socket.SOCK_STREAM)
    # os.close(fd)
    #
    # fromfd() dup()s the descriptor so we need to close the original,
    # otherwise it 'leaks' (stays open until program exits).

    f.seek(0)
    print f.read()

if __name__ == '__main__':
    main()
