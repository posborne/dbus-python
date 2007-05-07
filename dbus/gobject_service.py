"""Support code for implementing D-Bus services via GObjects."""

# Copyright (C) 2007 Collabora Ltd. <http://www.collabora.co.uk/>
#
# Licensed under the Academic Free License version 2.1
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Library General Public License as published by
# the Free Software Foundation; either version 2.1 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

import gobject
import dbus.service

class ExportedGObjectType(gobject.GObjectMeta, dbus.service.InterfaceType):
    """A metaclass which inherits from both GObjectMeta and
    `dbus.service.InterfaceType`. Used as the metaclass for `ExportedGObject`.
    """
    def __init__(cls, name, bases, dct):
        gobject.GObjectMeta.__init__(cls, name, bases, dct)
        dbus.service.InterfaceType.__init__(cls, name, bases, dct)

class ExportedGObject(gobject.GObject, dbus.service.Object):
    """A GObject which is exported on the D-Bus.

    Because GObject and `dbus.service.Object` both have custom metaclasses,
    the naive approach using simple multiple inheritance won't work. This
    class has `ExportedGObjectType` as its metaclass, which is sufficient
    to make it work correctly.
    """
    __metaclass__ = ExportedGObjectType

    def __init__(self, conn=None, object_path=None, **kwargs):
        """Initialize an exported GObject.

        :Parameters:
            `conn` : dbus.connection.Connection
                The D-Bus connection or bus
            `object_path` : str
                The object path at which to register this object.
        :Keywords:
            `bus_name` : dbus.service.BusName
                A bus name to be held on behalf of this object, or None.
            `gobject_properties` : dict
                GObject properties to be set on the constructed object.

                Any unrecognised keyword arguments will also be interpreted
                as GObject properties.
            """
        bus_name = kwargs.pop('bus_name', None)
        gobject_properties = kwargs.pop('gobject_properties', None)

        if gobject_properties is not None:
            kwargs.update(gobject_properties)
        gobject.GObject.__init__(self, **kwargs)
        dbus.service.Object.__init__(self, conn=conn,
                                     object_path=object_path,
                                     bus_name=bus_name)
