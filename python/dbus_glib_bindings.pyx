cdef extern from "dbus.h":
    ctypedef struct DBusConnection

cdef extern from "dbus-glib.h":
    ctypedef struct GMainContext
    cdef void dbus_connection_setup_with_g_main (DBusConnection *connection,
                                                 GMainContext   *context)
    cdef void dbus_g_thread_init ()

cimport dbus_bindings
import dbus_bindings

def setup_with_g_main(conn):
   cdef dbus_bindings.Connection connection
   connection = conn
   dbus_connection_setup_with_g_main(connection._get_conn(), NULL)

def init_gthreads ():
    dbus_g_thread_init ()
