# vim:set et sts=4 sw=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2010 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2007-2010 Red Hat, Inc.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this program; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place, Suite 330,
# Boston, MA  02111-1307  USA

import gettext
import os
import signal
import sys
import time
import gtk
import gobject
import pango
import ibus
import keyboardshortcut
import locale
from os import path
from xdg import BaseDirectory
from gtk import gdk
from enginecombobox import EngineComboBox
from enginetreeview import EngineTreeView
from engineabout import EngineAbout
from i18n import DOMAINNAME, _, N_, init as i18n_init

(
    COLUMN_NAME,
    COLUMN_ENABLE,
    COLUMN_PRELOAD,
    COLUMN_VISIBLE,
    COLUMN_ICON,
    COLUMN_DATA,
) = range(6)

(
    DATA_NAME,
    DATA_LOCAL_NAME,
    DATA_LANG,
    DATA_ICON,
    DATA_AUTHOR,
    DATA_CREDITS,
    DATA_EXEC,
    DATA_STARTED,
    DATA_PRELOAD
) = range(9)

class Setup(object):
    def __flush_gtk_events(self):
        while gtk.events_pending():
            gtk.main_iteration()

    def __init__(self):
        super(Setup, self).__init__()
        gtk_builder_file = path.join(path.dirname(__file__), "./setup.ui")
        self.__builder = gtk.Builder()
        self.__builder.set_translation_domain(DOMAINNAME)
        self.__builder.add_from_file(gtk_builder_file);
        self.__bus = None
        self.__init_bus()
        self.__init_ui()

    def __init_ui(self):
        # add icon search path
        self.__window = self.__builder.get_object("window_preferences")
        self.__window.connect("delete-event", gtk.main_quit)

        self.__button_close = self.__builder.get_object("button_close")
        self.__button_close.connect("clicked", gtk.main_quit)

        # auto start ibus
        self.__checkbutton_auto_start = self.__builder.get_object("checkbutton_auto_start")
        self.__checkbutton_auto_start.set_active(self.__is_auto_start())
        self.__checkbutton_auto_start.connect("toggled", self.__checkbutton_auto_start_toggled_cb)

        # keyboard shortcut
        # trigger
        self.__config = self.__bus.get_config()
        shortcuts = self.__config.get_value(
                        "general/hotkey", "trigger",
                        ibus.CONFIG_GENERAL_SHORTCUT_TRIGGER_DEFAULT)
        button = self.__builder.get_object("button_trigger")
        entry = self.__builder.get_object("entry_trigger")
        entry.set_text("; ".join(shortcuts))
        entry.set_tooltip_text("\n".join(shortcuts))
        button.connect("clicked", self.__shortcut_button_clicked_cb,
                    N_("trigger"), "general/hotkey", "trigger", entry)

        # enable (unconditional)
        shortcuts = self.__config.get_value(
                        "general/hotkey", "enable_unconditional",
                        ibus.CONFIG_GENERAL_SHORTCUT_ENABLE_DEFAULT)
        button = self.__builder.get_object("button_enable")
        entry = self.__builder.get_object("entry_enable")
        entry.set_text("; ".join(shortcuts))
        entry.set_tooltip_text("\n".join(shortcuts))
        button.connect("clicked", self.__shortcut_button_clicked_cb,
                    N_("enable"), "general/hotkey", "enable_unconditional", entry)

        # disable (unconditional)
        shortcuts = self.__config.get_value(
                        "general/hotkey", "disable_unconditional",
                        ibus.CONFIG_GENERAL_SHORTCUT_DISABLE_DEFAULT)
        button = self.__builder.get_object("button_disable")
        entry = self.__builder.get_object("entry_disable")
        entry.set_text("; ".join(shortcuts))
        entry.set_tooltip_text("\n".join(shortcuts))
        button.connect("clicked", self.__shortcut_button_clicked_cb,
                    N_("disable"), "general/hotkey", "disable_unconditional", entry)

        # next engine
        shortcuts = self.__config.get_value(
                        "general/hotkey", "next_engine_in_menu",
                        ibus.CONFIG_GENERAL_SHORTCUT_NEXT_ENGINE_DEFAULT)
        button = self.__builder.get_object("button_next_engine")
        entry = self.__builder.get_object("entry_next_engine")
        entry.set_text("; ".join(shortcuts))
        entry.set_tooltip_text("\n".join(shortcuts))
        button.connect("clicked", self.__shortcut_button_clicked_cb,
                    N_("next input method"), "general/hotkey", "next_engine_in_menu", entry)

        # prev engine
        shortcuts = self.__config.get_value(
                        "general/hotkey", "previous_engine",
                        ibus.CONFIG_GENERAL_SHORTCUT_PREV_ENGINE_DEFAULT)
        button = self.__builder.get_object("button_prev_engine")
        entry = self.__builder.get_object("entry_prev_engine")
        entry.set_text("; ".join(shortcuts))
        entry.set_tooltip_text("\n".join(shortcuts))
        button.connect("clicked", self.__shortcut_button_clicked_cb,
                    N_("previous input method"), "general/hotkey", "previous_engine", entry)

        # lookup table orientation
        self.__combobox_lookup_table_orientation = self.__builder.get_object("combobox_lookup_table_orientation")
        self.__combobox_lookup_table_orientation.set_active(
            self.__config.get_value("panel", "lookup_table_orientation", 0))
        self.__combobox_lookup_table_orientation.connect("changed",
            self.__combobox_lookup_table_orientation_changed_cb)

        # auto hide
        self.__combobox_panel_show = self.__builder.get_object("combobox_panel_show")
        self.__combobox_panel_show.set_active(
            self.__config.get_value("panel", "show", 0))
        self.__combobox_panel_show.connect("changed", self.__combobox_panel_show_changed_cb)

        # panel position
        self.__combobox_panel_position = self.__builder.get_object("combobox_panel_position")
        self.__combobox_panel_position.set_active(
            self.__config.get_value("panel", "position", 3))
        self.__combobox_panel_position.connect("changed", self.__combobox_panel_position_changed_cb)

        # custom font
        self.__checkbutton_custom_font = self.__builder.get_object("checkbutton_custom_font")
        self.__checkbutton_custom_font.set_active(
            self.__config.get_value("panel", "use_custom_font", False))
        self.__checkbutton_custom_font.connect("toggled", self.__checkbutton_custom_font_toggled_cb)

        self.__fontbutton_custom_font = self.__builder.get_object("fontbutton_custom_font")
        if self.__config.get_value("panel", "use_custom_font", False):
            self.__fontbutton_custom_font.set_sensitive(True)
        else:
            self.__fontbutton_custom_font.set_sensitive(False)
        font_name = gtk.settings_get_default().get_property("gtk-font-name")
        font_name = unicode(font_name, "utf-8")
        font_name = self.__config.get_value("panel", "custom_font", font_name)
        self.__fontbutton_custom_font.connect("notify::font-name", self.__fontbutton_custom_font_notify_cb)
        self.__fontbutton_custom_font.set_font_name(font_name)

        # show icon on system tray
        self.__checkbutton_show_icon_on_systray = self.__builder.get_object("checkbutton_show_icon_on_systray")
        self.__checkbutton_show_icon_on_systray.set_active(
            self.__config.get_value("panel", "show_icon_on_systray", True))
        self.__checkbutton_show_icon_on_systray.connect("toggled", self.__checkbutton_show_icon_on_systray_toggled_cb)

        # show ime name
        self.__checkbutton_show_im_name = self.__builder.get_object("checkbutton_show_im_name")
        self.__checkbutton_show_im_name.set_active(
            self.__config.get_value("panel", "show_im_name", False))
        self.__checkbutton_show_im_name.connect("toggled", self.__checkbutton_show_im_name_toggled_cb)

        # embed preedit text
        self.__checkbutton_embed_preedit_text = self.__builder.get_object("checkbutton_embed_preedit_text")
        self.__checkbutton_embed_preedit_text.set_active(
            self.__config.get_value("general", "embed_preedit_text", True))
        self.__checkbutton_embed_preedit_text.connect("toggled", self.__checkbutton_embed_preedit_text_toggled_cb)

        # use system keyboard layout setting
        self.__checkbutton_use_sys_layout = self.__builder.get_object("checkbutton_use_sys_layout")
        self.__checkbutton_use_sys_layout.set_active(
            self.__config.get_value("general", "use_system_keyboard_layout", True))
        self.__checkbutton_use_sys_layout.connect("toggled", self.__checkbutton_use_sys_layout_toggled_cb)

        # use global ime setting
        self.__checkbutton_use_global_engine = self.__builder.get_object("checkbutton_use_global_engine")
        self.__checkbutton_use_global_engine.set_active(
            self.__config.get_value("general", "use_global_engine", False))
        self.__checkbutton_use_global_engine.connect("toggled", self.__checkbutton_use_global_engine_toggled_cb)

        # init engine page
        self.__engines = self.__bus.list_engines()
        self.__combobox = self.__builder.get_object("combobox_engines")
        self.__combobox.set_engines(self.__engines)

        tmp_dict = {}
        for e in self.__engines:
            tmp_dict[e.name] = e
        engine_names = self.__config.get_value("general", "preload_engines", [])
        engines = [tmp_dict[name] for name in engine_names if name in tmp_dict]

        self.__treeview = self.__builder.get_object("treeview_engines")
        self.__treeview.set_engines(engines)

        button = self.__builder.get_object("button_engine_add")
        button.connect("clicked", self.__button_engine_add_cb)
        button = self.__builder.get_object("button_engine_remove")
        button.connect("clicked", lambda *args:self.__treeview.remove_engine())
        button = self.__builder.get_object("button_engine_up")
        button.connect("clicked", lambda *args:self.__treeview.move_up_engine())

        button = self.__builder.get_object("button_engine_down")
        button.connect("clicked", lambda *args:self.__treeview.move_down_engine())

        button = self.__builder.get_object("button_engine_about")
        button.connect("clicked", self.__button_engine_about_cb)

        self.__engine_setup_exec_list = {}
        button = self.__builder.get_object("button_engine_preferences")
        button.connect("clicked", self.__button_engine_preferences_cb)

        self.__combobox.connect("notify::active-engine", self.__combobox_notify_active_engine_cb)
        self.__treeview.connect("notify", self.__treeview_notify_cb)

    def __combobox_notify_active_engine_cb(self, combobox, property):
        engine = self.__combobox.get_active_engine()
        button = self.__builder.get_object("button_engine_add")
        button.set_sensitive(engine != None and engine not in self.__treeview.get_engines())

    def __get_engine_setup_exec_args(self, engine):
        args = []
        if engine == None:
           return args
        setup = str(engine.setup)
        if len(setup) != 0:
            args = setup.split()
            args.insert(1, path.basename(args[0]))
            return args
        name = str(engine.name)
        libexecdir = os.environ['IBUS_LIBEXECDIR']
        setup_path = (libexecdir + '/' + 'ibus-setup-' if libexecdir != None \
            else 'ibus-setup-') + name.split(':')[0]
        if path.exists(setup_path):
            args.append(setup_path)
            args.append(path.basename(setup_path))
        return args

    def __treeview_notify_cb(self, treeview, property):
        if property.name != "active-engine" and property.name != "engines":
            return

        engines = self.__treeview.get_engines()
        engine = self.__treeview.get_active_engine()

        self.__builder.get_object("button_engine_remove").set_sensitive(engine != None)
        self.__builder.get_object("button_engine_about").set_sensitive(engine != None)
        self.__builder.get_object("button_engine_up").set_sensitive(engine not in engines[:1])
        self.__builder.get_object("button_engine_down").set_sensitive(engine not in engines[-1:])

        obj = self.__builder.get_object("button_engine_preferences")
        if len(self.__get_engine_setup_exec_args(engine)) != 0:
            obj.set_sensitive(True)
        else:
            obj.set_sensitive(False)

        if property.name == "engines":
            engine_names = map(lambda e: e.name, engines)
            self.__config.set_list("general", "preload_engines", engine_names, "s")

    def __button_engine_add_cb(self, button):
        engine = self.__combobox.get_active_engine()
        self.__treeview.append_engine(engine)

    def __button_engine_about_cb(self, button):
        engine = self.__treeview.get_active_engine()
        if engine:
            about = EngineAbout(engine)
            about.run()
            about.destroy()

    def __button_engine_preferences_cb(self, button):
        engine = self.__treeview.get_active_engine()
        args = self.__get_engine_setup_exec_args(engine)
        if len(args) == 0:
            return
        name = engine.name
        if name in self.__engine_setup_exec_list.keys():
            try:
                wpid, sts = os.waitpid(self.__engine_setup_exec_list[name],
                                       os.WNOHANG)
                # the setup is still running.
                if wpid == 0:
                    return
            except OSError:
                pass
            del self.__engine_setup_exec_list[name]
        self.__engine_setup_exec_list[name] = os.spawnl(os.P_NOWAIT, *args)

    def __init_bus(self):
        try:
            self.__bus = ibus.Bus()
            # self.__bus.connect("config-value-changed", self.__config_value_changed_cb)
            # self.__bus.connect("config-reloaded", self.__config_reloaded_cb)
            # self.__bus.config_add_watch("/general")
            # self.__bus.config_add_watch("/general/hotkey")
            # self.__bus.config_add_watch("/panel")
        except:
            while self.__bus == None:
                message = _("The IBus daemon is not running. Do you wish to start it?")
                dlg = gtk.MessageDialog(type = gtk.MESSAGE_QUESTION,
                        buttons = gtk.BUTTONS_YES_NO,
                        message_format = message)
                id = dlg.run()
                dlg.destroy()
                self.__flush_gtk_events()
                if id != gtk.RESPONSE_YES:
                    sys.exit(0)
                pid = os.spawnlp(os.P_NOWAIT, "ibus-daemon", "ibus-daemon", "--xim")
                time.sleep(1)
                try:
                    self.__bus = ibus.Bus()
                except:
                    continue
                message = _("IBus has been started! "
                    "If you cannot use IBus, add the following lines to your $HOME/.bashrc; then relog into your desktop.\n"
                    "  export GTK_IM_MODULE=ibus\n"
                    "  export XMODIFIERS=@im=ibus\n"
                    "  export QT_IM_MODULE=ibus"
                    )
                dlg = gtk.MessageDialog(type = gtk.MESSAGE_INFO,
                                        buttons = gtk.BUTTONS_OK,
                                        message_format = message)
                id = dlg.run()
                dlg.destroy()
                self.__flush_gtk_events()

    def __shortcut_button_clicked_cb(self, button, name, section, _name, entry):
        buttons = (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_OK, gtk.RESPONSE_OK)
        title = _("Select keyboard shortcut for %s") %  _(name)
        dialog = keyboardshortcut.KeyboardShortcutSelectionDialog(buttons = buttons, title = title)
        text = entry.get_text()
        if text:
            shortcuts = text.split("; ")
        else:
            shortcuts = None
        dialog.set_shortcuts(shortcuts)
        id = dialog.run()
        shortcuts = dialog.get_shortcuts()
        dialog.destroy()
        if id != gtk.RESPONSE_OK:
            return
        self.__config.set_list(section, _name, shortcuts, "s")
        text = "; ".join(shortcuts)
        entry.set_text(text)
        entry.set_tooltip_text(text)


    def __item_started_column_toggled_cb(self, cell, path_str, model):

        # get toggled iter
        iter = model.get_iter_from_string(path_str)
        data = model.get_value(iter, COLUMN_DATA)

        # do something with the value
        if data[DATA_STARTED] == False:
            try:
                self.__bus.register_start_engine(data[DATA_LANG], data[DATA_NAME])
            except Exception, e:
                dlg = gtk.MessageDialog(type = gtk.MESSAGE_ERROR,
                        buttons = gtk.BUTTONS_CLOSE,
                        message_format = str(e))
                dlg.run()
                dlg.destroy()
                self.__flush_gtk_events()
                return
        else:
            try:
                self.__bus.register_stop_engine(data[DATA_LANG], data[DATA_NAME])
            except Exception, e:
                dlg = gtk.MessageDialog(type = gtk.MESSAGE_ERROR,
                        buttons = gtk.BUTTONS_CLOSE,
                        message_format = str(e))
                dlg.run()
                dlg.destroy()
                self.__flush_gtk_events()
                return
        data[DATA_STARTED] = not data[DATA_STARTED]

        # set new value
        model.set(iter, COLUMN_ENABLE, data[DATA_STARTED])

    def __item_preload_column_toggled_cb(self, cell, path_str, model):

        # get toggled iter
        iter = model.get_iter_from_string(path_str)
        data = model.get_value(iter, COLUMN_DATA)

        data[DATA_PRELOAD] = not data[DATA_PRELOAD]
        engine = "%s:%s" % (data[DATA_LANG], data[DATA_NAME])

        if data[DATA_PRELOAD]:
            if engine not in self.__preload_engines:
                self.__preload_engines.add(engine)
                self.__config.set_list("general", "preload_engines", list(self.__preload_engines), "s")
        else:
            if engine in self.__preload_engines:
                self.__preload_engines.remove(engine)
                self.__config.set_list("general", "preload_engines", list(self.__preload_engines), "s")

        # set new value
        model.set(iter, COLUMN_PRELOAD, data[DATA_PRELOAD])

    def __is_auto_start(self):
        link_file = path.join(BaseDirectory.xdg_config_home, "autostart/ibus.desktop")
        ibus_desktop = path.join(os.getenv("IBUS_PREFIX"), "share/applications/ibus.desktop")

        if not path.exists(link_file):
            return False
        if not path.islink(link_file):
            return False
        if path.realpath(link_file) != ibus_desktop:
            return False
        return True

    def __checkbutton_auto_start_toggled_cb(self, button):
        auto_start_dir = path.join(BaseDirectory.xdg_config_home, "autostart")
        if not path.isdir(auto_start_dir):
            os.makedirs(auto_start_dir)

        link_file = path.join(BaseDirectory.xdg_config_home, "autostart/ibus.desktop")
        ibus_desktop = path.join(os.getenv("IBUS_PREFIX"), "share/applications/ibus.desktop")
        # unlink file
        try:
            os.unlink(link_file)
        except:
            pass
        if self.__checkbutton_auto_start.get_active():
            os.symlink(ibus_desktop, link_file)

    def __combobox_lookup_table_orientation_changed_cb(self, combobox):
        self.__config.set_value(
            "panel", "lookup_table_orientation",
            self.__combobox_lookup_table_orientation.get_active())

    def __combobox_panel_show_changed_cb(self, combobox):
        self.__config.set_value(
            "panel", "show",
            self.__combobox_panel_show.get_active())

    def __combobox_panel_position_changed_cb(self, combobox):
        self.__config.set_value(
            "panel", "position",
            self.__combobox_panel_position.get_active())

    def __checkbutton_custom_font_toggled_cb(self, button):
        if self.__checkbutton_custom_font.get_active():
            self.__fontbutton_custom_font.set_sensitive(True)
            self.__config.set_value("panel", "use_custom_font", True)
        else:
            self.__fontbutton_custom_font.set_sensitive(False)
            self.__config.set_value("panel", "use_custom_font", False)

    def __fontbutton_custom_font_notify_cb(self, button, arg):
        font_name = self.__fontbutton_custom_font.get_font_name()
        font_name = unicode(font_name, "utf-8")
        self.__config.set_value("panel", "custom_font", font_name)

    def __checkbutton_show_icon_on_systray_toggled_cb(self, button):
        value = self.__checkbutton_show_icon_on_systray.get_active()
        self.__config.set_value("panel", "show_icon_on_systray", value)

    def __checkbutton_show_im_name_toggled_cb(self, button):
        value = self.__checkbutton_show_im_name.get_active()
        self.__config.set_value("panel", "show_im_name", value)

    def __checkbutton_embed_preedit_text_toggled_cb(self, button):
        value = self.__checkbutton_embed_preedit_text.get_active()
        self.__config.set_value("general", "embed_preedit_text", value)

    def __checkbutton_use_sys_layout_toggled_cb(self, button):
        value = self.__checkbutton_use_sys_layout.get_active()
        self.__config.set_value("general", "use_system_keyboard_layout", value)

    def __checkbutton_use_global_engine_toggled_cb(self, button):
        value = self.__checkbutton_use_global_engine.get_active()
        self.__config.set_value("general", "use_global_engine", value)

    def __config_value_changed_cb(self, bus, section, name, value):
        pass

    def __config_reloaded_cb(self, bus):
        pass

    def __sigusr1_cb(self, *args):
        self.__window.present()

    def run(self):
        self.__window.show_all()
        signal.signal(signal.SIGUSR1, self.__sigusr1_cb)
        gtk.main()

if __name__ == "__main__":
    locale.setlocale(locale.LC_ALL, '')
    i18n_init()
    setup = Setup()
    setup.run()
