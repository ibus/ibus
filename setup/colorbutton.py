# vim:set et sts=4 sw=4:
# -*- coding: utf-8 -*-
#
# ibus - The Input Bus
#
# Copyright (c) 2017 Ricardo Maes <ricmzn@gmail.com>
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
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
# USA

from gi import require_version as gi_require_version
gi_require_version('Gdk', '3.0')
gi_require_version('Gtk', '3.0')

from gi.repository import GObject
from gi.repository import Gdk
from gi.repository import Gtk

class IBusSetupColorButton(Gtk.ColorButton):
    __gtype_name__ = "IBusSetupColorButton"

    # Property for notify events
    rgba_string = GObject.Property(type=str)

    def __init__(self):
        Gtk.ColorButton.__init__(self)
        # Actual RGBA string value
        self.rgba_string_internal = ""
        # Tell GTK+ to inform us when the color button is changed so we can update the string value
        self.connect("notify::rgba", self.on_color_change)

    def set_custom_checkbutton(self, custom_checkbutton):
        # Checkbutton controlling the use of custom color
        self.custom_checkbutton = custom_checkbutton
        # Notify our property when the button state changes
        self.custom_checkbutton.connect("notify::active", self.on_color_change)

    @GObject.Property(type=str)
    def rgba_string(self):
        if self.custom_checkbutton.get_active():
            # Use the user-defined color
            return self.rgba_string_internal
        else:
            # Use the default dark blue color as defined in the DBus schema
            return "#415099"

    @rgba_string.setter
    def rgba_string(self, value):
        # Avoid looping back from the notify event by checking if the value is the same from the last call
        if value != self.rgba_string_internal:
            self.rgba_string_internal = value
            rgba = Gdk.RGBA()
            rgba.parse(value)
            self.set_rgba(rgba)

    def on_color_change(self, gparamstring, spec):
        self.rgba_string = self.get_rgba().to_string()
