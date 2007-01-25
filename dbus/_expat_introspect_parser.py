# Copyright (C) 2003, 2004, 2005, 2006 Red Hat Inc. <http://www.redhat.com/>
# Copyright (C) 2003 David Zeuthen
# Copyright (C) 2004 Rob Taylor
# Copyright (C) 2005, 2006 Collabora Ltd. <http://www.collabora.co.uk/>
#
# Licensed under the Academic Free License version 2.1
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

from xml.parsers.expat import ExpatError, ParserCreate
from dbus.exceptions import IntrospectionParserException

class _Parser(object):
    __slots__ = ('map', 'in_iface', 'in_method', 'sig')
    def __init__(self):
        self.map = {}
        self.in_iface = ''
        self.in_method = ''
        self.sig = ''

    def parse(self, data):
        parser = ParserCreate('UTF-8', ' ')
        parser.buffer_text = True
        parser.StartElementHandler = self.StartElementHandler
        parser.EndElementHandler = self.EndElementHandler
        parser.Parse(data)
        return self.map

    def StartElementHandler(self, name, attributes):
        if not self.in_iface:
            if (not self.in_method and name == 'interface'):
                self.in_iface = attributes['name']
        else:
            if (not self.in_method and name == 'method'):
                self.in_method = attributes['name']
            elif (self.in_method and name == 'arg'):
                if attributes.get('direction', 'in') == 'in':
                    self.sig += attributes['type']

    def EndElementHandler(self, name):
        if self.in_iface:
            if (not self.in_method and name == 'interface'):
                self.in_iface = ''
            elif (self.in_method and name == 'method'):
                self.map[self.in_iface + '.' + self.in_method] = self.sig
                self.in_method = ''
                self.sig = ''

def process_introspection_data(data):
    """Return a dict mapping ``interface.method`` strings to the
    concatenation of all their 'in' parameters, and mapping
    ``interface.signal`` strings to the concatenation of all their
    parameters.

    Example output::

        {
            'com.example.SignalEmitter.OneString': 's',
            'com.example.MethodImplementor.OneInt32Argument': 'i',
        }

    :Parameters:
        `data` : str
            The introspection XML. Must be an 8-bit string of UTF-8.
    """
    try:
        return _Parser().parse(data)
    except Exception, e:
        raise IntrospectionParserException('%s: %s' % (e.__class__, e))
