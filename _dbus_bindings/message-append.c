/* D-Bus Message serialization. This contains all the logic to map from
 * Python objects to D-Bus types.
 *
 * Copyright (C) 2006 Collabora Ltd. <http://www.collabora.co.uk/>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <config.h>

#include <assert.h>

#define DBG_IS_TOO_VERBOSE
#include "compat-internal.h"
#include "types-internal.h"
#include "message-internal.h"

/* Return the number of variants wrapping the given object. Return 0
 * if the object is not a D-Bus type.
 */
static long
get_variant_level(PyObject *obj)
{
    if (DBusPyString_Check(obj)) {
        return ((DBusPyString *)obj)->variant_level;
    }
#ifndef PY3
    else if (DBusPyIntBase_Check(obj)) {
        return ((DBusPyIntBase *)obj)->variant_level;
    }
#endif
    else if (DBusPyFloatBase_Check(obj)) {
        return ((DBusPyFloatBase *)obj)->variant_level;
    }
    else if (DBusPyArray_Check(obj)) {
        return ((DBusPyArray *)obj)->variant_level;
    }
    else if (DBusPyDict_Check(obj)) {
        return ((DBusPyDict *)obj)->variant_level;
    }
    else if (DBusPyLongBase_Check(obj) ||
#ifdef PY3
             DBusPyBytesBase_Check(obj) ||
#endif
             DBusPyStrBase_Check(obj) ||
             DBusPyStruct_Check(obj)) {
        return dbus_py_variant_level_get(obj);
    }
    else {
        return 0;
    }
}

char dbus_py_Message_append__doc__[] = (
"set_args(*args[, **kwargs])\n\n"
"Set the message's arguments from the positional parameter, according to\n"
"the signature given by the ``signature`` keyword parameter.\n"
"\n"
"The following type conversions are supported:\n\n"
"=============================== ===========================\n"
"D-Bus (in signature)            Python\n"
"=============================== ===========================\n"
"boolean (b)                     any object (via bool())\n"
"byte (y)                        string of length 1\n"
"                                any integer\n"
"any integer type                any integer\n"
"double (d)                      any float\n"
"object path                     anything with a __dbus_object_path__ attribute\n"
"string, signature, object path  str (must be UTF-8) or unicode\n"
"dict (a{...})                   any mapping\n"
"array (a...)                    any iterable over appropriate objects\n"
"struct ((...))                  any iterable over appropriate objects\n"
"variant                         any object above (guess type as below)\n"
"=============================== ===========================\n"
"\n"
"Here 'any integer' means anything on which int() or long()\n"
"(as appropriate) will work, except for basestring subclasses.\n"
"'Any float' means anything on which float() will work, except\n"
"for basestring subclasses.\n"
"\n"
"If there is no signature, guess from the arguments using\n"
"the static method `Message.guess_signature`.\n"
);

char dbus_py_Message_guess_signature__doc__[] = (
"guess_signature(*args) -> Signature [static method]\n\n"
"Guess a D-Bus signature which should be used to encode the given\n"
"Python objects.\n"
"\n"
"The signature is constructed as follows:\n\n"
"+-------------------------------+---------------------------+\n"
"|Python                         |D-Bus                      |\n"
"+===============================+===========================+\n"
"|D-Bus type, variant_level > 0  |variant (v)                |\n"
"+-------------------------------+---------------------------+\n"
"|D-Bus type, variant_level == 0 |the corresponding type     |\n"
"+-------------------------------+---------------------------+\n"
"|anything with a                |object path                |\n"
"|__dbus_object_path__ attribute |                           |\n"
"+-------------------------------+---------------------------+\n"
"|bool                           |boolean (y)                |\n"
"+-------------------------------+---------------------------+\n"
"|any other int subclass         |int32 (i)                  |\n"
"+-------------------------------+---------------------------+\n"
"|any other long subclass        |int64 (x)                  |\n"
"+-------------------------------+---------------------------+\n"
"|any other float subclass       |double (d)                 |\n"
"+-------------------------------+---------------------------+\n"
"|any other str subclass         |string (s)                 |\n"
"+-------------------------------+---------------------------+\n"
"|any other unicode subclass     |string (s)                 |\n"
"+-------------------------------+---------------------------+\n"
"|any other tuple subclass       |struct ((...))             |\n"
"+-------------------------------+---------------------------+\n"
"|any other list subclass        |array (a...), guess        |\n"
"|                               |contents' type according to|\n"
"|                               |type of first item         |\n"
"+-------------------------------+---------------------------+\n"
"|any other dict subclass        |dict (a{...}), guess key,  |\n"
"|                               |value type according to    |\n"
"|                               |types for an arbitrary item|\n"
"+-------------------------------+---------------------------+\n"
"|anything else                  |raise TypeError            |\n"
"+-------------------------------+---------------------------+\n"
);

/* return a new reference, possibly to None */
static PyObject *
get_object_path(PyObject *obj)
{
    PyObject *magic_attr = PyObject_GetAttr(obj, dbus_py__dbus_object_path__const);

    if (magic_attr) {
        if (PyUnicode_Check(magic_attr) || PyBytes_Check(magic_attr)) {
            return magic_attr;
        }
        else {
            Py_CLEAR(magic_attr);
            PyErr_SetString(PyExc_TypeError, "__dbus_object_path__ must be "
                            "a string");
            return NULL;
        }
    }
    else {
        /* Ignore exceptions, except for SystemExit and KeyboardInterrupt */
        if (PyErr_ExceptionMatches(PyExc_SystemExit) ||
            PyErr_ExceptionMatches(PyExc_KeyboardInterrupt))
            return NULL;
        PyErr_Clear();
        Py_RETURN_NONE;
    }
}

/* Return a new reference. If the object is a variant and variant_level_ptr
 * is not NULL, put the variant level in the variable pointed to, and
 * return the contained type instead of "v". */
