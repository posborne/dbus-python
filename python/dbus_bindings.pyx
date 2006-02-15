# -*- Mode: Python -*-

# jdahlin is the most coolest and awesomest person in the world
# and wrote all the good parts of this code. all the bad parts
# where python conditionals have a ( ) around them, thus violating
# PEP-8 were written by the lame wannabe python programmer seth

#FIXME: find memory leaks that I am sure exist

cdef extern from "sys/types.h":
    ctypedef size_t
    ctypedef __int64_t
    ctypedef __uint64_t

cdef extern from "sys/cdefs.h":
    ctypedef __signed

cdef extern from "stdlib.h":
    cdef void *malloc(size_t size)
    cdef void free(void *ptr)
    cdef void *calloc(size_t nmemb, size_t size)

cdef extern from "Python.h":
    void Py_XINCREF (object)
    void Py_XDECREF (object)
    object PyString_FromStringAndSize(char *, int)
    ctypedef void *PyGILState_STATE
    void PyErr_Clear()
    PyGILState_STATE PyGILState_Ensure()
    void PyGILState_Release(PyGILState_STATE)

ctypedef struct DBusError:
    char *name
    char *message
    unsigned int dummy1 
    unsigned int dummy2
    unsigned int dummy3
    unsigned int dummy4
    unsigned int dummy5
    void *padding1
    
ctypedef struct DBusMessageIter:
    void *dummy1
    void *dummy2
    dbus_uint32_t dummy3
    int dummy4
    int dummy5
    int dummy6
    int dummy7
    int dummy8
    int dummy9
    int dummy10
    int dummy11
    int pad1
    int pad2
    void *pad3

ctypedef struct DBusObjectPathVTable:
  DBusObjectPathUnregisterFunction   unregister_function
  DBusObjectPathMessageFunction      message_function
  void (* dbus_internal_pad1) (void *)
  void (* dbus_internal_pad2) (void *)
  void (* dbus_internal_pad3) (void *)
  void (* dbus_internal_pad4) (void *)

class DBusException(Exception):
    pass

class ConnectionError(Exception):
    pass

class ObjectPath(str):
    def __init__(self, value):
        str.__init__(self, value)

class ByteArray(str):
    def __init__(self, value):
        str.__init__(self, value)

class SignatureIter(object):
    def __init__(self, string):
        object.__init__(self)
        self.remaining = string

    def next(self):
        if self.remaining == '':
            raise StopIteration

        signature = self.remaining
        block_depth = 0
        block_type = None
        end = len(signature)

        for marker in range(0, end):
            cur_sig = ord(signature[marker])

            if cur_sig == TYPE_ARRAY:
                pass
            elif cur_sig == DICT_ENTRY_BEGIN or cur_sig == STRUCT_BEGIN:
                if block_type == None:
                    block_type = cur_sig

                if block_type == cur_sig:
                    block_depth = block_depth + 1

            elif cur_sig == DICT_ENTRY_END:
                if block_type == DICT_ENTRY_BEGIN:
                    block_depth = block_depth - 1

                if block_depth == 0:
                    end = marker
                    break

            elif cur_sig == STRUCT_END:
                if block_type == STRUCT_BEGIN:
                    block_depth = block_depth - 1

                if block_depth == 0:
                    end = marker
                    break

            else:
                if block_depth == 0:
                    end = marker
                    break

        end = end + 1
        self.remaining = signature[end:]
        return Signature(signature[0:end])

class Signature(str):
    """An iterable method signature. Iterating gives the signature of each
    argument in turn."""
    def __init__(self, value):
        return str.__init__(self, value)

    def __iter__(self):
        return SignatureIter(self)

class VariantSignature(object):
    """A fake method signature which when iterated, is an endless stream
    of variants (handy with zip()). It has no string representation."""
    def __iter__(self):
        return self

    def next(self):
        return 'v'

class Byte(int):
    def __init__(self, value):
        int.__init__(self, value)

class Boolean(int):
    def __init__(self, value):
        int.__init__(self, value)

class Int16(int):
    def __init__(self, value):
        int.__init__(self, value)

class UInt16(int):
    def __init__(self, value):
        if value < 0:
            raise TypeError('Unsigned integers must not have a negitive value') 
        int.__init__(self, value)

class Int32(int):
    def __init__(self, value):
        int.__init__(self, value)

class UInt32(long):
    def __init__(self, value):
        if value < 0:
            raise TypeError('Unsigned integers must not have a negitive value') 
        long.__init__(self, value)

class Int64(long):
    def __init__(self, value):
        long.__init__(self, value)

class UInt64(long):
    def __init__(self, value):
        if value < 0:
            raise TypeError('Unsigned integers must not have a negitive value') 
        long.__init__(self, value)

class Double(float):
    def __init__(self, value):
        float.__init__(self, value)

class String(unicode):
    def __init__(self, value):
        unicode.__init__(self, value)

class Array(list):
    def __init__(self, value, type=None, signature=None):
        if signature and type:
            raise TypeError('Can not mix type and signature arguments in a D-BUS Array')
    
        self.type = type
        self.signature = signature
        list.__init__(self, value)

class Variant:
    def __init__(self, value, type=None, signature=None):
        self.value = value
        if signature and type:
            raise TypeError('Can not mix type and signature arguments in a D-BUS Variant')

        self.type = type
        self.signature = signature

    def __repr__(self):
        return repr(self.value)

    def __str__(self):
        return str(self.value)

class Struct(tuple):
    def __init__(self, value):
        tuple.__init__(self, value)

class Dictionary(dict):
    def __init__(self, value, key_type=None, value_type=None, signature=None):
        if key_type and not value_type:
             raise TypeError('When specifying a key_type you must also have a value_type in a D-BUS Dictionary')
        elif value_type and not key_type:
             raise TypeError('When specifying a value_type you must also have a key_type in a D-BUS Dictionary')
        elif key_type and signature:
              raise TypeError('Can not mix type arguments with signature arguments in a D-BUS Dictionary')
              
        self.key_type = key_type
        self.value_type = value_type
        self.signature = signature
        dict.__init__(self, value)

#forward delcerations
cdef class Message
cdef class PendingCall
cdef class Watch
cdef class MessageIter

cdef void _GIL_safe_cunregister_function_handler (DBusConnection *connection,
                                                  void *user_data):
    cdef Connection conn

    tup = <object>user_data
    assert (type(tup) == tuple)    
    function = tup[1]
    conn = Connection()
    conn.__cinit__(None, connection)

    args = (conn)
    function(*args)
    Py_XDECREF(tup) 

cdef void cunregister_function_handler (DBusConnection *connection,
                                        void *user_data):
    cdef PyGILState_STATE gil
    gil = PyGILState_Ensure()
    try:
        _GIL_safe_cunregister_function_handler (connection, user_data);
    finally:
        PyGILState_Release(gil)



cdef DBusHandlerResult _GIL_safe_cmessage_function_handler ( 
                                                  DBusConnection *connection,
                                                  DBusMessage *msg,
                                                  void *user_data):
    cdef Connection conn
    cdef Message message

    tup = <object>user_data
    assert (type(tup) == tuple)
    function = tup[0]
    message = EmptyMessage()

    #we don't own the message so we need to ref it
    dbus_message_ref(msg)
    message._set_msg(msg)
    conn = Connection()
    conn.__cinit__(None, connection)
    args = (conn,
            message)
 
    retval = function(*args)

    if (retval == None):
        retval = DBUS_HANDLER_RESULT_HANDLED
    return retval

