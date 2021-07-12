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

import gtk
import gtk.gdk as gdk
import glib
import gobject
import ibus
import icon as _icon
import os
import sys
import signal
from os import path
from ibus import interface
from languagebar import LanguageBar
from candidatepanel import CandidatePanel
from engineabout import EngineAbout

from i18n import _, N_

ICON_KEYBOARD = ibus.get_ICON_KEYBOARD()
ICON_ENGINE = "ibus-engine"

def show_uri(screen, link):
    try:
        gtk.show_uri(screen, link, 0)
    except:
        print >> sys.stderr, "pygtk do not support show_uri"

def url_hook(about, link, user_data):
    show_uri(about.get_screen(), link)

def email_hook(about, email, user_data):
    show_uri(about.get_screen(), "mailto:%s" % email)

gtk.about_dialog_set_url_hook(url_hook, None)
gtk.about_dialog_set_email_hook(email_hook, None)

class Panel(ibus.PanelBase):
    __gtype_name__ = "IBusPanel"
    def __init__(self, bus):
        super(Panel, self).__init__(bus)
        self.__bus = bus
        self.__config = self.__bus.get_config()
        self.__focus_ic = None
        self.__setup_pid = None
        self.__prefix = os.getenv("IBUS_PREFIX")
        self.__data_dir = path.join(self.__prefix, "share", "ibus")
        # self.__icons_dir = path.join(self.__data_dir, "icons")
        self.__setup_cmd = path.join(self.__prefix, "bin", "ibus-setup")

        # connect bus signal
        self.__config.connect("value-changed", self.__config_value_changed_cb)
        self.__config.connect("reloaded", self.__config_reloaded_cb)
        # self.__bus.config_add_watch("panel")

        # add icon search path
        # icon_theme = gtk.icon_theme_get_default()
        # icon_theme.prepend_search_path(self.__icons_dir)

        self.__language_bar = LanguageBar()
        self.__language_bar.connect("property-activate",
                        lambda widget, prop_name, prop_state: self.property_activate(prop_name, prop_state))
        self.__language_bar.connect("get-im-menu",
                        self.__get_im_menu_cb)
        self.__language_bar.connect("show-engine-about",
                        self.__show_engine_about_cb)
        self.__language_bar.connect("position-changed",
                        self.__position_changed_cb)
        self.__language_bar.focus_out()
        self.__language_bar.show_all()

        self.__candidate_panel = CandidatePanel()
        self.__candidate_panel.connect("cursor-up",
                        lambda widget: self.cursor_up())
        self.__candidate_panel.connect("cursor-down",
                        lambda widget: self.cursor_down())
        self.__candidate_panel.connect("page-up",
                        lambda widget: self.page_up())
        self.__candidate_panel.connect("page-down",
                        lambda widget: self.page_down())
        self.__candidate_panel.connect("candidate-clicked",
                        lambda widget, index, button, state: self.candidate_clicked(index, button, state))


        self.__status_icon = gtk.StatusIcon()
        # gnome-shell checks XClassHint.res_class with ShellTrayIcon.
        # gtk_status_icon_set_name() can set XClassHint.res_class .
        # However gtk_status_icon_new() also calls gtk_window_realize() so
        # gtk_status_icon_set_visible() needs to be called to set WM_CLASS
        # so that gtk_window_realize() is called later again.
        # set_title is for gnome-shell notificationDaemon in bottom right.
        self.__status_icon.set_visible(False)
        # gtk_status_icon_set_name() is not available in pygtk2 2.17
        if hasattr(self.__status_icon, 'set_name'):
            self.__status_icon.set_name('ibus-ui-gtk')
        self.__status_icon.set_title(_("IBus Panel"))
        # Hide icon until bus get the name owner.
        #self.__status_icon.set_visible(True)
        self.__status_icon.connect("popup-menu", self.__status_icon_popup_menu_cb)
        self.__status_icon.connect("activate", self.__status_icon_activate_cb)
        self.__status_icon.set_from_icon_name(ICON_KEYBOARD)
        self.__status_icon.set_tooltip(_("IBus input method framework"))
        # Hide icon until bus get the name owner.
        #self.__status_icon.set_visible(True)

        self.__config_load_lookup_table_orientation()
        self.__config_load_show()
        self.__config_load_position()
        self.__config_load_custom_font()
        self.__config_load_custom_theme()
        # Hide icon until bus get the name owner.
        #self.__config_load_show_icon_on_systray()
        self.__config_load_show_im_name()
        # self.__bus.request_name(ibus.panel.IBUS_SERVICE_PANEL, 0)

    def set_cursor_location(self, x, y, w, h):
        self.__candidate_panel.set_cursor_location(x, y, w, h)

    def update_preedit_text(self, text, cursor_pos, visible):
        self.__candidate_panel.update_preedit_text(text, cursor_pos, visible)

    def show_preedit_text(self):
        self.__candidate_panel.show_preedit_text()

    def hide_preedit_text(self):
        self.__candidate_panel.hide_preedit_text()

    def update_auxiliary_text(self, text, visible):
        self.__candidate_panel.update_auxiliary_text(text, visible)

    def show_auxiliary_text(self):
        self.__candidate_panel.show_auxiliary_text()

    def hide_auxiliary_text(self):
        self.__candidate_panel.hide_auxiliary_text()

    def update_lookup_table(self, lookup_table, visible):
        self.__candidate_panel.update_lookup_table(lookup_table, visible)

    def show_lookup_table(self):
        self.__candidate_panel.show_lookup_table()

    def hide_lookup_table(self):
        self.__candidate_panel.hide_lookup_table()

    def page_up_lookup_table(self):
        self.__candidate_panel.page_up_lookup_table()

    def page_down_lookup_table(self):
        self.__candidate_panel.page_down_lookup_table()

    def cursor_up_lookup_table(self):
        self.__candidate_panel.cursor_up_lookup_table()

    def cursor_down_lookup_table(self):
        self.__candidate_panel.cursor_down_lookup_table()

    def show_candidate_window(self):
        self.__candidate_panel.show_all()

    def hide_candidate_window(self):
        self.__candidate_panel.hide_all()

    def show_language_bar(self):
        self.__language_bar.show_all()

    def hide_language_bar(self):
        self.__language_bar.hide_all()

    def register_properties(self, props):
        self.__language_bar.register_properties(props)

    def update_property(self, prop):
        self.__language_bar.update_property(prop)

    def get_status_icon(self):
        return self.__status_icon

    def hide(self):
        if self.__status_icon == None:
            return
        self.__status_icon.set_visible(False)

    def show(self):
        if self.__status_icon == None:
            return
        self.__config_load_show_icon_on_systray()

    def __set_im_icon(self, icon_name):
        if not icon_name:
            icon_name = ICON_ENGINE
        self.__language_bar.set_im_icon(icon_name)
        if icon_name.startswith("/"):
            self.__status_icon.set_from_file(icon_name)
        else:
            self.__status_icon.set_from_icon_name(icon_name)

    def __set_im_name(self, name):
        self.__language_bar.set_im_name(name)

    def focus_in(self, ic):
        self.reset()
        self.__focus_ic = ibus.InputContext(self.__bus, ic)
        enabled = True or self.__focus_ic.is_enabled()
        self.__language_bar.set_enabled(enabled)

        if not enabled:
            self.__set_im_icon(ICON_KEYBOARD)
            self.__set_im_name(None)
        else:
            engine = self.__focus_ic.get_engine()
            if engine:
                self.__set_im_icon(engine.icon)
                self.__set_im_name(engine.longname)
            else:
                self.__set_im_icon(ICON_KEYBOARD)
                self.__set_im_name(None)
        self.__language_bar.focus_in()

    def focus_out(self, ic):
        self.reset()
        self.__focus_ic = None
        self.__language_bar.set_enabled(False)
        self.__language_bar.focus_out()
        self.__set_im_icon(ICON_KEYBOARD)
        self.__set_im_name(None)

    def state_changed(self):
        if not self.__focus_ic:
            return

        enabled = self.__focus_ic.is_enabled()
        self.__language_bar.set_enabled(enabled)

        if enabled == False:
            self.reset()
            self.__set_im_icon(ICON_KEYBOARD)
            self.__set_im_name(None)
        else:
            engine = self.__focus_ic.get_engine()
            if engine:
                self.__set_im_icon(engine.icon)
                self.__set_im_name(engine.longname)
            else:
                self.__set_im_icon(ICON_KEYBOARD)
                self.__set_im_name(None)


    def reset(self):
        self.__candidate_panel.reset()
        self.__language_bar.reset()

    def start_setup(self):
        self.__start_setup()

    def do_destroy(self):
        gtk.main_quit()

    def __config_load_lookup_table_orientation(self):
        value = self.__config.get_value("panel", "lookup_table_orientation", 0)
        if value in (ibus.ORIENTATION_HORIZONTAL, ibus.ORIENTATION_VERTICAL):
            orientation = value
        else:
            orientation = ibus.ORIENTATION_HORIZONTAL
        self.__candidate_panel.set_orientation(orientation)

    def __config_load_show(self):
        show = self.__config.get_value("panel", "show", 0)
        self.__language_bar.set_show(show)

    def __config_load_position(self):
        x = self.__config.get_value("panel", "x", -1)
        y = self.__config.get_value("panel", "y", -1)
        self.__language_bar.set_position(x, y)

    def __config_load_custom_font(self):
        use_custom_font = self.__config.get_value("panel", "use_custom_font", False)
        font_name = gtk.settings_get_default().get_property("gtk-font-name")
        font_name = unicode(font_name, "utf-8")
        custom_font =  self.__config.get_value("panel", "custom_font", font_name)
        style_string = 'style "custom-font" { font_name="%s" }\n' \
            'class "IBusCandidateLabel" style "custom-font"\n'
        if use_custom_font:
            style_string = style_string % custom_font
            gtk.rc_parse_string(style_string)
        else:
            style_string = style_string % ""
            gtk.rc_parse_string(style_string)

        settings = gtk.settings_get_default()
        gtk.rc_reset_styles(settings)

    def __config_load_custom_theme(self):
        use_custom_theme = self.__config.get_value("panel", "use_custom_theme", False)
        custom_theme =  self.__config.get_value("panel", "custom_theme", "")
        theme_name = unicode(custom_theme, "utf-8")
        settings = gtk.settings_get_default()
        if use_custom_theme:
            settings.set_string_property('gtk-theme-name', theme_name, '')
        else:
            gtk.rc_reparse_all_for_settings(settings, False)

    def __config_load_show_icon_on_systray(self):
        value = self.__config.get_value("panel", "show_icon_on_systray", True)
        self.__status_icon.set_visible(True if value else False)

    def __config_load_show_im_name(self):
        value = self.__config.get_value("panel", "show_im_name", False)
        self.__language_bar.set_show_im_name(value)

    def __config_value_changed_cb(self, bus, section, name, value):
        if section != "panel":
            return
        if name == "lookup_table_orientation":
            self.__config_load_lookup_table_orientation()
        elif name == "show":
            self.__config_load_show()
        elif name == "use_custom_font" or name == "custom_font":
            self.__config_load_custom_font()
        elif name == "use_custom_theme" or name == "custom_theme":
            self.__config_load_custom_theme()
        elif name == "show_icon_on_systray":
            self.__config_load_show_icon_on_systray()
        elif name == "show_im_name":
            self.__config_load_show_im_name()
        elif name == "x" or name == "y":
            pass
        else:
            print >> sys.stderr, "Unknown config item [%s]" % name

    def __config_reloaded_cb(self, bus):
        pass

    def __create_sys_menu(self):
        menu = gtk.Menu()
        item = gtk.ImageMenuItem(gtk.STOCK_PREFERENCES)
        item.connect("activate",
            self.__sys_menu_item_activate_cb, gtk.STOCK_PREFERENCES)
        menu.add(item)
        item = gtk.ImageMenuItem(gtk.STOCK_ABOUT)
        item.connect("activate",
            self.__sys_menu_item_activate_cb, gtk.STOCK_ABOUT)
        menu.add(item)
        menu.add(gtk.SeparatorMenuItem())
        item = gtk.MenuItem(_("Restart"))
        item.connect("activate",
            self.__sys_menu_item_activate_cb, "Restart")
        menu.add(item)
        item = gtk.ImageMenuItem(gtk.STOCK_QUIT)
        item.connect("activate",
            self.__sys_menu_item_activate_cb, gtk.STOCK_QUIT)
        menu.add(item)

        menu.show_all()
        menu.set_take_focus(False)
        return menu

    # def __create_im_menu(self):
    #     menu = gtk.Menu()
    #     engines = self.__bus.list_active_engines()

    #     tmp = {}
    #     for engine in engines:
    #         lang = ibus.get_language_name(engine.language)
    #         if lang not in tmp:
    #             tmp[lang] = []
    #         tmp[lang].append(engine)

    #     langs = tmp.keys()
    #     other = tmp.get(_("Other"), [])
    #     if _("Other") in tmp:
    #         langs.remove(_("Other"))
    #         langs.append(_("Other"))

    #     size = gtk.icon_size_lookup(gtk.ICON_SIZE_MENU)
    #     for lang in langs:
    #         if len(tmp[lang]) == 1:
    #             engine = tmp[lang][0]
    #             item = gtk.ImageMenuItem("%s - %s" % (lang, engine.longname))
    #             if engine.icon:
    #                 item.set_image(_icon.IconWidget(engine.icon, size[0]))
    #             else:
    #                 item.set_image(_icon.IconWidget(ICON_ENGINE, size[0]))
    #             item.connect("activate", self.__im_menu_item_activate_cb, engine)
    #             menu.add(item)
    #         else:
    #             item = gtk.MenuItem(lang)
    #             menu.add(item)
    #             submenu = gtk.Menu()
    #             item.set_submenu(submenu)
    #             for engine in tmp[lang]:
    #                 item = gtk.ImageMenuItem(engine.longname)
    #                 if engine.icon:
    #                     item.set_image(_icon.IconWidget(engine.icon, size[0]))
    #                 else:
    #                     item.set_image(_icon.IconWidget(ICON_ENGINE, size[0]))
    #                 item.connect("activate", self.__im_menu_item_activate_cb, engine)
    #                 submenu.add(item)

    #     item = gtk.ImageMenuItem(_("Turn off input method"))
    #     item.set_image(_icon.IconWidget("gtk-close", size[0]))
    #     item.connect("activate", self.__im_menu_item_activate_cb, None)
    #     menu.add(item)

    #     menu.show_all()
    #     menu.set_take_focus(False)
    #     return menu

    def __create_im_menu(self):
        # FIXME
        # engines = self.__bus.list_engines()
        names = self.__config.get_value("general", "preload_engines",
                ["xkb:us::eng", "xkb:us:intl:eng", "pinyin"])
        engines = self.__bus.get_engines_by_names(names)
        current_engine = \
            (self.__focus_ic != None and self.__focus_ic.get_engine()) or \
            (engines and engines[0]) or \
            None

        size = gtk.icon_size_lookup(gtk.ICON_SIZE_MENU)
        menu = gtk.Menu()
        for i, engine in enumerate(engines):
            lang = ibus.get_language_name(engine.language)
            item = gtk.ImageMenuItem("%s - %s" % (lang, engine.longname))
            if current_engine and current_engine.name == engine.name:
                for widget in item.get_children():
                    if isinstance(widget, gtk.Label):
                        widget.set_markup("<b>%s</b>" % widget.get_text())
            if engine.icon:
                item.set_image(_icon.IconWidget(engine.icon, size[0]))
            else:
                item.set_image(_icon.IconWidget(ICON_ENGINE, size[0]))
            item.connect("activate", self.__im_menu_item_activate_cb, engine)
            menu.add(item)

        item = gtk.ImageMenuItem(_("Turn off input method"))
        item.set_image(_icon.IconWidget("gtk-close", size[0]))
        item.connect("activate", self.__im_menu_item_activate_cb, None)
        if self.__focus_ic == None:
            item.set_sensitive(False)
        menu.add(item)

        menu.show_all()
        menu.set_take_focus(False)
        return menu

    def __get_im_menu_cb(self, languagebar):
        menu = self.__create_im_menu()
        return menu

    def __show_engine_about_cb(self, langagebar):
        try:
            engine = self.__focus_ic.get_engine()
            dlg = EngineAbout(engine)
            dlg.run()
            dlg.destroy()
        except:
            pass

    def __position_changed_cb(self, langagebar, x, y):
        self.__config.set_value("panel", "x", x)
        self.__config.set_value("panel", "y", y)

    def __status_icon_popup_menu_cb(self, status_icon, button, active_time):
        menu = self.__create_sys_menu()
        menu.popup(None, None,
                gtk.status_icon_position_menu,
                button,
                active_time,
                self.__status_icon)

    def __status_icon_activate_cb(self, status_icon):
        if not self.__focus_ic:
            menu = gtk.Menu()
            item = gtk.ImageMenuItem(_("No input window"))
            size = gtk.icon_size_lookup(gtk.ICON_SIZE_MENU)
            item.set_image(_icon.IconWidget("gtk-dialog-info", size[0]))
            menu.add(item)
            menu.show_all()
        else:
            menu = self.__create_im_menu()
            self.__language_bar.create_im_menu(menu)
        menu.popup(None, None,
                gtk.status_icon_position_menu,
                0,
                gtk.get_current_event_time(),
                self.__status_icon)

    def __im_menu_item_activate_cb(self, item, engine):
        if not self.__focus_ic:
            return
        if engine:
            self.__focus_ic.set_engine(engine)
        else:
            self.__focus_ic.disable()

    def __sys_menu_item_activate_cb(self, item, command):
        if command == gtk.STOCK_PREFERENCES:
            self.__start_setup()
        elif command == gtk.STOCK_ABOUT:
            about_dialog = gtk.AboutDialog()
            about_dialog.set_program_name("IBus")
            about_dialog.set_version(ibus.get_version())
            about_dialog.set_copyright(ibus.get_copyright())
            about_dialog.set_license(ibus.get_license())
            about_dialog.set_comments(_("IBus is an intelligent input bus for Linux/Unix."))
            about_dialog.set_website("http://code.google.com/p/ibus")
            about_dialog.set_authors(["Peng Huang <shawn.p.huang@gmail.com>"])
            about_dialog.set_documenters(["Peng Huang <shawn.p.huang@gmail.com>"])
            about_dialog.set_translator_credits(_("translator-credits"))
            about_dialog.set_logo_icon_name("ibus")
            about_dialog.set_icon_name("ibus")
            about_dialog.run()
            about_dialog.destroy()
        elif command == gtk.STOCK_QUIT:
            self.__bus.exit(False)
        elif command == "Restart":
            self.__bus.exit(True)
        else:
            print >> sys.stderr, "Unknown command %s" % command

    def __child_watch_cb(self, pid, status):
        if self.__setup_pid == pid:
            self.__setup_pid.close()
            self.__setup_pid = None

    def __start_setup(self):
        if self.__setup_pid != None:
            try:
                # if setup dialog is running, bring the dialog to front by SIGUSR1
                os.kill(self.__setup_pid, signal.SIGUSR1)
                return
            except OSError:
                # seems the setup dialog is not running anymore
                self.__setup_pid.close()
                self.__setup_pid = None

        pid = glib.spawn_async(argv=[self.__setup_cmd, "ibus-setup"],
                               flags=glib.SPAWN_DO_NOT_REAP_CHILD)[0]
        self.__setup_pid = pid
        glib.child_watch_add(self.__setup_pid, self.__child_watch_cb)
