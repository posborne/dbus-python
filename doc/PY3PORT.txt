===============================
Porting python-dbus to Python 3
===============================

This is an experimental port to Python 3.x where x >= 2.  There are lots of
great sources for porting C extensions to Python 3, including:

 * http://python3porting.com/toc.html
 * http://docs.python.org/howto/cporting.html
 * http://docs.python.org/py3k/c-api/index.html

I also consulted an early take on this port by John Palmieri and David Malcolm
in the context of Fedora:

 * https://bugs.freedesktop.org/show_bug.cgi?id=26420

although I have made some different choices.  The patches in that tracker
issue also don't cover porting the Python bits (e.g. the test suite), nor the
pygtk -> pygi porting, both which I've also attempted to do in this branch.

This document outlines my notes and strategies for doing this port.  Please
feel free to contact me with any bugs, issues, disagreements, suggestions,
kudos, and curses.

Barry Warsaw
barry@python.org
2011-11-11


User visible changes
====================

You've got some dbus-python code that works great in Python 2.  This branch
should generally allow your existing Python 2 code to continue to work
unchanged.  There are a few changes you'll notice in Python 2 though:

 - The minimum supported Python 2 version is 2.6.
 - All object reprs are unicodes.  This change was made because it greatly
   simplifies the implementation and cross-compatibility with Python 3.
 - Some exception strings have changed.
 - `MethodCallMessage` and `SignalMessage` objects have better reprs now.

What do you need to do to port that to Python 3?  Here are the user visible
changes you should be aware of, relative to Python 2.  Python 3.2 is the
minimal required version:

 - `ByteArray` objects must be initialized with bytes objects, not unicodes.
   Use `b''` literals in the constructor.  This also works in Python 2, where
   bytes objects are aliases for 8-bit strings.
 - `Byte` objects must be initialized with either a length-1 bytes object
   (again, use `b''` literals to be compatible with either Python 2 or 3)
   or an integer.
 - byte signatures (i.e. `y` type codes) must be passed either a length-1
   bytes object or an integer. unicodes (str in Python 3) are not allowed.
 - `ByteArray` is now a subclass of `bytes`, where in Python 2 it is a
   subclass of `str`.
 - `dbus.UTF8String` is gone, use `dbus.String`.  Also `utf8_string` arguments
   are no longer allowed.
 - All longs are now ints, since Python 3 has only a single int type.  This
   also means that the class hierarchy for the dbus numeric types has changed
   (all derive from int in Python 3).


Bytes vs. Strings
=================

All strings in dbus are defined as UTF-8:

http://dbus.freedesktop.org/doc/dbus-specification.html#message-protocol-signatures

However, the dbus C API accepts `char*` which must be UTF-8 strings NUL
terminated and no other NUL bytes.

This page describes the mapping between Python types and dbus types:

    http://dbus.freedesktop.org/doc/dbus-python/doc/tutorial.html#basic-types

Notice that it maps dbus `string` (`'s'`) to `dbus.String` (unicode) or
`dbus.UTF8String` (str).  Also notice that there is no direct dbus equivalent
of Python's bytes type (although dbus does have byte arrays), so I am mapping
dbus strings to unicodes in all cases, and getting rid of `dbus.UTF8String` in
Python 3.  I've also added a `dbus._BytesBase` type which is unused in Python
2, but which forms the base class for `dbus.ByteArray` in Python 3.  This is
an implementation detail and not part of the public API.

In Python 3, object paths (`'o'` or `dbus.ObjectPath`), signatures (`'g'` or
`dbus.Signature`), bus names, interfaces, and methods are all strings.  A
previous aborted effort was made to use bytes for these, which at first blush
may makes some sense, but on deeper consideration does not.  This approach
also tended to impose too many changes on user code, and caused lots of
difficult to track down problems.

In Python 3, all such objects are subclasses of `str` (i.e. `unicode`).

(As an example, dbus-python's callback dispatching pretty much assumes all
these things are strings.  When they are bytes, the fact that `'foo' != b'foo'`
causes dispatch matching to fail in difficult to debug ways.  Even bus names
are not immune, since they do things like `bus_name[:1] == ':'` which fails in
multiple ways when `bus_name` is a bytes.  For sanity purposes, these are all
unicode strings now, and we just eat the complexity at the C level.)

I am using `#include <bytesobject.h>`, which exposes the PyBytes API to Python
2.6 and 2.7, and I have converted all internal PyString calls to PyBytes
calls.  Where this is inappropriate, we'll use PyUnicode calls explicitly.
E.g. all repr() implementations now return unicodes.  Most of these changes
shouldn't be noticed, even in existing Python 2 code.

