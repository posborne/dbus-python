cimport dbus_bindings
import dbus_bindings

cdef extern from "dbus-glib.h":
    ctypedef struct GMainContext
    cdef void dbus_g_thread_init ()

cdef extern from "dbus-glib-lowlevel.h":
    cdef void dbus_connection_setup_with_g_main (dbus_bindings.DBusConnection *connection,
                                                 GMainContext   *context)
def setup_with_g_main(conn):
   cdef dbus_bindings.Connection connection
   connection = conn
   dbus_connection_setup_with_g_main(connection._get_conn(), NULL)

def gthreads_init ():
    dbus_g_thread_init ()
