#!/usr/bin/env python
import sys
import os
import unittest
import time

builddir = os.environ["DBUS_TOP_BUILDDIR"]
pydir = builddir

sys.path.insert(0, pydir)
sys.path.insert(0, pydir + 'dbus')

import dbus
import dbus.dbus_bindings 
import gobject
import dbus.glib
import dbus.service

pkg = dbus.__file__
if not pkg.startswith(pydir):
    raise Exception("DBus modules (%s) are not being picked up from the package"%pkg)

if not dbus.dbus_bindings.__file__.startswith(pydir):
    raise Exception("DBus modules (%s) are not being picked up from the package"%dbus.dbus_bindings.__file__)

test_types_vals = [1, 12323231, 3.14159265, 99999999.99,
                 "dude", "123", "What is all the fuss about?", "gob@gob.com",
                 u'\\u310c\\u310e\\u3114', u'\\u0413\\u0414\\u0415',
                 u'\\u2200software \\u2203crack', u'\\xf4\\xe5\\xe8',
                 [1,2,3], ["how", "are", "you"], [1.23,2.3], [1], ["Hello"],
                 (1,2,3), (1,), (1,"2",3), ("2", "what"), ("you", 1.2),
                 {1:"a", 2:"b"}, {"a":1, "b":2}, #{"a":(1,"B")},
                 {1:1.1, 2:2.2}, [[1,2,3],[2,3,4]], [["a","b"],["c","d"]],
                 True, False,
                 dbus.Int16(-10), dbus.UInt16(10),
                 #([1,2,3],"c", 1.2, ["a","b","c"], {"a": (1,"v"), "b": (2,"d")})
                 ]

class TestDBusBindings(unittest.TestCase):
    def setUp(self):
        self.bus = dbus.SessionBus()
        self.remote_object = self.bus.get_object("org.freedesktop.DBus.TestSuitePythonService", "/org/freedesktop/DBus/TestSuitePythonObject")
        self.iface = dbus.Interface(self.remote_object, "org.freedesktop.DBus.TestSuiteInterface")

    def testInterfaceKeyword(self):
        #test dbus_interface parameter
        print self.remote_object.Echo("dbus_interface on Proxy test Passed", dbus_interface = "org.freedesktop.DBus.TestSuiteInterface")
        print self.iface.Echo("dbus_interface on Interface test Passed", dbus_interface = "org.freedesktop.DBus.TestSuiteInterface")
        self.assert_(True)
        
    def testIntrospection(self):
        #test introspection
        print "\n********* Introspection Test ************"
        print self.remote_object.Introspect(dbus_interface="org.freedesktop.DBus.Introspectable")
        print "Introspection test passed"
        self.assert_(True)

    def testPythonTypes(self):
        #test sending python types and getting them back
        print "\n********* Testing Python Types ***********"
                 
        for send_val in test_types_vals:
            print "Testing %s"% str(send_val)
            recv_val = self.iface.Echo(send_val)
            self.assertEquals(send_val, recv_val)

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

        
        main_loop = gobject.MainLoop()
        class async_check:
            def __init__(self, test_controler, expected_result, do_exit):
                self.expected_result = expected_result
                self.do_exit = do_exit
                self.test_controler = test_controler

            def callback(self, val):
                try:
                    if self.do_exit:
                        main_loop.quit()

                    self.test_controler.assertEquals(val, self.expected_result)
                except Exception, e:
                    print "%s:\n%s" % (e.__class__, e)

            def error_handler(self, error):
                print error
                if self.do_exit:
                    main_loop.quit()

                self.test_controler.assert_(val, False)
        
        last_type = test_types_vals[-1]
        for send_val in test_types_vals:
            print "Testing %s"% str(send_val)
            check = async_check(self, send_val, last_type == send_val) 
            recv_val = self.iface.Echo(send_val, 
                                       reply_handler = check.callback,
                                       error_handler = check.error_handler)
            
        main_loop.run()

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
                    (self.iface.ReturnTwoStrings, 'SignalTwoStrings', set([1, 5]), set([5])),
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
                    print "%s(%r) raised %s" % (method._method_name, values[value], e.__class__)

                    # should fail if it tried to marshal the wrong type
                    self.assert_(value not in success_values, "%s should succeed when we ask it to return %r\n%s\n%s" % (method._method_name, values[value], e.__class__, e))
                else:
                    print "%s(%r) returned %r" % (method._method_name, values[value], ret)

                    # should only succeed if it's the right return type
                    self.assert_(value in success_values, "%s should fail when we ask it to return %r" % (method._method_name, values[value]))

                    # check the value is right too :D
                    returns = map(lambda n: values[n], return_values)
                    self.assert_(ret in returns, "%s should return one of %r" % (method._method_name, returns))

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
        for (async, fail) in ((False, False), (False, True), (True, False), (True, True)):
            try:
                val = ('a', 1, False, [1,2], {1:2})
                print "calling AsynchronousMethod with %s %s %s" % (async, fail, val)
                ret = self.iface.AsynchronousMethod(async, fail, val)
            except Exception, e:
                print "%s:\n%s" % (e.__class__, e)
                self.assert_(fail)
            else:
                self.assert_(not fail)
                print val, ret
                self.assert_(val == ret)

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
                busname = dbus.service.BusName(name)
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
        ret = dbus.dbus_bindings.bus_name_has_owner(bus._connection, 'org.freedesktop.DBus.Python.TestName')
        self.assert_(not ret, 'deleting reference failed to release BusName org.freedesktop.DBus.Python.TestName')

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
            self.assertEquals(send_val, recv_val)
"""
if __name__ == '__main__':
    gobject.threads_init()
    dbus.glib.init_threads()

    unittest.main()
