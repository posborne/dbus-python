class MissingErrorHandlerException(Exception):
    def __init__(self):
        Exception.__init__(self)


    def __str__(self):
        return "error_handler not defined: if you define a reply_handler you must also define an error_handler"


class MissingReplyHandlerException(Exception):
    def __init__(self):
        Exception.__init__(self)

    def __str__(self):
        return "reply_handler not defined: if you define an error_handler you must also define a reply_handler"

class ValidationException(Exception):
    def __init__(self, msg=''):
        self.msg = msg
        Exception.__init__(self)

    def __str__(self):
        return "Error validating string: %s" % self.msg

class UnknownMethodException(Exception):
    def __init__(self, msg=''):
        self.msg = msg
        Exception.__init__(self)

    def __str__(self):
        return "Unknown method: %s" % self.msg