cdef DBusHandlerResult cmessage_function_handler (DBusConnection *connection,
                                                  DBusMessage *msg,
                                                  void *user_data):
    cdef PyGILState_STATE gil
    gil = PyGILState_Ensure()
    try:
        return _GIL_safe_cmessage_function_handler (connection, msg, user_data);
    finally:
        PyGILState_Release(gil)


cdef class Connection:
    def __init__(self, address=None, Connection _conn=None):
        cdef DBusConnection *c_conn
        cdef char *c_address
        c_conn=NULL
        self.conn = NULL
        if (_conn != None):
            c_conn = _conn.conn

        if (address != None or _conn != None):
            self.__cinit__(c_address, c_conn)

    # hack to be able to pass in a c pointer to the constructor
    # while still alowing python programs to create a Connection object
    cdef __cinit__(self, address, DBusConnection *_conn):
        cdef DBusError error
        dbus_error_init(&error)
        if _conn != NULL:
            self.conn = _conn
            dbus_connection_ref(self.conn)
        else:
            self.conn = dbus_connection_open(address,
                                         &error)
            if dbus_error_is_set(&error):
                errormsg = error.message
                dbus_error_free (&error)
                raise DBusException, errormsg

    def __dealloc__(self):
        if self.conn != NULL:
            dbus_connection_unref(self.conn)

    cdef _set_conn(self, DBusConnection *conn):
        self.conn = conn
    
    cdef DBusConnection *_get_conn(self):
        return self.conn
    
    def get_unique_name(self):
        return bus_get_unique_name(self)

    def close(self):
        dbus_connection_close(self.conn)

    def get_is_connected(self):
        return dbus_connection_get_is_connected(self.conn)
    
    def get_is_authenticated(self):
        return dbus_connection_get_is_authenticated(self.conn)

    def flush(self):
        dbus_connection_flush(self.conn)

    def borrow_message(self):
        cdef Message m
        m = EmptyMessage()
        m._set_msg(dbus_connection_borrow_message(self.conn))
        return m
    
    def return_message(self, Message message):
        cdef DBusMessage *msg
        msg = message._get_msg()
        dbus_connection_return_message(self.conn, msg)

    def steal_borrowed_message(self, Message message):
        cdef DBusMessage *msg
        msg = message._get_msg()
        dbus_connection_steal_borrowed_message(self.conn,
                                               msg)
    
    def pop_message(self):
        cdef DBusMessage *msg
        cdef Message m
 
        msg = dbus_connection_pop_message(self.conn)
        if msg != NULL:
            m = EmptyMessage()
            m._set_msg(msg)
        else:
            m = None
        return m        

    def get_dispatch_status(self):
        return dbus_connection_get_dispatch_status(self.conn)
    
    def dispatch(self):
        return dbus_connection_dispatch(self.conn)

    def send(self, Message message):
        #cdef dbus_uint32_t client_serial
        #if type(message) != Message:
        #    raise TypeError
        cdef DBusMessage *msg
        msg = message._get_msg()
        retval = dbus_connection_send(self.conn,
                                      msg,
                                      NULL)
        return retval

    def send_with_reply_handlers(self, Message message, timeout_milliseconds, reply_handler, error_handler):
        retval = False
        try:
            (retval, pending_call) = self.send_with_reply(message, timeout_milliseconds)
            if pending_call:
                pending_call.set_notify(reply_handler, error_handler)
        except Exception, e:
            error_handler(e)
            
        return (retval, pending_call)

    def send_with_reply(self, Message message, timeout_milliseconds):
        cdef dbus_bool_t retval
        cdef DBusPendingCall *cpending_call
        cdef DBusMessage *msg
        cdef PendingCall pending_call

        cpending_call = NULL
        
        msg = message._get_msg()

        retval = dbus_connection_send_with_reply(self.conn,
                                                 msg,
                                                 &cpending_call,
                                                 timeout_milliseconds)

        if (cpending_call != NULL):
            pending_call = PendingCall()
            pending_call.__cinit__(cpending_call)
        else:
            pending_call = None

        return (retval, pending_call)
                                
    def send_with_reply_and_block(self, Message message,
                                  timeout_milliseconds=-1):
        cdef DBusMessage * retval
        cdef DBusError error
        cdef DBusMessage *msg
        cdef Message m

        dbus_error_init(&error)

        msg = message._get_msg()

        retval = dbus_connection_send_with_reply_and_block(
            self.conn,
            msg,
            timeout_milliseconds,
            &error)

        if dbus_error_is_set(&error):
            errormsg = error.message
            dbus_error_free (&error)
            raise DBusException, errormsg

        assert(retval != NULL)

        m = EmptyMessage()
        m._set_msg(retval)

        return m

    def set_watch_functions(self, add_function, remove_function, data):
        pass

    def set_timeout_functions(self, add_function, remove_function, data):
        pass

    def set_wakeup_main_function(self, wakeup_main_function, data):
        pass

    # FIXME: set_dispatch_status_function, get_unix_user, set_unix_user_function

    def add_filter(self, filter_function):
        user_data = (filter_function,)
        Py_XINCREF(user_data)
       
        return dbus_connection_add_filter(self.conn,
                                          cmessage_function_handler,
                                          <void*>user_data,
                                          NULL)


    #FIXME: remove_filter
    #       this is pretty tricky, we want to only remove the filter
    #       if we truly have no more calls to our message_function_handler...ugh

    def set_data(self, slot, data):
        pass

    def get_data(self, slot):
        pass

    def set_max_message_size(self, size):
        dbus_connection_set_max_message_size(self.conn, size)

    def get_max_message_size(self):
        return dbus_connection_get_max_message_size(self.conn)

    def set_max_received_size(self, size):
        dbus_connection_set_max_received_size(self.conn, size)

    def get_max_received_size(self):
        return dbus_connection_get_max_received_size(self.conn)

    def get_outgoing_size(self):
        return dbus_connection_get_outgoing_size(self.conn)    

    # preallocate_send, free_preallocated_send, send_preallocated

    def register_object_path(self, path, unregister_cb, message_cb):
        cdef DBusObjectPathVTable cvtable
        
        cvtable.unregister_function = cunregister_function_handler 
        cvtable.message_function    = cmessage_function_handler

        user_data = (message_cb, unregister_cb)
        Py_XINCREF(user_data)
        
        return dbus_connection_register_object_path(self.conn, path, &cvtable,
                                                    <void*>user_data) 

    def register_fallback(self, path, unregister_cb, message_cb):
        cdef DBusObjectPathVTable cvtable

        cvtable.unregister_function = cunregister_function_handler 
        cvtable.message_function    = cmessage_function_handler

        user_data = (message_cb, unregister_cb)
        Py_XINCREF(user_data)
       
        return dbus_connection_register_fallback(self.conn, path, &cvtable,
                                                 <void*>user_data) 

    #FIXME: unregister_object_path , see problems with remove_filter

    def list_registered (self, parent_path):
        cdef char **cchild_entries
        cdef dbus_bool_t retval
        
        retval = dbus_connection_list_registered(self.conn, parent_path, &cchild_entries)

        if (not retval):
            #FIXME: raise out of memory exception?
            return None

        i = 0
        child_entries = []

        while (cchild_entries[i] != NULL):
            child_entries.append(cchild_entries[i])
            i = i + 1

        dbus_free_string_array(cchild_entries)

        return child_entries

