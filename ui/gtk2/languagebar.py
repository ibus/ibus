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

import gtk
import gtk.gdk as gdk
import gobject
import ibus
import icon
from handle import Handle
from menu import menu_position,\
    ImageMenuItem,\
    Menu,\
    CheckMenuItem,\
    RadioMenuItem,\
    SeparatorMenuItem
from engineabout import EngineAbout
from toolitem import ToolButton,\
    ToggleToolButton, \
    SeparatorToolItem, \
    MenuToolButton

from i18n import _, N_

ICON_SIZE = gtk.ICON_SIZE_MENU

class LanguageBar(gtk.Toolbar):
    __gtype_name__ = "IBusLanguagePanel"
    __gsignals__ = {
        "property-activate" : (
            gobject.SIGNAL_RUN_FIRST,
            gobject.TYPE_NONE,
            (gobject.TYPE_STRING, gobject.TYPE_INT)),
        "get-im-menu" : (
            gobject.SIGNAL_RUN_LAST,
            gobject.TYPE_PYOBJECT,
            ()),
        "show-engine-about" : (
            gobject.SIGNAL_RUN_LAST,
            gobject.TYPE_PYOBJECT,
            ()),
        "position-changed" : (
            gobject.SIGNAL_RUN_LAST,
            gobject.TYPE_NONE,
            (gobject.TYPE_INT, gobject.TYPE_INT)),
        }

    def __init__ (self):
        super(LanguageBar, self).__init__()
        self.__show = 0
        self.__enabled = False
        self.__has_focus = False
        self.__show_im_name = False
        self.__im_name = None
        self.__props = None
        self.__selected_menu_item = None
        self.set_style(gtk.TOOLBAR_BOTH_HORIZ)
        self.set_show_arrow(False)
        self.set_property("icon-size", ICON_SIZE)
        self.__create_ui()

        self.__properties = []
        self.__toplevel = gtk.Window(gtk.WINDOW_POPUP)
        self.__toplevel.connect("size-allocate", self.__toplevel_size_allocate_cb)
        self.__toplevel.add(self)
        self.__screen = gdk.screen_get_default()
        self.__screen.connect("size-changed", self.__screen_size_changed_cb)

        self.set_position(-1, -1)

    def __create_ui(self):
        # create move handle
        self.__handle = gtk.ToolItem()
        handle = Handle()
        self.__handle.add(handle)
        self.insert(self.__handle, -1)
        handle.connect("move-end", self.__handle_move_end_cb)

        # create input methods menu
        # prop = ibus.Property(key = "", type = ibus.PROP_TYPE_TOGGLE, icon = "ibus", tooltip = _("Switch input method"))
        self.__im_menu = gtk.ToggleToolButton()
        self.__im_menu.set_homogeneous(False)
        self.__im_menu.connect("toggled", self.__im_menu_toggled_cb)
        self.insert(self.__im_menu, -1)

        self.__about_button = gtk.ToolButton(gtk.STOCK_ABOUT)
        self.__about_button.set_no_show_all(True)
        self.__about_button.set_tooltip_text(_("About the input method"))
        self.__about_button.connect("clicked", self.__about_button_clicked_cb)
        self.insert(self.__about_button, -1)

    def __screen_size_changed_cb(self, screen):
        self.set_position(*self.__position)

    def __im_menu_toggled_cb(self, widget):
        if self.__im_menu.get_active():
            menu = self.emit("get-im-menu")
            menu.connect("deactivate", self.__im_menu_deactivate_cb)
            menu.popup(None, None,
                menu_position,
                0,
                gtk.get_current_event_time(),
                widget)

    def __about_button_clicked_cb(self, widget):
        if self.__enabled:
            self.emit("show-engine-about")

    def __im_menu_deactivate_cb(self, menu):
        self.__im_menu.set_active(False)

    def __handle_move_end_cb(self, handle):
        x, y = self.__toplevel.get_position()
        w, h = self.__toplevel.get_size()
        self.__position = x + w, y + h
        self.emit("position-changed", *self.__position)

    def __toplevel_size_allocate_cb(self, toplevel, allocation):
        x, y = self.__position
        w, h = self.__screen.get_width(), self.__screen.get_height()
        if x >= w - 80 or True:
            self.__toplevel.move(x - allocation.width, y - allocation.height)

    def __remove_properties(self):
        # reset all properties
        map(lambda i: i.destroy(), self.__properties)
        self.__properties = []

    def __set_opacity(self, opacity):
        if self.__toplevel.window == None:
            self.__toplevel.realize()
        self.__toplevel.window.set_opacity(opacity)

    def __replace_property(self, old_prop, new_prop):
        old_prop.label = new_prop.label
        old_prop.icon= new_prop.icon
        old_prop.tooltip = new_prop.tooltip
        old_prop.sensitive = new_prop.sensitive
        old_prop.visible = new_prop.visible
        old_prop.state = new_prop.state
        old_prop.sub_props = new_prop.sub_props

    def __label_select_cb(self, widget, data):
            self.__selected_menu_item = widget

    def __label_deselect_cb(self, widget, data):
            self.__selected_menu_item = None

    def __label_expose_cb(self, widget, event):
            x = widget.allocation.x
            y = widget.allocation.y
            label = widget._prop.label.get_text()
            if self.__selected_menu_item != widget:
                gc = widget.style.fg_gc[gtk.STATE_NORMAL]
            else:
                gc = widget.style.bg_gc[gtk.STATE_NORMAL]
            layout = widget.create_pango_layout(label)
            widget.window.draw_layout(gc, x + 5, y + 2, layout)

    def __set_item_icon(self, item, prop):
        item.set_property("always-show-image", True)
        if prop.icon:
            size = gtk.icon_size_lookup(gtk.ICON_SIZE_MENU)
            item.set_image(icon.IconWidget(prop.icon, size[0]))
        elif prop.label and prop.label.get_text() != "":
            item.set_label("")
            item.connect_after("expose-event", self.__label_expose_cb)
            item.connect("select", self.__label_select_cb, None)
            item.connect("deselect", self.__label_deselect_cb, None)
        else:
            size = gtk.icon_size_lookup(gtk.ICON_SIZE_MENU)
            item.set_image(icon.IconWidget("ibus", size[0]))

    def do_show(self):
        gtk.Toolbar.do_show(self)

    def do_size_request(self, requisition):
        gtk.Toolbar.do_size_request(self, requisition)
        self.__toplevel.resize(1, 1)

    def set_im_icon(self, icon_name):
        widget = icon.IconWidget(icon_name, 18)
        self.__im_menu.set_icon_widget(widget)

    def set_show_im_name(self, show):
        self.__show_im_name = show
        self.set_im_name(self.__im_name)
        self.__im_menu.set_is_important(show)

    def set_im_name(self, text):
        self.__im_name = text
        if text:
            self.__im_menu.set_tooltip_text(text)
            self.__im_menu.set_label(text)
        else:
            self.__im_menu.set_tooltip_text(_("Switch input method"))
            self.__im_menu.set_label("")

    def reset(self):
        self.__remove_properties()

    def set_enabled(self, enabled):
        self.__enabled = enabled
        if self.__enabled:
            self.__about_button.show()
            self.__set_opacity(1.0)
            if self.__has_focus:
                if self.__show in (1, 2):
                    self.show_all()
        else:
            self.__about_button.hide()
            self.__set_opacity(0.5)
            if self.__show in (1, 0):
                self.hide_all()

    def is_enabled(self):
        return self.__enabled

    def set_show(self, show):
        if show not in (0, 1, 2):
            show = 0
        self.__show = show
        if self.__has_focus:
            self.focus_in()
        else:
            self.focus_out()

    def set_position(self, x, y):
        w, h = self.__screen.get_width(), self.__screen.get_height()
        if x < 0 or y < 0:
            x = w - 20
            y = h - 40
        if x > w:
            x = w - 20
        if y > h:
            y = h - 40

        self.__position = x, y
        w, h = self.__toplevel.get_size()
        self.__toplevel.move(self.__position[0] - w, self.__position[1] - h)

    def get_show(self):
        return self.__show

    def register_properties(self, props):
        self.__props = props
        if self.__show == 0:
            return
        # refresh items on labguage bar
        self.__remove_properties()
        # create new properties
        for i, prop in enumerate(props):
            if prop.type == ibus.PROP_TYPE_NORMAL:
                item = ToolButton(prop = prop)
            elif prop.type == ibus.PROP_TYPE_TOGGLE:
                item = ToggleToolButton(prop = prop)
            elif prop.type == ibus.PROP_TYPE_MENU:
                item = MenuToolButton(prop = prop)
            elif prop.type == ibus.PROP_TYPE_SEPARATOR:
                item = SeparatorToolItem()
            else:
                raise IBusException("Unknown property type = %d" % prop.type)

            item.connect("property-activate",
                        lambda w, n, s: self.emit("property-activate", n, s))

            item.set_sensitive(prop.sensitive)

            item.set_no_show_all(True)

            if prop.visible:
                item.show()
            else:
                item.hide()

            self.__properties.append(item)
            self.insert(item, i + 2)

    def update_property(self, prop):
        if self.__show == 0 and self.__props:
            list = self.__props.get_properties()
            for i, p in enumerate(list):
                if p.key == prop.key and p.type == prop.type:
                    self.__replace_property(p, prop)
                    break
        map(lambda x: x.update_property(prop), self.__properties)

    def show_all(self):
        self.__toplevel.show_all()
        self.__toplevel.window.raise_()
        gtk.Toolbar.show_all(self)

    def hide_all(self):
        try:
            self.__toplevel.window.lower()
            self.__toplevel.window.hide_all()
        except:
            pass
        x, y = self.__toplevel.get_position()
        self.__toplevel.hide_all()
        gtk.Toolbar.hide_all(self)

        # save bar position
        self.__toplevel.move(x, y)

    def focus_in(self):
        self.__has_focus = True
        self.__im_menu.set_sensitive(True)
        if (self.__show == 1 and self.__enabled) or self.__show == 2:
            self.show_all()
        else:
            self.hide_all()

    def focus_out(self):
        self.__has_focus = False
        self.__im_menu.set_sensitive(False)
        if self.__show == 2:
            self.show_all()
        else:
            self.hide_all()

    def create_im_menu(self, menu):
        if not self.__enabled:
            return
        if self.__show != 0:
            return
        if not menu:
            assert False
        props = self.__props
        if not props:
            return

        self.__remove_properties()
        item = SeparatorMenuItem()
        item.show()
        self.__properties.append(item)
        menu.insert(item, 0)

        about_label = _("About") + " - " + (self.__im_name or "")
        prop = ibus.Property(key=u"about",
                             label=unicode(about_label),
                             icon=unicode(gtk.STOCK_ABOUT),
                             tooltip=unicode(_("About the Input Method")))
        item = ImageMenuItem(prop = prop)
        item.set_property("always-show-image", True)
        item.set_no_show_all(True)
        item.show()
        self.__properties.append(item)
        menu.insert(item, 0)
        item.connect("property-activate",
                     lambda w, n, s: self.emit("show-engine-about"))

        list = props.get_properties()
        list.reverse()
        radio_group = None

        for i, prop in enumerate(list):
            if prop.type == ibus.PROP_TYPE_NORMAL:
                item = ImageMenuItem(prop = prop)
                self.__set_item_icon(item, prop)
            elif prop.type == ibus.PROP_TYPE_TOGGLE:
                item = CheckMenuItem(prop = prop)
            elif prop.type == ibus.PROP_TYPE_RADIO:
                item = RadioMenuItem(radio_group, prop = prop)
                radio_group = item
            elif prop.type == ibus.PROP_TYPE_SEPARATOR:
                item = SeparatorMenuItem()
                radio_group = None
            elif prop.type == ibus.PROP_TYPE_MENU:
                item = ImageMenuItem(prop = prop)
                self.__set_item_icon(item, prop)
                submenu = Menu(prop)
                item.set_submenu(submenu)
                submenu.connect("property-activate",
                                lambda w, n, s: self.emit("property-activate", n, s))
            else:
                raise IBusException("Unknown property type = %d" % prop.type)

            item.set_sensitive(prop.sensitive)

            item.set_no_show_all(True)

            if prop.visible:
                item.show()
            else:
                item.hide()

            self.__properties.append(item)
            menu.insert(item, 0)

            item.connect("property-activate",
                         lambda w, n, s: self.emit("property-activate", n, s))

