import unittest

from _dbus_bindings import Int32, Int64, ObjectPath
from _dbus_bindings import Message, SignalMessage

class TestArgs(unittest.TestCase):
    def setUp(self):
        pass

    def testMisc(self):
        self.assertEquals(Message.guess_signature('abc', 123), 'si')

        m = SignalMessage('/', 'com.example.Stuff', 'Done')
        self.assertEquals(m.get_signature(), '')
        m.extend('abc', 123, signature='si')
        self.assertEquals(m.get_args(), ('abc', Int32(123)))
        self.assertEquals(m.get_signature(), 'si')
        self.assert_(m.has_signature('si'))
        self.assert_(not m.has_signature('sx'))

        m = SignalMessage('/', 'com.example.Stuff', 'Done')
        m.extend('abc', 123)
        self.assertEquals(m.get_args(), ('abc', Int32(123)))
        self.assertEquals(m.get_signature(), 'si')
        m.extend(('/foo', 1), signature='(ox)')
        self.assertEquals(m.get_args(), (u'abc', Int32(123),
                                         (ObjectPath('/foo'), Int64(1L))))

if __name__ == '__main__':
    unittest.main()