cdef void _GIL_safe_pending_call_notification (DBusPendingCall *pending_call, 
                                               void *user_data):
    cdef DBusMessage *dbus_message
    cdef Message message
  
    (reply_handler, error_handler) = <object>user_data
   
    dbus_message = dbus_pending_call_steal_reply(pending_call)
    message = EmptyMessage()
    message._set_msg(dbus_message)

    type = message.get_type()

    if type == MESSAGE_TYPE_METHOD_RETURN:
        args = message.get_args_list()
        reply_handler(*args)
    elif type == MESSAGE_TYPE_ERROR:
        args = message.get_args_list()
        if len(args) > 0:
            error_handler(DBusException(args[0]))
        else:
            error_handler(DBusException(""))
    else:
        error_handler(DBusException('Unexpected Message Type: ' + message.type_to_name(type)))

    dbus_pending_call_unref(pending_call)
    Py_XDECREF(<object>user_data)

cdef void _pending_call_notification(DBusPendingCall *pending_call, 
                                     void *user_data):
    cdef PyGILState_STATE gil
    gil = PyGILState_Ensure()
    try:
        _GIL_safe_pending_call_notification (pending_call, user_data);
    finally:
        PyGILState_Release(gil)

cdef void _pending_call_free_user_data(void *data):
    call_tuple = <object>data
    Py_XDECREF(call_tuple)

cdef class PendingCall:
    cdef DBusPendingCall *pending_call

    def __init__(self, PendingCall _pending_call=None):
        self.pending_call = NULL
        if (_pending_call != None):
            self.__cinit__(_pending_call.pending_call)

    cdef void __cinit__(self, DBusPendingCall *_pending_call):
        self.pending_call = _pending_call
        dbus_pending_call_ref(self.pending_call)

    def __dealloc__(self):
        if self.pending_call != NULL:
            dbus_pending_call_unref(self.pending_call)

    cdef DBusPendingCall *_get_pending_call(self):
        return self.pending_call

    def cancel(self):
        dbus_pending_call_cancel(self.pending_call)

    def get_completed(self):
        return dbus_pending_call_get_completed(self.pending_call)

    def get_reply(self):
        cdef Message message
        message = EmptyMessage()
        message._set_msg(dbus_pending_call_steal_reply(self.pending_call))
        return message

    def block(self):
        dbus_pending_call_block(self.pending_call)

    def set_notify(self, reply_handler, error_handler):
        user_data = (reply_handler, error_handler)
        Py_XINCREF(user_data)
        dbus_pending_call_ref(self.pending_call)
        dbus_pending_call_set_notify(self.pending_call, _pending_call_notification, 
                                     <void *>user_data, _pending_call_free_user_data)
        

cdef class Watch:
    cdef DBusWatch* watch

    def __init__(self):
        pass

    cdef __cinit__(self, DBusWatch *cwatch):
        self.watch = cwatch

    def get_fd(self):
        return dbus_watch_get_fd(self.watch)

    # FIXME: not picked up correctly by extract.py
    #def get_flags(self):
    #    return dbus_watch_get_flags(self.watch)

    def handle(self, flags):
        return dbus_watch_handle(self.watch, flags)

    def get_enabled(self):
        return dbus_watch_get_enabled(self.watch)
    
