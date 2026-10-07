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

class Handle(gtk.EventBox):
    __gtype_name__ = "IBusHandle"
    __gsignals__ = {
        "move-begin" : (
            gobject.SIGNAL_RUN_LAST,
            gobject.TYPE_NONE,
            ()),
        "move-end" : (
            gobject.SIGNAL_RUN_LAST,
            gobject.TYPE_NONE,
            ()),
        }

    def __init__ (self):
        super(Handle, self).__init__()
        self.set_visible_window(False)
        self.set_size_request(10, -1)
        self.set_events(
            gdk.EXPOSURE_MASK | \
            gdk.BUTTON_PRESS_MASK | \
            gdk.BUTTON_RELEASE_MASK | \
            gdk.BUTTON1_MOTION_MASK)

        self.__move_begined = False

        root = gdk.get_default_root_window()

    def do_button_press_event(self, event):
        if event.button == 1:
            root = gdk.get_default_root_window()
            try:
                desktop = root.property_get("_NET_CURRENT_DESKTOP")[2][0]
                self.__workarea = root.property_get("_NET_WORKAREA")[2][desktop * 4: (desktop + 1) * 4]
            except:
                self.__workarea = None
            self.__move_begined = True
            toplevel = self.get_toplevel()
            x, y = toplevel.get_position()
            self.__press_pos = event.x_root - x, event.y_root - y
            self.window.set_cursor(gdk.Cursor(gdk.FLEUR))
            self.emit("move-begin")
            return True
        return False

    def do_button_release_event(self, event):
        if event.button == 1:
            self.__move_begined = False
            del self.__press_pos
            del self.__workarea
            self.window.set_cursor(gdk.Cursor(gdk.LEFT_PTR))
            self.emit("move-end")
            return True

        return False

    def do_motion_notify_event(self, event):
        if not self.__move_begined:
            return
        toplevel = self.get_toplevel()
        x, y = toplevel.get_position()
        x  = int(event.x_root - self.__press_pos[0])
        y  = int(event.y_root - self.__press_pos[1])

        if self.__workarea == None:
            toplevel.move(x, y)
            return

        if x < self.__workarea[0] and x > self.__workarea[0] - 16:
            x = self.__workarea[0]
        if y < self.__workarea[1] and y > self.__workarea[1] - 16:
            y = self.__workarea[1]

        w, h = toplevel.get_size()
        if x + w > self.__workarea[0] + self.__workarea[2] and \
            x + w < self.__workarea[0] + self.__workarea[2] + 16:
            x = self.__workarea[0] + self.__workarea[2] - w
        if y + h > self.__workarea[1] + self.__workarea[3] and \
            y + h < self.__workarea[1] + self.__workarea[3] + 16:
            y =  self.__workarea[1] + self.__workarea[3] - h

        toplevel.move(x, y)

    def do_expose_event(self, event):
        self.style.paint_handle(
                    self.window,
                    gtk.STATE_NORMAL,
                    gtk.SHADOW_OUT,
                    event.area,
                    self,
                    "",
                    self.allocation.x, self.allocation.y, 
                    10, self.allocation.height,
                    gtk.ORIENTATION_VERTICAL)
        return True

