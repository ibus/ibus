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

import gtk
import gtk.gdk as gdk
import gobject
import ibus
from propitem import PropItem
import icon
from menu import *

class ToolButton(gtk.ToolButton, PropItem):
    __gtype_name__ = "IBusToolButton"
    __gsignals__ = {
        "property-activate" : (
            gobject.SIGNAL_RUN_FIRST,
            gobject.TYPE_NONE,
            (gobject.TYPE_STRING, gobject.TYPE_INT)),
        }

    def __init__(self, prop):
        gtk.ToolButton.__init__(self, label = prop.label.text)
        self.set_homogeneous(False)
        PropItem.__init__(self, prop)
        self.property_changed()

    def set_icon_name(self, icon_name):
        if icon_name:
            widget = icon.IconWidget(icon_name, 18)
            gtk.ToolButton.set_icon_widget(self, widget)
            self.set_is_important(False)
        elif self._prop.label.text:
            gtk.ToolButton.set_icon_widget(self, None)
            self.set_is_important(True)
        else:
            widget = icon.IconWidget("ibus", 18)
            gtk.ToolButton.set_icon_widget(self, widget)
            self.set_is_important(False)

        self._prop.icon = icon_name

    def set_tooltip_text(self, text):
        if text:
            gtk.ToolButton.set_tooltip_text(self, text.text)
        else:
            gtk.ToolButton.set_tooltip_text(self, None)

        self._prop.tooltip = text

    def property_changed(self):
        self.set_label(self._prop.label.text)
        self.set_tooltip_text(self._prop.tooltip)
        self.set_sensitive(self._prop.sensitive)
        self.set_icon_name(self._prop.icon)

        if self._prop.visible:
            self.show()
        else:
            self.hide()

    def do_clicked(self):
        self.emit("property-activate", self._prop.key, self._prop.state)


class ToggleToolButton(gtk.ToggleToolButton, PropItem):
    __gtype_name__ = "IBusToggleToolButton"
    __gsignals__ = {
        "property-activate" : (
            gobject.SIGNAL_RUN_FIRST,
            gobject.TYPE_NONE,
            (gobject.TYPE_STRING, gobject.TYPE_INT)),
        }

    def __init__(self, prop):
        gtk.ToggleToolButton.__init__(self)
        self.set_homogeneous(False)
        PropItem.__init__(self, prop)
        self.property_changed()

    def set_icon_name(self, icon_name):
        if icon_name:
            widget = icon.IconWidget(icon_name, 18)
            gtk.ToggleToolButton.set_icon_widget(self, widget)
            self.set_is_important(False)
        elif self._prop.label:
            gtk.ToggleToolButton.set_icon_widget(self, None)
            self.set_is_important(True)
        else:
            widget = icon.IconWidget("ibus", 18)
            gtk.ToggleToolButton.set_icon_widget(self, widget)
            self.set_is_important(False)

        self._prop.icon = icon_name

    def set_tooltip_text(self, text):
        if text:
            gtk.ToggleToolButton.set_tooltip_text(self, text.text)
        else:
            gtk.ToggleToolButton.set_tooltip_text(self, None)

        self._prop.tooltip = text

    def property_changed(self):
        self.set_tooltip_text(self._prop.tooltip)
        self.set_label(self._prop.label.text)
        self.set_icon_name(self._prop.icon)
        self.set_active(self._prop.state == ibus.PROP_STATE_CHECKED)
        self.set_sensitive(self._prop.sensitive)
        if self._prop.visible:
            self.show()
        else:
            self.hide()

    def do_toggled(self):
        # Do not send property-activate to engine in case the event is
        # sent from engine.
        do_emit = False
        if self.get_active():
            if self._prop.state != ibus.PROP_STATE_CHECKED:
                do_emit = True
            self._prop.state = ibus.PROP_STATE_CHECKED
        else:
            if self._prop.state != ibus.PROP_STATE_UNCHECKED:
                do_emit = True
            self._prop.state = ibus.PROP_STATE_UNCHECKED
        if do_emit:
            self.emit("property-activate", self._prop.key, self._prop.state)

class SeparatorToolItem(gtk.SeparatorToolItem, PropItem):
    __gtype_name__ = "IBusSeparatorToolItem"
    def __init__ (self, prop):
        gtk.SeparatorToolItem.__init__(self)
        self.set_homogeneous(False)
        PropItem.__init__(self, prop)

class MenuToolButton(ToggleToolButton):
    __gtype_name__ = "IBusMenuToolButton"
    # __gsignals__ = {
    #        "property-activate" : (
    #            gobject.SIGNAL_RUN_FIRST,
    #            gobject.TYPE_NONE,
    #            (gobject.TYPE_STRING, gobject.TYPE_INT)),
    #        }

    def __init__(self, prop):
        super(MenuToolButton, self).__init__(prop)
        self._menu = Menu(prop)
        self._menu.connect("deactivate", lambda m: self.set_active(False))
        self._menu.connect("property-activate", lambda w,n,s: self.emit("property-activate", n, s))

    def update_property(self, prop):
        PropItem.update_property(self, prop)
        self._menu.update_property(prop)

    def do_toggled(self):
        if self.get_active():
            self._menu.popup(0, gtk.get_current_event_time(), self)

    def destroy(self):
        self._menu.destroy()
        self._menu = None
        super(MenuToolButton, self).destroy()