cdef class MessageIter:
    cdef DBusMessageIter *iter
    cdef DBusMessageIter real_iter
    cdef dbus_uint32_t level

    def __init__(self, level=0):
        self.iter = &self.real_iter
        self.level = level
        if(self.level > 32):
            raise TypeError, 'Type recurion is too deep' 

    cdef __cinit__(self, DBusMessageIter *iter):
        self.real_iter = iter[0]

    cdef DBusMessageIter *_get_iter(self):
        return self.iter

    def has_next(self):
        return dbus_message_iter_has_next(self.iter)
    
    def next(self):
        return dbus_message_iter_next(self.iter)

    def get(self, arg_type=None):
        if(arg_type == None):
            arg_type = self.get_arg_type()

        if arg_type == TYPE_INVALID:
            raise TypeError, 'Invalid arg type in MessageIter'
        elif arg_type == TYPE_STRING:
            retval = self.get_string()
        elif arg_type == TYPE_INT16:
            retval = self.get_int16()
        elif arg_type == TYPE_UINT16:
            retval = self.get_uint16()
        elif arg_type == TYPE_INT32:
            retval = self.get_int32()
        elif arg_type == TYPE_UINT32:
            retval = self.get_uint32()
        elif arg_type == TYPE_INT64:
            retval = self.get_int64()
        elif arg_type == TYPE_UINT64:
            retval = self.get_uint64()
        elif arg_type == TYPE_DOUBLE:
            retval = self.get_double()
        elif arg_type == TYPE_BYTE:
            retval = self.get_byte()
        elif arg_type == TYPE_BOOLEAN:
            retval = self.get_boolean()
        elif arg_type == TYPE_SIGNATURE:
            retval = self.get_signature()
        elif arg_type == TYPE_ARRAY:
            array_type = self.get_element_type()
            if array_type == TYPE_DICT_ENTRY:
                retval = self.get_dict()
            else:
                retval = self.get_array(array_type)
        elif arg_type == TYPE_OBJECT_PATH:
            retval = self.get_object_path()
        elif arg_type == TYPE_STRUCT:
            retval = self.get_struct()
        elif arg_type == TYPE_VARIANT:
            retval = self.get_variant()
        elif arg_type == TYPE_DICT_ENTRY:
            raise TypeError, 'Dictionary Entries can only appear as part of an array container'
        else:
            raise TypeError, 'Unknown arg type %d in MessageIter' % (arg_type)

        return retval

    def get_arg_type(self):
        return dbus_message_iter_get_arg_type(self.iter)

    def get_element_type(self):
        return dbus_message_iter_get_element_type(self.iter)

    def get_byte(self):
        cdef unsigned char c_val
        dbus_message_iter_get_basic(self.iter, <unsigned char *>&c_val)
        return c_val

    def get_boolean(self):
        cdef dbus_bool_t c_val
        dbus_message_iter_get_basic(self.iter, <dbus_bool_t *>&c_val)

        if c_val:
            return True
        else:
            return False 

    def get_signature(self):
        signature_string = self.get_string()
        return Signature(signature_string)

    def get_int16(self):
        cdef dbus_int16_t c_val
        dbus_message_iter_get_basic(self.iter, <dbus_int16_t *>&c_val)

        return c_val

    def get_uint16(self):
        cdef dbus_uint16_t c_val
        dbus_message_iter_get_basic(self.iter, <dbus_uint16_t *>&c_val)
        return c_val

    def get_int32(self):
        cdef dbus_int32_t c_val
        dbus_message_iter_get_basic(self.iter, <dbus_int32_t *>&c_val)
        return c_val
        
    def get_uint32(self):
        cdef dbus_uint32_t c_val
        dbus_message_iter_get_basic(self.iter, <dbus_uint32_t *>&c_val)
        return c_val
        
    def get_int64(self):
        cdef dbus_int64_t c_val
        dbus_message_iter_get_basic(self.iter, <dbus_int64_t *>&c_val)
        return c_val

    def get_uint64(self):
        cdef dbus_uint64_t c_val
        dbus_message_iter_get_basic(self.iter, <dbus_uint64_t *>&c_val)
        return c_val

    def get_double(self):
        cdef double c_val
        dbus_message_iter_get_basic(self.iter, <double *>&c_val)
        return c_val

    def get_string(self):
        cdef char *c_str
        dbus_message_iter_get_basic(self.iter, <char **>&c_str)
        ret = c_str.decode('utf8')

        return ret

    def get_object_path(self):
        object_path_string = self.get_string()
        return ObjectPath(object_path_string)

    def get_dict(self):
        cdef DBusMessageIter c_dict_iter
        cdef MessageIter dict_iter
        level = self.level + 1

        dbus_message_iter_recurse(self.iter, <DBusMessageIter *>&c_dict_iter)
        dict_iter = MessageIter(level)
        dict_iter.__cinit__(&c_dict_iter)
        
        python_dict = {}
        cur_arg_type = dict_iter.get_arg_type()
        while cur_arg_type == TYPE_DICT_ENTRY:
            if cur_arg_type != TYPE_DICT_ENTRY:
                raise TypeError, "Dictionary elements must be of type TYPE_DICT_ENTRY '%s != %s'" % (TYPE_DICT_ENTRY, cur_arg_type)
                
            dict_entry = dict_iter.get_struct()
            if len(dict_entry) != 2:
                raise TypeError, "Dictionary entries must be structs of two elements.  This entry had %i elements.'" % (len(dict_entry))

            python_dict[dict_entry[0]] = dict_entry[1]
            
            dict_iter.next()
            cur_arg_type = dict_iter.get_arg_type()

        return python_dict
   
    def get_array(self, type):
        cdef DBusMessageIter c_array_iter
        cdef MessageIter array_iter
        level = self.level + 1

        dbus_message_iter_recurse(self.iter, <DBusMessageIter *>&c_array_iter)
        array_iter = MessageIter(level)
        array_iter.__cinit__(&c_array_iter)
        
        python_list = []
        cur_arg_type = array_iter.get_arg_type()
        while cur_arg_type != TYPE_INVALID:
            if cur_arg_type != type:
                raise TypeError, "Array elements must be of the same type '%s != %s'" % (type, cur_arg_type)
                
            value = array_iter.get(type)
            python_list.append(value)
            
            array_iter.next()
            cur_arg_type = array_iter.get_arg_type()

        return python_list

    def get_variant(self):
        cdef DBusMessageIter c_var_iter
        cdef MessageIter var_iter
        level = self.level + 1

        dbus_message_iter_recurse(self.iter, <DBusMessageIter *>&c_var_iter)
        var_iter = MessageIter(level)
        var_iter.__cinit__(&c_var_iter)
        
        return var_iter.get() 

    def get_struct(self):
        cdef DBusMessageIter c_struct_iter
        cdef MessageIter struct_iter
        level = self.level + 1

        dbus_message_iter_recurse(self.iter, <DBusMessageIter *>&c_struct_iter)
        struct_iter = MessageIter(level)
        struct_iter.__cinit__(&c_struct_iter)

        python_list = []
        while struct_iter.get_arg_type() != TYPE_INVALID:
            value = struct_iter.get()
            python_list.append(value)
            
            struct_iter.next()

        return tuple(python_list)

    def python_value_to_dbus_sig(self, value, level = 0):

        if(level > 32):
            raise TypeError, 'Type recurion is too deep' 

        level = level + 1

        ptype = type(value)
        ret = ""
        if ptype == bool:
            ret = TYPE_BOOLEAN
            ret = str(chr(ret))
        elif ptype == int:
            ret = TYPE_INT32
            ret = str(chr(ret))
        elif ptype == long:
            ret = TYPE_INT64
            ret = str(chr(ret))
        elif (ptype == str or ptype == unicode):
            ret = TYPE_STRING
            ret = str(chr(ret))
        elif ptype == float:
            ret = TYPE_DOUBLE
            ret = str(chr(ret))
        elif ptype == dict:
            dict_list = value.items()
            key, value = dict_list[0]

            ret = str(chr(TYPE_ARRAY)) + str(chr(DICT_ENTRY_BEGIN))
            ret = ret + self.python_value_to_dbus_sig(key, level)
            ret = ret + self.python_value_to_dbus_sig(value, level)
            ret = ret + str(chr(DICT_ENTRY_END))

        elif ptype == tuple:
            ret = str(chr(STRUCT_BEGIN))
            for item in value:
                ret = ret + self.python_value_to_dbus_sig(item, level)
            ret = ret + str(chr(STRUCT_END))
        elif ptype == list:
            ret = str(chr(TYPE_ARRAY))
            ret = ret + self.python_value_to_dbus_sig(value[0], level)
        elif isinstance(value, ObjectPath) or value == ObjectPath:

            ret = TYPE_OBJECT_PATH
            ret = str(chr(ret))
        elif isinstance(value, ByteArray) or value == ByteArray:
            ret = str(chr(TYPE_ARRAY)) + str(chr(TYPE_BYTE))
        elif isinstance(value, Signature) or value == Signature:
            ret = TYPE_SIGNATURE
            ret = str(chr(ret))
        elif isinstance(value, Byte) or value == Byte:
            ret = TYPE_BYTE
            ret = str(chr(ret))
        elif isinstance(value, Boolean) or value == Boolean:
            ret = TYPE_BOOLEAN
            ret = str(chr(ret))
        elif isinstance(value, Int16) or value == Int16:
            ret = TYPE_INT16
            ret = str(chr(ret))
        elif isinstance(value, UInt16) or value == UInt16:
            ret = TYPE_UINT16
            ret = str(chr(ret))
        elif isinstance(value, Int32) or value == Int32:
            ret = TYPE_INT32
            ret = str(chr(ret))
        elif isinstance(value, UInt32) or value == UInt32:
            ret = TYPE_UINT32
            ret = str(chr(ret))
        elif isinstance(value, Int64) or value == Int64:
            ret = TYPE_INT64
            ret = str(chr(ret))
        elif isinstance(value, UInt64) or value == UInt64:
            ret = TYPE_UINT64
            ret = str(chr(ret))
        elif isinstance(value, Double) or value == Double:
            ret = TYPE_DOUBLE
            ret = str(chr(ret))
        elif isinstance(value, String) or value == String:
            ret = TYPE_STRING
            ret = str(chr(ret))
        elif isinstance(value, Array):
            ret = str(chr(TYPE_ARRAY))
            if value.type == None:
                if value.signature:
                    ret = ret + value.signature
                else:
                    ret = ret + self.python_value_to_dbus_sig(value[0], level)
            else:
                ret = ret + self.python_value_to_dbus_sig(value.type, level)

        elif isinstance(value, Struct) or value == Struct:
            ret = str(chr(STRUCT_BEGIN))
            for item in value:
                ret = ret + self.python_value_to_dbus_sig(item, level)
            ret = ret + str(chr(STRUCT_END))
        elif isinstance(value, Dictionary):
            ret = str(chr(TYPE_ARRAY)) + str(chr(DICT_ENTRY_BEGIN))
            
            if value.key_type and value.value_type:
                ret = ret + self.python_value_to_dbus_sig(value.key_type, level)
                ret = ret + self.python_value_to_dbus_sig(value.value_type, level)
            elif value.signature:
                ret = ret + value.signature
            else:
                dict_list = value.items()

                key, val = dict_list[0]
                ret = ret + self.python_value_to_dbus_sig(key, level)
                ret = ret + self.python_value_to_dbus_sig(val, level)
                
            ret = ret + str(chr(DICT_ENTRY_END))
        elif isinstance(value, Variant) or value == Variant:
            ret = ret + str(chr(TYPE_VARIANT))
        else:
            raise TypeError, "Argument of unknown type '%s'" % (ptype)

        return ret

    def append_strict(self, value, sig):
    
        if sig == TYPE_INVALID or sig == None:
            raise TypeError, 'Invalid arg type sent to append_strict'

        sig_type = ord(sig[0])
	    
        if sig_type == TYPE_STRING:
            retval = self.append_string(value)
        elif sig_type == TYPE_INT16:
            retval = self.append_int16(value)
        elif sig_type == TYPE_UINT16:
            retval = self.append_uint16(value)
        elif sig_type == TYPE_INT32:
            retval = self.append_int32(value)
        elif sig_type == TYPE_UINT32:
            retval = self.append_uint32(value)
        elif sig_type == TYPE_INT64:
            retval = self.append_int64(value)
        elif sig_type == TYPE_UINT64:
            retval = self.append_uint64(value)
        elif sig_type == TYPE_DOUBLE:
            retval = self.append_double(value)
        elif sig_type == TYPE_BYTE:
            retval = self.append_byte(value)
        elif sig_type == TYPE_BOOLEAN:
            retval = self.append_boolean(value)
        elif sig_type == TYPE_SIGNATURE:
            retval = self.append_signature(value)
        elif sig_type == TYPE_ARRAY:
            if len(sig) < 2:
                raise TypeError, "Invalid array signature in append_strict.  Arrays must be followed by a type."

            array_type = ord(sig[1])            
            if array_type == DICT_ENTRY_BEGIN:
                if ord(sig[-1]) != DICT_ENTRY_END:
                    raise TypeError, "Invalid dict entry in append_strict.  No termination in signature %s."%(sig)

                tmp_sig = sig[2:-1]
                retval = self.append_dict(Dictionary(value, signature=tmp_sig))
            else:
                tmp_sig = sig[1:]
                retval = self.append_array(Array(value, signature=tmp_sig))
        elif sig_type == TYPE_OBJECT_PATH:
            retval = self.append_object_path(value)
        elif sig_type == STRUCT_BEGIN:
            if ord(sig[-1]) != STRUCT_END:
                raise TypeError, "Invalid struct entry in append_strict. No termination in signature %s." % (sig)

            tmp_sig = sig[1:-1]
            retval = self.append_struct(value, signature = tmp_sig)
        elif sig_type == TYPE_VARIANT:
            if isinstance(value, Variant):
                retval = self.append_variant(value)
            else:
                retval = self.append_variant(Variant(value))
        elif sig_type == DICT_ENTRY_BEGIN:
            raise TypeError, "Signiture is invalid in append_strict. A dict entry must be part of an array." 
        else:
            raise TypeError, "Argument of unknown type '%s' in append_strict" % (sig)

        return retval

    def append(self, value):
        value_type = type(value)
        if value_type == bool:
            retval = self.append_boolean(value)
        elif value_type == int:
            retval = self.append_int32(value)
        elif value_type == long:
            retval = self.append_int64(value)
        elif (value_type == str or value_type == unicode):
            retval = self.append_string(value)
        elif value_type == float:
            retval = self.append_double(value)
        elif value_type == dict:
            retval = self.append_dict(value)
        elif value_type == tuple:
            retval = self.append_struct(value)
        elif value_type == list:
             retval = self.append_array(value)
        #elif value_type == None.__class__:
        #    retval = self.append_nil()
        elif isinstance(value, ObjectPath):
            retval = self.append_object_path(value)
        elif isinstance(value, ByteArray):
            retval = self.append_array(value)
        elif isinstance(value, Signature):
            retval = self.append_signature(value)
        elif isinstance(value, Byte):
            retval = self.append_byte(value)
        elif isinstance(value, Boolean):
            retval = self.append_boolean(value)
        elif isinstance(value, Int16):
            retval = self.append_int16(value)
        elif isinstance(value, UInt16):
            retval = self.append_uint16(value)
        elif isinstance(value, Int32):
            retval = self.append_int32(value)
        elif isinstance(value, UInt32):
            retval = self.append_uint32(value)
        elif isinstance(value, Int64):
            retval = self.append_int64(value)
        elif isinstance(value, UInt64):
            retval = self.append_uint64(value)
        elif isinstance(value, Double):
            retval = self.append_double(value)
        elif isinstance(value, String):
            retval = self.append_string(value)
        elif isinstance(value, Array):
            retval = self.append_array(value)
        elif isinstance(value, Struct):
            retval = self.append_struct(value)
        elif isinstance(value, Dictionary):
            retval = self.append_dict(value)
        elif isinstance(value, Variant):
            retval = self.append_variant(value)
        else:
            raise TypeError, "Argument of unknown type '%s'" % (value_type)

        return retval

    def append_boolean(self, value):
        cdef dbus_bool_t c_value
        c_value = value
        return dbus_message_iter_append_basic(self.iter, TYPE_BOOLEAN, <dbus_bool_t *>&c_value)

    def append_byte(self, value):
        cdef char b
        if type(value) == str and len(value) == 1:
                b = ord(value)
        elif type(value) == Byte:
                b = value
        else:
            raise TypeError

        return dbus_message_iter_append_basic(self.iter, TYPE_BYTE, <char *>&b)

    def append_int16(self, value):
        cdef dbus_int16_t c_value
        c_value = value
        return dbus_message_iter_append_basic(self.iter, TYPE_INT16, <dbus_int16_t *>&c_value)

    def append_uint16(self, value):
        cdef dbus_uint16_t c_value
        c_value = value
        return dbus_message_iter_append_basic(self.iter, TYPE_UINT16, <dbus_uint16_t *>&c_value)

    def append_int32(self, value):
        cdef dbus_int32_t c_value
        c_value = value
        return dbus_message_iter_append_basic(self.iter, TYPE_INT32, <dbus_int32_t *>&c_value)

    def append_uint32(self, value):
        cdef dbus_uint32_t c_value
        c_value = value
        return dbus_message_iter_append_basic(self.iter, TYPE_UINT32, <dbus_uint32_t *>&c_value)

    def append_int64(self, value):
        cdef dbus_int64_t c_value
        c_value = value
        return dbus_message_iter_append_basic(self.iter, TYPE_INT64, <dbus_int64_t *>&c_value)

    def append_uint64(self, value):
        cdef dbus_uint64_t c_value
        c_value = value
        return dbus_message_iter_append_basic(self.iter, TYPE_UINT64, <dbus_uint64_t *>&c_value)

    def append_double(self, value):
        cdef double c_value
        c_value = value
        return dbus_message_iter_append_basic(self.iter, TYPE_DOUBLE, <double *>&c_value)

    def append_string(self, value):
        cdef char *c_value
        tmp = value.encode('utf8')
        c_value = tmp
        return dbus_message_iter_append_basic(self.iter, TYPE_STRING, <char **>&c_value)

    def append_object_path(self, value):
        cdef char *c_value
        c_value = value
        return dbus_message_iter_append_basic(self.iter, TYPE_OBJECT_PATH, <char **>&c_value)

    def append_signature(self, value):
        cdef char *c_value
        c_value = value
        return dbus_message_iter_append_basic(self.iter, TYPE_SIGNATURE, <char **>&c_value)


    def append_dict(self, python_dict):
        cdef DBusMessageIter c_dict_iter, c_dict_entry_iter
        cdef MessageIter dict_iter, dict_entry_iter
        
        level = self.level + 1

        key = None
        value = None

        sig = str(chr(DICT_ENTRY_BEGIN))

        if isinstance(python_dict, Dictionary):
            key = python_dict.key_type
            value = python_dict.value_type
            signature = python_dict.signature

        dict_list = python_dict.items()

        if signature:
            sig = sig + signature
        else: 
            if not (key and value):
                key, value = dict_list[0]

            sig = sig + self.python_value_to_dbus_sig(key)
            sig = sig + self.python_value_to_dbus_sig(value)

        sig = sig + str(chr(DICT_ENTRY_END))

        dbus_message_iter_open_container(self.iter, TYPE_ARRAY, sig, <DBusMessageIter *>&c_dict_iter)
        dict_iter = MessageIter(level)
        dict_iter.__cinit__(&c_dict_iter)

        for key, value in dict_list:
            dbus_message_iter_open_container(dict_iter.iter, TYPE_DICT_ENTRY, sig, <DBusMessageIter *>&c_dict_entry_iter)
            dict_entry_iter = MessageIter(level)
            dict_entry_iter.__cinit__(&c_dict_entry_iter)

            if signature:
                signature_iter = iter(Signature(signature))
                tmp_sig = signature_iter.next()
                if not dict_entry_iter.append_strict(key, tmp_sig):
                    dbus_message_iter_close_container(dict_iter.iter, dict_entry_iter.iter)
                    dbus_message_iter_close_container(self.iter, dict_iter.iter)
                    return False

                tmp_sig = signature_iter.next()
                if not dict_entry_iter.append_strict(value, tmp_sig):
                    dbus_message_iter_close_container(dict_iter.iter, dict_entry_iter.iter)
                    dbus_message_iter_close_container(self.iter, dict_iter.iter)
                    return False

            else:
                if not dict_entry_iter.append(key):
                    dbus_message_iter_close_container(dict_iter.iter, dict_entry_iter.iter)
                    dbus_message_iter_close_container(self.iter, dict_iter.iter)
                    return False
                
                if not dict_entry_iter.append(value):
                    dbus_message_iter_close_container(dict_iter.iter, dict_entry_iter.iter)
                    dbus_message_iter_close_container(self.iter, dict_iter.iter)
                    return False

            dbus_message_iter_close_container(dict_iter.iter, dict_entry_iter.iter)

        dbus_message_iter_close_container(self.iter, dict_iter.iter)

        return True

    def append_struct(self, python_struct, signature = None):
        cdef DBusMessageIter c_struct_iter
        cdef MessageIter struct_iter

        level = self.level + 1
        dbus_message_iter_open_container(self.iter, TYPE_STRUCT, NULL, <DBusMessageIter *>&c_struct_iter)
        struct_iter = MessageIter(level)
        struct_iter.__cinit__(&c_struct_iter)

        signature_iter = iter(Signature(signature))
        for item in python_struct:
            if signature:
                sig = signature_iter.next()

                if sig == '':
                    dbus_message_iter_close_container(self.iter, struct_iter.iter)
                    return False

                if not struct_iter.append_strict(item, sig):
                    dbus_message_iter_close_container(self.iter, struct_iter.iter)
                    return False
            else:
                if not struct_iter.append(item):
                    dbus_message_iter_close_container(self.iter, struct_iter.iter)
                    return False

        dbus_message_iter_close_container(self.iter, struct_iter.iter)

        return True

    def append_array(self, python_list):
        cdef DBusMessageIter c_array_iter
        cdef MessageIter array_iter

        level = self.level + 1

        sig = None
        if isinstance(python_list, Array):
            if python_list.type:
                sig = self.python_value_to_dbus_sig(python_list.type)
            elif python_list.signature:
                sig = python_list.signature
            else:
                sig = self.python_value_to_dbus_sig(python_list[0])
        else:
            sig = self.python_value_to_dbus_sig(python_list[0])

        dbus_message_iter_open_container(self.iter, TYPE_ARRAY, sig, <DBusMessageIter *>&c_array_iter)
        array_iter = MessageIter(level)
        array_iter.__cinit__(&c_array_iter)

        length = len(python_list)
        for item in python_list:
            if not array_iter.append_strict(item, sig):
                dbus_message_iter_close_container(self.iter, array_iter.iter)
                return False

        dbus_message_iter_close_container(self.iter, array_iter.iter)

        return True

    def append_variant(self, value):
        cdef DBusMessageIter c_variant_iter
        cdef MessageIter variant_iter

        level = self.level + 1
    
        if value.signature:
            sig = value.signature
        elif value.type:
            sig = self.python_value_to_dbus_sig(value.type)
        else:
            sig = self.python_value_to_dbus_sig(value.value)
    
        dbus_message_iter_open_container(self.iter, TYPE_VARIANT, sig, <DBusMessageIter *>&c_variant_iter)
        
        variant_iter = MessageIter(level)
        variant_iter.__cinit__(&c_variant_iter)

        if not variant_iter.append(value.value):
            dbus_message_iter_close_container(self.iter, variant_iter.iter)
            return False

        dbus_message_iter_close_container(self.iter, variant_iter.iter)
        return True

    def __str__(self):
        cdef DBusMessageIter c_array_iter
        cdef MessageIter array_iter

        value_at_iter = True
        retval = ""
        while (value_at_iter):
            type = self.get_arg_type()
            if type == TYPE_INVALID:
                break
            elif type == TYPE_STRING:
                str = iter.get_string()
                arg = 'string:%s\n' % (str)
            elif type == TYPE_OBJECT_PATH:
                path = iter.get_object_path()
                arg = 'object_path:%s\n' % (path)
            elif type == TYPE_INT16:
                num = iter.get_int16()
                arg = 'int16:%d\n' % (num)
            elif type == TYPE_UINT16:
                num = iter.get_uint16()
                arg = 'uint16:%u\n' % (num)
            elif type == TYPE_INT32:
                num = iter.get_int32()
                arg = 'int32:%d\n' % (num)
            elif type == TYPE_UINT32:
                num = iter.get_uint32()
                arg = 'uint32:%u\n' % (num)
            elif type == TYPE_INT64:
                num = iter.get_int64()
                arg = 'int64:%d\n' % (num)
            elif type == TYPE_UINT64:
                num = iter.get_uint64()
                arg = 'uint64:%u\n' % (num)
            elif type == TYPE_DOUBLE:
                num = iter.get_double()
                arg = 'double:%f\n' % (num)
            elif type == TYPE_BYTE:
                num = iter.get_byte()
                arg = 'byte:%x(%s)\n' % (num, str(chr(num)))
            elif type == TYPE_BOOLEAN:
                bool = iter.get_boolean()
                if (bool):
                    str = "true"
                else:
                    str = "false"
                arg = 'boolean:%s\n' % (str)
            elif type == TYPE_ARRAY:
                dbus_message_iter_recurse(self.iter, <DBusMessageIter *>&c_array_iter)
                array_iter = MessageIter(self.level + 1)
                array_iter.__cinit__(&c_array_iter)
                if array_iter.has_next():
                    arg = 'array [' + str(array_iter) + ']'
                else:
                    arg = 'array []'
            else:
                arg = '(unknown arg type %d)\n' % type

            retval = retval + arg
            value_at_iter = self.next()

        return retval


