/* D-Bus Message unserialization. This contains all the logic to map from
 * D-Bus types to Python objects.
 *
 * Copyright (C) 2006-2007 Collabora Ltd. <http://www.collabora.co.uk/>
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

#define PY_SIZE_T_CLEAN 1

#define DBG_IS_TOO_VERBOSE
#include "compat-internal.h"
#include "types-internal.h"
#include "message-internal.h"

char dbus_py_Message_get_args_list__doc__[] = (
"get_args_list(**kwargs) -> list\n\n"
"Return the message's arguments. Keyword arguments control the translation\n"
"of D-Bus types to Python:\n"
"\n"
":Keywords:\n"
"   `byte_arrays` : bool\n"
"       If true, convert arrays of byte (signature 'ay') into dbus.ByteArray,\n"
"       a str subclass. In practice, this is usually what you want, but\n"
"       it's off by default for consistency.\n"
"\n"
"       If false (default), convert them into a dbus.Array of Bytes.\n"
#ifndef PY3
"   `utf8_strings` : bool\n"
"       If true, return D-Bus strings as Python 8-bit strings (of UTF-8).\n"
"       If false (default), return D-Bus strings as Python unicode objects.\n"
#endif
"\n"
"Most of the type mappings should be fairly obvious:\n"
"\n"
"===============  ===================================================\n"
"D-Bus            Python\n"
"===============  ===================================================\n"
"byte (y)         dbus.Byte (int subclass)\n"
"bool (b)         dbus.Boolean (int subclass)\n"
"Signature (g)    dbus.Signature (str subclass)\n"
"intNN, uintNN    dbus.IntNN, dbus.UIntNN (int or long subclasses)\n"
"double (d)       dbus.Double\n"
"string (s)       dbus.String (unicode subclass)\n"
"                 (or dbus.UTF8String, str subclass, if utf8_strings set)\n"
"Object path (o)  dbus.ObjectPath (str subclass)\n"
"dict (a{...})    dbus.Dictionary\n"
"array (a...)     dbus.Array (list subclass) containing appropriate types\n"
"byte array (ay)  dbus.ByteArray (str subclass) if byte_arrays set; or\n"
"                 list of Byte\n"
"struct ((...))   dbus.Struct (tuple subclass) of appropriate types\n"
"variant (v)      contained type, but with variant_level > 0\n"
"===============  ===================================================\n"
);

typedef struct {
    int byte_arrays;
#ifndef PY3
    int utf8_strings;
#endif
} Message_get_args_options;

static PyObject *_message_iter_get_pyobject(DBusMessageIter *iter,
                                            Message_get_args_options *opts,
                                            long extra_variants);

/* Append all the items iterated over to the given Python list object.
   * Return 0 on success/-1 with exception on failure. */
static int
_message_iter_append_all_to_list(DBusMessageIter *iter, PyObject *list,
                                 Message_get_args_options *opts)
{
    int ret, type;
    while ((type = dbus_message_iter_get_arg_type(iter))
            != DBUS_TYPE_INVALID) {
        PyObject *item;
        DBG("type == %d '%c'", type, type);

        item = _message_iter_get_pyobject(iter, opts, 0);
        if (!item) return -1;
#ifdef USING_DBG
        fprintf(stderr, "DBG/%ld: appending to list: %p == ", (long)getpid(), item);
        PyObject_Print(item, stderr, 0);
        fprintf(stderr, " of type %p\n", Py_TYPE(item));
#endif
        ret = PyList_Append(list, item);
        Py_CLEAR(item);
        if (ret < 0) return -1;
#ifdef USING_DBG
        fprintf(stderr, "DBG/%ld: list now contains: ", (long)getpid());
        PyObject_Print(list, stderr, 0);
        fprintf(stderr, "\n");
#endif
        dbus_message_iter_next(iter);
    }
    return 0;
}

