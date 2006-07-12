import os
import sys

sys.path.append("dbus")

from distutils.core import setup
from distutils.extension import Extension
from distutils.command.clean import clean
from Pyrex.Distutils import build_ext

import extract

def remove(filename):
    if os.path.exists(filename):
        os.remove(filename)

class full_clean(clean):
    def run(self):
        clean.run(self)
        remove("dbus/extract.pyo")
        remove("dbus/dbus_bindings.pxd")
        remove("dbus/dbus_bindings.c")
        remove("dbus/dbus_glib_bindings.c")

includedirs_flag = ['-I.']
dbus_includes = ['.']
dbus_glib_includes = ['.']

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

output = open("dbus/dbus_bindings.pxd", 'w')
includedirs_flag.append('dbus')
extract.main("dbus/dbus_bindings.pxd.in", includedirs_flag, output)
output.close()

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
    name='dbus',
    version='0.70',
    description='D-Bus Python bindings',
    long_description=long_desc,
    url='http://dbus.freedesktop.org/',
    author='John (J5) Palmieri',
    author_email='johnp@redhat.com',
    maintainer='Osvaldo Santana Neto',
    maintainer_email='osvaldo.santana@indt.org.br',
    package_dir={'':'dbus'},
    py_modules=[
        "_dbus",
        "exceptions",
        "glib",
        "__init__",
        "matchrules",
        "service",
        "types",
        "decorators",
        "introspect_parser",
        "proxies",
        "_util",
    ],
    ext_modules=[
        Extension("dbus_bindings", ["dbus/dbus_bindings.pyx"],
            include_dirs=dbus_includes,
            libraries=["dbus-1"],

        ),
        Extension("dbus_glib_bindings", ["dbus/dbus_glib_bindings.pyx"],
            include_dirs=dbus_glib_includes,
            libraries=["dbus-glib-1", "dbus-1", "glib-2.0"],
            define_macros=[
                ('DBUS_API_SUBJECT_TO_CHANGE', '1')
            ],
        ),
    ],
    cmdclass={'build_ext': build_ext, 'clean': full_clean}
)

# vim:ts=4:sw=4:tw=80:si:ai:showmatch:et
