# vim:set et sts=4 sw=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2015 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2007-2015 Red Hat, Inc.
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

from gi.repository import IBus
from gi.repository import Gdk
from gi.repository import GdkPixbuf
from gi.repository import Gtk
from gi.repository import Pango

import i18n

from i18n import _, N_

class EngineAbout(Gtk.Dialog):
    def __init__(self, engine, transient_for = None):
        self.__engine_desc = engine
        super(EngineAbout, self).__init__(
                title = _("About"),
                transient_for = transient_for)

        buttons = (_("_Close"), Gtk.ResponseType.CLOSE)
        self.add_buttons(*buttons)
        self.__init_ui()

    def __init_ui(self):
        # set_icon_name() cannot fallback any stock ids to the real files.
        self.set_icon_name('help-about')
        sw = Gtk.ScrolledWindow()
        sw.set_shadow_type(Gtk.ShadowType.ETCHED_IN)
        sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        sw.set_size_request(400, 400)
        self.__text_view = Gtk.TextView()
        self.__text_view.set_editable(False)
        sw.add(self.__text_view)
        sw.show_all()
        self.vbox.pack_start(sw, True, True, 0)

        self.__fill_text_view()

    def __fill_text_view(self):
        text_buffer = self.__text_view.get_buffer()
        self.__create_tags(text_buffer)

        iter = text_buffer.get_iter_at_offset(0)
        text_buffer.insert_with_tags_by_name(iter, "\n ",
                                             "left_margin_16")
        text_buffer.insert_pixbuf(iter,
                self.__load_icon(self.__engine_desc.get_icon()))
        text_buffer.insert_with_tags_by_name(iter,
                "\n%s\n" % i18n.gettext_engine_longname(self.__engine_desc),
                "heading", "left_margin_16")
        text_buffer.insert_with_tags_by_name(iter,
                _("Language: %s\n") % IBus.get_language_name(self.__engine_desc.get_language()),
                "small", "bold", "left_margin_16")
        text_buffer.insert_with_tags_by_name(iter,
                _("Keyboard layout: %s\n") % self.__engine_desc.get_layout(),
                "small", "bold", "left_margin_16")
        text_buffer.insert_with_tags_by_name(iter,
                _("Author: %s\n") % self.__engine_desc.get_author(),
                "small", "bold", "left_margin_16")
        text_buffer.insert_with_tags_by_name(iter,
                _("Description:\n"), "small", "bold", "left_margin_16")
        text_buffer.insert_with_tags_by_name(iter,
                i18n.gettext_engine_description(self.__engine_desc),
                "wrap_text", "left_margin_32")


    def __create_tags(self, text_buffer):
        text_buffer.create_tag("heading",
                        weight=Pango.Weight.BOLD,
                        size = 16 * Pango.SCALE)
        text_buffer.create_tag("bold",
                        weight=Pango.Weight.BOLD)
        text_buffer.create_tag("italic",
                        style=Pango.Style.ITALIC)
        text_buffer.create_tag("small",
                        scale=0.833333333333) # Pango.SCALE_SMALL ?
        text_buffer.create_tag("gray_foreground",
                        foreground="dark gray")
        text_buffer.create_tag("wrap_text",
                        wrap_mode=Gtk.WrapMode.WORD)
        text_buffer.create_tag("left_margin_16",
                        left_margin=16)
        text_buffer.create_tag("left_margin_32",
                        left_margin=32)

    def __load_icon(self, icon_name):
        try:
            pixbuf = GdkPixbuf.Pixbuf.new_from_file_at_scale(icon_name,
                                                             48,
                                                             48,
                                                             True)
        except:
            theme = Gtk.IconTheme.get_default()
            icon = theme.lookup_icon(icon_name, 48, 0)
            if icon == None:
                icon = theme.lookup_icon("ibus-engine", 48, 0)
            if icon == None:
                icon = theme.lookup_icon("image-missing", 48, 0)
            pixbuf = icon.load_icon()
        return pixbuf

if __name__ == "__main__":
    desc = IBus.EngineDesc()
    EngineAbout(desc).run()
