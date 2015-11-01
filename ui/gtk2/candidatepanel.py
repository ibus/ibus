# vim:set et sts=4 sw=4:
#
# ibus - The Input Bus
#
# Copyright(c) 2007-2010 Peng Huang <shawn.p.huang@gmail.com>
# Copyright(c) 2007-2010 Red Hat, Inc.
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

import operator
import gtk
import gtk.gdk as gdk
import gobject
import pango
import ibus
from ibus._gtk import PangoAttrList
from handle import Handle
from i18n import _, N_

class EventBox(gtk.EventBox):
    __gtype_name__ = "IBusEventBox"
    def __init__(self):
        super(EventBox, self).__init__()
        self.connect("realize", self.__realize_cb)

    def __realize_cb(self, widget):
        widget.window.set_cursor(gdk.Cursor(gdk.HAND2))

class Label(gtk.Label):
    __gtype_name__ = "IBusCandidateLabel"

class HSeparator(gtk.HBox):
    __gtype_name__ = "IBusHSeparator"
    def __init__(self):
        super(HSeparator, self).__init__()
        self.pack_start(gtk.HSeparator(), True, True, 4)

class VSeparator(gtk.VBox):
    __gtype_name__ = "IBusVSeparator"
    def __init__(self):
        super(VSeparator, self).__init__()
        self.pack_start(gtk.VSeparator(), True, True, 4)

class CandidateArea(gtk.HBox):
    __gtype_name__ = "IBusCandidateArea"
    __gsignals__ = {
        "candidate-clicked" : (
            gobject.SIGNAL_RUN_FIRST,
            gobject.TYPE_NONE,
            (gobject.TYPE_UINT, gobject.TYPE_UINT, gobject.TYPE_UINT )),
    }

    def __init__(self, orientation):
        super(CandidateArea, self).__init__()
        self.set_name("IBusCandidateArea")
        self.__orientation = orientation
        self.__labels = []
        self.__candidates = []
        self.__create_ui()

    def __create_ui(self):
        if self.__orientation == ibus.ORIENTATION_VERTICAL:
            self.__vbox1 = gtk.VBox()
            self.__vbox1.set_homogeneous(True)
            self.__vbox2 = gtk.VBox()
            self.__vbox2.set_homogeneous(True)
            self.pack_start(self.__vbox1, False, False, 4)
            self.pack_start(VSeparator(), False, False, 0)
            self.pack_start(self.__vbox2, True, True, 4)

        for i in range(0, 16):
            label1 = Label("%c." % ("1234567890abcdef"[i]))
            label1.set_alignment(0.0, 0.5)
            label1.show()

            label2 = Label()
            label2.set_alignment(0.0, 0.5)
            label1.show()

            if self.__orientation == ibus.ORIENTATION_VERTICAL:
                label1.set_property("xpad", 8)
                label2.set_property("xpad", 8)
                ebox1 = EventBox()
                ebox1.set_no_show_all(True)
                ebox1.add(label1)
                ebox2 = EventBox()
                ebox2.set_no_show_all(True)
                ebox2.add(label2)
                self.__vbox1.pack_start(ebox1, False, False, 2)
                self.__vbox2.pack_start(ebox2, False, False, 2)
                self.__candidates.append((ebox1, ebox2))
            else:
                hbox = gtk.HBox()
                hbox.show()
                hbox.pack_start(label1, False, False, 1)
                hbox.pack_start(label2, False, False, 1)
                ebox = EventBox()
                ebox.set_no_show_all(True)
                ebox.add(hbox)
                self.pack_start(ebox, False, False, 4)
                self.__candidates.append((ebox,))

            self.__labels.append((label1, label2))

        for i, ws in enumerate(self.__candidates):
            for w in ws:
                w.connect("button-press-event", lambda w, e, i:self.emit("candidate-clicked", i, e.button, e.state), i)

    def set_labels(self, labels):
        if not labels:
            for i in xrange(0, 16):
                self.__labels[i][0].set_text("%c." % ("1234567890abcdef"[i]))
                self.__labels[i][0].set_property("attributes", None)
            return

        i = 0
        for text, attrs in labels:
            self.__labels[i][0].set_text(text)
            self.__labels[i][0].set_property("attributes", attrs)
            i += 1
            if i >= 16:
                break

    def set_candidates(self, candidates, focus_candidate = 0, show_cursor = True):
        assert len(candidates) <= len(self.__labels)
        for i, (text, attrs) in enumerate(candidates):
            if i == focus_candidate and show_cursor:
                if attrs == None:
                    attrs = pango.AttrList()
                color = self.__labels[i][1].style.base[gtk.STATE_SELECTED]
                end_index = len(text.encode("utf8"))
                attr = pango.AttrBackground(color.red, color.green, color.blue, 0, end_index)
                attrs.change(attr)
                color = self.__labels[i][1].style.text[gtk.STATE_SELECTED]
                attr = pango.AttrForeground(color.red, color.green, color.blue, 0, end_index)
                attrs.insert(attr)

            self.__labels[i][1].set_text(text)
            self.__labels[i][1].show()
            self.__labels[i][1].set_property("attributes", attrs)
            for w in self.__candidates[i]:
                w.show()

        for w in reduce(operator.add, self.__candidates[len(candidates):]):
            w.hide()

