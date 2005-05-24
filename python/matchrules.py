from exceptions import *

class SignalMatchNode:
    def __init__(self):
        self.wildcard = None
        self.finite = {}
        self.rules = []
        
    def add(self, key, leaf=None):
        node = None
        
        if key:
            if self.finite.has_key(key):
                node = self.finite[key]
            else:
                node = SignalMatchNode()
                self.finite[key]  = node
        else:
            if self.wildcard:
                node = self.wildcard
            else:
                node = SignalMatchNode()
                self.wildcard = node
         
        node.rules.append(leaf)
        return node
    
    def get_matches(self, key):
        result = []
        if self.wildcard:
            result.append(self.wildcard)
        
        if self.finite.has_key(key):
            result.append(self.finite[key])
            
        return result
        
    def get_match(self, key):
        if key:
           if self.finite.has_key(key):
               return self.finite[key]
           else:
               return None
        
        return self.wildcard
        
    def has_children(self):
        if self.wildcard or len(self.finite.iterkeys()) > 0:
            return True
        return False

    def remove_child(self, child, key=None):
        if self.wildcard == child:
            self.wildcard = None
        elif self.finite.had_key(key):
            del self.finite[key]

class SignalMatchTree:
    """This class creates an ordered tree of SignalMatchRules
        to speed searchs.  Left branches are wildcard elements
        and all other branches are concreet elements.
    """
    def __init__(self):
        self._tree = SignalMatchNode()
    
    def add(self, rule):
        interface = self._tree.add(rule.sender)
        signal = interface.add(rule.dbus_interface)
        path = signal.add(rule.signal_name)
        path.add(rule.path, leaf=rule)
       
    def exec_matches(self, match_rule, *args):
        sender_matches = self._tree.get_matches(match_rule.sender)
        for sender_node in sender_matches:
            interface_matches = sender_node.get_matches(match_rule.dbus_interface)
            for interface_node in interface_matches:
                signal_matches = interface_node.get_matches(match_rule.signal_name)
                for signal_node in signal_matches:
                    path_matches = signal_node.get_matches(match_rule.path)
                    for path_node in path_matches:
                        if(path_node.rules):
                            for rule in path_node.rules:
                                rule.execute(*args)
            
    def remove(self, rule):
        try:
            sender = self._tree.get_match(rule.sender)
            interface = sender.get_match(rule.dbus_interface)
            signal = interface.get_match(rule.signal_name)
            path = signal.get_match(rule.path)
            
            rule_matches = []
            for _rule in path.rules:
                if _rule.is_match(rule):
                    rule_matches.append(_rule)
                    
            for _rule in rule_matches:
                path.rules.remove(_rule)
                
            #clean up tree
            if len(path.rules) == 0:
                signal.remove_child(path, key = rule.path)
                if not signal.has_children():
                    interface.remove_child(signal, key = rule.signal_name)
                    if not interface.has_children():
                        sender.remove_child(interface, key = rule.dbus_interface)
                        if not sender.has_children():
                            self._tree.remove_child(sender, key = rule.sender)
            
        except:
            raise DBusException ("Trying to remove unkown rule: %s"%str(rule))

class SignalMatchRule:
    """This class represents a dbus rule used to filter signals.
        When a rule matches a filter, the signal is propagated to the handler_funtions
    """
    def __init__(self, signal_name, dbus_interface, sender, path):
        self.handler_functions = []

        self.signal_name = signal_name
        self.dbus_interface = dbus_interface
        self.sender = sender
        self.path = path

    def execute(self, *args):
        for handler in self.handler_functions:
            handler(*args)

    def add_handler(self, handler):
        self.handler_functions.append(handler)
        
    def is_match(self, rule):
        if (self.signal_name == rule.signal_name and
            self.dbus_interface == rule.dbus_interface and
            self.sender == rule.sender and
            self.path == rule.path):
                _funcs_copy_a = self.dbus.handler_functions[0:]
                _funcs_copy_b = rule.dbus.handler_functions[0:]
                _funcs_copy_a.sort()
                _funcs_copy_b.sort()
                return a == b
 
        return False

    def __repr__(self):
        """Returns a custom representation of this DBusMatchRule that can
            be used with dbus_bindings
        """
        repr = "type='signal'"
        if (self.dbus_interface):
            repr = repr + ",interface='%s'" % (self.dbus_interface)
        else:
            repr = repr + ",interface='*'"

        if (self.sender):     
            repr = repr + ",sender='%s'" % (self.sender)
        else:
            repr = repr + ",sender='*'"
    
        if (self.path):
            repr = repr + ",path='%s'" % (self.path)
        else:
            repr = repr + ",path='*'" 
            
        if (self.signal_name):
            repr = repr + ",member='%s'" % (self.signal_name)
        else:
            repr = repr + ",member='*'"
    
        return repr
