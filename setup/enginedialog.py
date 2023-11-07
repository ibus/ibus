# vim:set et sts=4 sw=4:
# -*- coding: utf-8 -*-
#
# ibus - The Input Bus
#
# Copyright (c) 2015 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2015-2023 Takao Fujiwara <takao.fujiwara1@gmail.com>
# Copyright (c) 2013-2015 Red Hat, Inc.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.

# This file is ported from
# gnome-control-center/panels/region/cc-input-chooser.c

from gi.repository import Gtk
from gi.repository import GLib
from gi.repository import IBus

import functools
import gettext
import i18n
import locale

from icon import load_icon
from i18n import _, N_

ROW_TRAVEL_DIRECTION_NONE,      \
ROW_TRAVEL_DIRECTION_FORWARD,   \
ROW_TRAVEL_DIRECTION_BACKWARD = list(range(3))

class EngineDialog(Gtk.Dialog):
    __gtype_name__ = 'EngineDialog'
    __initial_languages = [ IBus.get_language_name('en_US'),
                            IBus.get_language_name('en_GB'),
                            IBus.get_language_name('de_DE'),
                            IBus.get_language_name('fr_FR'),
                            IBus.get_language_name('es_ES'),
                            IBus.get_language_name('zh_CN'),
                            IBus.get_language_name('ja_JP'),
                            IBus.get_language_name('ru_RU'),
                            IBus.get_language_name('ar_EG') ]


    def __init__(self, transient_for = None):
        super(EngineDialog, self).__init__(
                title = _("Select an input method"),
                transient_for = transient_for,
                resizable = True)
        buttons = (_("_Cancel"), Gtk.ResponseType.CANCEL,
                   _("_Add"), Gtk.ResponseType.APPLY)
        self.add_buttons(*buttons)
        self.set_response_sensitive(Gtk.ResponseType.APPLY, False)

        self.__engines_for_lang = {}
        self.__untrans_for_lang = {}
        self.__langs = {}

        self.__scrolled = Gtk.ScrolledWindow(
                hscrollbar_policy = Gtk.PolicyType.NEVER,
                vscrollbar_policy = Gtk.PolicyType.NEVER,
                shadow_type = Gtk.ShadowType.IN,
                margin_start = 6,
                margin_end = 6,
                margin_top = 6,
                margin_bottom = 6)
        self.vbox.add(self.__scrolled)
        viewport = Gtk.Viewport()
        self.__scrolled.add(viewport)
        self.__list = Gtk.ListBox(vexpand = True,
                                  halign = Gtk.Align.FILL,
                                  valign = Gtk.Align.FILL)
        viewport.add(self.__list)

        self.__adjustment = self.__scrolled.get_vadjustment()
        self.__list.set_adjustment(self.__adjustment)
        self.__list.set_filter_func(self.__list_filter, None)
        self.__list.connect('row-activated', self.__row_activated)
        self.__list.connect('row-selected', self.__row_selected)

        self.__showing_extra = False
        self.__more_row = self.__more_row_new()

        self.__filter_timeout_id = 0
        self.__filter_word = None
        self.__filter_entry = Gtk.SearchEntry(hexpand = True,
                                              margin_start = 6,
                                              margin_end = 6,
                                              margin_top = 6,
                                              margin_bottom = 6)
        self.__filter_entry.set_no_show_all(True)
        self.__filter_entry.connect('search-changed', self.__filter_changed)
        self.vbox.add(self.__filter_entry)

        self.show_all()


    def __list_filter(self, row, data):
        if row == self.__more_row:
            return not self.__showing_extra
        if not self.__showing_extra and row.is_extra:
            return False
        if self.__filter_word == None:
            return True
        if row.back:
            return True

        words = self.__filter_word.lower().strip().split()
        if all(word in row.name.lower() for word in words):
            return True
        if all(word in row.untrans.lower() for word in words):
            return True
        # Search engine name in language list
        if row.lang_info:
            if row.name in self.__engines_for_lang.keys():
                for row_l in self.__engines_for_lang[row.name]:
                    if all(word in row_l.name.lower() for word in words):
                        return True
                    if all(word in row_l.untrans.lower() for word in words):
                        return True
        # Search language name in engine list
        if not row.lang_info:
            for l in self.__engines_for_lang.keys():
                if all(word in l.lower() for word in words):
                    for row_l in self.__engines_for_lang[l]:
                        if row.name == row_l.name:
                            return True
            for (trans, untrans) in self.__untrans_for_lang.items():
                if all(word in untrans.lower() for word in words):
                    for row_l in self.__engines_for_lang[trans]:
                        if row.name == row_l.name:
                            return True
        return False


    def __row_activated(self, box, row):
        if row == self.__more_row:
            # Undo sensitive Gtk.ResponseType.APPLY button
            self.__list.unselect_row(row)
            self.__show_more()
            return
        if row.back:
            self.__show_lang_rows()
            return
        if row.lang_info:
            self.__show_engines_for_lang(row)
            return


    def __row_selected(self, box, row):
        self.set_response_sensitive(Gtk.ResponseType.APPLY, row != None)


    def __padded_label_new(self, text, icon, alignment, direction):
        hbox = Gtk.Box(orientation = Gtk.Orientation.HORIZONTAL)

        if direction == ROW_TRAVEL_DIRECTION_BACKWARD:
            rtl = (Gtk.Widget.get_default_direction() == \
                   Gtk.TextDirection.RTL)
            if rtl:
                arrow = Gtk.Image.new_from_icon_name(
                    'go-previous-rtl-symbolic', Gtk.IconSize.MENU)
            else:
                arrow = Gtk.Image.new_from_icon_name(
                    'go-previous-symbolic', Gtk.IconSize.MENU)
            hbox.pack_start(arrow, False, True, 0)

        if icon != None:
            pixbuf = load_icon(icon, Gtk.IconSize.LARGE_TOOLBAR)
            image = Gtk.Image(pixbuf = pixbuf)
            hbox.pack_start(image, False, True, 0)

        label = Gtk.Label(label = text)
        label.set_halign(alignment)
        label.set_valign(Gtk.Align.CENTER)
        label.set_margin_start(20)
        label.set_margin_end(20)
        label.set_margin_top(6)
        label.set_margin_bottom(6)
        hbox.pack_start(label, True, True, 0)
        return hbox


    def __list_box_row_new(self, text):
        row = Gtk.ListBoxRow()
        row.name = text
        row.is_extra = False
        row.lang_info = False
        row.back = False
        row.untrans = ''
        row.engine = None
        return row


    def __lang_row_new(self, text):
        row = self.__list_box_row_new(text)
        row.lang_info = True
        if len(self.__untrans_for_lang) != 0:
            row.untrans = self.__untrans_for_lang[text]
        if not self.__showing_extra and text not in self.__initial_languages:
            row.is_extra = True
        widget = self.__padded_label_new(text,
                                         None,
                                         Gtk.Align.CENTER,
                                         ROW_TRAVEL_DIRECTION_NONE)
        row.add(widget)
        return row


    def __more_row_new(self):
        row = Gtk.ListBoxRow()
        hbox = Gtk.Box(orientation = Gtk.Orientation.HORIZONTAL)
        row.add(hbox)
        row.set_tooltip_text(_("More…"))
        arrow = Gtk.Image.new_from_icon_name('view-more-symbolic',
                                             Gtk.IconSize.MENU)
        arrow.set_margin_start(20)
        arrow.set_margin_end(20)
        arrow.set_margin_top(6)
        arrow.set_margin_bottom(6)
        arrow.set_halign(Gtk.Align.CENTER)
        arrow.set_valign(Gtk.Align.CENTER)
        hbox.pack_start(arrow, True, True, 0)
        return row


    def __back_row_new(self, text):
        row = self.__list_box_row_new(text)
        row.lang_info = True
        row.back = True
        widget = self.__padded_label_new(text,
                                         None,
                                         Gtk.Align.CENTER,
                                         ROW_TRAVEL_DIRECTION_BACKWARD)
        row.add(widget)
        return row


    def __engine_row_new(self, engine):
        longname = i18n.gettext_engine_longname(engine)
        description = i18n.gettext_engine_description(engine)
        row = self.__list_box_row_new(longname)
        row.untrans = engine.get_longname()
        row.rank = engine.get_rank()
        row.set_tooltip_text(description)
        row.engine = engine
        widget = self.__padded_label_new(longname,
                                         engine.get_icon(),
                                         Gtk.Align.START,
                                         ROW_TRAVEL_DIRECTION_NONE)
        row.add(widget)
        return row


    def __set_fixed_size(self):
        if self.__scrolled.get_policy()[0] == Gtk.PolicyType.AUTOMATIC:
            return
        (width, height) = self.get_size()
        self.set_size_request(width, height)
        self.__scrolled.set_policy(Gtk.PolicyType.AUTOMATIC,
                                   Gtk.PolicyType.AUTOMATIC)


    def __remove_all_children(self):
        for l in self.__list.get_children():
            self.__list.remove(l)


    def __add_engine_rows_for_lang(self, row):
        lang = row.name

        def cmp_engine(a, b):
            if a.rank == b.rank:
                return locale.strcoll(a.name, b.name)
            return int(b.rank - a.rank)

        self.__engines_for_lang[lang].sort(
                key = functools.cmp_to_key(cmp_engine))
        for row in self.__engines_for_lang[lang]:
            self.__list.add(row)


    def __show_lang_rows(self):
        self.__remove_all_children()
        for lang in self.__langs:
            row = self.__lang_row_new(lang)
            self.__list.add(row)
        self.__list.add(self.__more_row)
        self.__list.show_all()
        self.__adjustment.set_value(self.__adjustment.get_lower())
        self.__list.invalidate_filter()
        self.__list.set_selection_mode(Gtk.SelectionMode.SINGLE)


    def __show_more(self):
        self.__set_fixed_size()
        self.__filter_entry.show()
        # Get focus after clicking on the view-more row:
        self.__filter_entry.grab_focus_without_selecting()
        self.__showing_extra = True
        self.__list.invalidate_filter()


    def __show_engines_for_lang(self, row):
        text = row.name
        self.__set_fixed_size()
        self.__remove_all_children()
        row = self.__back_row_new(text)
        self.__list.add(row)
        self.__add_engine_rows_for_lang(row)
        self.__list.show_all()
        self.__adjustment.set_value(self.__adjustment.get_lower())


    def __do_filter(self):
        text = self.__filter_entry.get_text()
        if text == '':
            self.__filter_word = None
        else:
            self.__filter_word = text
        self.__list.invalidate_filter()
        self.__filter_timeout_id = 0
        return False


    def __filter_changed(self, entry):
        if self.__filter_timeout_id == 0:
            self.__filter_timeout_id = GLib.timeout_add(150, self.__do_filter)


    def set_engines(self, engines):
        self.__engines_for_lang = {}
        self.__untrans_for_lang = {}
        for e in engines:
            l = IBus.get_language_name(e.get_language())
            if l == None:
                l = ''
            if l not in self.__engines_for_lang:
                self.__engines_for_lang[l] = []
            i18n.init_textdomain(e.get_textdomain())
            row = self.__engine_row_new(e)
            self.__engines_for_lang[l].append(row)

            # Retrieve Untranslated language names.
            untrans = IBus.get_untranslated_language_name(e.get_language())
            if untrans == None:
                untrans = ''
            self.__untrans_for_lang[l] = untrans

        keys = list(self.__engines_for_lang.keys())
        keys.sort(key=functools.cmp_to_key(locale.strcoll))
        loc = locale.getlocale()[0]
        # None on C locale
        if loc == None or loc == 'C':
            loc = 'en_US'
        current_lang = IBus.get_language_name(loc)
        # move current language to the first place
        if current_lang in keys:
            keys.remove(current_lang)
            keys.insert(0, current_lang)

        # move English to the second place
        en_lang = IBus.get_language_name('en_US')
        if en_lang != current_lang and en_lang in keys:
            keys.remove(en_lang)
            keys.insert(1, en_lang)

        #add 'Others' to the end of the combo box
        if IBus.get_language_name('Other') in keys:
            keys.remove(IBus.get_language_name('Other'))
            keys += [IBus.get_language_name('Other')]

        self.__langs = keys
        self.__show_lang_rows()


    def get_selected_engine(self):
        row = self.__list.get_selected_row()
        if row == None:
            return None
        return row.engine