(MESSAGE_TYPE_INVALID, MESSAGE_TYPE_METHOD_CALL, MESSAGE_TYPE_METHOD_RETURN, MESSAGE_TYPE_ERROR, MESSAGE_TYPE_SIGNAL) = range(5)
(TYPE_INVALID, TYPE_BYTE, TYPE_BOOLEAN, TYPE_INT16, TYPE_UINT16, TYPE_INT32, TYPE_UINT32, TYPE_INT64, TYPE_UINT64, TYPE_DOUBLE, TYPE_STRING, TYPE_OBJECT_PATH, TYPE_SIGNATURE, TYPE_ARRAY, TYPE_STRUCT, STRUCT_BEGIN, STRUCT_END, TYPE_VARIANT, TYPE_DICT_ENTRY, DICT_ENTRY_BEGIN, DICT_ENTRY_END) = (0, ord('y'), ord('b'), ord('n'), ord('q'), ord('i'), ord('u'), ord('x'), ord('t'), ord('d'), ord('s'), ord('o'), ord('g'), ord('a'), ord('r'), ord('('), ord(')'), ord('v'), ord('e'), ord('{'), ord('}'))
(HANDLER_RESULT_HANDLED, HANDLER_RESULT_NOT_YET_HANDLED, HANDLER_RESULT_NEED_MEMORY) = range(3)
    
