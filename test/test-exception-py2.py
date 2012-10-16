#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys
import unittest

import dbus

# from test-service.py
class ServiceError(dbus.DBusException):
    """Exception representing a normal "environmental" error"""
    include_traceback = False
    _dbus_error_name = 'com.example.Networking.ServerError'


class DBusExceptionTestCase(unittest.TestCase):
    """Test the DBusException str/unicode behavior with py2"""

    def test_dbus_exception_normal(self):
        """Test the normal Exception behavior"""
        e = dbus.exceptions.DBusException("baaa")
        msg = e.get_dbus_message()
        self.assertEqual(msg, u"baaa")

    def test_dbus_exception_unicode(self):
        """Test that DBusExceptions that take a py2 unicode work"""
        e = dbus.exceptions.DBusException(u"bäää")
        msg = e.get_dbus_message()
        self.assertEqual(msg, u"bäää")

    def test_dbus_exception_convert_str(self):
        """Test that converting a DBusException to str() works as expected"""
        e = dbus.exceptions.DBusException(u"bxxx")
        self.assertEqual(str(e), "bxxx")

    def test_dbus_exception_convert_str_fail(self):
        """Test that a non-ascii Exception fails to convert to str"""
        if sys.getdefaultencoding() == 'ascii':
            self.assertRaises(UnicodeEncodeError,
                    lambda: str(dbus.exceptions.DBusException(u"bä")))
        else:
            self.skipTest("you're using a weird non-ascii "
                    "sys.getdefaultencoding()")

    def test_dbus_exception_convert_unicode(self):
        """Test that converting a DBusEception to unicode works"""
        e = dbus.exceptions.DBusException(u"bäää")
        self.assertEqual(e.get_dbus_message(), u"bäää")
        self.assertEqual(e.__unicode__(), u"bäää")
        self.assertEqual(unicode(e), u"bäää")

    def test_subclass_exception_unicode(self):
        """Test that DBusExceptions that take a py2 unicode work"""
        e = ServiceError(u"bäää")
        msg = e.get_dbus_message()
        self.assertEqual(msg, u"bäää")
        self.assertEqual(
            unicode(e), u"com.example.Networking.ServerError: bäää")


if __name__ == "__main__":
    unittest.main()
