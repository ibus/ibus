# vim:set et sts=4 sw=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2014 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2007-2014 Red Hat, Inc.
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

import locale
import functools
import sys

from gi.repository import GObject
from gi.repository import Gtk
from gi.repository import IBus
from gi.repository import Pango

from icon import load_icon
from i18n import _, N_

class EngineComboBox(Gtk.ComboBox):
    __gtype_name__ = 'EngineComboBox'
    __gproperties__ = {
        'active-engine' : (
            object,
            'selected engine',
            'selected engine',
            GObject.ParamFlags.READABLE)
    }

    def __init__(self):
        super(EngineComboBox, self).__init__()
        self.connect("notify::active", self.__notify_active_cb)

        self.__model = None

        renderer = Gtk.CellRendererPixbuf()
        renderer.set_property("xalign", 0)
        renderer.set_property("xpad", 2)
        self.pack_start(renderer, False)
        self.set_cell_data_func(renderer, self.__icon_cell_data_cb, None)

        renderer = Gtk.CellRendererText()
        renderer.set_property("xalign", 0)
        renderer.set_property("xpad", 2)
        self.pack_start(renderer, True)
        self.set_cell_data_func(renderer, self.__name_cell_data_cb, None)

    def set_engines(self, engines):
        self.__model = Gtk.TreeStore(object)

        iter1 = self.__model.append(None)
        self.__model.set(iter1, 0, 0)
        langs = {}
        for e in engines:
            l = IBus.get_language_name(e.get_language())
            if l == None:
                l = ""
            if l not in langs:
                langs[l] = []
            langs[l].append(e)

        keys = list(langs.keys())
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

        #add "Others" to the end of the combo box
        if IBus.get_language_name("Other") in keys:
            keys.remove(IBus.get_language_name("Other"))
            keys += [IBus.get_language_name("Other")]
        for l in keys:
            iter1 = self.__model.append(None)
            self.__model.set(iter1, 0, l)
            def cmp_engine(a, b):
                if a.get_rank() == b.get_rank():
                    return locale.strcoll(a.get_longname(), b.get_longname())
                return int(b.get_rank() - a.get_rank())
            langs[l].sort(key=functools.cmp_to_key(cmp_engine))
            for e in langs[l]:
                iter2 = self.__model.append(iter1)
                self.__model.set(iter2, 0, e)

        self.set_model(self.__model)
        self.set_active(0)

    def __icon_cell_data_cb(self, celllayout, renderer, model, iter, data):
        engine = self.__model.get_value(iter, 0)

        if isinstance(engine, str):
            renderer.set_property("visible", False)
            renderer.set_property("sensitive", False)
        elif sys.version < '3' and isinstance (engine, unicode):
            renderer.set_property("visible", False)
            renderer.set_property("sensitive", False)
        elif isinstance(engine, int):
            renderer.set_property("visible", False)
            renderer.set_property("sensitive", False)
        else:
            renderer.set_property("visible", True)
            renderer.set_property("sensitive", True)
            pixbuf = load_icon(engine.get_icon(), Gtk.IconSize.LARGE_TOOLBAR)
            renderer.set_property("pixbuf", pixbuf)

    def __name_cell_data_cb(self, celllayout, renderer, model, iter, data):
        engine = self.__model.get_value(iter, 0)

        if isinstance (engine, str):
            renderer.set_property("sensitive", False)
            renderer.set_property("text", engine)
            renderer.set_property("weight", Pango.Weight.NORMAL)
        elif sys.version < '3' and isinstance (engine, unicode):
            renderer.set_property("sensitive", False)
            renderer.set_property("text", engine)
            renderer.set_property("weight", Pango.Weight.NORMAL)
        elif isinstance(engine, int):
            renderer.set_property("sensitive", True)
            renderer.set_property("text", _("Select an input method"))
            renderer.set_property("weight", Pango.Weight.NORMAL)
        else:
            renderer.set_property("sensitive", True)
            renderer.set_property("text", engine.get_longname())
            renderer.set_property("weight",
                    Pango.Weight.BOLD if engine.get_rank() > 0 else Pango.Weight.NORMAL)

    def __notify_active_cb(self, combobox, property):
        self.notify("active-engine")

    def do_get_property(self, property):
        if property.name == "active-engine":
            i = self.get_active()
            if i == 0 or i == -1:
                return None
            iter = self.get_active_iter()
            return self.get_model()[iter][0]
        else:
            raise AttributeError('unknown property %s' % property.name)

    def get_active_engine(self):
        return self.get_property("active-engine")

if __name__ == "__main__":
    combo = EngineComboBox()
    combo.set_engines([IBus.EngineDesc(language="zh")])
    w = Gtk.Window()
    w.add(combo)
    w.show_all()
    Gtk.main()