cdef class Message:
    cdef DBusMessage *msg

    def __init__(self, message_type=MESSAGE_TYPE_INVALID,
                 service=None, path=None, dbus_interface=None, method=None,
                 Message method_call=None,
                 name=None,
                 Message reply_to=None, error_name=None, error_message=None,
                 _create=1):

        self.msg = NULL

        cdef char *cservice
        cdef char *ciface
        cdef DBusMessage *cmsg

        ciface = NULL
        if (dbus_interface != None):
            ciface = dbus_interface

        cservice = NULL
        if (service != None):
            cservice = service
            
        if _create:
            if message_type == MESSAGE_TYPE_METHOD_CALL:
                self.msg = dbus_message_new_method_call(cservice, path, ciface, method)
            elif message_type == MESSAGE_TYPE_METHOD_RETURN:
                cmsg = method_call._get_msg()
                self.msg = dbus_message_new_method_return(cmsg)
            elif message_type == MESSAGE_TYPE_SIGNAL:
                self.msg = dbus_message_new_signal(path, ciface, name)
            elif message_type == MESSAGE_TYPE_ERROR:
                cmsg = reply_to._get_msg()
                self.msg = dbus_message_new_error(cmsg, error_name, error_message)
 

    def __dealloc__(self):
        if self.msg != NULL:
            dbus_message_unref(self.msg)
            
    def type_to_name(self, type):
        if type == MESSAGE_TYPE_SIGNAL:
            return "signal"
        elif type == MESSAGE_TYPE_METHOD_CALL:
            return "method call"
        elif type == MESSAGE_TYPE_METHOD_RETURN:
            return "method return"
        elif type == MESSAGE_TYPE_ERROR:
            return "error"
        else:
            return "(unknown message type)"

    def __str__(self):
        message_type = self.get_type()
        sender = self.get_sender()

        if sender == None:
            sender = "(no sender)"

        if (message_type == MESSAGE_TYPE_METHOD_CALL) or (message_type == MESSAGE_TYPE_SIGNAL):
            retval = '%s interface=%s; member=%s; sender=%s' % (self.type_to_name(message_type),
                                                                self.get_interface(),
                                                                self.get_member(),
                                                                sender)
        elif message_type == MESSAGE_TYPE_METHOD_RETURN:
            retval = '%s sender=%s' % (self.type_to_name(message_type),
                                        sender)
        elif message_type == MESSAGE_TYPE_ERROR:
            retval = '%s name=%s; sender=%s' % (self.type_to_name(message_type),
                                                self.get_error_name(),
                                                sender)
        else:
            retval = "Message of unknown type %d" % (message_type)


        # FIXME: should really use self.convert_to_tuple() here
        
        iter = self.get_iter()

        retval = retval + "\n" + str(iter) 

        return retval
    
    cdef _set_msg(self, DBusMessage *msg):
        self.msg = msg

    cdef DBusMessage *_get_msg(self):
        return self.msg

    def get_iter(self, append=False):
        cdef DBusMessageIter iter
        cdef MessageIter message_iter
        cdef DBusMessage *msg

        msg = self._get_msg()

        if append:
            dbus_message_iter_init_append(msg, &iter)
        else:
            dbus_message_iter_init(msg, &iter)

        message_iter = MessageIter(0)
        message_iter.__cinit__(&iter)

        return message_iter

    def get_args_list(self):
        retval = [ ]

        iter = self.get_iter()
        try:
            retval.append(iter.get())
        except TypeError, e:
            return [ ]
            
        value_at_iter = iter.next()
        while (value_at_iter):
            retval.append(iter.get())
            value_at_iter = iter.next()        

        return retval
            
    # FIXME: implement dbus_message_copy?

    def get_type(self):
        return dbus_message_get_type(self.msg)

    def set_path(self, object_path):
        return dbus_message_set_path(self.msg, object_path)
    
    def get_path(self):
        return dbus_message_get_path(self.msg)
    
    def set_interface(self, interface):
        return dbus_message_set_interface(self.msg, interface)

    def get_interface(self):
        return dbus_message_get_interface(self.msg)    

    def set_member(self, member):
        return dbus_message_set_member(self.msg, member)

    def get_member(self):
        return dbus_message_get_member(self.msg)

    def set_error_name(self, name):
        return dbus_message_set_error_name(self.msg, name)

    def get_error_name(self):
        return dbus_message_get_error_name(self.msg)

    def set_destination(self, destination):
        return dbus_message_set_destination(self.msg, destination)

    def get_destination(self):
        return dbus_message_get_destination(self.msg)

    def set_sender(self, sender):
        return dbus_message_set_sender(self.msg, sender)
    
    def get_sender(self):
        cdef char *sender
        sender = dbus_message_get_sender(self.msg)
        if (sender == NULL):
            return None
        else:
            return sender

    def set_no_reply(self, no_reply):
        dbus_message_set_no_reply(self.msg, no_reply)

    def get_no_reply(self):
        return dbus_message_get_no_reply(self.msg)

    def is_method_call(self, interface, method):
        return dbus_message_is_method_call(self.msg, interface, method)

    def is_signal(self, interface, signal_name):
        return dbus_message_is_signal(self.msg, interface, signal_name)

    def is_error(self, error_name):
        return dbus_message_is_error(self.msg, error_name)

    def has_destination(self, service):
        return dbus_message_has_destination(self.msg, service)

    def has_sender(self, service):
        return dbus_message_has_sender(self.msg, service)

    def get_serial(self):
        return dbus_message_get_serial(self.msg)

    def set_reply_serial(self, reply_serial):
        return dbus_message_set_reply_serial(self.msg, reply_serial)

    def get_reply_serial(self):
        return dbus_message_get_reply_serial(self.msg)    

    #FIXME: dbus_message_get_path_decomposed
    
    # FIXME: all the different dbus_message_*args* methods