static inline PyObject *
_message_iter_get_dict(DBusMessageIter *iter,
                       Message_get_args_options *opts,
                       PyObject *kwargs)
{
    DBusMessageIter entries;
    char *sig_str = dbus_message_iter_get_signature(iter);
    PyObject *sig;
    PyObject *ret;
    int status;

    if (!sig_str) {
        PyErr_NoMemory();
        return NULL;
    }
    sig = PyObject_CallFunction((PyObject *)&DBusPySignature_Type,
                                "(s#)", sig_str+2,
                                (Py_ssize_t)strlen(sig_str)-3);
    dbus_free(sig_str);
    if (!sig) {
        return NULL;
    }
    status = PyDict_SetItem(kwargs, dbus_py_signature_const, sig);
    Py_CLEAR(sig);
    if (status < 0) {
        return NULL;
    }

    ret = PyObject_Call((PyObject *)&DBusPyDict_Type, dbus_py_empty_tuple, kwargs);
    if (!ret) {
        return NULL;
    }

    dbus_message_iter_recurse(iter, &entries);
    while (dbus_message_iter_get_arg_type(&entries) == DBUS_TYPE_DICT_ENTRY) {
        PyObject *key = NULL;
        PyObject *value = NULL;
        DBusMessageIter kv;

        DBG("%s", "dict entry...");

        dbus_message_iter_recurse(&entries, &kv);

        key = _message_iter_get_pyobject(&kv, opts, 0);
        if (!key) {
            Py_CLEAR(ret);
            return NULL;
        }
        dbus_message_iter_next(&kv);

        value = _message_iter_get_pyobject(&kv, opts, 0);
        if (!value) {
            Py_CLEAR(key);
            Py_CLEAR(ret);
            return NULL;
        }

        status = PyDict_SetItem(ret, key, value);
        Py_CLEAR(key);
        Py_CLEAR(value);

        if (status < 0) {
            Py_CLEAR(ret);
            return NULL;
        }
        dbus_message_iter_next(&entries);
    }

    return ret;
}

