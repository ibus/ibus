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

__all__ = (
    "KeyboardShortcutSelection",
    "KeyboardShortcutSelectionDialog",
);

from gi.repository import Gdk
from gi.repository import GObject
from gi.repository import Gtk
from gi.repository import IBus
from gi.repository import Pango

from i18n import _, N_

MAX_HOTKEY = 6

class KeyboardShortcutSelection(Gtk.Box):
    def __init__(self, shortcuts = None):
        super(KeyboardShortcutSelection, self).__init__(
                orientation=Gtk.Orientation.VERTICAL)
        self.__init_ui()
        self.set_shortcuts(shortcuts)

    def __init_ui(self):
        # label = Gtk.Label(_("Keyboard shortcuts:"))
        # label.set_justify(Gtk.Justification.LEFT)
        # label.set_alignment(0.0, 0.5)
        # self.pack_start(label, False, True, 4)

        # shortcuts view
        self.__shortcut_view = Gtk.TreeView(
                model = Gtk.ListStore(GObject.TYPE_STRING))
        renderer = Gtk.CellRendererText()
        column = Gtk.TreeViewColumn(_("Keyboard shortcuts"), renderer, text = 0)
        self.__shortcut_view.append_column(column)
        self.__shortcut_view.connect("cursor-changed", self.__shortcut_view_cursor_changed_cb)
        scrolledwindow = Gtk.ScrolledWindow()
        scrolledwindow.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        scrolledwindow.set_min_content_height(100)
        scrolledwindow.add(self.__shortcut_view)
        scrolledwindow.set_shadow_type(Gtk.ShadowType.IN)
        self.pack_start(scrolledwindow, True, True, 4)

        # key code
        hbox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        label = Gtk.Label(label = _("Key code:"))
        label.set_justify(Gtk.Justification.LEFT)
        label.set_alignment(0.0, 0.5)
        hbox.pack_start(label, False, True, 4)

        self.__keycode_entry = Gtk.Entry()
        self.__keycode_entry.connect("notify::text", self.__keycode_entry_notify_cb)
        hbox.pack_start(self.__keycode_entry, True, True, 4)
        self.__keycode_button = Gtk.Button(label = "...")
        self.__keycode_button.connect("clicked", self.__keycode_button_clicked_cb)
        hbox.pack_start(self.__keycode_button, False, True, 4)
        self.pack_start(hbox, False, True, 4)

        # modifiers
        hbox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        label = Gtk.Label(label = _("Modifiers:"))
        label.set_justify(Gtk.Justification.LEFT)
        label.set_alignment(0.0, 0.5)
        hbox.pack_start(label, False, True, 4)

        table = Gtk.Table(n_rows = 4, n_columns = 2)
        self.__modifier_buttons = []
        self.__modifier_buttons.append(("Control",
                                        Gtk.CheckButton.new_with_mnemonic("_Control"),
                                        Gdk.ModifierType.CONTROL_MASK))
        self.__modifier_buttons.append(("Alt",
                                        Gtk.CheckButton.new_with_mnemonic("A_lt"),
                                        Gdk.ModifierType.MOD1_MASK))
        self.__modifier_buttons.append(("Shift",
                                        Gtk.CheckButton.new_with_mnemonic("_Shift"),
                                        Gdk.ModifierType.SHIFT_MASK))
        self.__modifier_buttons.append(("Meta",
                                        Gtk.CheckButton.new_with_mnemonic("_Meta"),
                                        Gdk.ModifierType.META_MASK))
        self.__modifier_buttons.append(("Super",
                                        Gtk.CheckButton.new_with_mnemonic("S_uper"),
                                        Gdk.ModifierType.SUPER_MASK))
        self.__modifier_buttons.append(("Hyper",
                                        Gtk.CheckButton.new_with_mnemonic("_Hyper"),
                                        Gdk.ModifierType.HYPER_MASK))
        # <CapsLock> is not parsed by gtk_accelerator_parse()
        # <Release> is not supported by XIGrabKeycode()
        for name, button, mask in self.__modifier_buttons:
            button.connect("toggled", self.__modifier_button_toggled_cb, name)

        table.attach(self.__modifier_buttons[0][1], 0, 1, 0, 1)
        table.attach(self.__modifier_buttons[1][1], 1, 2, 0, 1)
        table.attach(self.__modifier_buttons[2][1], 2, 3, 0, 1)
        table.attach(self.__modifier_buttons[3][1], 0, 1, 1, 2)
        table.attach(self.__modifier_buttons[4][1], 1, 2, 1, 2)
        table.attach(self.__modifier_buttons[5][1], 2, 3, 1, 2)
        hbox.pack_start(table, True, True, 4)
        self.pack_start(hbox, False, True, 4)

        # buttons
        hbox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        # add button
        self.__add_button = Gtk.Button(label = _("_Add"),
                                       use_underline = True)
        self.__add_button.set_sensitive(False)
        self.__add_button.connect("clicked", self.__add_button_clicked_cb)
        hbox.pack_start(self.__add_button, False, True, 0)
        # apply button
        self.__apply_button = Gtk.Button(label = _("_Apply"),
                                         use_underline = True)
        self.__apply_button.set_sensitive(False)
        self.__apply_button.connect("clicked", self.__apply_button_clicked_cb)
        hbox.pack_start(self.__apply_button, False, True, 0)
        # delete button
        self.__delete_button = Gtk.Button(label = _("_Delete"),
                                          use_underline = True)
        self.__delete_button.set_sensitive(False)
        self.__delete_button.connect("clicked", self.__delete_button_clicked_cb)
        hbox.pack_start(self.__delete_button, False, True, 0)
        self.pack_start(hbox, False, True, 4)

    def set_shortcuts(self, shortcuts = None):
        if shortcuts == None:
            shortcuts = []
        model = self.__shortcut_view.get_model()
        model.clear()

        added = []
        for shortcut in shortcuts:
            if shortcut not in added:
                it = model.insert(0)
                model[it][0] = shortcut
                added.append(shortcut)

    def get_shortcuts(self):
        model = self.__shortcut_view.get_model()
        try:
            return [i[0] for i in model]
        except:
            return []

    def add_shortcut(self, shortcut):
        model = self.__shortcut_view.get_model()
        if len(model) > MAX_HOTKEY:
            return
        if shortcut in self.get_shortcuts():
            return
        it = model.insert(0)
        model[it][0] = shortcut
        self.__add_button.set_sensitive(False)
        path = model.get_path(it)
        self.__shortcut_view.set_cursor(path, None, False)

    def __get_shortcut_from_buttons(self):
        modifiers = []
        keycode = self.__keycode_entry.get_text()
        if Gdk.keyval_from_name(keycode) == 0:
            return None

        for name, button, mask in self.__modifier_buttons:
            if button.get_active():
                modifiers.append(name)
        if keycode.startswith("_"):
            keycode = keycode[1:]
        shortcut = "".join(['<' + m + '>' for m in modifiers])
        shortcut += keycode
        return shortcut

    def __set_shortcut_to_buttons(self, shortcut):
        (keyval, state) = Gtk.accelerator_parse(shortcut)
        if keyval == 0 and state == 0:
            return
        for name, button, mask in self.__modifier_buttons:
            if state & mask:
                button.set_active(True)
            else:
                button.set_active(False)
        self.__keycode_entry.set_text(shortcut.rsplit('>', 1)[-1])

    def __get_selected_shortcut(self):
        model = self.__shortcut_view.get_model()
        path, column = self.__shortcut_view.get_cursor()
        if path == None:
            return None
        return model[path.get_indices()[0]][0]

    def __set_selected_shortcut(self, shortcut):
        model = self.__shortcut_view.get_model()
        path, column = self.__shortcut_view.get_cursor()
        model[path[0]][0] = shortcut
        self.__update_add_and_apply_buttons()

    def __del_selected_shortcut(self):
        model = self.__shortcut_view.get_model()
        path, column = self.__shortcut_view.get_cursor()
        model.remove(model.get_iter(path))
        self.__update_add_and_apply_buttons()

    def __shortcut_view_cursor_changed_cb(self, treeview):
        shortcut = self.__get_selected_shortcut()
        if shortcut != None:
            self.__set_shortcut_to_buttons(shortcut)
            self.__delete_button.set_sensitive(True)
        else:
            self.__delete_button.set_sensitive(False)

    def __update_add_and_apply_buttons(self):
        shortcut = self.__get_shortcut_from_buttons()
        selected_shortcut = self.__get_selected_shortcut()
        shortcuts = self.get_shortcuts()
        can_add = shortcut != None and \
                  shortcut not in shortcuts \
                  and len(shortcuts) < MAX_HOTKEY
        self.__add_button.set_sensitive(can_add)
        can_apply = shortcut != selected_shortcut and \
                    shortcut != None and \
                    selected_shortcut != None and \
                    shortcut not in shortcuts
        self.__apply_button.set_sensitive(can_apply)

    def __modifier_button_toggled_cb(self, button, name):
        self.__update_add_and_apply_buttons()

    def __keycode_entry_notify_cb(self, entry, arg):
        self.__update_add_and_apply_buttons()

    def __keycode_button_clicked_cb(self, button):
        out = []
        dlg = Gtk.MessageDialog(transient_for = self.get_toplevel(),
                                buttons = Gtk.ButtonsType.CLOSE)
        message = _("Please press a key (or a key combination).\n" \
                    "The dialog will be closed when the key is released.")
        dlg.set_markup(message)
        dlg.set_title(_("Please press a key (or a key combination)"))
        sw = Gtk.ScrolledWindow()

        def __accel_edited_cb(c, path, keyval, state, keycode):
            out.append(keyval)
            out.append(state)
            out.append(keycode)
            dlg.response(Gtk.ResponseType.OK)

        model = Gtk.ListStore(GObject.TYPE_INT,
                              GObject.TYPE_UINT,
                              GObject.TYPE_UINT)
        accel_view = Gtk.TreeView(model = model)
        accel_view.set_headers_visible(False)
        sw.add(accel_view)
        sw.set_min_content_height(30)
        column = Gtk.TreeViewColumn()
        renderer = Gtk.CellRendererAccel(accel_mode=Gtk.CellRendererAccelMode.OTHER,
                                         editable=True)
        renderer.connect('accel-edited', __accel_edited_cb)
        column.pack_start(renderer, True)
        column.add_attribute(renderer, 'accel-mods', 0)
        column.add_attribute(renderer, 'accel-key', 1)
        column.add_attribute(renderer, 'keycode', 2)
        accel_view.append_column(column)
        it = model.append(None)
        area = dlg.get_message_area()
        area.pack_end(sw, True, True, 0)
        sw.show_all()
        id = dlg.run()
        dlg.destroy()
        if id != Gtk.ResponseType.OK or len(out) < 3:
            return
        keyval = out[0]
        state = out[1]
        keycode = out[2]

        for name, button, mask in self.__modifier_buttons:
            if state & mask:
                button.set_active(True)
            else:
                button.set_active(False)

        shortcut = Gtk.accelerator_name_with_keycode(None,
                                                     keyval,
                                                     keycode,
                                                     state)
        shortcut = shortcut.replace('<Primary>', '<Control>')
        self.__keycode_entry.set_text(shortcut.rsplit('>', 1)[-1])

    def __add_button_clicked_cb(self, button):
        shortcut = self.__get_shortcut_from_buttons()
        self.add_shortcut(shortcut)

    def __apply_button_clicked_cb(self, button):
        shortcut = self.__get_shortcut_from_buttons()
        self.__set_selected_shortcut(shortcut)

    def __delete_button_clicked_cb(self, button):
        self.__del_selected_shortcut()
        self.__delete_button.set_sensitive(False)
        self.__apply_button.set_sensitive(False)

class KeyboardShortcutSelectionDialog(Gtk.Dialog):
    def __init__(self, title = None, transient_for = None, flags = 0):
        super(KeyboardShortcutSelectionDialog, self).__init__(
                title = title, transient_for = transient_for, flags = flags)
        self.__selection_view = KeyboardShortcutSelection()
        self.vbox.pack_start(self.__selection_view, False, True, 0)
        self.vbox.show_all()

    def set_shortcuts(self, shotrcuts = None):
        self.__selection_view.set_shortcuts(shotrcuts)

    def add_shortcut(self, shotrcut):
        self.__selection_view.add_shortcut(shotrcut)

    def get_shortcuts(self):
        return self.__selection_view.get_shortcuts()



if __name__ == "__main__":
    dlg = KeyboardShortcutSelectionDialog(title = "Select test")
    buttons = (_("_Cancel"), Gtk.ResponseType.CANCEL,
               _("_OK"), Gtk.ResponseType.OK)
    dlg.add_buttons(buttons)
    dlg.add_shortcut("Control+Shift+space")
    dlg.set_shortcuts(None)
    print((dlg.run()))
    print((dlg.get_shortcuts()))

