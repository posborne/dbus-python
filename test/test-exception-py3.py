#!/usr/bin/python
# -*- coding: utf-8 -*-

import unittest

import dbus

# from test-service.py
class ServiceError(dbus.DBusException):
    """Exception representing a normal "environmental" error"""
    include_traceback = False
    _dbus_error_name = 'com.example.Networking.ServerError'


class DBusExceptionTestCase(unittest.TestCase):

    def test_dbus_exception(self):
        e = dbus.exceptions.DBusException("bäää")
        msg = e.get_dbus_message()
        self.assertEqual(msg, "bäää")
        self.assertEqual(str(e), "bäää")

    def test_subclass_exception(self):
        e = ServiceError("bäää")
        msg = e.get_dbus_message()
        self.assertEqual(msg, "bäää")
        self.assertEqual(str(e), "com.example.Networking.ServerError: bäää")


if __name__ == "__main__":
    unittest.main()
