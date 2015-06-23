# This file was created SPECIFICALLY to create a bdist that can be generated on an x64 Ubuntu and an x64 Centos
# machines.
#
# Centos notes:
# You must:
# - yum install autoconf libtool
# - yum install dbus-glib-devel dbus-devel (and it must be >= 1.6.0 for dbus-python >= 1.2.0 to work)
from setuptools import setup, find_packages
from distutils.extension import Extension
from distutils.sysconfig import get_python_inc
from distutils.util import spawn
from distutils.command.build_ext import build_ext as _build_ext
from distutils import log
from glob import glob
import os
import sys
import shutil

root_dir = os.path.abspath(os.path.dirname(__file__))


def run_autogen():
    global root_dir
    os.environ['PYTHON'] = sys.executable
    spawn([os.path.join(root_dir, "autogen.sh")], search_path=False)


def load_version():
    import re
    from contextlib import closing
    version = [None, None, None]
    with closing(open(os.path.join(root_dir, "configure.ac"), "r")) as f:
        for line in f:
            # lines:
            # m4_define(dbus_python_major_version, 1)
            # m4_define(dbus_python_minor_version, 2)
            # m4_define(dbus_python_micro_version, 0)
            line = line.strip()
            if line.startswith("m4_define(dbus_python_major_version,"):
                index = 0
            elif line.startswith("m4_define(dbus_python_minor_version,"):
                index = 1
            elif line.startswith("m4_define(dbus_python_micro_version,"):
                index = 2
            else:
                index = None
            if index is not None:
                version[index] = re.search(r"\s+(\d+)\)", line).group(1)
    return ".".join(version)


class build_ext(_build_ext):
    def run(self, *args, **kwargs):
        # Make sure you have libtool >= 2.2.6 for Centos
        # We ignore distutils build because the native build creates a valid shared libraries and python's default
        # build doesn't (it then complains that _dbus_bindings.so is missing a symbol).
        os.environ['PYTHON'] = sys.executable
        run_autogen()
        spawn(["make", "-C", root_dir], search_path=True)
        for name in [ext.name for ext in self.extensions]:
            from_path = os.path.join(root_dir, name, ".libs", "{}.so".format(name))
            to_path = self.get_ext_fullpath(name)
            log.info("copying extension {} from {} to {}".format(name, from_path, to_path))
            shutil.copyfile(from_path, to_path)


if sys.platform.startswith('linux'):
    ext_modules = [
        Extension('_dbus_bindings', []),
        Extension('_dbus_glib_bindings', []),
    ]
else:
    ext_modules = []

setup(
    name='dbus-python',
    version=load_version(),
    description="DBus for Python",
    packages=find_packages(exclude=[]),
    zip_safe=False,
    ext_modules=ext_modules,
    cmdclass={'build_ext': build_ext}
)
