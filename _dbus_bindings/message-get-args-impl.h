/* D-Bus Message unserialization. This contains all the logic to map from
 * D-Bus types to Python objects.
 *
 * Copyright (C) 2006 Collabora Ltd.
 *
 * Licensed under the Academic Free License version 2.1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

PyDoc_STRVAR(Message_get_args_list__doc__,
"get_args_list(**kwargs) -> list\n\n"
"Return the message's arguments. Keyword arguments control the translation\n"
"of D-Bus types to Python:\n"
"\n"
":Parameters:\n"
"   `integer_bytes` : bool\n"
"       If true, return D-Bus bytes as Python ints (loses type information).\n"
"       If false (default and recommended), return D-Bus bytes as Byte\n"
"       objects, a subclass of str with length always 1.\n"
"   `byte_arrays` : bool\n"
"       If true, convert arrays of byte (signature 'ay') into ByteArray,\n"
"       a str subclass whose subscript operator returns Byte objects.\n"
"       If false (default), convert them like any other array (into a\n"
"       list of Bytes, or a list of ints if integer_bytes is true)."
"   `untyped_integers` : bool\n"
"       If true, fold all intNN, uintNN types into Python int or long (loses\n"
"       type information).\n"
"       If false (default), convert them into the appropriate subclasses.\n"
"   `variant_unpack_level` : int\n"
"       If >1, unpack e.g. a variant containing a variant containing a\n"
"       string as if it was a string (loses type information).\n"
"       If 1, unpack it as a Variant containing a string (i.e. remove\n"
"       exactly one level of \"variantness\".\n"
"       If 0 (default), unpack it as a Variant containing a Variant\n"
"       containing a string, to avoid ambiguity completely.\n"
"   `utf8_strings` : bool\n"
"       If true, return D-Bus strings as Python 8-bit strings (of UTF-8).\n"
"       If false (default), return D-Bus strings as Python unicode objects.\n"
"\n"
"Most of the type mappings should be fairly obvious:\n"
"\n"
"--------------- ------------\n"
"D-Bus           Python\n"
"=============== ============\n"
"byte (y)        Byte (int if integer_bytes set)\n"
"bool (b)        bool\n"
"Signature (g)   Signature (str subclass)\n"
"intNN, uintNN   IntNN, UIntNN (int/long subclass)\n"
"                (int/long if untyped_integers set)\n"
"double (d)      float\n"
"string (s)      unicode (str if utf8_strings set)\n"
"Object path (o) ObjectPath (str subclass)\n"
"dict (a{...})   dict\n"
"array (a...)    list of appropriate types\n"
"byte array (ay) ByteArray (str subclass) if byte_arrays set; or\n"
"                list of int, if integer_bytes set; or\n"
"                list of Byte\n"
"variant (v)     Variant containing appropriate type\n"
"struct ((...))  tuple of appropriate types\n"
"--------------- ------------\n"
);

typedef struct {
    int integer_bytes;
    int byte_arrays;
    int untyped_integers;
    int variant_unpack_level;
    int utf8_strings;
} Message_get_args_options;

static PyObject *_message_iter_get_pyobject(DBusMessageIter *iter,
                                            Message_get_args_options *opts,
                                            int top_level);

/* Append all the items iterated over to the given Python list object.
   * Return 0 on success/-1 with exception on failure. */
