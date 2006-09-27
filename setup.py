import os
import sys

sys.path.append("dbus")

from distutils.core import setup
from distutils.extension import Extension
from distutils.command.clean import clean

import extract

sys.path.append("test")
from dbus_python_check import dbus_python_check

def remove(filename):
    if os.path.exists(filename):
        os.remove(filename)

class full_clean(clean):
    def run(self):
        clean.run(self)
        remove("ChangeLog")

includedirs_flag = ['-I.']
dbus_includes = ['.', 'include']
dbus_glib_includes = ['.', 'include']

pipe = os.popen3("pkg-config --cflags dbus-1")
output = pipe[1].read().strip()
error = pipe[2].read().strip()
for p in pipe:
    p.close()
if error:
    print "ERROR: running pkg-config (%s)" % (error)
    raise SystemExit
includedirs_flag.extend(output.split())
dbus_includes.extend([ x.replace("-I", "") for x in output.split() ])

pipe = os.popen3("pkg-config --cflags dbus-glib-1")
output = pipe[1].read().strip()
error = pipe[2].read().strip()
for p in pipe:
    p.close()
if error:
    print "ERROR: running pkg-config (%s)" % (error)
    raise SystemExit
includedirs_flag.extend(output.split())
dbus_glib_includes.extend([ x.replace("-I", "") for x in output.split() ])

#create ChangeLog only if this is a git repo
if os.path.exists(".git"):
    pipe = ""
    pipe = os.popen3("git-log --stat")

    output = pipe[1].read().strip()
    error = pipe[2].read().strip()

    if error:
        for p in pipe:
            p.close()

        pipe = os.popen3("git-log")
        output = pipe[1].read().strip()
        error = pipe[2].read().strip()

        if error:
            print "ERROR: running git-log (%s)" % (error)
            raise SystemExit

    for p in pipe:
        p.close()

    file = open("ChangeLog", "w")
    file.writelines(output)
    file.close()

dbus_libs = []
dbus_glib_libs = []

pipe = os.popen3("pkg-config --libs-only-L dbus-1")
output = pipe[1].read().strip()
error = pipe[2].read().strip()
for p in pipe:
    p.close()
if error:
    print "ERROR: running pkg-config (%s)" % (error)
    raise SystemExit
dbus_libs.extend([ x.replace("-L", "") for x in output.split() ])

pipe = os.popen3("pkg-config --libs-only-L dbus-glib-1")
output = pipe[1].read().strip()
error = pipe[2].read().strip()
for p in pipe:
    p.close()
if error:
    print "ERROR: running pkg-config (%s)" % (error)
    raise SystemExit
dbus_glib_libs.extend([ x.replace("-L", "") for x in output.split() ])

long_desc = '''D-BUS is a message bus system, a simple way for applications to
talk to one another.

D-BUS supplies both a system daemon (for events such as "new hardware device
added" or "printer queue changed") and a per-user-login-session daemon (for
general IPC needs among user applications). Also, the message bus is built on
top of a general one-to-one message passing framework, which can be used by any
two apps to communicate directly (without going through the message bus daemon).
Currently the communicating applications are on one computer, but TCP/IP option
is available and remote support planned.'''

setup(
    name='dbus-python',
    version='0.71',
    description='D-Bus Python bindings',
    long_description=long_desc,
    url='http://dbus.freedesktop.org/',
    author='John (J5) Palmieri',
    author_email='johnp@redhat.com',
    maintainer='John (J5) Palmieri',
    maintainer_email='johnp@redhat.com',
    package_dir={'dbus':'dbus'},
    py_modules=[
        "dbus/_dbus",
        "dbus/exceptions",
        "dbus/glib",
        "dbus/__init__",
        "dbus/matchrules",
        "dbus/service",
        "dbus/types",
        "dbus/decorators",
        "dbus/introspect_parser",
        "dbus/proxies",
    ],
    ext_modules=[
        Extension("_dbus_bindings", ["_dbus_bindings/module.c"],
            include_dirs=dbus_includes,
            library_dirs=dbus_libs,
            libraries=["dbus-1"],
        ),
        Extension("_dbus_glib_bindings", ["_dbus_glib_bindings/module.c"],
            include_dirs=dbus_glib_includes,
            library_dirs=dbus_glib_libs,
            libraries=["dbus-glib-1", "dbus-1", "glib-2.0"],
        ),
    ],
    cmdclass={'clean': full_clean, 'check': dbus_python_check}
)

# vim:ts=4:sw=4:tw=80:si:ai:showmatch:et