class CandidatePanel(gtk.VBox):
    __gtype_name__ = "IBusCandidate"
    __gsignals__ = {
        "cursor-up" : (
            gobject.SIGNAL_RUN_FIRST,
            gobject.TYPE_NONE,
            ()),
        "cursor-down" : (
            gobject.SIGNAL_RUN_FIRST,
            gobject.TYPE_NONE,
            ()),
        "page-up" : (
            gobject.SIGNAL_RUN_FIRST,
            gobject.TYPE_NONE,
            ()),
        "page-down" : (
            gobject.SIGNAL_RUN_FIRST,
            gobject.TYPE_NONE,
            ()),
        "candidate-clicked" : (
            gobject.SIGNAL_RUN_FIRST,
            gobject.TYPE_NONE,
            (gobject.TYPE_UINT, gobject.TYPE_UINT, gobject.TYPE_UINT)),
    }

    def __init__(self):
        super(CandidatePanel, self).__init__()
        self.set_name("IBusCandidate")

        self.__toplevel = gtk.Window(gtk.WINDOW_POPUP)
        self.__viewport = gtk.Viewport()
        self.__viewport.set_shadow_type(gtk.SHADOW_IN)
        self.__toplevel.add(self.__viewport)

        hbox = gtk.HBox()
        handle = Handle()
        handle.connect("move-end", self.__handle_move_end_cb)
        hbox.pack_start(handle)
        hbox.pack_start(self)

        self.__viewport.add(hbox)
        self.__toplevel.add_events(
            gdk.BUTTON_PRESS_MASK | \
            gdk.BUTTON_RELEASE_MASK | \
            gdk.BUTTON1_MOTION_MASK)
        self.__toplevel.connect("size-allocate", lambda w, a: self.__check_position())

        self.__orientation = ibus.ORIENTATION_VERTICAL
        self.__current_orientation = self.__orientation
        self.__preedit_visible = False
        self.__aux_string_visible = False
        self.__lookup_table_visible = False
        self.__preedit_string = ""
        self.__preedit_attrs = pango.AttrList()
        self.__aux_string = ""
        self.__aux_attrs = pango.AttrList()
        self.__lookup_table = None

        self.__cursor_location = (0, 0, 0, 0)
        self.__moved_cursor_location = None

        self.__recreate_ui()

    def __handle_move_end_cb(self, handle):
        # store moved location
        self.__moved_cursor_location = self.__toplevel.get_position() + (self.__cursor_location[2], self.__cursor_location[3])

    def __recreate_ui(self):
        for w in self:
            self.remove(w)
            w.destroy()
        # create preedit label
        self.__preedit_label = Label(self.__preedit_string)
        self.__preedit_label.set_attributes(self.__preedit_attrs)
        self.__preedit_label.set_alignment(0.0, 0.5)
        self.__preedit_label.set_padding(8, 0)
        self.__preedit_label.set_no_show_all(True)
        if self.__preedit_visible:
            self.__preedit_label.show()

        # create aux label
        self.__aux_label = Label(self.__aux_string)
        self.__aux_label.set_attributes(self.__aux_attrs)
        self.__aux_label.set_alignment(0.0, 0.5)
        self.__aux_label.set_padding(8, 0)
        self.__aux_label.set_no_show_all(True)
        if self.__aux_string_visible:
            self.__aux_label.show()

        # create candidates area
        self.__candidate_area = CandidateArea(self.__current_orientation)
        self.__candidate_area.set_no_show_all(True)
        self.__candidate_area.connect("candidate-clicked", lambda x, i, b, s: self.emit("candidate-clicked", i, b, s))
        # self.update_lookup_table(self.__lookup_table, self.__lookup_table_visible)

        # create state label
        self.__state_label = Label()
        self.__state_label.set_size_request(20, -1)

        # create buttons
        self.__prev_button = gtk.Button()
        self.__prev_button.connect("clicked", lambda x: self.emit("page-up"))
        self.__prev_button.set_relief(gtk.RELIEF_NONE)
        self.__prev_button.set_tooltip_text(_("Previous page"))

        self.__next_button = gtk.Button()
        self.__next_button.connect("clicked", lambda x: self.emit("page-down"))
        self.__next_button.set_relief(gtk.RELIEF_NONE)
        self.__next_button.set_tooltip_text(_("Next page"))

        self.__pack_all_widgets()

    def __pack_all_widgets(self):
        if self.__current_orientation == ibus.ORIENTATION_VERTICAL:
            # package all widgets in vertical mode
            image = gtk.Image()
            image.set_from_stock(gtk.STOCK_GO_UP, gtk.ICON_SIZE_MENU)
            self.__prev_button.set_image(image)

            image = gtk.Image()
            image.set_from_stock(gtk.STOCK_GO_DOWN, gtk.ICON_SIZE_MENU)
            self.__next_button.set_image(image)

            vbox = gtk.VBox()
            vbox.pack_start(self.__preedit_label, False, False, 0)
            vbox.pack_start(self.__aux_label, False, False, 0)
            self.pack_start(vbox, False, False, 5)
            self.pack_start(HSeparator(), False, False)
            self.pack_start(self.__candidate_area, False, False, 2)
            self.pack_start(HSeparator(), False, False)
            hbox= gtk.HBox()
            hbox.pack_start(self.__state_label, True, True)
            hbox.pack_start(VSeparator(), False, False)
            hbox.pack_start(self.__prev_button, False, False, 2)
            hbox.pack_start(self.__next_button, False, False, 2)
            self.pack_start(hbox, False, False)
        else:
            # package all widgets in HORIZONTAL mode
            image = gtk.Image()
            image.set_from_stock(gtk.STOCK_GO_UP, gtk.ICON_SIZE_MENU)
            self.__prev_button.set_image(image)

            image = gtk.Image()
            image.set_from_stock(gtk.STOCK_GO_DOWN, gtk.ICON_SIZE_MENU)
            self.__next_button.set_image(image)

            vbox = gtk.VBox()
            vbox.pack_start(self.__preedit_label, False, False, 0)
            vbox.pack_start(self.__aux_label, False, False, 0)
            self.pack_start(vbox, False, False, 5)
            self.pack_start(HSeparator(), False, False)
            hbox = gtk.HBox()
            hbox.pack_start(self.__candidate_area, True, True, 2)
            hbox.pack_start(VSeparator(), False, False)
            hbox.pack_start(self.__prev_button, False, False, 2)
            hbox.pack_start(self.__next_button, False, False, 2)
            self.pack_start(hbox, False, False)

        # self.hide_all()
        # self.show_all()

    def show_preedit_text(self):
        self.__preedit_visible = True
        self.__preedit_label.show()
        self.__check_show_states()

    def hide_preedit_text(self):
        self.__preedit_visible = False
        self.__preedit_label.hide()
        self.__check_show_states()

    def update_preedit_text(self, text, cursor_pos, visible):
        attrs = PangoAttrList(text.attributes, text.text)
        if visible:
            self.show_preedit_text()
        else:
            self.hide_preedit_text()
        self.__preedit_stribg = text.text
        self.__preedit_label.set_text(text.text)
        self.__preedit_attrs = attrs
        self.__preedit_label.set_attributes(attrs)

    def show_auxiliary_text(self):
        self.__aux_string_visible = True
        self.__aux_label.show()
        self.__check_show_states()

    def hide_auxiliary_text(self):
        self.__aux_string_visible = False
        self.__aux_label.hide()
        self.__check_show_states()

    def update_auxiliary_text(self, text, show):
        attrs = PangoAttrList(text.attributes, text.text)

        if show:
            self.show_auxiliary_text()
        else:
            self.hide_auxiliary_text()

        self.__aux_string = text.text
        self.__aux_label.set_text(text.text)
        self.__aux_attrs = attrs
        self.__aux_label.set_attributes(attrs)

    def __refresh_labels(self):
        labels = self.__lookup_table.get_labels()
        if labels:
            labels = map(lambda x: (x.text, PangoAttrList(x.attributes, x.text)), labels)
        else:
            labels = None
        self.__candidate_area.set_labels(labels)


    def __refresh_candidates(self):
        candidates = self.__lookup_table.get_candidates_in_current_page()
        candidates = map(lambda x: (x.text, PangoAttrList(x.attributes, x.text)), candidates)
        self.__candidate_area.set_candidates(candidates,
                self.__lookup_table.get_cursor_pos_in_current_page(),
                self.__lookup_table.is_cursor_visible()
                )

    def update_lookup_table(self, lookup_table, visible):
        # hide lookup table
        if not visible:
            self.hide_lookup_table()

        self.__lookup_table = lookup_table or ibus.LookupTable()
        orientation = self.__lookup_table.get_orientation()
        if orientation not in (ibus.ORIENTATION_HORIZONTAL, ibus.ORIENTATION_VERTICAL):
            orientation = self.__orientation
        self.set_current_orientation(orientation)
        self.__refresh_candidates()
        self.__refresh_labels()

        # show lookup table
        if visible:
            self.show_lookup_table()

    def show_lookup_table(self):
        self.__lookup_table_visible = True
        self.__candidate_area.set_no_show_all(False)
        self.__candidate_area.show_all()
        self.__check_show_states()

    def hide_lookup_table(self):
        self.__lookup_table_visible = False
        self.__candidate_area.hide_all()
        self.__candidate_area.set_no_show_all(True)
        self.__check_show_states()

    def page_up_lookup_table(self):
        self.__lookup_table.page_up()
        self.__refresh_candidates()

    def page_down_lookup_table(self):
        self.__lookup_table.page_down()
        self.__refresh_candidates()

    def cursor_up_lookup_table(self):
        self.__lookup_table.cursor_up()
        self.__refresh_candidates()

    def cursor_down_lookup_table(self):
        self.__lookup_table.cursor_down()
        self.__refresh_candidates()

    def set_cursor_location(self, x, y, w, h):
        # if cursor location is changed, we reset the moved cursor location
        if self.__cursor_location != (x, y, w, h):
            self.__cursor_location = (x, y, w, h)
            self.__moved_cursor_location = None
            self.__check_position()

    def __check_show_states(self):
        if self.__preedit_visible or \
            self.__aux_string_visible or \
            self.__lookup_table_visible:
            self.show_all()
            self.emit("show")
        else:
            self.hide_all()
            self.emit("hide")

    def reset(self):
        self.update_preedit_text(ibus.Text(""), 0, False)
        self.update_auxiliary_text(ibus.Text(""), False)
        self.update_lookup_table(None, False)
        self.hide()

    def set_current_orientation(self, orientation):
        if self.__current_orientation == orientation:
            return
        self.__current_orientation = orientation
        self.__recreate_ui()
        if self.__toplevel.flags() & gtk.VISIBLE:
            self.show_all()

    def set_orientation(self, orientation):
        self.__orientation = orientation
        self.update_lookup_table(self.__lookup_table, self.__lookup_table_visible)

    def get_current_orientation(self):
        return self.__current_orientation

    # def do_expose_event(self, event):
    #     self.style.paint_box(self.window,
    #                 gtk.STATE_NORMAL,
    #                 gtk.SHADOW_IN,
    #                 event.area,
    #                 self,
    #                 "panel",
    #                 self.allocation.x, self.allocation.y,
    #                 self.allocation.width, self.allocation.height)

    #     gtk.VBox.do_expose_event(self, event)

    def do_size_request(self, requisition):
        gtk.VBox.do_size_request(self, requisition)
        self.__toplevel.resize(1, 1)

    def __check_position(self):
        cursor_location = self.__moved_cursor_location or self.__cursor_location

        cursor_right = cursor_location[0] + cursor_location[2]
        cursor_bottom = cursor_location[1] + cursor_location[3]

        window_right = cursor_right + self.__toplevel.allocation.width
        window_bottom = cursor_bottom + self.__toplevel.allocation.height

        screen = gdk.screen_get_default()
        monitor_num = screen.get_monitor_at_point(cursor_location[0],
                                                  cursor_location[1])
        monitor_area = screen.get_monitor_geometry(monitor_num)

        monitor_right = monitor_area.x + monitor_area.width
        monitor_bottom = monitor_area.y + monitor_area.height

        if window_right > monitor_right:
            x = monitor_right - self.__toplevel.allocation.width
        else:
            x = cursor_right

        if window_bottom > monitor_bottom:
            # move the window just above the cursor so the window and a preedit string do not overlap.
            y = cursor_location[1] - self.__toplevel.allocation.height
        else:
            y = cursor_bottom

        self.move(x, y)

    def show_all(self):
        gtk.VBox.show_all(self)
        self.__toplevel.show_all()

    def hide_all(self):
        gtk.VBox.hide_all(self)
        self.__toplevel.hide_all()

    def move(self, x, y):
        self.__toplevel.move(x, y)

if __name__ == "__main__":
    table = ibus.LookupTable()
    table.append_candidate(ibus.Text("AAA"))
    table.append_candidate(ibus.Text("BBB"))
    cp = CandidatePanel()
    cp.show_all()
    cp.update_lookup_table(table, True)
    gtk.main()