class Signal(Message):
    def __init__(self, spath, sinterface, sname):
        Message.__init__(self, MESSAGE_TYPE_SIGNAL, path=spath, dbus_interface=sinterface, name=sname)

class EmptyMessage(Message):
    def __init__(self):
        Message.__init__(self, _create=False)

class MethodCall(Message):
    def __init__(self, mpath, minterface, mmethod):
        Message.__init__(self, MESSAGE_TYPE_METHOD_CALL, path=mpath, dbus_interface=minterface, method=mmethod)

class MethodReturn(Message):
    def __init__(self, method_call):
        Message.__init__(self, MESSAGE_TYPE_METHOD_RETURN, method_call=method_call)

class Error(Message):
    def __init__(self, reply_to, error_name, error_message):
        Message.__init__(self, MESSAGE_TYPE_ERROR, reply_to=reply_to, error_name=error_name, error_message=error_message)
        
cdef class Server:
    cdef DBusServer *server
    def __init__(self, address):
        cdef DBusError error
        dbus_error_init(&error)
        self.server = dbus_server_listen(address,
                                         &error)
        if dbus_error_is_set(&error):
            errormsg = error.message
            dbus_error_free (&error)
            raise DBusException, errormsg

    def disconnect(self):
        dbus_server_disconnect(self.server)

    def get_is_connected(self):
        return dbus_server_get_is_connected(self.server)

