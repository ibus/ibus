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

class IconWidget(gtk.Image):
    def __init__(self, icon, size):
        super(IconWidget, self).__init__()
        pixbuf = None
        try:
            if icon.startswith("/"):
                pixbuf = gdk.pixbuf_new_from_file(icon)
            else:
                theme = gtk.icon_theme_get_default()
                pixbuf = theme.load_icon(icon, size, 0)
        except:
            theme = gtk.icon_theme_get_default()
            pixbuf = theme.load_icon(gtk.STOCK_MISSING_IMAGE, size, 0)

        width = pixbuf.get_width()
        height = pixbuf.get_height()
        scale = float(size) / float(max(width, height))
        width = int(scale * width)
        height = int(scale * height)
        pixbuf = pixbuf.scale_simple(width, height, gdk.INTERP_BILINEAR)

        self.set_from_pixbuf(pixbuf)
        self.show()
