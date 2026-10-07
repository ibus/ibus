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
from gtk import gdk
import pango
import ibus

from i18n import _, N_

class EngineAbout(gtk.Dialog):
    def __init__(self, enginedesc):
        self.__engine_desc = enginedesc
        super(EngineAbout, self).__init__(_("About"), None, gtk.DIALOG_MODAL, (gtk.STOCK_CLOSE, gtk.RESPONSE_CLOSE))

        self.__init_ui()

    def __init_ui(self):
        self.set_icon_name("gtk-about")
        sw = gtk.ScrolledWindow()
        sw.set_shadow_type(gtk.SHADOW_ETCHED_IN)
        sw.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        self.__text_view = gtk.TextView()
        self.__text_view.set_size_request(400, 400)
        self.__text_view.set_editable(False)
        sw.add(self.__text_view)
        sw.show_all()
        self.vbox.pack_start(sw)

        self.__fill_text_view()

    def __fill_text_view(self):
        text_buffer = self.__text_view.get_buffer()
        self.__create_tags(text_buffer)
        
        iter = text_buffer.get_iter_at_offset(0)
        text_buffer.insert_with_tags_by_name(iter, "\n ",
                                             "left_margin_16")
        text_buffer.insert_pixbuf(iter, self.__load_icon(self.__engine_desc.icon))
        text_buffer.insert_with_tags_by_name(iter, "\n%s\n" % self.__engine_desc.longname,
                                             "heading", "left_margin_16")
        text_buffer.insert_with_tags_by_name(iter, _("Language: %s\n") % ibus.get_language_name(self.__engine_desc.language),
                                            "small", "bold", "left_margin_16")
        text_buffer.insert_with_tags_by_name(iter, _("Keyboard layout: %s\n") % self.__engine_desc.layout,
                                            "small", "bold", "left_margin_16")
        text_buffer.insert_with_tags_by_name(iter, _("Author: %s\n") % self.__engine_desc.author,
                                            "small", "bold", "left_margin_16")
        text_buffer.insert_with_tags_by_name(iter, _("Description:\n"),
                                            "small", "bold", "left_margin_16")
        text_buffer.insert_with_tags_by_name(iter, self.__engine_desc.description,
                                            "wrap_text", "left_margin_32")


    def __create_tags(self, text_buffer):
        text_buffer.create_tag("heading",
                        weight=pango.WEIGHT_BOLD,
                        size = 16 * pango.SCALE)
        text_buffer.create_tag("bold",
                        weight=pango.WEIGHT_BOLD)
        text_buffer.create_tag("italic",
                        style=pango.STYLE_ITALIC)
        text_buffer.create_tag("small",
                        scale=pango.SCALE_SMALL)
        text_buffer.create_tag("gray_foreground",
                        foreground="dark gray")
        text_buffer.create_tag("wrap_text",
                        wrap_mode=gtk.WRAP_WORD)
        text_buffer.create_tag("left_margin_16",
                        left_margin=16)
        text_buffer.create_tag("left_margin_32",
                        left_margin=32)

    def __load_icon(self, icon):
        try:
            pixbuf = gdk.pixbuf_new_from_file_at_scale(icon, 48, 48, True)
        except:
            theme = gtk.icon_theme_get_default()
            icon = theme.lookup_icon("ibus-engine", 48, 0)
            if icon == None:
                icon = theme.lookup_icon("gtk-missing-image", 48, 0)
            pixbuf = icon.load_icon()
        return pixbuf
