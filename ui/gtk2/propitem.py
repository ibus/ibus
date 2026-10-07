# vim:set et sts=4 sw=4:
#
# ibus - The Input Bus
#
# Copyright(c) 2007-2010 Peng Huang <shawn.p.huang@gmail.com>
# Copyright(c) 2007-2010 Red Hat, Inc.
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

class PropItem:
    def __init__(self, prop):
        self._prop = prop
        self._sub_items = []

    def update_property(self, prop):
        if self._prop == None:
            return False

        retval = False

        if self._prop.key == prop.key and self._prop.type == prop.type:
            self._prop = prop
            self.property_changed()
            retval =  True

        if any(map(lambda i: i.update_property(prop), self._sub_items)):
            retval = True

        return retval

    def set_prop_label(self, label):
        self._prop.label = label
        self.property_changed()

    def set_icon(self, icon):
        self._prop.icon = icon
        self.property_changed()

    def set_tooltip(self, tooltip):
        self._prop.tooltip = tooltip
        self.property_changed()

    def set_state(self, state):
        self._prop.state = state
        self.property_changed()

    def property_changed(self):
        pass