Generally, I've left the descriptions and docstrings saying "str" instead of
"unicode" since there's no distinction in Python 3.

APIs which previously returned PyStrings will usually return PyUnicodes, not
PyBytes.


Ints vs. Longs
==============

Python 3 only has PyLong types; PyInts are gone.  For that reason, I've
switched all PyInt calls to use PyLong in both Python 2 and Python 3.  Python
3.0 had a nice `<intobject.h>` header that aliased PyInt to PyLong, but that's
gone as of Python 3.1, and the minimal required Python 3 version is 3.2.

In the above page mapping basic types, you'll notice that the Python int type
is mapped to 32-bit signed integers ('i') and the Python long type is mapped
to 64-bit signed integers ('x').  Python 3 doesn't have this distinction, so
ints map to 'i' even though ints can be larger in Python 3.  Use the
dbus-specific integer types if you must have more exact mappings.

APIs which accepted ints in Python 2 will still do so, but they'll also now
accept longs.  These APIs obviously only accept longs in Python 3.

Long literals in Python code are an interesting thing to have to port.  Don't
use them if you want your code to work in both Python versions.

`dbus._IntBase` is removed in Python 3, you only have `dbus._LongBase`, which
inherits from a Python 3 int (i.e. a PyLong).  Again, this is an
implementation detail that users should never care about.


Macros
======

In types-internal.h, I define `PY3K` when `PY_MAJOR_VERSION` >= 3, so you'll
see ifdefs on the former symbol within the C code.

Python 3 really could use a PY_REFCNT() wrapper for ob_refcnt access.


PyCapsule vs. PyCObject
=======================

`_dbus_bindings._C_API` is an attribute exposed to Python in the module.  In
Python 2, this is a PyCObject, but these do not exist in Python >= 3.2, so it
is replaced with a PyCapsules for Python 3.  However, since PyCapsules were
only introduced in Python 2.7, and I want to support Python 2.6, PyCObjects
are still used when this module is compiled for Python 2.


Python level compatibility
==========================

`from dbus import _is_py3` gives you a flag to check if you must do something
different in Python 3.  In general I use this flag to support both versions in
one set of sources, which seems better than trying to use 2to3.  It's not part
of the dbus-python public API, so you must not use it in third-party projects.


Miscellaneous
=============

The PyDoc_STRVAR() documentation is probably out of date.  Once the API
choices have been green-lighted upstream, I'll make a pass through the code to
update them.  It might be tricky based on any differences between Python 2 and
Python 3.

There were a few places where I noticed what might be considered bugs,
unchecked exception conditions, or possible reference count leaks.  In these
cases, I've just fixed what I can and hopefully haven't made the situation
worse.

`dbus_py_variant_level_get()` did not check possible error conditions, nor did
their callers.  When `dbus_py_variant_level_get()` encounters an error, it now
returns -1, and callers check this.

As much as possible, I've refrained from general code cleanups (e.g. 80
columns), unless it just bugged me too much or I touched the code for reasons
related to the port.  I've also tried to stick to existing C code style,
e.g. through the use of pervasive `Py_CLEAR()` calls, comparison against NULL
usually with `!foo`, and such.  As Bart Simpson might write on his classroom
blackboard::

    This is not a rewrite
    This is not a rewrite
    This is not a rewrite
    This is not a rewrite
    ...

and so on.  Well, mostly ;).

I think I fixed a reference leak in `DBusPyServer_set_auth_mechanisms()`.
`PySequence_Fast()` returns a new reference, which wasn't getting decref'd in
any return path.

 - Instantiation of metaclasses uses different, incompatible syntax in Python
   2 and 3.  You have to use direct calling of the metaclass to work across
   versions, i.e. `Interface = InterfaceType('Interface', (object,), {})`
 - `iteritems()` and friends are gone.  I dropped the "iter" prefixes.
 - `xrange() is gone.  I changed them to use `range()`.
 - `isSequenceType()` is gone in Python 3, so I use a different idiom there.
 - `__next__()` vs. `next()`
 - `PyUnicode_FromFormat()` `%V` flag is a clever hack!
 - `sys.version_info` is a tuple in Python 2.6, not a namedtuple.  i.e. there
   is no `sys.version_info.major`
 - `PyArg_Parse()`: No 'y' code in Python 2; in Python 3, no equivalent of 'z'
   for bytes objects.


Open issues
===========

Here are a few things that still need to be done, or for which there may be
open questions::

 - Update all C extension docstrings for accuracy.
