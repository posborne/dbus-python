import dbus
import dbus_bindings

def ensure_same(expected, received):
    if type(received) != type(expected):
        raise Exception ("Sending %s, expected echo of type %s, but got %s" % (expected, type(expected), type(received)))

    if received.__class__ != expected.__class__:
        raise Exception ("Sending %s, expected echo to be of class %s, but got %s" % (expected, expected.__class__, received.__class__))

    if received != expected:
        raise Exception("Sending %s, expected echo to be the same, but was %s" % (expected, received))

def TestEcho(value):
    global remote_object
    echoed = remote_object.Echo(value)
    ensure_same(value, echoed)

def TestEchoList(sent_list):
    assert(type(sent_list) == list)

    global remote_object

    reply_list = remote_object.Echo(sent_list)

    if type(reply_list) != list:
        raise Exception ("Sending list %s, expected echo to be a list, but it was %s" % (sent_list, type(reply_list)))

    if len(reply_list) != len(sent_list):
        raise Exception ("Sending list %s, expected echo of length %d, but length was %d" % (len(sent_list), len(reply_list)))

    for i in range(len(sent_list)):
        ensure_same(sent_list[i], reply_list[i])

def TestEchoDict(sent_dict):
    assert(type(sent_dict) == dict)

    global remote_object
    
    reply_dict = remote_object.Echo(sent_dict)


    assert(type(reply_dict) == dict)

    assert(len(reply_dict) == len(sent_dict))

    for key in sent_dict.keys():
        ensure_same(reply_dict[key], sent_dict[key])
    
session_bus = dbus.SessionBus()

remote_service = session_bus.get_service("org.designfu.Test")
remote_object = remote_service.get_object("/TestObject", "org.designfu.Test")

TestEcho(chr(120))
TestEcho(10)
TestEcho(39.5)
TestEcho("HelloWorld")
TestEcho(dbus_bindings.ObjectPath("/test/path"))


string_list = []
for i in range(200):
    string_list.append("List item " + str(i))
TestEchoList(string_list)

int_list = range(200)
TestEchoList(int_list)

path_list = []
for i in range(200):
    path_list.append(dbus_bindings.ObjectPath("/some/object/path" + str(i)))
TestEchoList(path_list)

double_list = []
for i in range(200):
    double_list.append(float(i) / 1000)
TestEchoList(double_list)

#FIXME: this currently fails!
#empty_list = []
#TestEchoList(empty_list)

string_to_int_dict = {}
for i in range(200):
    string_to_int_dict["key" + str(i)] = i
TestEchoDict(string_to_int_dict)

string_to_double_dict = {}
for i in range(200):
    string_to_double_dict["key" + str(i)] = float(i) / 1000
TestEchoDict(string_to_double_dict)

string_to_string_dict = {}
for i in range(200):
    string_to_string_dict["key" + str(i)] = "value" + str(i)
TestEchoDict(string_to_string_dict)

#FIXME: this currently crashes dbus in c code
#empty_dict = {}
#TestEchoDict(empty_dict)
