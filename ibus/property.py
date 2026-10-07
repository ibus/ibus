# vim:set et sts=4 sw=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2010 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2007-2010 Red Hat, Inc.
#
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, see <https://www.gnu.org/licenses/>.

__all__ = (
        "PROP_TYPE_NORMAL",
        "PROP_TYPE_TOGGLE",
        "PROP_TYPE_RADIO",
        "PROP_TYPE_SEPARATOR",
        "PROP_TYPE_MENU",
        "PROP_STATE_UNCHECKED",
        "PROP_STATE_CHECKED",
        "PROP_STATE_INCONSISTENT",
        "Property",
        "PropList",
    )

import dbus
from text import Text
from serializable import *

PROP_TYPE_NORMAL = 0
PROP_TYPE_TOGGLE = 1
PROP_TYPE_RADIO = 2
PROP_TYPE_MENU = 3
PROP_TYPE_SEPARATOR = 4

PROP_STATE_UNCHECKED = 0
PROP_STATE_CHECKED = 1
PROP_STATE_INCONSISTENT = 2

def _to_unicode(text):
    if isinstance(text, unicode):
        return text
    if isinstance(text, str):
        return unicode(text, "utf8")
    raise TypeError("text must be instance of unicode or str")
def _to_text(text):
    if isinstance(text, Text):
        return text
    text = _to_unicode(text)
    return Text(text)

class Property(Serializable):
    __gtype_name__ = "PYIBusProperty"
    __NAME__ = "IBusProperty"
    def __init__(self, key="", type=PROP_TYPE_NORMAL, label=u"", icon=u"", tooltip=u"",
                 sensitive=True, visible=True, state=PROP_STATE_UNCHECKED,
                 symbol=u""):
        super(Property, self).__init__()
        self.__key = _to_unicode(key)
        self.__type = type
        self.label = label
        self.symbol = symbol
        self.icon = icon
        self.tooltip = tooltip
        self.sensitive = sensitive
        self.visible = visible
        self.state = state
        self.sub_props = PropList()

    def set_sub_props(self, props):
        self.__sub_props = props

    def get_sub_props(self):
        return self.__sub_props

    def get_key(self):
        return self.__key

    def get_type(self):
        return self.__type

    def set_label(self, label):
        self.__label = _to_text(label)

    def get_label(self):
        return self.__label

    def set_symbol(self, symbol):
        self.__symbol = _to_text(symbol)

    def get_symbol(self):
        return self.__symbol

    def set_icon(self, icon):
        self.__icon = _to_unicode(icon)

    def get_icon(self):
        return self.__icon

    def set_tooltip(self, tooltip):
        self.__tooltip = _to_text(tooltip)

    def get_tooltip(self):
        return self.__tooltip

    def set_state(self, state):
        self.__state = state

    def get_state(self):
        return self.__state

    def set_sensitive(self, sensitive):
        self.__sensitive = sensitive

    def get_sensitive(self):
        return self.__sensitive

    def set_visible(self, visible):
        self.__visible = visible

    def get_visible(self):
        return self.__visible

    key         = property(get_key)
    type        = property(get_type)
    label       = property(get_label, set_label)
    symbol      = property(get_symbol, set_symbol)
    icon        = property(get_icon, set_icon)
    tooltip     = property(get_tooltip, set_tooltip)
    state       = property(get_state, set_state)
    sensitive   = property(get_sensitive, set_sensitive)
    visible     = property(get_visible, set_visible)
    sub_props   = property(get_sub_props, set_sub_props)

    def is_same(self, prop, test_all = True):
        if self.__name != prop.__name or self.__type != prop.__type:
            return False
        if not test_all:
            return True
        if self.__label != prop.__label or \
            self.__symbol != prop.__symbol or \
            self.__icon != prop.__icon or \
            self.__tooltip != prop.__tooltip or \
            self.__sensitive != prop.__sensitive or \
            self.__visible != prop.__visible or \
            self.__state != prop.__state:
            return False
        return self.__sub_props.is_same(prop.__sub_props, test_all)


    def serialize(self, struct):
        super(Property, self).serialize(struct)
        struct.append(dbus.String(self.__key))
        struct.append(dbus.UInt32(self.__type))
        struct.append(serialize_object(self.__label))
        struct.append(dbus.String(self.__icon))
        struct.append(serialize_object(self.__tooltip))
        struct.append(dbus.Boolean(self.__sensitive))
        struct.append(dbus.Boolean(self.__visible))
        struct.append(dbus.UInt32(self.__state))
        sub_props = serialize_object(self.__sub_props)
        struct.append(sub_props)
        struct.append(serialize_object(self.__symbol))

    def deserialize(self, struct):
        super(Property, self).deserialize(struct)
        self.__key = struct.pop(0)
        self.__type = struct.pop(0)
        self.__label = deserialize_object(struct.pop(0))
        self.__icon = struct.pop(0)
        self.__tooltip = deserialize_object(struct.pop(0))
        self.__sensitive = deserialize_object(struct.pop(0))
        self.__visible = struct.pop(0)
        self.__state = struct.pop(0)
        props = struct.pop(0)

        self.__sub_props = deserialize_object(props)
        self.__symbol    = deserialize_object(struct.pop(0))

class PropList(Serializable):
    __gtype_name__ = "PYIBusPropList"
    __NAME__ = "IBusPropList"
    def __init__(self):
        super(PropList, self).__init__()
        self.__props = []

    def append(self, prop):
        self.__props.append(prop)

    def prepend(self, prop):
        self.__props.insert(0, prop)

    def insert(self, index, prop):
        self.__props.insert(index, prop)

    def get_properties(self):
        return self.__props[:]

    def is_same(self, props, test_all = True):
        if len(props.get_properties()) != len(self.get_properties()):
            return False

        for a, b in zip(self.get_properties(), props.get_properties()):
            if not a.is_same(b, test_all):
                return False
        return False

    def serialize(self, struct):
        super(PropList, self).serialize(struct)
        props = map(lambda p: serialize_object(p), self.__props)
        struct.append (dbus.Array(props, signature = "v"))

    def deserialize(self, struct):
        super(PropList, self).deserialize(struct)
        props = map(lambda v: deserialize_object(v), struct.pop(0))
        self.__props = props

    def __iter__(self):
        return self.__props.__iter__()

    def __getitem__(self, i):
        return self.__props.__getitem__(i)

def test():
    props = PropList()
    props.append(Property(u"a"))
    props.append(Property(u"b"))
    props.append(Property(u"c"))
    props.append(Property(u"d"))
    value = serialize_object(props)
    props = deserialize_object(value)
    print props

if __name__ == "__main__":
    test()
