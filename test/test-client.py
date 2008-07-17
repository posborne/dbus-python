#!/usr/bin/env python

# Copyright (C) 2004 Red Hat Inc. <http://www.redhat.com/>
# Copyright (C) 2005-2007 Collabora Ltd. <http://www.collabora.co.uk/>
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use, copy,
# modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

import sys
import os
import unittest
import time
import logging
import weakref

builddir = os.path.normpath(os.environ["DBUS_TOP_BUILDDIR"])
pydir = os.path.normpath(os.environ["DBUS_TOP_SRCDIR"])

import dbus
import _dbus_bindings
import gobject
import dbus.glib
import dbus.service


logging.basicConfig()


pkg = dbus.__file__
if not pkg.startswith(pydir):
    raise Exception("DBus modules (%s) are not being picked up from the package"%pkg)

if not _dbus_bindings.__file__.startswith(builddir):
    raise Exception("DBus modules (%s) are not being picked up from the package"%_dbus_bindings.__file__)

test_types_vals = [1, 12323231, 3.14159265, 99999999.99,
                 "dude", "123", "What is all the fuss about?", "gob@gob.com",
                 u'\\u310c\\u310e\\u3114', u'\\u0413\\u0414\\u0415',
                 u'\\u2200software \\u2203crack', u'\\xf4\\xe5\\xe8',
                 [1,2,3], ["how", "are", "you"], [1.23,2.3], [1], ["Hello"],
                 (1,2,3), (1,), (1,"2",3), ("2", "what"), ("you", 1.2),
                 {1:"a", 2:"b"}, {"a":1, "b":2}, #{"a":(1,"B")},
                 {1:1.1, 2:2.2}, [[1,2,3],[2,3,4]], [["a","b"],["c","d"]],
                 True, False,
                 dbus.Int16(-10), dbus.UInt16(10), 'SENTINEL',
                 #([1,2,3],"c", 1.2, ["a","b","c"], {"a": (1,"v"), "b": (2,"d")})
                 ]

NAME = "org.freedesktop.DBus.TestSuitePythonService"
IFACE = "org.freedesktop.DBus.TestSuiteInterface"
OBJECT = "/org/freedesktop/DBus/TestSuitePythonObject"

