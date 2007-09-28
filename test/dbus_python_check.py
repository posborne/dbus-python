"""dbus_python_check

Implements the Distutils 'check' command.
"""

# Copyright (C) 2006 Red Hat Inc. <http://www.redhat.com/>
# created 2006/07/12, John (J5) Palmieri
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

__revision__ = "0.1"

from distutils.core import Command
from distutils.command.build import build
from distutils.dist import Distribution
import os
 
class dbus_python_check (Command):

    # Brief (40-50 characters) description of the command
    description = "Runs tests in this package"

    # List of option tuples: long name, short name (None if no short
    # name), and help string.
    user_options = [('help', '?',
                     "Shows Help"),
                   ]


    def initialize_options (self):
        self.dummy = None

    # initialize_options()


    def finalize_options (self):
      pass
    # finalize_options()

    def run (self):
      b = build(self.distribution)
      b.finalize_options()
      b.run()
      top_builddir = os.path.join (os.getcwd(), b.build_lib) 
      cmd = 'DBUS_TOP_BUILDDIR="%s" test/run-test.sh'%top_builddir
      print cmd
      os.system (cmd)

    # run()

# class check
