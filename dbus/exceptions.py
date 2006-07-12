import dbus_bindings

DBusException = dbus_bindings.DBusException
ConnectionError = dbus_bindings.ConnectionError

class MissingErrorHandlerException(DBusException):
    def __init__(self):
        DBusException.__init__(self, "error_handler not defined: if you define a reply_handler you must also define an error_handler")

class MissingReplyHandlerException(DBusException):
    def __init__(self):
        DBusException.__init__(self, "reply_handler not defined: if you define an error_handler you must also define a reply_handler")

class ValidationException(DBusException):
    def __init__(self, msg=''):
        DBusException.__init__(self, "Error validating string: %s"%msg)

class IntrospectionParserException(DBusException):
    def __init__(self, msg=''):
        DBusException.__init__(self, "Error parsing introspect data: %s"%msg)

class UnknownMethodException(DBusException):
    def __init__(self, method):
        DBusException.__init__(self, "Unknown method: %s"%method)

class NameExistsException(DBusException):
    def __init__(self, name):
        DBusException.__init__(self, "Bus name already exists: %s"%name)