class TestDBusBindings(unittest.TestCase):
    def setUp(self):
        self.bus = dbus.SessionBus()
        self.remote_object = self.bus.get_object(NAME, OBJECT)
        self.remote_object_follow = self.bus.get_object(NAME, OBJECT,
                follow_name_owner_changes=True)
        self.iface = dbus.Interface(self.remote_object, IFACE)

    def testGObject(self):
        print "Testing ExportedGObject... ",
        remote_gobject = self.bus.get_object(NAME, OBJECT + '/GObject')
        iface = dbus.Interface(remote_gobject, IFACE)
        print "introspection, ",
        remote_gobject.Introspect(dbus_interface=dbus.INTROSPECTABLE_IFACE)
        print "method call, ",
        self.assertEquals(iface.Echo('123'), '123')
        print "... OK"

    def testWeakRefs(self):
        # regression test for Sugar crash caused by smcv getting weak refs
        # wrong - pre-bugfix, this would segfault
        bus = dbus.SessionBus(private=True)
        ref = weakref.ref(bus)
        self.assert_(ref() is bus)
        del bus
        self.assert_(ref() is None)

    def testInterfaceKeyword(self):
        #test dbus_interface parameter
        print self.remote_object.Echo("dbus_interface on Proxy test Passed", dbus_interface = IFACE)
        print self.iface.Echo("dbus_interface on Interface test Passed", dbus_interface = IFACE)
        self.assert_(True)

    def testGetDBusMethod(self):
        self.assertEquals(self.iface.get_dbus_method('AcceptListOfByte')('\1\2\3'), [1,2,3])
        self.assertEquals(self.remote_object.get_dbus_method('AcceptListOfByte', dbus_interface=IFACE)('\1\2\3'), [1,2,3])

    def testCallingConventionOptions(self):
        self.assertEquals(self.iface.AcceptListOfByte('\1\2\3'), [1,2,3])
        self.assertEquals(self.iface.AcceptListOfByte('\1\2\3', byte_arrays=True), '\1\2\3')
        self.assertEquals(self.iface.AcceptByteArray('\1\2\3'), [1,2,3])
        self.assertEquals(self.iface.AcceptByteArray('\1\2\3', byte_arrays=True), '\1\2\3')
        self.assert_(isinstance(self.iface.AcceptUTF8String('abc'), unicode))
        self.assert_(isinstance(self.iface.AcceptUTF8String('abc', utf8_strings=True), str))
        self.assert_(isinstance(self.iface.AcceptUnicodeString('abc'), unicode))
        self.assert_(isinstance(self.iface.AcceptUnicodeString('abc', utf8_strings=True), str))

    def testIntrospection(self):
        #test introspection
        print "\n********* Introspection Test ************"
        print self.remote_object.Introspect(dbus_interface="org.freedesktop.DBus.Introspectable")
        print "Introspection test passed"
        self.assert_(True)

    def testMultiPathIntrospection(self):
        # test introspection on an object exported in multiple places
        # https://bugs.freedesktop.org/show_bug.cgi?id=11794
        remote_object = self.bus.get_object(NAME, OBJECT + '/Multi1')
        remote_object.Introspect(dbus_interface="org.freedesktop.DBus.Introspectable")
        remote_object = self.bus.get_object(NAME, OBJECT + '/Multi2')
        remote_object.Introspect(dbus_interface="org.freedesktop.DBus.Introspectable")
        remote_object = self.bus.get_object(NAME, OBJECT + '/Multi2/3')
        remote_object.Introspect(dbus_interface="org.freedesktop.DBus.Introspectable")
        self.assert_(True)

    def testPythonTypes(self):
        #test sending python types and getting them back
        print "\n********* Testing Python Types ***********"
                 
        for send_val in test_types_vals:
            print "Testing %s"% str(send_val)
            recv_val = self.iface.Echo(send_val)
            self.assertEquals(send_val, recv_val)
            self.assertEquals(recv_val.variant_level, 1)

    def testMethodExtraInfoKeywords(self):
        print "Testing MethodExtraInfoKeywords..."
        sender, path, destination, message_cls = self.iface.MethodExtraInfoKeywords()
        self.assert_(sender.startswith(':'))
        self.assertEquals(path, '/org/freedesktop/DBus/TestSuitePythonObject')
        # we're using the "early binding" form of get_object (without
        # follow_name_owner_changes), so the destination we actually sent it
        # to will be the unique name
        self.assert_(destination.startswith(':'))
        self.assertEquals(message_cls, 'dbus.lowlevel.MethodCallMessage')

    def testUtf8StringsSync(self):
        send_val = u'foo'
        recv_val = self.iface.Echo(send_val, utf8_strings=True)
        self.assert_(isinstance(recv_val, str))
        self.assert_(isinstance(recv_val, dbus.UTF8String))
        recv_val = self.iface.Echo(send_val, utf8_strings=False)
        self.assert_(isinstance(recv_val, unicode))
        self.assert_(isinstance(recv_val, dbus.String))

    def testBenchmarkIntrospect(self):
        print "\n********* Benchmark Introspect ************"
        a = time.time()
        print a
        print self.iface.GetComplexArray()
        b = time.time()
        print b
        print "Delta: %f" % (b - a)
        self.assert_(True)

    def testAsyncCalls(self):
        #test sending python types and getting them back async
        print "\n********* Testing Async Calls ***********"

        failures = []
        main_loop = gobject.MainLoop()

        class async_check:
            def __init__(self, test_controler, expected_result, do_exit, utf8):
                self.expected_result = expected_result
                self.do_exit = do_exit
                self.utf8 = utf8
                self.test_controler = test_controler

            def callback(self, val):
                try:
                    if self.do_exit:
                        main_loop.quit()

                    self.test_controler.assertEquals(val, self.expected_result)
                    self.test_controler.assertEquals(val.variant_level, 1)
                    if self.utf8 and not isinstance(val, dbus.UTF8String):
                        failures.append('%r should have been utf8 but was not' % val)
                        return
                    elif not self.utf8 and isinstance(val, dbus.UTF8String):
                        failures.append('%r should not have been utf8' % val)
                        return
                except Exception, e:
                    failures.append("%s:\n%s" % (e.__class__, e))

            def error_handler(self, error):
                print error
                if self.do_exit:
                    main_loop.quit()

                failures.append('%s: %s' % (error.__class__, error))
        
        last_type = test_types_vals[-1]
        for send_val in test_types_vals:
            print "Testing %s" % str(send_val)
            utf8 = (send_val == 'gob@gob.com')
            check = async_check(self, send_val, last_type == send_val,
                                utf8)
            recv_val = self.iface.Echo(send_val,
                                       reply_handler=check.callback,
                                       error_handler=check.error_handler,
                                       utf8_strings=utf8)
        main_loop.run()
        if failures:
            self.assert_(False, failures)

    def testStrictMarshalling(self):
        print "\n********* Testing strict return & signal marshalling ***********"

        # these values are the same as in the server, and the
        # methods should only succeed when they are called with
        # the right value number, because they have out_signature
        # decorations, and return an unmatching type when called
        # with a different number
        values = ["", ("",""), ("","",""), [], {}, ["",""], ["","",""]]
        methods = [
                    (self.iface.ReturnOneString, 'SignalOneString', set([0]), set([0])),
                    (self.iface.ReturnTwoStrings, 'SignalTwoStrings', set([1, 5]), set([1])),
                    (self.iface.ReturnStruct, 'SignalStruct', set([1, 5]), set([1])),
                    # all of our test values are sequences so will marshall correctly into an array :P
                    (self.iface.ReturnArray, 'SignalArray', set(range(len(values))), set([3, 5, 6])),
                    (self.iface.ReturnDict, 'SignalDict', set([0, 3, 4]), set([4]))
                ]

        for (method, signal, success_values, return_values) in methods:
            print "\nTrying correct behaviour of", method._method_name
            for value in range(len(values)):
                try:
                    ret = method(value)
                except Exception, e:
                    print "%s(%r) raised %s: %s" % (method._method_name, values[value], e.__class__, e)

                    # should fail if it tried to marshal the wrong type
                    self.assert_(value not in success_values, "%s should succeed when we ask it to return %r\n%s\n%s" % (method._method_name, values[value], e.__class__, e))
                else:
                    print "%s(%r) returned %r" % (method._method_name, values[value], ret)

                    # should only succeed if it's the right return type
                    self.assert_(value in success_values, "%s should fail when we ask it to return %r" % (method._method_name, values[value]))

                    # check the value is right too :D
                    returns = map(lambda n: values[n], return_values)
                    self.assert_(ret in returns, "%s should return one of %r but it returned %r instead" % (method._method_name, returns, ret))

            print "\nTrying correct emission of", signal
            for value in range(len(values)):
                try:
                    self.iface.EmitSignal(signal, value)
                except Exception, e:
                    print "EmitSignal(%s, %r) raised %s" % (signal, values[value], e.__class__)

                    # should fail if it tried to marshal the wrong type
                    self.assert_(value not in success_values, "EmitSignal(%s) should succeed when we ask it to return %r\n%s\n%s" % (signal, values[value], e.__class__, e))
                else:
                    print "EmitSignal(%s, %r) appeared to succeed" % (signal, values[value])

                    # should only succeed if it's the right return type
                    self.assert_(value in success_values, "EmitSignal(%s) should fail when we ask it to return %r" % (signal, values[value]))

                    # FIXME: wait for the signal here

        print

    def testInheritance(self):
        print "\n********* Testing inheritance from dbus.method.Interface ***********"
        ret = self.iface.CheckInheritance()
        print "CheckInheritance returned %s" % ret
        self.assert_(ret, "overriding CheckInheritance from TestInterface failed")

    def testAsyncMethods(self):
        print "\n********* Testing asynchronous method implementation *******"
        for async in (True, False):
            for fail in (True, False):
                try:
                    val = ('a', 1, False, [1,2], {1:2})
                    print "calling AsynchronousMethod with %s %s %s" % (async, fail, val)
                    ret = self.iface.AsynchronousMethod(async, fail, val)
                except Exception, e:
                    self.assert_(fail, '%s: %s' % (e.__class__, e))
                    print "Expected failure: %s: %s" % (e.__class__, e)
                else:
                    self.assert_(not fail, 'Expected failure but succeeded?!')
                    self.assertEquals(val, ret)
                    self.assertEquals(1, ret.variant_level)

    def testBusInstanceCaching(self):
        print "\n********* Testing dbus.Bus instance sharing *********"

        # unfortunately we can't test the system bus here
        # but the codepaths are the same
        for (cls, type, func) in ((dbus.SessionBus, dbus.Bus.TYPE_SESSION, dbus.Bus.get_session), (dbus.StarterBus, dbus.Bus.TYPE_STARTER, dbus.Bus.get_starter)):
            print "\nTesting %s:" % cls.__name__

            share_cls = cls()
            share_type = dbus.Bus(bus_type=type)
            share_func = func()

            private_cls = cls(private=True)
            private_type = dbus.Bus(bus_type=type, private=True)
            private_func = func(private=True)

            print " - checking shared instances are the same..."
            self.assert_(share_cls == share_type, '%s should equal %s' % (share_cls, share_type))
            self.assert_(share_type == share_func, '%s should equal %s' % (share_type, share_func))

            print " - checking private instances are distinct from the shared instance..."
            self.assert_(share_cls != private_cls, '%s should not equal %s' % (share_cls, private_cls))
            self.assert_(share_type != private_type, '%s should not equal %s' % (share_type, private_type))
            self.assert_(share_func != private_func, '%s should not equal %s' % (share_func, private_func))

            print " - checking private instances are distinct from each other..."
            self.assert_(private_cls != private_type, '%s should not equal %s' % (private_cls, private_type))
            self.assert_(private_type != private_func, '%s should not equal %s' % (private_type, private_func))
            self.assert_(private_func != private_cls, '%s should not equal %s' % (private_func, private_cls))

    def testSenderName(self):
        print '\n******** Testing sender name keyword ********'
        myself = self.iface.WhoAmI()
        print "I am", myself

    def testBusGetNameOwner(self):
        ret = self.bus.get_name_owner(NAME)
        self.assert_(ret.startswith(':'), ret)

    def testBusListNames(self):
        ret = self.bus.list_names()
        self.assert_(NAME in ret, ret)

    def testBusListActivatableNames(self):
        ret = self.bus.list_activatable_names()
        self.assert_(NAME in ret, ret)

    def testBusNameHasOwner(self):
        self.assert_(self.bus.name_has_owner(NAME))
        self.assert_(not self.bus.name_has_owner('badger.mushroom.snake'))

    def testBusNameCreation(self):
        print '\n******** Testing BusName creation ********'
        test = [('org.freedesktop.DBus.Python.TestName', True),
                ('org.freedesktop.DBus.Python.TestName', True),
                ('org.freedesktop.DBus.Python.InvalidName&^*%$', False)]
        # Do some more intelligent handling/testing of queueing vs success?
        # ('org.freedesktop.DBus.TestSuitePythonService', False)]
        # For some reason this actually succeeds
        # ('org.freedesktop.DBus', False)]

        # make a method call to ensure the test service is active
        self.iface.Echo("foo")

        names = {}
        for (name, succeed) in test:
            try:
                print "requesting %s" % name
                busname = dbus.service.BusName(name, dbus.SessionBus())
            except Exception, e:
                print "%s:\n%s" % (e.__class__, e)
                self.assert_(not succeed, 'did not expect registering bus name %s to fail' % name)
            else:
                print busname
                self.assert_(succeed, 'expected registering bus name %s to fail'% name)
                if name in names:
                    self.assert_(names[name] == busname, 'got a new instance for same name %s' % name)
                    print "instance of %s re-used, good!" % name
                else:
                    names[name] = busname

                del busname

            print

        del names

        bus = dbus.Bus()
        ret = bus.name_has_owner('org.freedesktop.DBus.Python.TestName')
        self.assert_(not ret, 'deleting reference failed to release BusName org.freedesktop.DBus.Python.TestName')

    def testMultipleReturnWithoutSignature(self):
        # https://bugs.freedesktop.org/show_bug.cgi?id=10174
        ret = self.iface.MultipleReturnWithoutSignature()
        self.assert_(not isinstance(ret, dbus.Struct), repr(ret))
        self.assertEquals(ret, ('abc', 123))

    def testListExportedChildObjects(self):
        self.assert_(self.iface.TestListExportedChildObjects())

    def testRemoveFromConnection(self):
        # https://bugs.freedesktop.org/show_bug.cgi?id=10457
        self.assert_(not self.iface.HasRemovableObject())
        self.assert_(self.iface.AddRemovableObject())
        self.assert_(self.iface.HasRemovableObject())

        removable = self.bus.get_object(NAME, OBJECT + '/RemovableObject')
        iface = dbus.Interface(removable, IFACE)
        self.assert_(iface.IsThere())
        self.assert_(iface.RemoveSelf())

        self.assert_(not self.iface.HasRemovableObject())

        # and again...
        self.assert_(self.iface.AddRemovableObject())
        self.assert_(self.iface.HasRemovableObject())
        self.assert_(iface.IsThere())
        self.assert_(iface.RemoveSelf())
        self.assert_(not self.iface.HasRemovableObject())

    def testFallbackObjectTrivial(self):
        obj = self.bus.get_object(NAME, OBJECT + '/Fallback')
        iface = dbus.Interface(obj, IFACE)
        path, rel, unique_name = iface.TestPathAndConnKeywords()
        self.assertEquals(path, OBJECT + '/Fallback')
        self.assertEquals(rel, '/')
        self.assertEquals(unique_name, obj.bus_name)

    def testFallbackObjectNested(self):
        obj = self.bus.get_object(NAME, OBJECT + '/Fallback/Nested')
        iface = dbus.Interface(obj, IFACE)
        path, rel, unique_name = iface.TestPathAndConnKeywords()
        self.assertEquals(path, OBJECT + '/Fallback/Nested')
        self.assertEquals(rel, '/')
        self.assertEquals(unique_name, obj.bus_name)

        obj = self.bus.get_object(NAME, OBJECT + '/Fallback/Nested/Badger/Mushroom')
        iface = dbus.Interface(obj, IFACE)
        path, rel, unique_name = iface.TestPathAndConnKeywords()
        self.assertEquals(path, OBJECT + '/Fallback/Nested/Badger/Mushroom')
        self.assertEquals(rel, '/Badger/Mushroom')
        self.assertEquals(unique_name, obj.bus_name)

    def testFallbackObject(self):
        obj = self.bus.get_object(NAME, OBJECT + '/Fallback/Badger/Mushroom')
        iface = dbus.Interface(obj, IFACE)
        path, rel, unique_name = iface.TestPathAndConnKeywords()
        self.assertEquals(path, OBJECT + '/Fallback/Badger/Mushroom')
        self.assertEquals(rel, '/Badger/Mushroom')
        self.assertEquals(unique_name, obj.bus_name)

    def testTimeoutSync(self):
        self.assert_(self.iface.BlockFor500ms(timeout=1.0) is None)
        self.assertRaises(dbus.DBusException,
                          lambda: self.iface.BlockFor500ms(timeout=0.25))

    def testAsyncRaise(self):
        self.assertRaises(dbus.DBusException, self.iface.AsyncRaise)
        try:
            self.iface.AsyncRaise()
        except dbus.DBusException, e:
            self.assert_(e.get_dbus_name() ==
                         'org.freedesktop.bugzilla.bug12403',
                         e.get_dbus_name())
        else:
            self.assert_(False)

    def testClosePrivateBus(self):
        # fd.o #12096
        dbus.Bus(private=True).close()

    def testTimeoutAsyncClient(self):
        loop = gobject.MainLoop()
        passes = []
        fails = []
        def correctly_returned():
            passes.append('1000')
            if len(passes) + len(fails) >= 2:
                loop.quit()
        def correctly_failed(exc):
            passes.append('250')
            if len(passes) + len(fails) >= 2:
                loop.quit()
        def incorrectly_returned():
            fails.append('250')
            if len(passes) + len(fails) >= 2:
                loop.quit()
        def incorrectly_failed(exc):
            fails.append('1000')
            if len(passes) + len(fails) >= 2:
                loop.quit()
        self.iface.BlockFor500ms(timeout=1.0,
                                 reply_handler=correctly_returned,
                                 error_handler=incorrectly_failed)
        self.iface.BlockFor500ms(timeout=0.25,
                                 reply_handler=incorrectly_returned,
                                 error_handler=correctly_failed)
        loop.run()
        self.assertEquals(passes, ['250', '1000'])
        self.assertEquals(fails, [])

    def testTimeoutAsyncService(self):
        self.assert_(self.iface.AsyncWait500ms(timeout=1.0) is None)
        self.assertRaises(dbus.DBusException,
                          lambda: self.iface.AsyncWait500ms(timeout=0.25))

    def testExceptions(self):
        #self.assertRaises(dbus.DBusException,
        #                  lambda: self.iface.RaiseValueError)
        #self.assertRaises(dbus.DBusException,
        #                  lambda: self.iface.RaiseDBusExceptionNoTraceback)
        #self.assertRaises(dbus.DBusException,
        #                  lambda: self.iface.RaiseDBusExceptionWithTraceback)

        try:
            self.iface.RaiseValueError()
        except Exception, e:
            self.assert_(isinstance(e, dbus.DBusException), e.__class__)
            self.assert_('.ValueError: Traceback ' in str(e),
                         'Wanted a traceback but got:\n"""%s"""' % str(e))
        else:
            raise AssertionError('Wanted an exception')

        try:
            self.iface.RaiseDBusExceptionNoTraceback()
        except Exception, e:
            self.assert_(isinstance(e, dbus.DBusException), e.__class__)
            self.assertEquals(str(e),
                              'com.example.Networking.ServerError: '
                              'Server not responding')
        else:
            raise AssertionError('Wanted an exception')

        try:
            self.iface.RaiseDBusExceptionWithTraceback()
        except Exception, e:
            self.assert_(isinstance(e, dbus.DBusException), e.__class__)
            self.assert_(str(e).startswith('com.example.Misc.RealityFailure: '
                                           'Traceback '),
                         'Wanted a traceback but got:\n%s' % str(e))
        else:
            raise AssertionError('Wanted an exception')

