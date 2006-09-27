# Copyright (C) 2003, 2004 Seth Nickell
# Copyright (C) 2003 David Zeuthen
# Copyright (C) 2004 Rob Taylor
# Copyright (C) 2004, 2005, 2006 Red Hat Inc. <http://www.redhat.com/>
# Copyright (C) 2005, 2006 Collabora Ltd. <http://www.collabora.co.uk/>
#
# Licensed under the Academic Free License version 2.1
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
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

import libxml2
import cStringIO
import exceptions

def process_introspection_data(data):
    method_map = {}

    XMLREADER_START_ELEMENT_NODE_TYPE = 1
    XMLREADER_END_ELEMENT_NODE_TYPE = 15

    stream = cStringIO.StringIO(data.encode('utf-8'))
    input_source = libxml2.inputBuffer(stream)
    reader = input_source.newTextReader("urn:introspect")

    ret = reader.Read()
    current_iface = None
    current_method = None
    current_sigstr = ''

    while ret == 1:
        name = reader.LocalName()
        if reader.NodeType() == XMLREADER_START_ELEMENT_NODE_TYPE:
            if (not current_iface and not current_method and name == 'interface'):
                current_iface = reader.GetAttribute('name')
            elif (current_iface and not current_method and name == 'method'):
                current_method = reader.GetAttribute('name')
                if reader.IsEmptyElement():
                    method_map[current_iface + '.' + current_method] = ''
                    current_method = None
                    current_sigstr = ''

            elif (current_iface and current_method and name == 'arg'):
                direction = reader.GetAttribute('direction')

                if not direction or direction == 'in':
                    current_sigstr = current_sigstr + reader.GetAttribute('type')

        elif reader.NodeType() == XMLREADER_END_ELEMENT_NODE_TYPE:
            if (current_iface and not current_method and name == 'interface'):
                current_iface = None
            if (current_iface and current_method and name == 'method'):
                method_map[current_iface + '.' + current_method] = current_sigstr
                current_method = None
                current_sigstr = ''

        ret = reader.Read()

    if ret != 0:
        raise exceptions.IntrospectionParserException(data)

    return method_map
