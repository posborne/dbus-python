import dbus
import dbus_glib_bindings

def _setup_with_g_main(conn):
    dbus_glib_bindings.setup_with_g_main(conn._connection)

_dbus_gthreads_initialized = False
def init_threads():
    global _dbus_gthreads_initialized
    if not _dbus_gthreads_initialized:
        dbus_glib_bindings.init_gthreads ()
        _dbus_gthreads_initialized = True


setattr(dbus, "_dbus_mainloop_setup_function", _setup_with_g_main)