""" Remove this for now
class TestDBusPythonToGLibBindings(unittest.TestCase):
    def setUp(self):
        self.bus = dbus.SessionBus()
        self.remote_object = self.bus.get_object("org.freedesktop.DBus.TestSuiteGLibService", "/org/freedesktop/DBus/Tests/MyTestObject")
        self.iface = dbus.Interface(self.remote_object, "org.freedesktop.DBus.Tests.MyObject")

    def testIntrospection(self):
        #test introspection
        print "\n********* Introspection Test ************"
        print self.remote_object.Introspect(dbus_interface="org.freedesktop.DBus.Introspectable")
        print "Introspection test passed"
        self.assert_(True)

    def testCalls(self):
        print "\n********* Call Test ************"
        result =  self.iface.ManyArgs(1000, 'Hello GLib', 2)
        print result
        self.assert_(result == [2002.0, 'HELLO GLIB'])

        arg0 = {"Dude": 1, "john": "palmieri", "python": 2.4}
        result = self.iface.ManyStringify(arg0)
        print result
       
        print "Call test passed"
        self.assert_(True)

    def testPythonTypes(self):
        print "\n********* Testing Python Types ***********"
                 
        for send_val in test_types_vals:
            print "Testing %s"% str(send_val)
            recv_val = self.iface.EchoVariant(send_val)
            self.assertEquals(send_val, recv_val.object)
"""
if __name__ == '__main__':
    gobject.threads_init()
    dbus.glib.init_threads()

    unittest.main()