static PyObject *
_signature_string_from_pyobject(PyObject *obj, long *variant_level_ptr)
{
    PyObject *magic_attr;
    long variant_level = get_variant_level(obj);

    if (variant_level < 0)
        return NULL;

    if (variant_level_ptr) {
        *variant_level_ptr = variant_level;
    }
    else if (variant_level > 0) {
        return NATIVESTR_FROMSTR(DBUS_TYPE_VARIANT_AS_STRING);
    }

    if (obj == Py_True || obj == Py_False) {
      return NATIVESTR_FROMSTR(DBUS_TYPE_BOOLEAN_AS_STRING);
    }

    magic_attr = get_object_path(obj);
    if (!magic_attr)
        return NULL;
    if (magic_attr != Py_None) {
        Py_CLEAR(magic_attr);
        return NATIVESTR_FROMSTR(DBUS_TYPE_OBJECT_PATH_AS_STRING);
    }
    Py_CLEAR(magic_attr);

    /* Ordering is important: some of these are subclasses of each other. */
#ifdef PY3
    if (PyLong_Check(obj)) {
        if (DBusPyUInt64_Check(obj))
            return NATIVESTR_FROMSTR(DBUS_TYPE_UINT64_AS_STRING);
        else if (DBusPyInt64_Check(obj))
            return NATIVESTR_FROMSTR(DBUS_TYPE_INT64_AS_STRING);
        else if (DBusPyUInt32_Check(obj))
            return NATIVESTR_FROMSTR(DBUS_TYPE_UINT32_AS_STRING);
        else if (DBusPyInt32_Check(obj))
            return NATIVESTR_FROMSTR(DBUS_TYPE_INT32_AS_STRING);
        else if (DBusPyUInt16_Check(obj))
            return NATIVESTR_FROMSTR(DBUS_TYPE_UINT16_AS_STRING);
        else if (DBusPyInt16_Check(obj))
            return NATIVESTR_FROMSTR(DBUS_TYPE_INT16_AS_STRING);
        else if (DBusPyByte_Check(obj))
            return NATIVESTR_FROMSTR(DBUS_TYPE_BYTE_AS_STRING);
        else if (DBusPyBoolean_Check(obj))
            return NATIVESTR_FROMSTR(DBUS_TYPE_BOOLEAN_AS_STRING);
        else
            return NATIVESTR_FROMSTR(DBUS_TYPE_INT32_AS_STRING);
    }
#else  /* !PY3 */
    if (PyInt_Check(obj)) {
        if (DBusPyInt16_Check(obj))
            return NATIVESTR_FROMSTR(DBUS_TYPE_INT16_AS_STRING);
        else if (DBusPyInt32_Check(obj))
            return NATIVESTR_FROMSTR(DBUS_TYPE_INT32_AS_STRING);
        else if (DBusPyByte_Check(obj))
            return NATIVESTR_FROMSTR(DBUS_TYPE_BYTE_AS_STRING);
        else if (DBusPyUInt16_Check(obj))
            return NATIVESTR_FROMSTR(DBUS_TYPE_UINT16_AS_STRING);
        else if (DBusPyBoolean_Check(obj))
            return NATIVESTR_FROMSTR(DBUS_TYPE_BOOLEAN_AS_STRING);
        else
            return NATIVESTR_FROMSTR(DBUS_TYPE_INT32_AS_STRING);
    }
    else if (PyLong_Check(obj)) {
        if (DBusPyInt64_Check(obj))
            return NATIVESTR_FROMSTR(DBUS_TYPE_INT64_AS_STRING);
        else if (DBusPyUInt32_Check(obj))
            return NATIVESTR_FROMSTR(DBUS_TYPE_UINT32_AS_STRING);
        else if (DBusPyUInt64_Check(obj))
            return NATIVESTR_FROMSTR(DBUS_TYPE_UINT64_AS_STRING);
        else
            return NATIVESTR_FROMSTR(DBUS_TYPE_INT64_AS_STRING);
    }
#endif  /* PY3 */
    else if (PyUnicode_Check(obj)) {
        /* Object paths and signatures are unicode subtypes in Python 3
         * (the first two cases will never be true in Python 2) */
        if (DBusPyObjectPath_Check(obj))
            return NATIVESTR_FROMSTR(DBUS_TYPE_OBJECT_PATH_AS_STRING);
        else if (DBusPySignature_Check(obj))
            return NATIVESTR_FROMSTR(DBUS_TYPE_SIGNATURE_AS_STRING);
        else
            return NATIVESTR_FROMSTR(DBUS_TYPE_STRING_AS_STRING);
    }
#if defined(DBUS_TYPE_UNIX_FD)
    else if (DBusPyUnixFd_Check(obj))
        return NATIVESTR_FROMSTR(DBUS_TYPE_UNIX_FD_AS_STRING);
#endif
    else if (PyFloat_Check(obj)) {
#ifdef WITH_DBUS_FLOAT32
        if (DBusPyDouble_Check(obj))
            return NATIVESTR_FROMSTR(DBUS_TYPE_DOUBLE_AS_STRING);
        else if (DBusPyFloat_Check(obj))
            return NATIVESTR_FROMSTR(DBUS_TYPE_FLOAT_AS_STRING);
        else
#endif
            return NATIVESTR_FROMSTR(DBUS_TYPE_DOUBLE_AS_STRING);
    }
    else if (PyBytes_Check(obj)) {
        /* Object paths and signatures are bytes subtypes in Python 2
         * (the first two cases will never be true in Python 3) */
        if (DBusPyObjectPath_Check(obj))
            return NATIVESTR_FROMSTR(DBUS_TYPE_OBJECT_PATH_AS_STRING);
        else if (DBusPySignature_Check(obj))
            return NATIVESTR_FROMSTR(DBUS_TYPE_SIGNATURE_AS_STRING);
        else if (DBusPyByteArray_Check(obj))
            return NATIVESTR_FROMSTR(DBUS_TYPE_ARRAY_AS_STRING
                                     DBUS_TYPE_BYTE_AS_STRING);
        else
            return NATIVESTR_FROMSTR(DBUS_TYPE_STRING_AS_STRING);
    }
    else if (PyTuple_Check(obj)) {
        Py_ssize_t len = PyTuple_GET_SIZE(obj);
        PyObject *list = PyList_New(len + 2);   /* new ref */
        PyObject *item;                         /* temporary new ref */
        PyObject *empty_str;                    /* temporary new ref */
        PyObject *ret;
        Py_ssize_t i;

        if (!list) return NULL;
        if (len == 0) {
            PyErr_SetString(PyExc_ValueError, "D-Bus structs cannot be empty");
            Py_CLEAR(list);
            return NULL;
        }
        /* Set the first and last elements of list to be the parentheses */
        item = NATIVESTR_FROMSTR(DBUS_STRUCT_BEGIN_CHAR_AS_STRING);
        if (PyList_SetItem(list, 0, item) < 0) {
            Py_CLEAR(list);
            return NULL;
        }
        item = NATIVESTR_FROMSTR(DBUS_STRUCT_END_CHAR_AS_STRING);
        if (PyList_SetItem(list, len + 1, item) < 0) {
            Py_CLEAR(list);
            return NULL;
        }
        if (!item || !PyList_GET_ITEM(list, 0)) {
            Py_CLEAR(list);
            return NULL;
        }
        item = NULL;

        for (i = 0; i < len; i++) {
            item = PyTuple_GetItem(obj, i);
            if (!item) {
                Py_CLEAR(list);
                return NULL;
            }
            item = _signature_string_from_pyobject(item, NULL);
            if (!item) {
                Py_CLEAR(list);
                return NULL;
            }
            if (PyList_SetItem(list, i + 1, item) < 0) {
                Py_CLEAR(list);
                return NULL;
            }
            item = NULL;
        }
        empty_str = NATIVESTR_FROMSTR("");
        if (!empty_str) {
            /* really shouldn't happen */
            Py_CLEAR(list);
            return NULL;
        }
        ret = PyObject_CallMethod(empty_str, "join", "(O)", list); /* new ref */
        /* whether ret is NULL or not, */
        Py_CLEAR(empty_str);
        Py_CLEAR(list);
        return ret;
    }
    else if (PyList_Check(obj)) {
        PyObject *tmp;
        PyObject *ret = NATIVESTR_FROMSTR(DBUS_TYPE_ARRAY_AS_STRING);
        if (!ret) return NULL;
#ifdef PY3
        if (DBusPyArray_Check(obj) &&
            PyUnicode_Check(((DBusPyArray *)obj)->signature))
        {
            PyObject *concat = PyUnicode_Concat(
                ret, ((DBusPyArray *)obj)->signature);
            Py_CLEAR(ret);
            return concat;
        }
#else
        if (DBusPyArray_Check(obj) &&
            PyBytes_Check(((DBusPyArray *)obj)->signature))
        {
            PyBytes_Concat(&ret, ((DBusPyArray *)obj)->signature);
            return ret;
        }
#endif
        if (PyList_GET_SIZE(obj) == 0) {
            /* No items, so fail. Or should we guess "av"? */
            PyErr_SetString(PyExc_ValueError, "Unable to guess signature "
                            "from an empty list");
            return NULL;
        }
        tmp = PyList_GetItem(obj, 0);
        tmp = _signature_string_from_pyobject(tmp, NULL);
        if (!tmp) return NULL;
#ifdef PY3
        {
            PyObject *concat = PyUnicode_Concat(ret, tmp);
            Py_CLEAR(ret);
            Py_CLEAR(tmp);
            return concat;
        }
#else
        PyBytes_ConcatAndDel(&ret, tmp);
        return ret;
#endif
    }
    else if (PyDict_Check(obj)) {
        PyObject *key, *value, *keysig, *valuesig;
        Py_ssize_t pos = 0;
        PyObject *ret = NULL;

#ifdef PY3
        if (DBusPyDict_Check(obj) &&
            PyUnicode_Check(((DBusPyDict *)obj)->signature))
        {
            return PyUnicode_FromFormat((DBUS_TYPE_ARRAY_AS_STRING
                                         DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                                         "%U"
                                         DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
                                        ((DBusPyDict *)obj)->signature);
        }
#else
        if (DBusPyDict_Check(obj) &&
            PyBytes_Check(((DBusPyDict *)obj)->signature))
        {
            const char *sig = PyBytes_AS_STRING(((DBusPyDict *)obj)->signature);

            return PyBytes_FromFormat((DBUS_TYPE_ARRAY_AS_STRING
                                       DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                                       "%s"
                                       DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
                                      sig);
        }
#endif
        if (!PyDict_Next(obj, &pos, &key, &value)) {
            /* No items, so fail. Or should we guess "a{vv}"? */
            PyErr_SetString(PyExc_ValueError, "Unable to guess signature "
                             "from an empty dict");
            return NULL;
        }
        keysig = _signature_string_from_pyobject(key, NULL);
        valuesig = _signature_string_from_pyobject(value, NULL);
        if (keysig && valuesig) {
#ifdef PY3
            ret = PyUnicode_FromFormat((DBUS_TYPE_ARRAY_AS_STRING
                                        DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                                        "%U%U"
                                        DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
                                       keysig, valuesig);
#else
            ret = PyBytes_FromFormat((DBUS_TYPE_ARRAY_AS_STRING
                                      DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                                      "%s%s"
                                      DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
                                     PyBytes_AS_STRING(keysig),
                                     PyBytes_AS_STRING(valuesig));
#endif
        }
        Py_CLEAR(keysig);
        Py_CLEAR(valuesig);
        return ret;
    }
    else {
        PyErr_Format(PyExc_TypeError, "Don't know which D-Bus type "
                     "to use to encode type \"%s\"",
                     Py_TYPE(obj)->tp_name);
        return NULL;
    }
}

PyObject *
dbus_py_Message_guess_signature(PyObject *unused UNUSED, PyObject *args)
{
    PyObject *tmp, *ret = NULL;

    if (!args) {
        if (!PyErr_Occurred()) {
            PyErr_BadInternalCall();
        }
        return NULL;
    }

#ifdef USING_DBG
    fprintf(stderr, "DBG/%ld: called Message_guess_signature", (long)getpid());
    PyObject_Print(args, stderr, 0);
    fprintf(stderr, "\n");
#endif

    if (!PyTuple_Check(args)) {
        DBG("%s", "Message_guess_signature: args not a tuple");
        PyErr_BadInternalCall();
        return NULL;
    }

    /* if there were no args, easy */
    if (PyTuple_GET_SIZE(args) == 0) {
        DBG("%s", "Message_guess_signature: no args, so return Signature('')");
        return PyObject_CallFunction((PyObject *)&DBusPySignature_Type, "(s)", "");
    }

    /* if there were args, the signature we want is, by construction,
     * exactly the signature we get for the tuple args, except that we don't
     * want the parentheses. */
    tmp = _signature_string_from_pyobject(args, NULL);
    if (!tmp) {
        DBG("%s", "Message_guess_signature: failed");
        return NULL;
    }
    if (PyUnicode_Check(tmp)) {
        PyObject *as_bytes = PyUnicode_AsUTF8String(tmp);
        Py_CLEAR(tmp);
        if (!as_bytes)
            return NULL;
        if (PyBytes_GET_SIZE(as_bytes) < 2) {
            PyErr_SetString(PyExc_RuntimeError, "Internal error: "
                            "_signature_string_from_pyobject returned "
                            "a bad result");
            Py_CLEAR(as_bytes);
            return NULL;
        }
        tmp = as_bytes;
    }
    if (!PyBytes_Check(tmp) || PyBytes_GET_SIZE(tmp) < 2) {
        PyErr_SetString(PyExc_RuntimeError, "Internal error: "
                        "_signature_string_from_pyobject returned "
                        "a bad result");
        Py_CLEAR(tmp);
        return NULL;
    }
    ret = PyObject_CallFunction((PyObject *)&DBusPySignature_Type, "(s#)",
                                PyBytes_AS_STRING(tmp) + 1,
                                PyBytes_GET_SIZE(tmp) - 2);
    Py_CLEAR(tmp);
    return ret;
}

static int _message_iter_append_pyobject(DBusMessageIter *appender,
                                         DBusSignatureIter *sig_iter,
                                         PyObject *obj,
                                         dbus_bool_t *more);
static int _message_iter_append_variant(DBusMessageIter *appender,
                                        PyObject *obj);

static int
_message_iter_append_string(DBusMessageIter *appender,
                            int sig_type, PyObject *obj,
                            dbus_bool_t allow_object_path_attr)
{
    char *s;
    PyObject *utf8;

    if (sig_type == DBUS_TYPE_OBJECT_PATH && allow_object_path_attr) {
        PyObject *object_path = get_object_path (obj);

        if (object_path == Py_None) {
            Py_CLEAR(object_path);
        }
        else if (!object_path) {
            return -1;
        }
        else {
            int ret = _message_iter_append_string(appender, sig_type,
                                                  object_path, FALSE);
            Py_CLEAR(object_path);
            return ret;
        }
    }

    if (PyBytes_Check(obj)) {
        utf8 = obj;
        Py_INCREF(obj);
    }
    else if (PyUnicode_Check(obj)) {
        utf8 = PyUnicode_AsUTF8String(obj);
        if (!utf8) return -1;
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "Expected a string or unicode object");
        return -1;
    }

    /* Raise TypeError if the string has embedded NULs */
    if (PyBytes_AsStringAndSize(utf8, &s, NULL) < 0)
        return -1;

    /* Validate UTF-8, strictly */
#ifdef HAVE_DBUS_VALIDATE_UTF8
    if (!dbus_validate_utf8(s, NULL)) {
        PyErr_SetString(PyExc_UnicodeError, "String parameters "
                        "to be sent over D-Bus must be valid UTF-8 "
                        "with no noncharacter code points");
        return -1;
    }
#else
    {
        PyObject *back_to_unicode;
        PyObject *utf32;
        Py_ssize_t i;

        /* This checks for syntactically valid UTF-8, but does not check
         * for noncharacters (U+nFFFE, U+nFFFF for any n, or U+FDD0..U+FDEF).
         */
        back_to_unicode = PyUnicode_DecodeUTF8(s, PyBytes_GET_SIZE(utf8),
                                               "strict");

        if (!back_to_unicode) {
            return -1;
        }

        utf32 = PyUnicode_AsUTF32String(back_to_unicode);
        Py_CLEAR(back_to_unicode);

        if (!utf32) {
            return -1;
        }

        for (i = 0; i < PyBytes_GET_SIZE(utf32) / 4; i++) {
            dbus_uint32_t *p;

            p = (dbus_uint32_t *) (PyBytes_AS_STRING(utf32)) + i;

            if (/* noncharacters U+nFFFE, U+nFFFF */
                (*p & 0xFFFF) == 0xFFFE ||
                (*p & 0xFFFF) == 0xFFFF ||
                /* noncharacters U+FDD0..U+FDEF */
                (*p >= 0xFDD0 && *p <= 0xFDEF) ||
                /* surrogates U+D800..U+DBFF (low), U+DC00..U+DFFF (high) */
                (*p >= 0xD800 && *p <= 0xDFFF) ||
                (*p >= 0x110000)) {
                Py_CLEAR(utf32);
                PyErr_SetString(PyExc_UnicodeError, "String parameters "
                                "to be sent over D-Bus must be valid UTF-8 "
                                "with no noncharacter code points");
                return -1;
            }
        }

        Py_CLEAR(utf32);
    }
#endif

    DBG("Performing actual append: string (from unicode) %s", s);
    if (!dbus_message_iter_append_basic(appender, sig_type, &s)) {
        Py_CLEAR(utf8);
        PyErr_NoMemory();
        return -1;
    }

    Py_CLEAR(utf8);
    return 0;
}

static int
_message_iter_append_byte(DBusMessageIter *appender, PyObject *obj)
{
    unsigned char y;

    if (PyBytes_Check(obj)) {
        if (PyBytes_GET_SIZE(obj) != 1) {
            PyErr_Format(PyExc_ValueError,
                         "Expected a length-1 bytes but found %d bytes",
                         (int)PyBytes_GET_SIZE(obj));
            return -1;
        }
        y = *(unsigned char *)PyBytes_AS_STRING(obj);
    }
    else {
        /* on Python 2 this accepts either int or long */
        long i = PyLong_AsLong(obj);

        if (i == -1 && PyErr_Occurred()) return -1;
        if (i < 0 || i > 0xff) {
            PyErr_Format(PyExc_ValueError,
                         "%d outside range for a byte value",
                         (int)i);
            return -1;
        }
        y = i;
    }
    DBG("Performing actual append: byte \\x%02x", (unsigned)y);
    if (!dbus_message_iter_append_basic(appender, DBUS_TYPE_BYTE, &y)) {
        PyErr_NoMemory();
        return -1;
    }
    return 0;
}

static dbus_bool_t
dbuspy_message_iter_close_container(DBusMessageIter *iter,
                                    DBusMessageIter *sub,
                                    dbus_bool_t is_ok)
{
    if (!is_ok) {
        dbus_message_iter_abandon_container(iter, sub);
        return TRUE;
    }
    return dbus_message_iter_close_container(iter, sub);
}

#if defined(DBUS_TYPE_UNIX_FD)
static int
_message_iter_append_unixfd(DBusMessageIter *appender, PyObject *obj)
{
    int fd;
    long original_fd;

    if (INTORLONG_CHECK(obj))
    {
        /* on Python 2 this accepts either int or long */
        original_fd = PyLong_AsLong(obj);
        if (original_fd == -1 && PyErr_Occurred())
            return -1;
        if (original_fd < INT_MIN || original_fd > INT_MAX) {
            PyErr_Format(PyExc_ValueError, "out of int range: %ld",
                         original_fd);
            return -1;
        }
        fd = (int)original_fd;
    }
    else if (PyObject_IsInstance(obj, (PyObject*) &DBusPyUnixFd_Type)) {
        fd = dbus_py_unix_fd_get_fd(obj);
    }
    else {
        return -1;
    }

    DBG("Performing actual append: fd %d", fd);
    if (!dbus_message_iter_append_basic(appender, DBUS_TYPE_UNIX_FD, &fd)) {
        PyErr_NoMemory();
        return -1;
    }
    return 0;
}
#endif

static int
_message_iter_append_dictentry(DBusMessageIter *appender,
                               DBusSignatureIter *sig_iter,
                               PyObject *dict, PyObject *key)
{
    DBusSignatureIter sub_sig_iter;
    DBusMessageIter sub;
    int ret = -1;
    PyObject *value = PyObject_GetItem(dict, key);
    dbus_bool_t more;

    if (!value) return -1;

#ifdef USING_DBG
    fprintf(stderr, "Append dictentry: ");
    PyObject_Print(key, stderr, 0);
    fprintf(stderr, " => ");
    PyObject_Print(value, stderr, 0);
    fprintf(stderr, "\n");
#endif

    DBG("Recursing signature iterator %p -> %p", sig_iter, &sub_sig_iter);
    dbus_signature_iter_recurse(sig_iter, &sub_sig_iter);
#ifdef USING_DBG
    {
        char *s;
        s = dbus_signature_iter_get_signature(sig_iter);
        DBG("Signature of parent iterator %p is %s", sig_iter, s);
        dbus_free(s);
        s = dbus_signature_iter_get_signature(&sub_sig_iter);
        DBG("Signature of sub-iterator %p is %s", &sub_sig_iter, s);
        dbus_free(s);
    }
#endif

    DBG("%s", "Opening DICT_ENTRY container");
    if (!dbus_message_iter_open_container(appender, DBUS_TYPE_DICT_ENTRY,
                                          NULL, &sub)) {
        PyErr_NoMemory();
        goto out;
    }
    ret = _message_iter_append_pyobject(&sub, &sub_sig_iter, key, &more);
    if (ret == 0) {
        ret = _message_iter_append_pyobject(&sub, &sub_sig_iter, value, &more);
    }
    DBG("%s", "Closing DICT_ENTRY container");
    if (!dbuspy_message_iter_close_container(appender, &sub, (ret == 0))) {
        PyErr_NoMemory();
        ret = -1;
    }
out:
    Py_CLEAR(value);
    return ret;
}

static int
_message_iter_append_multi(DBusMessageIter *appender,
                           const DBusSignatureIter *sig_iter,
                           int mode, PyObject *obj)
{
    DBusMessageIter sub_appender;
    DBusSignatureIter sub_sig_iter;
    PyObject *contents;
    int ret;
    PyObject *iterator = PyObject_GetIter(obj);
    char *sig = NULL;
    int container = mode;
    dbus_bool_t is_byte_array = DBusPyByteArray_Check(obj);
    int inner_type;
    dbus_bool_t more;

    assert(mode == DBUS_TYPE_DICT_ENTRY || mode == DBUS_TYPE_ARRAY ||
            mode == DBUS_TYPE_STRUCT);

#ifdef USING_DBG
    fprintf(stderr, "Appending multiple: ");
    PyObject_Print(obj, stderr, 0);
    fprintf(stderr, "\n");
#endif

    if (!iterator) return -1;
    if (mode == DBUS_TYPE_DICT_ENTRY) container = DBUS_TYPE_ARRAY;

    DBG("Recursing signature iterator %p -> %p", sig_iter, &sub_sig_iter);
    dbus_signature_iter_recurse(sig_iter, &sub_sig_iter);
#ifdef USING_DBG
    {
        char *s;
        s = dbus_signature_iter_get_signature(sig_iter);
        DBG("Signature of parent iterator %p is %s", sig_iter, s);
        dbus_free(s);
        s = dbus_signature_iter_get_signature(&sub_sig_iter);
        DBG("Signature of sub-iterator %p is %s", &sub_sig_iter, s);
        dbus_free(s);
    }
#endif
    inner_type = dbus_signature_iter_get_current_type(&sub_sig_iter);

    if (mode == DBUS_TYPE_ARRAY || mode == DBUS_TYPE_DICT_ENTRY) {
        sig = dbus_signature_iter_get_signature(&sub_sig_iter);
        if (!sig) {
            PyErr_NoMemory();
            ret = -1;
            goto out;
        }
    }
    /* else leave sig set to NULL. */

    DBG("Opening '%c' container", container);
    if (!dbus_message_iter_open_container(appender, container,
                                          sig, &sub_appender)) {
        PyErr_NoMemory();
        ret = -1;
        goto out;
    }
    ret = 0;
    more = TRUE;
    while ((contents = PyIter_Next(iterator))) {

        if (mode == DBUS_TYPE_ARRAY || mode == DBUS_TYPE_DICT_ENTRY) {
            DBG("Recursing signature iterator %p -> %p", sig_iter, &sub_sig_iter);
            dbus_signature_iter_recurse(sig_iter, &sub_sig_iter);
#ifdef USING_DBG
            {
                char *s;
                s = dbus_signature_iter_get_signature(sig_iter);
                DBG("Signature of parent iterator %p is %s", sig_iter, s);
                dbus_free(s);
                s = dbus_signature_iter_get_signature(&sub_sig_iter);
                DBG("Signature of sub-iterator %p is %s", &sub_sig_iter, s);
                dbus_free(s);
            }
#endif
        }
        else /* struct */ {
            if (!more) {
                PyErr_Format(PyExc_TypeError, "Fewer items found in struct's "
                             "D-Bus signature than in Python arguments ");
                ret = -1;
                break;
            }
        }

        if (mode == DBUS_TYPE_DICT_ENTRY) {
            ret = _message_iter_append_dictentry(&sub_appender, &sub_sig_iter,
                                                 obj, contents);
        }
        else if (mode == DBUS_TYPE_ARRAY && is_byte_array
                 && inner_type == DBUS_TYPE_VARIANT) {
            /* Subscripting a ByteArray gives a str of length 1, but if the
             * container is a ByteArray and the parameter is an array of
             * variants, we want to produce an array of variants containing
             * bytes, not strings.
             */
            PyObject *args = Py_BuildValue("(O)", contents);
            PyObject *byte;

            if (!args)
                break;
            byte = PyObject_Call((PyObject *)&DBusPyByte_Type, args, NULL);
            Py_CLEAR(args);
            if (!byte)
                break;
            ret = _message_iter_append_variant(&sub_appender, byte);
            Py_CLEAR(byte);
        }
        else {
            /* advances sub_sig_iter and sets more on success - for array
             * this doesn't matter, for struct it's essential */
            ret = _message_iter_append_pyobject(&sub_appender, &sub_sig_iter,
                                                contents, &more);
        }

        Py_CLEAR(contents);
        if (ret < 0) {
            break;
        }
    }

    if (PyErr_Occurred()) {
        ret = -1;
    }
    else if (mode == DBUS_TYPE_STRUCT && more) {
        PyErr_Format(PyExc_TypeError, "More items found in struct's D-Bus "
                     "signature than in Python arguments ");
        ret = -1;
    }

    /* This must be run as cleanup, even on failure. */
    DBG("Closing '%c' container", container);
    if (!dbuspy_message_iter_close_container(appender, &sub_appender, (ret == 0))) {
        PyErr_NoMemory();
        ret = -1;
    }

out:
    Py_CLEAR(iterator);
    dbus_free(sig);
    return ret;
}

static int
_message_iter_append_string_as_byte_array(DBusMessageIter *appender,
                                          PyObject *obj)
{
    /* a bit of a faster path for byte arrays that are strings */
    Py_ssize_t len = PyBytes_GET_SIZE(obj);
    const char *s;
    DBusMessageIter sub;
    int ret;

    s = PyBytes_AS_STRING(obj);
    DBG("%s", "Opening ARRAY container");
    if (!dbus_message_iter_open_container(appender, DBUS_TYPE_ARRAY,
                                          DBUS_TYPE_BYTE_AS_STRING, &sub)) {
        PyErr_NoMemory();
        return -1;
    }
    DBG("Appending fixed array of %d bytes", (int)len);
    if (dbus_message_iter_append_fixed_array(&sub, DBUS_TYPE_BYTE, &s, len)) {
        ret = 0;
    }
    else {
        PyErr_NoMemory();
        ret = -1;
    }
    DBG("%s", "Closing ARRAY container");
    if (!dbus_message_iter_close_container(appender, &sub)) {
        PyErr_NoMemory();
        return -1;
    }
    return ret;
}

/* Encode some Python object into a D-Bus variant slot. */
static int
_message_iter_append_variant(DBusMessageIter *appender, PyObject *obj)
{
    DBusSignatureIter obj_sig_iter;
    const char *obj_sig_str;
    PyObject *obj_sig;
    int ret;
    long variant_level;
    dbus_bool_t dummy;

    /* Separate the object into the contained object, and the number of
     * variants it's wrapped in. */
    obj_sig = _signature_string_from_pyobject(obj, &variant_level);
    if (!obj_sig) return -1;

    if (PyUnicode_Check(obj_sig)) {
        PyObject *obj_sig_as_bytes = PyUnicode_AsUTF8String(obj_sig);
        Py_CLEAR(obj_sig);
        if (!obj_sig_as_bytes)
            return -1;
        obj_sig = obj_sig_as_bytes;
    }
    obj_sig_str = PyBytes_AsString(obj_sig);
    if (!obj_sig_str) {
        Py_CLEAR(obj_sig);
        return -1;
    }

    if (variant_level < 1) {
        variant_level = 1;
    }

    dbus_signature_iter_init(&obj_sig_iter, obj_sig_str);

    { /* scope for variant_iters */
        DBusMessageIter variant_iters[variant_level];
        long i;

        for (i = 0; i < variant_level; i++) {
            DBusMessageIter *child = &variant_iters[i];
            /* The first is a special case: its parent is the iter passed in
             * to this function, instead of being the previous one in the
             * stack
             */
            DBusMessageIter *parent = (i == 0
                                        ? appender
                                        : &(variant_iters[i-1]));
            /* The last is also a special case: it contains the actual
             * object, rather than another variant
             */
            const char *sig_str = (i == variant_level-1
                                        ? obj_sig_str
                                        : DBUS_TYPE_VARIANT_AS_STRING);

            DBG("Opening VARIANT container %p inside %p containing '%s'",
                child, parent, sig_str);
            if (!dbus_message_iter_open_container(parent, DBUS_TYPE_VARIANT,
                                                  sig_str, child)) {
                PyErr_NoMemory();
                ret = -1;
                goto out;
            }
        }

        /* Put the object itself into the innermost variant */
        ret = _message_iter_append_pyobject(&variant_iters[variant_level-1],
                                            &obj_sig_iter, obj, &dummy);

        /* here we rely on i (and variant_level) being a signed long */
        for (i = variant_level - 1; i >= 0; i--) {
            DBusMessageIter *child = &variant_iters[i];
            /* The first is a special case: its parent is the iter passed in
             * to this function, instead of being the previous one in the
             * stack
             */
            DBusMessageIter *parent = (i == 0 ? appender
                                              : &(variant_iters[i-1]));

            DBG("Closing VARIANT container %p inside %p", child, parent);
            if (!dbus_message_iter_close_container(parent, child)) {
                PyErr_NoMemory();
                ret = -1;
                goto out;
            }
        }

    }

out:
    Py_CLEAR(obj_sig);
    return ret;
}

/* On success, *more is set to whether there's more in the signature. */
static int
_message_iter_append_pyobject(DBusMessageIter *appender,
                              DBusSignatureIter *sig_iter,
                              PyObject *obj,
                              dbus_bool_t *more)
{
    int sig_type = dbus_signature_iter_get_current_type(sig_iter);
    DBusBasicValue u;
    int ret = -1;

#ifdef USING_DBG
    fprintf(stderr, "Appending object at %p: ", obj);
    PyObject_Print(obj, stderr, 0);
    fprintf(stderr, " into appender at %p, dbus wants type %c\n",
            appender, sig_type);
#endif

    switch (sig_type) {
      /* The numeric types are relatively simple to deal with, so are
       * inlined here. */

      case DBUS_TYPE_BOOLEAN:
          if (PyObject_IsTrue(obj)) {
              u.bool_val = 1;
          }
          else {
              u.bool_val = 0;
          }
          DBG("Performing actual append: bool(%ld)", (long)u.bool_val);
          if (!dbus_message_iter_append_basic(appender, sig_type, &u.bool_val)) {
              PyErr_NoMemory();
              ret = -1;
              break;
          }
          ret = 0;
          break;

      case DBUS_TYPE_DOUBLE:
          u.dbl = PyFloat_AsDouble(obj);
          if (PyErr_Occurred()) {
              ret = -1;
              break;
          }
          DBG("Performing actual append: double(%f)", u.dbl);
          if (!dbus_message_iter_append_basic(appender, sig_type, &u.dbl)) {
              PyErr_NoMemory();
              ret = -1;
              break;
          }
          ret = 0;
          break;

#ifdef WITH_DBUS_FLOAT32
      case DBUS_TYPE_FLOAT:
          u.dbl = PyFloat_AsDouble(obj);
          if (PyErr_Occurred()) {
              ret = -1;
              break;
          }
          /* FIXME: DBusBasicValue will need to grow a float member if
           * float32 becomes supported */
          u.f = (float)u.dbl;
          DBG("Performing actual append: float(%f)", u.f);
          if (!dbus_message_iter_append_basic(appender, sig_type, &u.f)) {
              PyErr_NoMemory();
              ret = -1;
              break;
          }
          ret = 0;
          break;
#endif

          /* The integer types are all basically the same - we delegate to
          intNN_range_check() */
#define PROCESS_INTEGER(size, member) \
          u.member = dbus_py_##size##_range_check(obj);\
          if (u.member == (dbus_##size##_t)(-1) && PyErr_Occurred()) {\
              ret = -1; \
              break; \
          }\
          DBG("Performing actual append: " #size "(%lld)", (long long)u.member); \
          if (!dbus_message_iter_append_basic(appender, sig_type, &u.member)) {\
              PyErr_NoMemory();\
              ret = -1;\
              break;\
          } \
          ret = 0;

      case DBUS_TYPE_INT16:
          PROCESS_INTEGER(int16, i16)
          break;
      case DBUS_TYPE_UINT16:
          PROCESS_INTEGER(uint16, u16)
          break;
      case DBUS_TYPE_INT32:
          PROCESS_INTEGER(int32, i32)
          break;
      case DBUS_TYPE_UINT32:
          PROCESS_INTEGER(uint32, u32)
          break;
#if defined(DBUS_HAVE_INT64) && defined(HAVE_LONG_LONG)
      case DBUS_TYPE_INT64:
          PROCESS_INTEGER(int64, i64)
          break;
      case DBUS_TYPE_UINT64:
          PROCESS_INTEGER(uint64, u64)
          break;
#else
      case DBUS_TYPE_INT64:
      case DBUS_TYPE_UINT64:
          PyErr_SetString(PyExc_NotImplementedError, "64-bit integer "
                          "types are not supported on this platform");
          ret = -1;
          break;
#endif
#undef PROCESS_INTEGER

      /* Now the more complicated cases, which are delegated to helper
       * functions (although in practice, the compiler will hopefully
       * inline them anyway). */

      case DBUS_TYPE_STRING:
      case DBUS_TYPE_SIGNATURE:
      case DBUS_TYPE_OBJECT_PATH:
          ret = _message_iter_append_string(appender, sig_type, obj, TRUE);
          break;

      case DBUS_TYPE_BYTE:
          ret = _message_iter_append_byte(appender, obj);
          break;

      case DBUS_TYPE_ARRAY:
          /* 3 cases - it might actually be a dict, or it might be a byte array
           * being copied from a string (for which we have a faster path),
           * or it might be a generic array. */

          sig_type = dbus_signature_iter_get_element_type(sig_iter);
          if (sig_type == DBUS_TYPE_DICT_ENTRY)
            ret = _message_iter_append_multi(appender, sig_iter,
                                             DBUS_TYPE_DICT_ENTRY, obj);
          else if (sig_type == DBUS_TYPE_BYTE && PyBytes_Check(obj))
            ret = _message_iter_append_string_as_byte_array(appender, obj);
          else
            ret = _message_iter_append_multi(appender, sig_iter,
                                             DBUS_TYPE_ARRAY, obj);
          DBG("_message_iter_append_multi(): %d", ret);
          break;

      case DBUS_TYPE_STRUCT:
          ret = _message_iter_append_multi(appender, sig_iter, sig_type, obj);
          break;

      case DBUS_TYPE_VARIANT:
          ret = _message_iter_append_variant(appender, obj);
          break;

      case DBUS_TYPE_INVALID:
          PyErr_SetString(PyExc_TypeError, "Fewer items found in D-Bus "
                          "signature than in Python arguments");
          ret = -1;
          break;

#if defined(DBUS_TYPE_UNIX_FD)
      case DBUS_TYPE_UNIX_FD:
          ret = _message_iter_append_unixfd(appender, obj);
          break;
#endif

      default:
          PyErr_Format(PyExc_TypeError, "Unknown type '\\x%x' in D-Bus "
                       "signature", sig_type);
          ret = -1;
          break;
    }
    if (ret < 0) return -1;

    DBG("Advancing signature iter at %p", sig_iter);
    *more = dbus_signature_iter_next(sig_iter);
#ifdef USING_DBG
    DBG("- result: %ld, type %02x '%c'", (long)(*more),
        (int)dbus_signature_iter_get_current_type(sig_iter),
        (int)dbus_signature_iter_get_current_type(sig_iter));
#endif
    return 0;
}


PyObject *
dbus_py_Message_append(Message *self, PyObject *args, PyObject *kwargs)
{
    const char *signature = NULL;
    PyObject *signature_obj = NULL;
    DBusSignatureIter sig_iter;
    DBusMessageIter appender;
    static char *argnames[] = {"signature", NULL};
    dbus_bool_t more;

    if (!self->msg) return DBusPy_RaiseUnusableMessage();

#ifdef USING_DBG
    fprintf(stderr, "DBG/%ld: called Message_append(*", (long)getpid());
    PyObject_Print(args, stderr, 0);
    if (kwargs) {
        fprintf(stderr, ", **");
        PyObject_Print(kwargs, stderr, 0);
    }
    fprintf(stderr, ")\n");
#endif

    /* only use kwargs for this step: deliberately ignore args for now */
    if (!PyArg_ParseTupleAndKeywords(dbus_py_empty_tuple, kwargs, "|z:append",
                                     argnames, &signature)) return NULL;

    if (!signature) {
        DBG("%s", "No signature for message, guessing...");
        signature_obj = dbus_py_Message_guess_signature(NULL, args);
        if (!signature_obj) return NULL;
        if (PyUnicode_Check(signature_obj)) {
            PyObject *signature_as_bytes;
            signature_as_bytes = PyUnicode_AsUTF8String(signature_obj);
            Py_CLEAR(signature_obj);
            if (!signature_as_bytes)
                return NULL;
            signature_obj = signature_as_bytes;
        }
        else {
            assert(PyBytes_Check(signature_obj));
        }
        signature = PyBytes_AS_STRING(signature_obj);
    }
    /* from here onwards, you have to do a goto rather than returning NULL
    to make sure signature_obj gets freed */

    /* iterate over args and the signature, together */
    if (!dbus_signature_validate(signature, NULL)) {
        PyErr_SetString(PyExc_ValueError, "Corrupt type signature");
        goto err;
    }
    dbus_message_iter_init_append(self->msg, &appender);

    if (signature[0] != '\0') {
        int i = 0;

        more = TRUE;
        dbus_signature_iter_init(&sig_iter, signature);
        while (more) {
            if (i >= PyTuple_GET_SIZE(args)) {
                PyErr_SetString(PyExc_TypeError, "More items found in D-Bus "
                                "signature than in Python arguments");
                goto hosed;
            }
            if (_message_iter_append_pyobject(&appender, &sig_iter,
                                              PyTuple_GET_ITEM(args, i),
                                              &more) < 0) {
                goto hosed;
            }
            i++;
        }
        if (i < PyTuple_GET_SIZE(args)) {
            PyErr_SetString(PyExc_TypeError, "Fewer items found in D-Bus "
                    "signature than in Python arguments");
            goto hosed;
        }
    }

    /* success! */
    Py_CLEAR(signature_obj);
    Py_RETURN_NONE;

hosed:
    /* "If appending any of the arguments fails due to lack of memory,
     * generally the message is hosed and you have to start over" -libdbus docs
     * Enforce this by throwing away the message structure.
     */
    dbus_message_unref(self->msg);
    self->msg = NULL;
err:
    Py_CLEAR(signature_obj);
    return NULL;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
