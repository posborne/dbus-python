"""dbus_python_check

Implements the Distutils 'check' command.
"""

# created 2006/07/12, John (J5) Palmieri

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