static int
_message_iter_append_all_to_list(DBusMessageIter *iter, PyObject *list,
                                 Message_get_args_options *opts,
                                 int top_level)
{
    int ret, type;
    while ((type = dbus_message_iter_get_arg_type (iter))
            != DBUS_TYPE_INVALID) {
        PyObject *item;
        DBG("type == %d '%c'", type, type);

        item = _message_iter_get_pyobject(iter, opts, top_level);
        if (!item) return -1;
#ifdef USING_DBG
        fprintf(stderr, "DBG/%ld: appending to list: %p == ", (long)getpid(), item);
        PyObject_Print(item, stderr, 0);
        fprintf(stderr, " of type %p (Byte is %p)\n", item->ob_type,
                &ByteType);
#endif
        ret = PyList_Append(list, item);
        Py_DECREF(item);
        item = NULL;
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

/* Returns a new reference. */
static PyObject *
_message_iter_get_pyobject(DBusMessageIter *iter,
                           Message_get_args_options *opts, int top_level)
{
    union {
        const char *s;
        unsigned char y;
        dbus_bool_t b;
        double d;
        dbus_uint16_t u16;
        dbus_int16_t i16;
        dbus_uint32_t u32;
        dbus_int32_t i32;
#if defined(DBUS_HAVE_INT64) && defined(HAVE_LONG_LONG)
        dbus_uint64_t u64;
        dbus_int64_t i64;
#endif
    } u;

    int type = dbus_message_iter_get_arg_type(iter);

    switch (type) {
      case DBUS_TYPE_STRING:
          DBG("%s", "found a string");
          dbus_message_iter_get_basic(iter, &u.s);
          if (opts->utf8_strings)
              return PyString_FromString(u.s);
          else
              return PyUnicode_DecodeUTF8(u.s, strlen(u.s), NULL);

      case DBUS_TYPE_SIGNATURE:
          DBG("%s", "found a signature");
          dbus_message_iter_get_basic(iter, &u.s);
          return PyObject_CallFunction((PyObject *)&SignatureType, "(s)", u.s);

      case DBUS_TYPE_OBJECT_PATH:
          DBG("%s", "found an object path");
          dbus_message_iter_get_basic(iter, &u.s);
          return PyObject_CallFunction((PyObject *)&ObjectPathType, "(s)", u.s);

      case DBUS_TYPE_DOUBLE:
          DBG("%s", "found a double");
          dbus_message_iter_get_basic(iter, &u.d);
          return PyFloat_FromDouble(u.d);

      case DBUS_TYPE_INT16:
          DBG("%s", "found an int16");
          dbus_message_iter_get_basic(iter, &u.i16);
          if (opts->untyped_integers)
              return PyInt_FromLong(u.i16);
          else
              return PyObject_CallFunction((PyObject *)&Int16Type, "(i)", (int)u.i16);

      case DBUS_TYPE_UINT16:
          DBG("%s", "found a uint16");
          dbus_message_iter_get_basic(iter, &u.u16);
          if (opts->untyped_integers)
              return PyInt_FromLong(u.u16);
          else
              return PyObject_CallFunction((PyObject *)&UInt16Type, "(i)", (int)u.u16);

      case DBUS_TYPE_INT32:
          DBG("%s", "found an int32");
          dbus_message_iter_get_basic(iter, &u.i32);
          if (opts->untyped_integers)
              return PyInt_FromLong(u.i32);
          else
              return PyObject_CallFunction((PyObject *)&Int32Type, "(l)", (long)u.i32);

      case DBUS_TYPE_UINT32:
          DBG("%s", "found a uint32");
          dbus_message_iter_get_basic(iter, &u.u32);
          if (opts->untyped_integers)
              return PyLong_FromUnsignedLong(u.u32);
          else
              return PyObject_CallFunction((PyObject *)&UInt32Type, "(k)",
                                           (unsigned long)u.u32);

#if defined(DBUS_HAVE_INT64) && defined(HAVE_LONG_LONG)
      case DBUS_TYPE_INT64:
          DBG("%s", "found an int64");
          dbus_message_iter_get_basic(iter, &u.i64);
          if (opts->untyped_integers)
              return PyLong_FromLongLong(u.i64);
          else
              return PyObject_CallFunction((PyObject *)&Int64Type, "(L)",
                                           (long long)u.i64);

      case DBUS_TYPE_UINT64:
          DBG("%s", "found a uint64");
          dbus_message_iter_get_basic(iter, &u.u64);
          if (opts->untyped_integers)
              return PyLong_FromUnsignedLongLong(u.u64);
          else
              return PyObject_CallFunction((PyObject *)&UInt64Type, "(K)",
                                           (unsigned long long)u.u64);
#else
      case DBUS_TYPE_INT64:
      case DBUS_TYPE_UINT64:
          PyErr_SetString (PyExc_RuntimeError, "64-bit integers not "
                           "supported on this platform");
          return NULL;
#endif

      case DBUS_TYPE_BYTE:
          DBG("%s", "found a byte");
          dbus_message_iter_get_basic(iter, &u.y);
          if (opts->integer_bytes)
              return PyInt_FromLong(u.y);
          else
              return Byte_from_uchar(u.y);

      case DBUS_TYPE_BOOLEAN:
          DBG("%s", "found a bool");
          dbus_message_iter_get_basic(iter, &u.b);
          return PyBool_FromLong(u.b);

      case DBUS_TYPE_ARRAY:
          DBG("%s", "found an array...");
          /* Dicts are arrays of DBUS_TYPE_DICT_ENTRY on the wire.
          Also, we special-case arrays of DBUS_TYPE_BYTE sometimes. */
          type = dbus_message_iter_get_element_type(iter);
          if (type == DBUS_TYPE_DICT_ENTRY) {
              DBusMessageIter entries, kv;
              PyObject *key, *value, *dict;

              DBG("%s", "no, actually it's a dict...");
              dict = PyObject_CallObject((PyObject *)&DictType,
                                          empty_tuple);
              if (!dict) return NULL;

              dbus_message_iter_recurse(iter, &entries);
              while ((type = dbus_message_iter_get_arg_type(&entries))
                     == DBUS_TYPE_DICT_ENTRY) {
                  DBG("%s", "dict entry...");
                  key = NULL;
                  value = NULL;
                  dbus_message_iter_recurse(&entries, &kv);
                  key = _message_iter_get_pyobject(&kv, opts, 1);
                  if (key) {
                      dbus_message_iter_next(&kv);
                      value = _message_iter_get_pyobject(&kv, opts, 1);
                      if (value && PyDict_SetItem(dict, key, value) == 0) {
                          Py_XDECREF(key);
                          Py_XDECREF(value);
                          dbus_message_iter_next(&entries);
                          continue;
                      }
                  }
                  /* error */
                  Py_XDECREF(key);
                  Py_XDECREF(value);
                  Py_XDECREF(dict);
                  return NULL;
              }
              return dict;
          }
          else if (opts->byte_arrays && type == DBUS_TYPE_BYTE) {
              int n;
              DBG("%s", "actually, a byte array...");
              dbus_message_iter_get_fixed_array(iter,
                                                (const unsigned char **)&u.s,
                                                &n);
              return ByteArray_from_uchars((const unsigned char *)u.s, n);
          }
          else {
              DBusMessageIter sub;
              PyObject *list = PyObject_CallObject((PyObject *)&ArrayType,
                                                   empty_tuple);
              DBG("%s", "a normal array...");
              if (!list) return NULL;
              dbus_message_iter_recurse(iter, &sub);
              if (_message_iter_append_all_to_list(&sub, list, opts, 1) < 0) {
                  Py_DECREF(list);
                  return NULL;
              }
              return list;
          }

      case DBUS_TYPE_STRUCT:
        {
            PyObject *tuple;
            DBusMessageIter sub;
            PyObject *list = PyList_New(0);
            DBG("%s", "found a struct...");
            if (!list) return NULL;
            dbus_message_iter_recurse(iter, &sub);
            if (_message_iter_append_all_to_list (&sub, list, opts, 1) < 0) {
                Py_DECREF(list);
                return NULL;
            }
            tuple = PyList_AsTuple(list);
            /* whether tuple is NULL or not, we take the same action: */
            Py_DECREF(list);
            return tuple;
        }

      case DBUS_TYPE_VARIANT:
        {
            DBusMessageIter sub;

            DBG("%s", "found a variant...");
            dbus_message_iter_recurse(iter, &sub);
            if (opts->variant_unpack_level > 1) {
                /* throw away arbitrarily many layers of Variant wrapper */
                return _message_iter_get_pyobject(&sub, opts, 1);
            }
            else {
                /* recurse into the object, and this time don't throw away
                 * Variant wrappers */
                PyObject *object = _message_iter_get_pyobject(&sub, opts, 0);
                PyObject *ret;
                if (!object) return NULL;
                if (top_level && opts->variant_unpack_level == 1) return object;
                ret = PyObject_CallFunction((PyObject *)&VariantType,
                                            "(Os)", object,
                                            dbus_message_iter_get_signature (&sub));
                /* whether ret is NULL or not, we take the same action: */
                Py_DECREF(object);
                return ret;
            }
        }

      default:
          PyErr_Format(PyExc_TypeError, "Unknown type '\\%x' in D-Bus "
                       "message", type);
          return NULL;
    }
}

static PyObject *
Message_get_args_list(Message *self, PyObject *args, PyObject *kwargs)
{
    Message_get_args_options opts = { 0, 0, 0, 0, 0 };
    static char *argnames[] = { "integer_bytes", "byte_arrays",
                                "untyped_integers",
                                "variant_unpack_level",
                                "utf8_strings", NULL };
    PyObject *list, *tuple = NULL;
    DBusMessageIter iter;

    if (PyTuple_Size(args) != 0) {
        PyErr_SetString(PyExc_TypeError, "get_args_list takes no positional "
                        "arguments");
        return NULL;
    }
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|iiiii:get_args_list",
                                     argnames,
                                     &(opts.integer_bytes), &(opts.byte_arrays),
                                     &(opts.untyped_integers),
                                     &(opts.variant_unpack_level),
                                     &(opts.utf8_strings))) return NULL;
    if (!self->msg) return DBusException_UnusableMessage();

    list = PyList_New(0);
    if (!list) return NULL;

    /* Iterate over args, if any, appending to list */
    if (dbus_message_iter_init(self->msg, &iter)) {
        if (_message_iter_append_all_to_list(&iter, list, &opts, 1) < 0) {
            Py_DECREF(list);
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