/* Returns a new reference. */
static PyObject *
_message_iter_get_pyobject(DBusMessageIter *iter,
                           Message_get_args_options *opts,
                           long variant_level)
{
    DBusBasicValue u;
    int type = dbus_message_iter_get_arg_type(iter);
    PyObject *args = NULL;
    PyObject *kwargs = NULL;
    PyObject *ret = NULL;

    /* If the variant-level is >0, prepare a dict for the kwargs.
     * For variant wrappers optimize slightly by skipping this.
     */
    if (variant_level > 0 && type != DBUS_TYPE_VARIANT) {
        PyObject *variant_level_int;

        variant_level_int = NATIVEINT_FROMLONG(variant_level);
        if (!variant_level_int) {
            return NULL;
        }
        kwargs = PyDict_New();
        if (!kwargs) {
            Py_CLEAR(variant_level_int);
            return NULL;
        }
        if (PyDict_SetItem(kwargs, dbus_py_variant_level_const,
                           variant_level_int) < 0) {
            Py_CLEAR(variant_level_int);
            Py_CLEAR(kwargs);
            return NULL;
        }
        Py_CLEAR(variant_level_int);
    }
    /* From here down you need to break from the switch to exit, so the
     * dict is freed if necessary
     */

    switch (type) {
        PyObject *unicode;

        case DBUS_TYPE_STRING:
            DBG("%s", "found a string");
            dbus_message_iter_get_basic(iter, &u.str);
#ifndef PY3
            if (opts->utf8_strings) {
                args = Py_BuildValue("(s)", u.str);
                if (!args) break;
                ret = PyObject_Call((PyObject *)&DBusPyUTF8String_Type,
                                    args, kwargs);
            }
            else {
#endif
                unicode = PyUnicode_DecodeUTF8(u.str, strlen(u.str), NULL);
                if (!unicode) {
                    break;
                }
                args = Py_BuildValue("(N)", unicode);
                if (!args) {
                    break;
                }
                ret = PyObject_Call((PyObject *)&DBusPyString_Type,
                                    args, kwargs);
#ifndef PY3
            }
#endif
            break;

        case DBUS_TYPE_SIGNATURE:
            DBG("%s", "found a signature");
            dbus_message_iter_get_basic(iter, &u.str);
            args = Py_BuildValue("(s)", u.str);
            if (!args) break;
            ret = PyObject_Call((PyObject *)&DBusPySignature_Type, args, kwargs);
            break;

        case DBUS_TYPE_OBJECT_PATH:
            DBG("%s", "found an object path");
            dbus_message_iter_get_basic(iter, &u.str);
            args = Py_BuildValue("(s)", u.str);
            if (!args) break;
            ret = PyObject_Call((PyObject *)&DBusPyObjectPath_Type, args, kwargs);
            break;

        case DBUS_TYPE_DOUBLE:
            DBG("%s", "found a double");
            dbus_message_iter_get_basic(iter, &u.dbl);
            args = Py_BuildValue("(f)", u.dbl);
            if (!args) break;
            ret = PyObject_Call((PyObject *)&DBusPyDouble_Type, args, kwargs);
            break;

#ifdef WITH_DBUS_FLOAT32
        case DBUS_TYPE_FLOAT:
            DBG("%s", "found a float");
            /* FIXME: DBusBasicValue will need to grow a float member if
             * float32 becomes supported */
            dbus_message_iter_get_basic(iter, &u.f);
            args = Py_BuildValue("(f)", (double)u.f);
            if (!args) break;
            ret = PyObject_Call((PyObject *)&DBusPyFloat_Type, args, kwargs);
            break;
#endif

        case DBUS_TYPE_INT16:
            DBG("%s", "found an int16");
            dbus_message_iter_get_basic(iter, &u.i16);
            args = Py_BuildValue("(i)", (int)u.i16);
            if (!args) break;
            ret = PyObject_Call((PyObject *)&DBusPyInt16_Type, args, kwargs);
            break;

        case DBUS_TYPE_UINT16:
            DBG("%s", "found a uint16");
            dbus_message_iter_get_basic(iter, &u.u16);
            args = Py_BuildValue("(i)", (int)u.u16);
            if (!args) break;
            ret = PyObject_Call((PyObject *)&DBusPyUInt16_Type, args, kwargs);
            break;

        case DBUS_TYPE_INT32:
            DBG("%s", "found an int32");
            dbus_message_iter_get_basic(iter, &u.i32);
            args = Py_BuildValue("(l)", (long)u.i32);
            if (!args) break;
            ret = PyObject_Call((PyObject *)&DBusPyInt32_Type, args, kwargs);
            break;

        case DBUS_TYPE_UINT32:
            DBG("%s", "found a uint32");
            dbus_message_iter_get_basic(iter, &u.u32);
            args = Py_BuildValue("(k)", (unsigned long)u.u32);
            if (!args) break;
            ret = PyObject_Call((PyObject *)&DBusPyUInt32_Type, args, kwargs);
            break;

#ifdef DBUS_TYPE_UNIX_FD
        case DBUS_TYPE_UNIX_FD:
            DBG("%s", "found an unix fd");
            dbus_message_iter_get_basic(iter, &u.fd);
            args = Py_BuildValue("(i)", u.fd);
            if (args) {
                ret = PyObject_Call((PyObject *)&DBusPyUnixFd_Type, args,
                                    kwargs);
            }
            if (u.fd >= 0) {
                close(u.fd);
            }
            break;
#endif

#if defined(DBUS_HAVE_INT64) && defined(HAVE_LONG_LONG)
        case DBUS_TYPE_INT64:
            DBG("%s", "found an int64");
            dbus_message_iter_get_basic(iter, &u.i64);
            args = Py_BuildValue("(L)", (PY_LONG_LONG)u.i64);
            if (!args) break;
            ret = PyObject_Call((PyObject *)&DBusPyInt64_Type, args, kwargs);
            break;

        case DBUS_TYPE_UINT64:
            DBG("%s", "found a uint64");
            dbus_message_iter_get_basic(iter, &u.u64);
            args = Py_BuildValue("(K)", (unsigned PY_LONG_LONG)u.u64);
            if (!args) break;
            ret = PyObject_Call((PyObject *)&DBusPyUInt64_Type, args, kwargs);
            break;
#else
        case DBUS_TYPE_INT64:
        case DBUS_TYPE_UINT64:
            PyErr_SetString(PyExc_NotImplementedError,
                            "64-bit integer types are not supported on "
                            "this platform");
            break;
#endif

        case DBUS_TYPE_BYTE:
            DBG("%s", "found a byte");
            dbus_message_iter_get_basic(iter, &u.byt);
            args = Py_BuildValue("(l)", (long)u.byt);
            if (!args)
                break;
            ret = PyObject_Call((PyObject *)&DBusPyByte_Type, args, kwargs);
            break;

        case DBUS_TYPE_BOOLEAN:
            DBG("%s", "found a bool");
            dbus_message_iter_get_basic(iter, &u.bool_val);
            args = Py_BuildValue("(l)", (long)u.bool_val);
            if (!args)
                break;
            ret = PyObject_Call((PyObject *)&DBusPyBoolean_Type, args, kwargs);
            break;

        case DBUS_TYPE_ARRAY:
            DBG("%s", "found an array...");
            /* Dicts are arrays of DBUS_TYPE_DICT_ENTRY on the wire.
            Also, we special-case arrays of DBUS_TYPE_BYTE sometimes. */
            type = dbus_message_iter_get_element_type(iter);
            if (type == DBUS_TYPE_DICT_ENTRY) {
                DBG("%s", "no, actually it's a dict...");
                if (!kwargs) {
                    kwargs = PyDict_New();
                    if (!kwargs) break;
                }
                ret = _message_iter_get_dict(iter, opts, kwargs);
            }
            else if (opts->byte_arrays && type == DBUS_TYPE_BYTE) {
                DBusMessageIter sub;
                int n;

                DBG("%s", "actually, a byte array...");
                dbus_message_iter_recurse(iter, &sub);
                dbus_message_iter_get_fixed_array(&sub,
                                                  (const unsigned char **)&u.str,
                                                  &n);
                if (n == 0 && u.str == NULL) {
                    /* fd.o #21831: s# turns (NULL, 0) into None, but
                     * dbus_message_iter_get_fixed_array produces (NULL, 0)
                     * for an empty byte-blob... */
                    u.str = "";
                }
#ifdef PY3
                args = Py_BuildValue("(y#)", u.str, (Py_ssize_t)n);
#else
                args = Py_BuildValue("(s#)", u.str, (Py_ssize_t)n);
#endif
                if (!args) break;
                ret = PyObject_Call((PyObject *)&DBusPyByteArray_Type,
                                    args, kwargs);
            }
            else {
                DBusMessageIter sub;
                char *sig;
                PyObject *sig_obj;
                int status;

                DBG("%s", "a normal array...");
                if (!kwargs) {
                    kwargs = PyDict_New();
                    if (!kwargs) break;
                }
                dbus_message_iter_recurse(iter, &sub);
                sig = dbus_message_iter_get_signature(&sub);
                if (!sig) break;
                sig_obj = PyObject_CallFunction((PyObject *)&DBusPySignature_Type,
                                                "(s)", sig);
                dbus_free(sig);
                if (!sig_obj) break;
                status = PyDict_SetItem(kwargs, dbus_py_signature_const, sig_obj);
                Py_CLEAR(sig_obj);
                if (status < 0) break;
                ret = PyObject_Call((PyObject *)&DBusPyArray_Type,
                                    dbus_py_empty_tuple, kwargs);
                if (!ret) break;
                if (_message_iter_append_all_to_list(&sub, ret, opts) < 0) {
                    Py_CLEAR(ret);
                }
            }
            break;

        case DBUS_TYPE_STRUCT:
            {
                DBusMessageIter sub;
                PyObject *list = PyList_New(0);
                PyObject *tuple;

                DBG("%s", "found a struct...");
                if (!list) break;
                dbus_message_iter_recurse(iter, &sub);
                if (_message_iter_append_all_to_list(&sub, list, opts) < 0) {
                    Py_CLEAR(list);
                    break;
                }
                tuple = Py_BuildValue("(O)", list);
                if (tuple) {
                    ret = PyObject_Call((PyObject *)&DBusPyStruct_Type, tuple, kwargs);
                }
                else {
                    ret = NULL;
                }
                /* whether successful or not, we take the same action: */
                Py_CLEAR(list);
                Py_CLEAR(tuple);
            }
            break;

        case DBUS_TYPE_VARIANT:
            {
                DBusMessageIter sub;

                DBG("%s", "found a variant...");
                dbus_message_iter_recurse(iter, &sub);
                ret = _message_iter_get_pyobject(&sub, opts, variant_level+1);
            }
            break;

        default:
            PyErr_Format(PyExc_TypeError, "Unknown type '\\%x' in D-Bus "
                         "message", type);
    }

    Py_CLEAR(args);
    Py_CLEAR(kwargs);
    return ret;
}

