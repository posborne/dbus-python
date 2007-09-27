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

#define DBG_IS_TOO_VERBOSE
#include "types-internal.h"
#include "message-internal.h"

/* Return the number of variants wrapping the given object. Return 0
 * if the object is not a D-Bus type.
 */
static long
get_variant_level(PyObject *obj)
{
    if (DBusPyIntBase_Check(obj)) {
        return ((DBusPyIntBase *)obj)->variant_level;
    }
    else if (DBusPyFloatBase_Check(obj)) {
        return ((DBusPyFloatBase *)obj)->variant_level;
    }
    else if (DBusPyArray_Check(obj)) {
        return ((DBusPyArray *)obj)->variant_level;
    }
    else if (DBusPyDict_Check(obj)) {
        return ((DBusPyDict *)obj)->variant_level;
    }
    else if (DBusPyString_Check(obj)) {
        return ((DBusPyString *)obj)->variant_level;
    }
    else if (DBusPyLongBase_Check(obj) ||
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
        if (PyString_Check(magic_attr)) {
            return magic_attr;
        }
        else {
            Py_DECREF(magic_attr);
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
    if (variant_level_ptr) {
        *variant_level_ptr = variant_level;
    }
    else if (variant_level > 0) {
        return PyString_FromString(DBUS_TYPE_VARIANT_AS_STRING);
    }

    if (obj == Py_True || obj == Py_False) {
      return PyString_FromString(DBUS_TYPE_BOOLEAN_AS_STRING);
    }

    magic_attr = get_object_path(obj);
    if (!magic_attr)
        return NULL;
    if (magic_attr != Py_None) {
        Py_DECREF(magic_attr);
        return PyString_FromString(DBUS_TYPE_OBJECT_PATH_AS_STRING);
    }
    Py_DECREF(magic_attr);

    /* Ordering is important: some of these are subclasses of each other. */
    if (PyInt_Check(obj)) {
        if (DBusPyInt16_Check(obj))
            return PyString_FromString(DBUS_TYPE_INT16_AS_STRING);
        else if (DBusPyInt32_Check(obj))
            return PyString_FromString(DBUS_TYPE_INT32_AS_STRING);
        else if (DBusPyByte_Check(obj))
            return PyString_FromString(DBUS_TYPE_BYTE_AS_STRING);
        else if (DBusPyUInt16_Check(obj))
            return PyString_FromString(DBUS_TYPE_UINT16_AS_STRING);
        else if (DBusPyBoolean_Check(obj))
            return PyString_FromString(DBUS_TYPE_BOOLEAN_AS_STRING);
        else
            return PyString_FromString(DBUS_TYPE_INT32_AS_STRING);
    }
    else if (PyLong_Check(obj)) {
        if (DBusPyInt64_Check(obj))
            return PyString_FromString(DBUS_TYPE_INT64_AS_STRING);
        else if (DBusPyUInt32_Check(obj))
            return PyString_FromString(DBUS_TYPE_UINT32_AS_STRING);
        else if (DBusPyUInt64_Check(obj))
            return PyString_FromString(DBUS_TYPE_UINT64_AS_STRING);
        else
            return PyString_FromString(DBUS_TYPE_INT64_AS_STRING);
    }
    else if (PyUnicode_Check(obj))
        return PyString_FromString(DBUS_TYPE_STRING_AS_STRING);
    else if (PyFloat_Check(obj)) {
#ifdef WITH_DBUS_FLOAT32
        if (DBusPyDouble_Check(obj))
            return PyString_FromString(DBUS_TYPE_DOUBLE_AS_STRING);
        else if (DBusPyFloat_Check(obj))
            return PyString_FromString(DBUS_TYPE_FLOAT_AS_STRING);
        else
#endif
            return PyString_FromString(DBUS_TYPE_DOUBLE_AS_STRING);
    }
    else if (PyString_Check(obj)) {
        if (DBusPyObjectPath_Check(obj))
            return PyString_FromString(DBUS_TYPE_OBJECT_PATH_AS_STRING);
        else if (DBusPySignature_Check(obj))
            return PyString_FromString(DBUS_TYPE_SIGNATURE_AS_STRING);
        else if (DBusPyByteArray_Check(obj))
            return PyString_FromString(DBUS_TYPE_ARRAY_AS_STRING
                                       DBUS_TYPE_BYTE_AS_STRING);
        else
            return PyString_FromString(DBUS_TYPE_STRING_AS_STRING);
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
            Py_DECREF(list);
            return NULL;
        }
        /* Set the first and last elements of list to be the parentheses */
        item = PyString_FromString(DBUS_STRUCT_BEGIN_CHAR_AS_STRING);
        if (PyList_SetItem(list, 0, item) < 0) {
            Py_DECREF(list);
            return NULL;
        }
        item = PyString_FromString(DBUS_STRUCT_END_CHAR_AS_STRING);
        if (PyList_SetItem(list, len + 1, item) < 0) {
            Py_DECREF(list);
            return NULL;
        }
        if (!item || !PyList_GET_ITEM(list, 0)) {
            Py_DECREF(list);
            return NULL;
        }
        item = NULL;

        for (i = 0; i < len; i++) {
            item = PyTuple_GetItem(obj, i);
            if (!item) {
                Py_DECREF(list);
                return NULL;
            }
            item = _signature_string_from_pyobject(item, NULL);
            if (!item) {
                Py_DECREF(list);
                return NULL;
            }
            if (PyList_SetItem(list, i + 1, item) < 0) {
                Py_DECREF(list);
                return NULL;
            }
            item = NULL;
        }
        empty_str = PyString_FromString("");
        if (!empty_str) {
            /* really shouldn't happen */
            Py_DECREF(list);
            return NULL;
        }
        ret = PyObject_CallMethod(empty_str, "join", "(O)", list); /* new ref */
        /* whether ret is NULL or not, */
        Py_DECREF(empty_str);
        Py_DECREF(list);
        return ret;
    }
    else if (PyList_Check(obj)) {
        PyObject *tmp;
        PyObject *ret = PyString_FromString(DBUS_TYPE_ARRAY_AS_STRING);
        if (!ret) return NULL;
        if (DBusPyArray_Check(obj) && PyString_Check(((DBusPyArray *)obj)->signature)) {
            PyString_Concat(&ret, ((DBusPyArray *)obj)->signature);
            return ret;
        }
        if (PyList_GET_SIZE(obj) == 0) {
            /* No items, so fail. Or should we guess "av"? */
            PyErr_SetString(PyExc_ValueError, "Unable to guess signature "
                            "from an empty list");
            return NULL;
        }
        tmp = PyList_GetItem(obj, 0);
        tmp = _signature_string_from_pyobject(tmp, NULL);
        if (!tmp) return NULL;
        PyString_ConcatAndDel(&ret, tmp);
        return ret;
    }
    else if (PyDict_Check(obj)) {
        PyObject *key, *value, *keysig, *valuesig;
        Py_ssize_t pos = 0;
        PyObject *ret = NULL;

        if (DBusPyDict_Check(obj) && PyString_Check(((DBusPyDict *)obj)->signature)) {
            const char *sig = PyString_AS_STRING(((DBusPyDict *)obj)->signature);

            return PyString_FromFormat((DBUS_TYPE_ARRAY_AS_STRING
                                        DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                                        "%s"
                                        DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
                                       sig);
        }
        if (!PyDict_Next(obj, &pos, &key, &value)) {
            /* No items, so fail. Or should we guess "a{vv}"? */
            PyErr_SetString(PyExc_ValueError, "Unable to guess signature "
                             "from an empty dict");
            return NULL;
        }
        keysig = _signature_string_from_pyobject(key, NULL);
        valuesig = _signature_string_from_pyobject(value, NULL);
        if (keysig && valuesig) {
            ret = PyString_FromFormat((DBUS_TYPE_ARRAY_AS_STRING
                                       DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                                       "%s%s"
                                       DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
                                      PyString_AS_STRING(keysig),
                                      PyString_AS_STRING(valuesig));
        }
        Py_XDECREF(keysig);
        Py_XDECREF(valuesig);
        return ret;
    }
    else {
        PyErr_Format(PyExc_TypeError, "Don't know how which D-Bus type "
                     "to use to encode type \"%s\"",
                     obj->ob_type->tp_name);
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
    if (!PyString_Check(tmp) || PyString_GET_SIZE(tmp) < 2) {
        PyErr_SetString(PyExc_RuntimeError, "Internal error: "
                        "_signature_string_from_pyobject returned "
                        "a bad result");
        Py_DECREF(tmp);
        return NULL;
    }
    ret = PyObject_CallFunction((PyObject *)&DBusPySignature_Type, "(s#)",
                                PyString_AS_STRING(tmp) + 1,
                                PyString_GET_SIZE(tmp) - 2);
    DBG("Message_guess_signature: returning Signature at %p \"%s\"", ret,
        ret ? PyString_AS_STRING(ret) : "(NULL)");
    Py_DECREF(tmp);
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

    if (sig_type == DBUS_TYPE_OBJECT_PATH && allow_object_path_attr) {
        PyObject *object_path = get_object_path (obj);

        if (object_path == Py_None) {
            Py_DECREF(object_path);
        }
        else if (!object_path) {
            return -1;
        }
        else {
            int ret = _message_iter_append_string(appender, sig_type,
                                                  object_path, FALSE);
            Py_DECREF(object_path);
            return ret;
        }
    }

    if (PyString_Check(obj)) {
        PyObject *unicode;

        /* Raise TypeError if the string has embedded NULs */
        if (PyString_AsStringAndSize(obj, &s, NULL) < 0) return -1;
        /* Surely there's a faster stdlib way to validate UTF-8... */
        unicode = PyUnicode_DecodeUTF8(s, PyString_GET_SIZE(obj), NULL);
        if (!unicode) {
            PyErr_SetString(PyExc_UnicodeError, "String parameters "
                            "to be sent over D-Bus must be valid UTF-8");
            return -1;
        }
        Py_DECREF(unicode);
        unicode = NULL;

        DBG("Performing actual append: string %s", s);
        if (!dbus_message_iter_append_basic(appender, sig_type,
                                            &s)) {
            PyErr_NoMemory();
            return -1;
        }
    }
    else if (PyUnicode_Check(obj)) {
        PyObject *utf8 = PyUnicode_AsUTF8String(obj);
        if (!utf8) return -1;
        /* Raise TypeError if the string has embedded NULs */
        if (PyString_AsStringAndSize(utf8, &s, NULL) < 0) return -1;
        DBG("Performing actual append: string (from unicode) %s", s);
        if (!dbus_message_iter_append_basic(appender, sig_type, &s)) {
            PyErr_NoMemory();
            return -1;
        }
        Py_DECREF(utf8);
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "Expected a string or unicode object");
        return -1;
    }
    return 0;
}

static int
_message_iter_append_byte(DBusMessageIter *appender, PyObject *obj)
{
    unsigned char y;

    if (PyString_Check(obj)) {
        if (PyString_GET_SIZE(obj) != 1) {
            PyErr_Format(PyExc_ValueError, "Expected a string of "
                         "length 1 byte, but found %d bytes",
                         PyString_GET_SIZE(obj));
            return -1;
        }
        y = *(unsigned char *)PyString_AS_STRING(obj);
    }
    else {
        long i = PyInt_AsLong(obj);

        if (i == -1 && PyErr_Occurred()) return -1;
        if (i < 0 || i > 0xff) {
            PyErr_Format(PyExc_ValueError, "%d outside range for a "
                         "byte value", (int)i);
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
    if (!dbus_message_iter_close_container(appender, &sub)) {
        PyErr_NoMemory();
        ret = -1;
    }
out:
    Py_DECREF(value);
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

    DBG("Opening %c container", container);
    if (!dbus_message_iter_open_container(appender, container,
                                          sig, &sub_appender)) {
        PyErr_NoMemory();
        ret = -1;
        goto out;
    }
    ret = 0;
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
            Py_DECREF(args);
            if (!byte)
                break;
            ret = _message_iter_append_variant(&sub_appender, byte);
            Py_DECREF(byte);
        }
        else {
            /* advances sub_sig_iter and sets more on success - for array
             * this doesn't matter, for struct it's essential */
            ret = _message_iter_append_pyobject(&sub_appender, &sub_sig_iter,
                                                contents, &more);
        }

        Py_DECREF(contents);
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
    DBG("Closing %c container", container);
    if (!dbus_message_iter_close_container(appender, &sub_appender)) {
        PyErr_NoMemory();
        ret = -1;
    }

out:
    Py_XDECREF(iterator);
    dbus_free(sig);
    return ret;
}

static int
_message_iter_append_string_as_byte_array(DBusMessageIter *appender,
                                          PyObject *obj)
{
    /* a bit of a faster path for byte arrays that are strings */
    Py_ssize_t len = PyString_GET_SIZE(obj);
    const char *s;
    DBusMessageIter sub;
    int ret;

    s = PyString_AS_STRING(obj);
    DBG("%s", "Opening ARRAY container");
    if (!dbus_message_iter_open_container(appender, DBUS_TYPE_ARRAY,
                                          DBUS_TYPE_BYTE_AS_STRING, &sub)) {
        PyErr_NoMemory();
        return -1;
    }
    DBG("Appending fixed array of %d bytes", len);
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

    obj_sig_str = PyString_AsString(obj_sig);
    if (!obj_sig_str) return -1;

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
    Py_XDECREF(obj_sig);
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
    union {
      dbus_bool_t b;
      double d;
      dbus_uint16_t uint16;
      dbus_int16_t int16;
      dbus_uint32_t uint32;
      dbus_int32_t int32;
#if defined(DBUS_HAVE_INT64) && defined(HAVE_LONG_LONG)
      dbus_uint64_t uint64;
      dbus_int64_t int64;
#endif
    } u;
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
              u.b = 1;
          }
          else {
              u.b = 0;
          }
          DBG("Performing actual append: bool(%ld)", (long)u.b);
          if (!dbus_message_iter_append_basic(appender, sig_type, &u.b)) {
              PyErr_NoMemory();
              ret = -1;
              break;
          }
          ret = 0;
          break;

      case DBUS_TYPE_DOUBLE:
          u.d = PyFloat_AsDouble(obj);
          if (PyErr_Occurred()) {
              ret = -1;
              break;
          }
          DBG("Performing actual append: double(%f)", u.d);
          if (!dbus_message_iter_append_basic(appender, sig_type, &u.d)) {
              PyErr_NoMemory();
              ret = -1;
              break;
          }
          ret = 0;
          break;

#ifdef WITH_DBUS_FLOAT32
      case DBUS_TYPE_FLOAT:
          u.d = PyFloat_AsDouble(obj);
          if (PyErr_Occurred()) {
              ret = -1;
              break;
          }
          u.f = (float)u.d;
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
#define PROCESS_INTEGER(size) \
          u.size = dbus_py_##size##_range_check(obj);\
          if (u.size == (dbus_##size##_t)(-1) && PyErr_Occurred()) {\
              ret = -1; \
              break; \
          }\
          DBG("Performing actual append: " #size "(%lld)", (long long)u.size); \
          if (!dbus_message_iter_append_basic(appender, sig_type, &u.size)) {\
              PyErr_NoMemory();\
              ret = -1;\
              break;\
          } \
          ret = 0;

      case DBUS_TYPE_INT16:
          PROCESS_INTEGER(int16)
          break;
      case DBUS_TYPE_UINT16:
          PROCESS_INTEGER(uint16)
          break;
      case DBUS_TYPE_INT32:
          PROCESS_INTEGER(int32)
          break;
      case DBUS_TYPE_UINT32:
          PROCESS_INTEGER(uint32)
          break;
#if defined(DBUS_HAVE_INT64) && defined(HAVE_LONG_LONG)
      case DBUS_TYPE_INT64:
          PROCESS_INTEGER(int64)
          break;
      case DBUS_TYPE_UINT64:
          PROCESS_INTEGER(uint64)
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
          else if (sig_type == DBUS_TYPE_BYTE && PyString_Check(obj))
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
    int i;
    static char *argnames[] = {"signature", NULL};
    /* must start FALSE for the case where there's nothing there and we
     * never iterate at all */
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
        signature = PyString_AS_STRING(signature_obj);
    }
    /* from here onwards, you have to do a goto rather than returning NULL
    to make sure signature_obj gets freed */

    /* iterate over args and the signature, together */
    if (!dbus_signature_validate(signature, NULL)) {
        PyErr_SetString(PyExc_ValueError, "Corrupt type signature");
        goto err;
    }
    dbus_signature_iter_init(&sig_iter, signature);
    dbus_message_iter_init_append(self->msg, &appender);
    more = (signature[0] != '\0');
    for (i = 0; i < PyTuple_GET_SIZE(args); i++) {
        if (_message_iter_append_pyobject(&appender, &sig_iter,
                                          PyTuple_GET_ITEM(args, i),
                                          &more) < 0) {
            goto hosed;
        }
    }
    if (more) {
        PyErr_SetString(PyExc_TypeError, "More items found in D-Bus "
                        "signature than in Python arguments");
        goto hosed;
    }

    /* success! */
    Py_XDECREF(signature_obj);
    Py_RETURN_NONE;

hosed:
    /* "If appending any of the arguments fails due to lack of memory,
     * generally the message is hosed and you have to start over" -libdbus docs
     * Enforce this by throwing away the message structure.
     */
    dbus_message_unref(self->msg);
    self->msg = NULL;
err:
    Py_XDECREF(signature_obj);
    return NULL;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