#    def set_new_connection_function(self, function, data):
#        dbus_server_set_new_connection_function(self.conn, function,
#                                                data, NULL)
        
#    def set_watch_functions(self, add_function, remove_function, data):
#        dbus_server_set_watch_functions(self.server,
#                                        add_function, remove_function,
#                                        data, NULL)
        
#    def set_timeout_functions(self, add_function, remove_function, data):
#        dbus_server_set_timeout_functions(self.server,
#                                          add_function, remove_function,
#                                          data, NULL)
        
#    def handle_watch(self, watch, condition):
#        dbus_server_handle_watch(self.conn, watch, condition)

BUS_SESSION = DBUS_BUS_SESSION
BUS_SYSTEM = DBUS_BUS_SYSTEM
BUS_STARTER = DBUS_BUS_STARTER

def bus_get (bus_type, private=False):
    cdef DBusError error
    cdef Connection conn
    cdef DBusConnection *connection

    dbus_error_init(&error)
    if private:
        connection = dbus_bus_get_private(bus_type,
                                          &error)
    else:
        connection = dbus_bus_get(bus_type,
                                  &error)

    if dbus_error_is_set(&error):
        errormsg = error.message
        dbus_error_free(&error)
        raise DBusException, errormsg 

    conn = Connection()
    conn.__cinit__(None, connection)
    return conn 

def bus_get_unique_name(Connection connection):
    cdef DBusConnection *conn
    conn = connection._get_conn()
    return dbus_bus_get_unique_name(conn)

def bus_get_unix_user(Connection connection, service_name):
    cdef DBusError error
    dbus_error_init(&error)
    cdef int retval
    cdef DBusConnection *conn

    conn = connection._get_conn()
    retval = dbus_bus_get_unix_user(conn, service_name, &error)

    if dbus_error_is_set(&error):
        errormsg = error.message
        dbus_error_free(&error)
        raise DBusException, errormsg 

    return retval

# these are defines, not enums, so they aren't auto generated
DBUS_START_REPLY_SUCCESS = 0
DBUS_START_REPLY_ALREADY_RUNNING = 1

def bus_start_service_by_name(Connection connection, service_name, flags=0):
    cdef DBusError error
    dbus_error_init(&error)
    cdef dbus_bool_t retval
    cdef dbus_uint32_t results
    cdef DBusConnection *conn

    conn = connection._get_conn()

    retval = dbus_bus_start_service_by_name(conn, service_name, flags, &results, &error)

    if dbus_error_is_set(&error):
        errormsg = error.message
        dbus_error_free(&error)
        raise DBusException, errormsg

    return (retval, results) 

def bus_register(Connection connection):
    cdef DBusError error
    dbus_error_init(&error)
    cdef dbus_bool_t retval
    cdef DBusConnection *conn

    conn = connection._get_conn()
    retval = dbus_bus_register(conn,
                               &error)
    if dbus_error_is_set(&error):
        msg = error.message
        dbus_error_free(&error)
        raise DBusException, errormsg 

    return retval

NAME_FLAG_ALLOW_REPLACEMENT    = 0x1
NAME_FLAG_REPLACE_EXISTING     = 0x2
NAME_FLAG_DO_NOT_QUEUE         = 0x4

REQUEST_NAME_REPLY_PRIMARY_OWNER = 1
REQUEST_NAME_REPLY_IN_QUEUE      = 2
REQUEST_NAME_REPLY_EXISTS        = 3
REQUEST_NAME_REPLY_ALREADY_OWNER = 4

def bus_request_name(Connection connection, service_name, flags=0):
    cdef DBusError error
    dbus_error_init(&error)
    cdef int retval
    cdef DBusConnection *conn

    conn = connection._get_conn()
    retval = dbus_bus_request_name(conn,
                                   service_name,
                                   flags,
                                   &error)
    if dbus_error_is_set(&error):
        errormsg = error.message
        dbus_error_free(&error)
        raise DBusException, errormsg

    return retval

RELEASE_NAME_REPLY_RELEASED = 1
RELEASE_NAME_REPLY_NON_EXISTENT = 2
RELEASE_NAME_REPLY_NOT_OWNER = 3

def bus_release_name(Connection connection, service_name):
    cdef DBusError error
    dbus_error_init(&error)
    cdef int retval
    cdef DBusConnection *conn

    conn = connection._get_conn()
    retval = dbus_bus_release_name(conn,
                                   service_name,
                                   &error)
    if dbus_error_is_set(&error):
        errormsg = error.message
        dbus_error_free(&error)
        raise DBusException, errormsg

    return retval

def bus_name_has_owner(Connection connection, service_name):
    cdef DBusError error
    dbus_error_init(&error)
    cdef dbus_bool_t retval
    cdef DBusConnection *conn

    conn = connection._get_conn()
    retval = dbus_bus_name_has_owner(conn,
                                     service_name,
                                     &error)
    if dbus_error_is_set(&error):
        errormsg = error.message
        dbus_error_free(&error)
        raise DBusException, errormsg
        
    return retval

def bus_add_match(Connection connection, rule):
    cdef DBusError error
    cdef DBusConnection *conn

    dbus_error_init(&error)
    
    conn = connection._get_conn()
    dbus_bus_add_match (conn, rule, &error)
    
    if dbus_error_is_set(&error):
        errormsg = error.message
        dbus_error_free(&error)
        raise DBusException, errormsg

def bus_remove_match(Connection connection, rule):
    cdef DBusError error
    cdef DBusConnection *conn

    dbus_error_init(&error)

    conn = connection._get_conn()
    dbus_bus_remove_match (conn, rule, &error)
    
    if dbus_error_is_set(&error):
        errormsg = error.message
        dbus_error_free(&error)
        raise DBusException, errormsg