PyObject *
dbus_py_Message_get_args_list(Message *self, PyObject *args, PyObject *kwargs)
{
#ifdef PY3
    Message_get_args_options opts = { 0 };
    static char *argnames[] = { "byte_arrays", NULL };
#else
    Message_get_args_options opts = { 0, 0 };
    static char *argnames[] = { "byte_arrays", "utf8_strings", NULL };
#endif
    PyObject *list;
    DBusMessageIter iter;

#ifdef USING_DBG
    fprintf(stderr, "DBG/%ld: called Message_get_args_list(self, *",
            (long)getpid());
    PyObject_Print(args, stderr, 0);
    if (kwargs) {
        fprintf(stderr, ", **");
        PyObject_Print(kwargs, stderr, 0);
    }
    fprintf(stderr, ")\n");
#endif

    if (PyTuple_Size(args) != 0) {
        PyErr_SetString(PyExc_TypeError, "get_args_list takes no positional "
                        "arguments");
        return NULL;
    }
#ifdef PY3
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i:get_args_list",
                                     argnames,
                                     &(opts.byte_arrays))) return NULL;
#else
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|ii:get_args_list",
                                     argnames,
                                     &(opts.byte_arrays),
                                     &(opts.utf8_strings))) return NULL;
#endif
    if (!self->msg) return DBusPy_RaiseUnusableMessage();

    list = PyList_New(0);
    if (!list) return NULL;

    /* Iterate over args, if any, appending to list */
    if (dbus_message_iter_init(self->msg, &iter)) {
        if (_message_iter_append_all_to_list(&iter, list, &opts) < 0) {
            Py_CLEAR(list);
            DBG_EXC("%s", "Message_get_args: appending all to list failed:");
            return NULL;
        }
    }

#ifdef USING_DBG
    fprintf(stderr, "DBG/%ld: message has args list ", (long)getpid());
    PyObject_Print(list, stderr, 0);
    fprintf(stderr, "\n");
#endif

    return list;
}

/* vim:set ft=c cino< sw=4 sts=4 et: */
