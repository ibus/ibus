/* vim:set et sts=4 sw=4:
 *
 * ibus - The Input Bus
 *
 * Copyright(c) 2011-2014 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright(c) 2015-2020 Takao Fujwiara <takao.fujiwara1@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

class Panel : IBus.PanelService {

    private enum IconType {
        STATUS_ICON,
        INDICATOR,
    }

    private IBus.Bus m_bus;
    private GLib.Settings m_settings_general = null;
    private GLib.Settings m_settings_hotkey = null;
    private GLib.Settings m_settings_panel = null;
    private IconType m_icon_type = IconType.STATUS_ICON;
    private Indicator m_indicator;
#if INDICATOR
    private GLib.DBusConnection m_session_bus_connection;
#endif
    private Gtk.StatusIcon m_status_icon;
    private Gtk.Menu m_ime_menu;
    private Gtk.Menu m_sys_menu;
    private IBus.EngineDesc[] m_engines = {};
    private GLib.HashTable<string, IBus.EngineDesc> m_engine_contexts =
            new GLib.HashTable<string, IBus.EngineDesc>(GLib.str_hash,
                                                        GLib.str_equal);
    private string m_current_context_path = "";
    private string m_real_current_context_path = "";
    private bool m_use_global_engine = true;
    private bool m_use_engine_lang = true;
    private CandidatePanel m_candidate_panel;
    private Switcher m_switcher;
    private uint m_switcher_focus_set_engine_id;
    private PropertyManager m_property_manager;
    private PropertyPanel m_property_panel;
    private GLib.Pid m_setup_pid = 0;
    private Gtk.AboutDialog m_about_dialog;
    private Gtk.CssProvider m_css_provider;
    private int m_switcher_delay_time = 400;
    private bool m_use_system_keyboard_layout = false;
    private GLib.HashTable<string, Gdk.Pixbuf> m_xkb_icon_pixbufs =
            new GLib.HashTable<string, Gdk.Pixbuf>(GLib.str_hash,
                                                   GLib.str_equal);
    private GLib.HashTable<string, Cairo.ImageSurface> m_xkb_icon_image =
            new GLib.HashTable<string, Cairo.ImageSurface>(GLib.str_hash,
                                                           GLib.str_equal);
    private Gdk.RGBA m_xkb_icon_rgba = Gdk.RGBA(){
            red = 0.0, green = 0.0, blue = 0.0, alpha = 1.0 };
    private XKBLayout m_xkblayout = new XKBLayout();
    private bool inited_engines_order = true;
    private uint m_preload_engines_id;
    private const uint PRELOAD_ENGINES_DELAY_TIME = 30000;
    private string m_icon_prop_key = "";
    private int m_property_icon_delay_time = 500;
    private uint m_property_icon_delay_time_id;
#if INDICATOR
    private bool m_is_kde = is_kde();
#else
    private bool m_is_kde = false;
#endif
    private ulong m_popup_menu_id;
    private ulong m_activate_id;
    private ulong m_registered_status_notifier_item_id;

    private GLib.List<BindingCommon.Keybinding> m_keybindings =
            new GLib.List<BindingCommon.Keybinding>();

    public Panel(IBus.Bus bus) {
        GLib.assert(bus.is_connected());
        // Chain up base class constructor
        GLib.Object(connection : bus.get_connection(),
                    object_path : "/org/freedesktop/IBus/Panel");

        m_bus = bus;

        init_settings();

        // init ui
#if INDICATOR
        if (m_is_kde) {
            init_indicator();
        } else {
            init_status_icon();
        }
#else
        init_status_icon();
#endif

        m_candidate_panel = new CandidatePanel();
        m_candidate_panel.page_up.connect((w) => this.page_up());
        m_candidate_panel.page_down.connect((w) => this.page_down());
        m_candidate_panel.cursor_up.connect((w) => this.cursor_up());
        m_candidate_panel.cursor_down.connect((w) => this.cursor_down());
        m_candidate_panel.candidate_clicked.connect(
                (w, i, b, s) => this.candidate_clicked(i, b, s));

        m_switcher = new Switcher();
        // The initial shortcut is "<Super>space"
        bind_switch_shortcut();

        if (m_switcher_delay_time >= 0) {
            m_switcher.set_popup_delay_time((uint) m_switcher_delay_time);
        }

        m_property_manager = new PropertyManager();
        m_property_manager.property_activate.connect((w, k, s) => {
            property_activate(k, s);
        });

        m_property_panel = new PropertyPanel();
        m_property_panel.property_activate.connect((w, k, s) => {
            property_activate(k, s);
        });

        state_changed();
    }

    ~Panel() {
#if INDICATOR
        // unref m_indicator
        if (m_indicator != null)
            m_indicator.unregister_connection();
#endif
        BindingCommon.unbind_switch_shortcut(
                BindingCommon.KeyEventFuncType.ANY, m_keybindings);
        m_keybindings = null;
    }

    private void init_settings() {
        m_settings_general = new GLib.Settings("org.freedesktop.ibus.general");
        m_settings_hotkey =
                new GLib.Settings("org.freedesktop.ibus.general.hotkey");
        m_settings_panel = new GLib.Settings("org.freedesktop.ibus.panel");

        m_settings_general.changed["preload-engines"].connect((key) => {
                update_engines(m_settings_general.get_strv(key),
                               null);
        });

        m_settings_general.changed["switcher-delay-time"].connect((key) => {
                set_switcher_delay_time();
        });

        m_settings_general.changed["use-system-keyboard-layout"].connect(
            (key) => {
                set_use_system_keyboard_layout();
        });

        m_settings_general.changed["embed-preedit-text"].connect((key) => {
                set_embed_preedit_text();
        });

        m_settings_general.changed["use-global-engine"].connect((key) => {
                set_use_global_engine();
        });

        m_settings_general.changed["use-xmodmap"].connect((key) => {
                set_use_xmodmap();
        });

        m_settings_hotkey.changed["triggers"].connect((key) => {
                BindingCommon.unbind_switch_shortcut(
                        BindingCommon.KeyEventFuncType.IME_SWITCHER,
                        m_keybindings);
                m_keybindings = null;
                bind_switch_shortcut();
        });

        m_settings_panel.changed["custom-font"].connect((key) => {
                BindingCommon.set_custom_font(m_settings_panel,
                                              null,
                                              ref m_css_provider);
        });

        m_settings_panel.changed["use-custom-font"].connect((key) => {
                BindingCommon.set_custom_font(m_settings_panel,
                                              null,
                                              ref m_css_provider);
        });

        m_settings_panel.changed["custom-theme"].connect((key) => {
                BindingCommon.set_custom_theme(m_settings_panel);
        });

        m_settings_panel.changed["use-custom-theme"].connect((key) => {
                BindingCommon.set_custom_theme(m_settings_panel);
        });

        m_settings_panel.changed["use-glyph-from-engine-lang"].connect((key) =>
        {
                m_use_engine_lang = m_settings_panel.get_boolean(
                        "use-glyph-from-engine-lang");
                var engine = m_bus.get_global_engine();
                if (engine != null)
                    set_language_from_engine(engine);
        });

        m_settings_panel.changed["show-icon-on-systray"].connect((key) => {
                set_show_icon_on_systray();
        });

        m_settings_panel.changed["lookup-table-orientation"].connect((key) => {
                set_lookup_table_orientation();
        });

        m_settings_panel.changed["show"].connect((key) => {
                set_show_property_panel();
        });

        m_settings_panel.changed["timeout"].connect((key) => {
                set_timeout_property_panel();
        });

        m_settings_panel.changed["follow-input-cursor-when-always-shown"]
            .connect((key) => {
                set_follow_input_cursor_when_always_shown_property_panel();
        });

        m_settings_panel.changed["xkb-icon-rgba"].connect((key) => {
                set_xkb_icon_rgba();
        });

        m_settings_panel.changed["property-icon-delay-time"].connect((key) => {
                set_property_icon_delay_time();
        });
    }

    private void popup_menu_at_area_window(Gtk.Menu              menu,
                                           Gdk.Rectangle         area,
                                           Gdk.Window?           window,
                                           Gtk.MenuPositionFunc? func) {
#if VALA_0_34
        Gdk.Gravity rect_anchor = Gdk.Gravity.SOUTH_WEST;
        Gdk.Gravity menu_anchor = Gdk.Gravity.NORTH_WEST;

        // Gtk.Menu.popup() is now deprecated but
        // Gtk.Menu.popup_at_rect() requires a Gdk.Window and
        // Gtk.Menu.popup_at_rect() outputs a warning of
        // "no trigger event for menu popup"
        // for the foreigner QT window which is generated by
        // Gdk.X11.Window.foreign_for_display.
        // https://git.gnome.org/browse/gtk+/tree/gtk/gtkmenu.c?h=gtk-3-22#n2251
        menu.popup_at_rect(window, area, rect_anchor, menu_anchor, null);
#else
        menu.popup(null, null, func, 0, Gtk.get_current_event_time());
#endif
    }

#if INDICATOR
    private static bool is_kde() {
        if (Environment.get_variable("XDG_CURRENT_DESKTOP") == "KDE")
            return true;
        warning ("If you launch KDE5 on xterm, " +
                 "export XDG_CURRENT_DESKTOP=KDE before launch KDE5.");
        return false;
    }

    private void popup_menu_at_pointer_window(Gtk.Menu              menu,
                                              int                   x,
                                              int                   y,
                                              Gdk.Window?           window,
                                              Gtk.MenuPositionFunc? func) {
        int win_x = 0;
        int win_y = 0;
        window.get_origin(out win_x, out win_y);
        Gdk.Rectangle area = { x - win_x, y - win_y, 1, 1 };
        // window is a bottom wide panel instead of status icon
        popup_menu_at_area_window(menu, area, window, func);
    }

    private void init_indicator() {
        m_icon_type = IconType.INDICATOR;
        GLib.Bus.get.begin(GLib.BusType.SESSION, null, (obj, res) => {
            try {
                m_session_bus_connection = GLib.Bus.get.end(res);
                m_indicator =
                        new Indicator("ibus-ui-gtk3",
                                      m_session_bus_connection,
                                      Indicator.Category.APPLICATION_STATUS);
                m_indicator.title = _("IBus Panel");
                m_registered_status_notifier_item_id =
                        m_indicator.registered_status_notifier_item.connect(
                                () => {
                    m_indicator.set_status(Indicator.Status.ACTIVE);
                    state_changed();
                });
                m_popup_menu_id =
                        m_indicator.context_menu.connect((x, y, w, b, t) => {
                    popup_menu_at_pointer_window(
                        create_context_menu(),
                        x, y, w,
                        m_indicator.position_context_menu);
                });
                m_activate_id =
                        m_indicator.activate.connect((x, y, w) => {
                    popup_menu_at_pointer_window(
                            create_activate_menu(),
                            x, y, w,
                            m_indicator.position_activate_menu);
                });
            } catch (GLib.IOError e) {
                warning("Failed to get the session bus: %s", e.message);
            }
        });
    }
#endif

    private void init_status_icon() {
        m_status_icon = new Gtk.StatusIcon();
        m_status_icon.set_name("ibus-ui-gtk");
        m_status_icon.set_title(_("IBus Panel"));

        // Gdk.Window.get_width() is needed for the menu position
        if (m_status_icon.get_size() > 0)
            init_status_icon_menu();
        else
            m_status_icon.notify["size"].connect(init_status_icon_menu);
    }

    private void init_status_icon_menu() {
        Gdk.Rectangle area = { 0, 0, 0, 0 };
        Gdk.Window? window = null;
        Gtk.MenuPositionFunc? func = null;
#if VALA_0_34
        window = Gdk.X11.Window.lookup_for_display(
                Gdk.Display.get_default() as Gdk.X11.Display,
                m_status_icon.get_x11_window_id()) as Gdk.Window;
        if (window == null) {
            warning("StatusIcon does not have GdkWindow");
            return;
        }
        Gtk.Orientation orient;
        m_status_icon.get_geometry(null, out area, out orient);
        int win_x = 0;
        int win_y = 0;
        window.get_origin(out win_x, out win_y);
        // The (x, y) is converted by gdk_window_get_root_coords()
        // in gdk_window_impl_move_to_rect()
        area.x -= win_x;
        area.y -= win_y;
#else
        func = m_status_icon.position_menu;
#endif
        m_popup_menu_id = m_status_icon.popup_menu.connect((b, t) => {
                popup_menu_at_area_window(
                        create_context_menu(),
                        area, window, func);
        });
        m_activate_id = m_status_icon.activate.connect(() => {
                popup_menu_at_area_window(
                        create_activate_menu(),
                        area, window, func);
        });
        m_status_icon.set_from_icon_name("ibus-keyboard");
    }

    private void bind_switch_shortcut() {
        string[] accelerators = m_settings_hotkey.get_strv("triggers");

        var keybinding_manager = KeybindingManager.get_instance();

        foreach (var accelerator in accelerators) {
            BindingCommon.keybinding_manager_bind(
                    keybinding_manager,
                    ref m_keybindings,
                    accelerator,
                    BindingCommon.KeyEventFuncType.IME_SWITCHER,
                    handle_engine_switch_normal,
                    handle_engine_switch_reverse);
        }
    }

/*
    public static void
    unbind_switch_shortcut(KeyEventFuncType      ftype,
                           GLib.List<Keybinding> keybindings) {
        var keybinding_manager = KeybindingManager.get_instance();

        while (keybindings != null) {
            Keybinding keybinding = keybindings.data;

            if (ftype == KeyEventFuncType.ANY ||
                ftype == keybinding.ftype) {
                keybinding_manager.unbind(keybinding.keysym,
                                          keybinding.modifiers);
            }
            keybindings = keybindings.next;
        }
    }
*/

    /**
     * panel_get_engines_from_xkb:
     * @self: #Panel class
     * @engines: all engines from ibus_bus_list_engines()
     * @returns: ibus xkb engines
     *
     * Made ibus engines from the current XKB keymaps.
     * This returns only XKB engines whose name start with "xkb:".
     */
    private GLib.List<IBus.EngineDesc>
            get_engines_from_xkb(GLib.List<IBus.EngineDesc> engines) {
        string layouts;
        string variants;
        string option;
        XKBLayout.get_layout(out layouts, out variants, out option);

        GLib.List<IBus.EngineDesc> xkb_engines =
                new GLib.List<IBus.EngineDesc>();
        IBus.EngineDesc us_engine =
                new IBus.EngineDesc("xkb:us::eng",
                                    "", "", "", "", "", "", "");
        string[] layout_array = layouts.split(",");
        string[] variant_array = variants.split(",");

        for (int i = 0; i < layout_array.length; i++) {
            string layout = layout_array[i];
            string variant = null;
            IBus.EngineDesc current_engine = null;

            if (i < variant_array.length)
                variant = variant_array[i];

            /* If variants == "", variants.split(",") is { null }.
             * To meet engine.get_layout_variant(), convert null to ""
             * here.
             */
            if (variant == null)
                variant = "";

            foreach (unowned IBus.EngineDesc engine in engines) {

                string name = engine.get_name();
                if (!name.has_prefix("xkb:"))
                    continue;

                if (engine.get_layout() == layout &&
                    engine.get_layout_variant() == variant) {
                    current_engine = engine;
                    break;
                }
            }

            if (current_engine != null) {
                xkb_engines.append(current_engine);
            } else if (xkb_engines.find(us_engine) == null) {
                warning("Fallback %s(%s) to us layout.", layout, variant);
                xkb_engines.append(us_engine);
            }
        }

        if (xkb_engines.length() == 0)
            warning("Not found IBus XKB engines from the session.");

        return xkb_engines;
    }

    /**
     * panel_get_engines_from_locale:
     * @self: #Panel class
     * @engines: all engines from ibus_bus_list_engines()
     * @returns: ibus im engines
     *
     * Made ibus engines from the current locale and IBus.EngineDesc.lang .
     * This returns non-XKB engines whose name does not start "xkb:".
     */
    private GLib.List<IBus.EngineDesc>
            get_engines_from_locale(GLib.List<IBus.EngineDesc> engines) {
        string locale = Intl.setlocale(LocaleCategory.CTYPE, null);

        if (locale == null)
            locale = "C";

        string lang = locale.split(".")[0];
        GLib.List<IBus.EngineDesc> im_engines =
                new GLib.List<IBus.EngineDesc>();

        foreach (unowned IBus.EngineDesc engine in engines) {
            string name = engine.get_name();

            if (name.has_prefix("xkb:"))
                continue;

            if (engine.get_language() == lang &&
                engine.get_rank() > 0)
                im_engines.append(engine);
        }

        if (im_engines.length() == 0) {
            lang = lang.split("_")[0];

            foreach (unowned IBus.EngineDesc engine in engines) {
                string name = engine.get_name();

                if (name.has_prefix("xkb:"))
                    continue;

                if (engine.get_language() == lang &&
                    engine.get_rank() > 0)
                    im_engines.append(engine);
            }
        }

        if (im_engines.length() == 0)
            return im_engines;

        im_engines.sort((a, b) => {
            return (int) b.get_rank() - (int) a.get_rank();
        });

        return im_engines;
    }

    private void init_engines_order() {
        m_xkblayout.set_latin_layouts(
                m_settings_general.get_strv("xkb-latin-layouts"));

        if (inited_engines_order)
            return;

        if (m_settings_general.get_strv("preload-engines").length > 0)
            return;

        GLib.List<IBus.EngineDesc> engines = m_bus.list_engines();
        GLib.List<IBus.EngineDesc> xkb_engines = get_engines_from_xkb(engines);
        GLib.List<IBus.EngineDesc> im_engines =
                get_engines_from_locale(engines);

        string[] names = {};
        foreach (unowned IBus.EngineDesc engine in xkb_engines)
            names += engine.get_name();
        foreach (unowned IBus.EngineDesc engine in im_engines)
            names += engine.get_name();

        m_settings_general.set_strv("preload-engines", names);
    }


    private void set_switcher_delay_time() {
        m_switcher_delay_time =
                m_settings_general.get_int("switcher-delay-time");

        if (m_switcher != null && m_switcher_delay_time >= 0) {
            m_switcher.set_popup_delay_time((uint) m_switcher_delay_time);
        }
    }

    private void set_use_system_keyboard_layout() {
        m_use_system_keyboard_layout =
                m_settings_general.get_boolean("use-system-keyboard-layout");
    }

    private void set_embed_preedit_text() {
        Variant variant =
                    m_settings_general.get_value("embed-preedit-text");

        if (variant == null) {
            return;
        }

        m_bus.set_ibus_property("EmbedPreeditText", variant);
    }

    private void set_use_global_engine() {
        m_use_global_engine =
                m_settings_general.get_boolean("use-global-engine");
    }

    private void set_use_xmodmap() {
        m_xkblayout.set_use_xmodmap(
                m_settings_general.get_boolean("use-xmodmap"));
    }

    private void set_show_icon_on_systray() {
        if (m_icon_type == IconType.STATUS_ICON) {
            if (m_status_icon == null)
                return;

            m_status_icon.set_visible(
                    m_settings_panel.get_boolean("show-icon-on-systray"));
        } else if (m_icon_type == IconType.INDICATOR) {
            if (m_indicator == null)
                return;

            if (m_settings_panel.get_boolean("show-icon-on-systray"))
                m_indicator.set_status(Indicator.Status.ACTIVE);
            else
                m_indicator.set_status(Indicator.Status.PASSIVE);
        }
    }

    private void set_lookup_table_orientation() {
        if (m_candidate_panel == null)
            return;

        m_candidate_panel.set_vertical(
                m_settings_panel.get_int("lookup-table-orientation")
                == IBus.Orientation.VERTICAL);
    }

    private void set_show_property_panel() {
        if (m_property_panel == null)
            return;

        m_property_panel.set_show(m_settings_panel.get_int("show"));
    }

    private void set_timeout_property_panel() {
        if (m_property_panel == null)
            return;

        m_property_panel.set_auto_hide_timeout(
                (uint) m_settings_panel.get_int("auto-hide-timeout"));
    }

    private void set_follow_input_cursor_when_always_shown_property_panel() {
        if (m_property_panel == null)
            return;

        m_property_panel.set_follow_input_cursor_when_always_shown(
                m_settings_panel.get_boolean(
                        "follow-input-cursor-when-always-shown"));
    }

    private void set_xkb_icon_rgba() {
        string spec = m_settings_panel.get_string("xkb-icon-rgba");

        Gdk.RGBA rgba = { 0, };

        if (!rgba.parse(spec)) {
            warning("invalid format of xkb-icon-rgba: %s", spec);
            m_xkb_icon_rgba = Gdk.RGBA(){
                    red = 0.0, green = 0.0, blue = 0.0, alpha = 1.0 };
        } else
            m_xkb_icon_rgba = rgba;

        if (m_icon_type == IconType.STATUS_ICON) {
            if (m_xkb_icon_pixbufs.size() > 0) {
                m_xkb_icon_pixbufs.remove_all();

                if (m_status_icon != null && m_switcher != null)
                    state_changed();
            }
        } else if (m_icon_type == IconType.INDICATOR) {
            if (m_xkb_icon_image.size() > 0) {
                m_xkb_icon_image.remove_all();

                if (m_indicator != null && m_switcher != null)
                    state_changed();
            }
        }
    }

    private void set_property_icon_delay_time() {
        m_property_icon_delay_time =
                m_settings_panel.get_int("property-icon-delay-time");
    }


    private int compare_versions(string version1, string version2) {
        string[] version1_list = version1.split(".");
        string[] version2_list = version2.split(".");
        int major1, minor1, micro1, major2, minor2, micro2;

        if (version1 == version2) {
            return 0;
        }

        // The initial dconf value of "version" is "".
        if (version1 == "") {
            return -1;
        }
        if (version2 == "") {
            return 1;
        }

        assert(version1_list.length >= 3);
        assert(version2_list.length >= 3);

        major1 = int.parse(version1_list[0]);
        minor1 = int.parse(version1_list[1]);
        micro1 = int.parse(version1_list[2]);

        major2 = int.parse(version2_list[0]);
        minor2 = int.parse(version2_list[1]);
        micro2 = int.parse(version2_list[2]);

        if (major1 == minor1 && minor1 == minor2 && micro1 == micro2) {
            return 0;
        }
        if ((major1 > major2) ||
            (major1 == major2 && minor1 > minor2) ||
            (major1 == major2 && minor1 == minor2 &&
             micro1 > micro2)) {
            return 1;
        }
        return -1;
    }

    private void update_version_1_5_8() {
        inited_engines_order = false;
    }

    private void set_version() {
        string prev_version = m_settings_general.get_string("version");
        string current_version = null;

        if (compare_versions(prev_version, "1.5.8") < 0)
            update_version_1_5_8();

        current_version = "%d.%d.%d".printf(IBus.MAJOR_VERSION,
                                            IBus.MINOR_VERSION,
                                            IBus.MICRO_VERSION);

        if (prev_version == current_version) {
            return;
        }

        m_settings_general.set_string("version", current_version);
    }

    public void load_settings() {
        set_version();

        init_engines_order();

        // Update m_use_system_keyboard_layout before update_engines()
        // is called.
        set_use_system_keyboard_layout();
        set_use_global_engine();
        set_use_xmodmap();
        m_use_engine_lang = m_settings_panel.get_boolean(
                        "use-glyph-from-engine-lang");
        update_engines(m_settings_general.get_strv("preload-engines"),
                       m_settings_general.get_strv("engines-order"));
        BindingCommon.unbind_switch_shortcut(
                BindingCommon.KeyEventFuncType.ANY,
                m_keybindings);
        m_keybindings = null;
        bind_switch_shortcut();
        set_switcher_delay_time();
        set_embed_preedit_text();
        BindingCommon.set_custom_font(m_settings_panel,
                                      null,
                                      ref m_css_provider);
        BindingCommon.set_custom_theme(m_settings_panel);
        set_show_icon_on_systray();
        set_lookup_table_orientation();
        set_show_property_panel();
        set_timeout_property_panel();
        set_follow_input_cursor_when_always_shown_property_panel();
        set_xkb_icon_rgba();
        set_property_icon_delay_time();
    }

    /**
     * disconnect_signals:
     *
     * Call this API before m_panel = null so that the ref_count becomes 0
     */
    public void disconnect_signals() {
        unowned GLib.Object object = m_status_icon;
#if INDICATOR
        if (m_is_kde)
            object = m_indicator;
#endif
        if (m_popup_menu_id > 0) {
            // No name functions refer m_panel in m_status_icon
            if (GLib.SignalHandler.is_connected(object, m_popup_menu_id))
                object.disconnect(m_popup_menu_id);
            m_popup_menu_id = 0;
        }
        if (m_activate_id > 0) {
            if (GLib.SignalHandler.is_connected(object, m_activate_id))
                object.disconnect(m_activate_id);
            m_activate_id = 0;
        }
        if (m_registered_status_notifier_item_id > 0) {
            if (GLib.SignalHandler.is_connected(
                    object,
                    m_registered_status_notifier_item_id)) {
                object.disconnect(m_registered_status_notifier_item_id);
            }
            m_registered_status_notifier_item_id = 0;
        }
        if (m_preload_engines_id > 0) {
            GLib.Source.remove(m_preload_engines_id);
            m_preload_engines_id = 0;
        }
    }

    private void engine_contexts_insert(IBus.EngineDesc engine) {
        if (m_use_global_engine)
            return;

        if (m_engine_contexts.size() >= 200) {
            warning ("Contexts by windows are too much counted!");
            m_engine_contexts.remove_all();
        }

        m_engine_contexts.replace(m_current_context_path, engine);
    }

    private void set_language_from_engine(IBus.EngineDesc engine) {
        if (m_use_engine_lang) {
            m_candidate_panel.set_language(new Pango.AttrLanguage(
                    Pango.Language.from_string(engine.get_language())));
        } else {
            m_candidate_panel.set_language(new Pango.AttrLanguage(
                    Pango.Language.from_string(null)));
        }

    }

    private void set_engine(IBus.EngineDesc engine) {
        if (m_property_icon_delay_time_id > 0) {
            GLib.Source.remove(m_property_icon_delay_time_id);
            m_property_icon_delay_time_id = 0;
        }

        if (!m_bus.set_global_engine(engine.get_name())) {
            warning("Switch engine to %s failed.", engine.get_name());
            return;
        }
        /* Panel.update_property() will be called with a time lag
         * by another engine because of DBus delay so need to
         * clear m_icon_prop_key here to avoid wrong panel icon in
         * disabled m_use_global_engine.
         */
        m_icon_prop_key = "";

        // set xkb layout
        if (!m_use_system_keyboard_layout)
            m_xkblayout.set_layout(engine);

        set_language_from_engine(engine);
        engine_contexts_insert(engine);
    }

    private void switch_engine(int i, bool force = false) {
        GLib.assert(i >= 0 && i < m_engines.length);

        // Do not need switch
        if (i == 0 && !force)
            return;

        IBus.EngineDesc engine = m_engines[i];

        set_engine(engine);
    }

    private void handle_engine_switch_normal(Gdk.Event event) {
        handle_engine_switch(event, false);
    }

    private void handle_engine_switch_reverse(Gdk.Event event) {
        handle_engine_switch(event, true);
    }

    private void handle_engine_switch(Gdk.Event event, bool reverse) {
        // Do not need switch IME
        if (m_engines.length <= 1)
            return;

        uint keyval = event.key.keyval;
        uint modifiers = KeybindingManager.MODIFIER_FILTER & event.key.state;

        uint primary_modifiers =
            KeybindingManager.get_primary_modifier(event.key.state);

        bool pressed = KeybindingManager.primary_modifier_still_pressed(
                event, primary_modifiers);

        if (reverse) {
            modifiers &= ~Gdk.ModifierType.SHIFT_MASK;
        }

        if (pressed && m_switcher_delay_time >= 0) {
            int i = reverse ? m_engines.length - 1 : 1;

            /* The flag of m_switcher.is_running avoids the following problem:
             *
             * When an IME is chosen on m_switcher, focus_in() is called
             * for the fake input context. If an engine is set in focus_in()
             * during running m_switcher when m_use_global_engine is false,
             * state_changed() is also called and m_engines[] is modified
             * in state_changed() and m_switcher.run() returns the index
             * for m_engines[] but m_engines[] was modified by state_changed()
             * and the index is not correct. */
            i = m_switcher.run(keyval, modifiers, event, m_engines, i,
                               m_real_current_context_path);

            if (i < 0) {
                debug("switch cancelled");
            } else if (i == 0) {
                debug("do not have to switch");
            } else {
                this.switcher_focus_set_engine();
            }
        } else {
            int i = reverse ? m_engines.length - 1 : 1;
            switch_engine(i);
        }
    }


    private void run_preload_engines(IBus.EngineDesc[] engines, int index) {
        string[] names = {};

        if (engines.length <= index) {
            return;
        }

        if (m_preload_engines_id != 0) {
            GLib.Source.remove(m_preload_engines_id);
            m_preload_engines_id = 0;
        }

        names += engines[index].get_name();
        m_preload_engines_id =
                Timeout.add(
                        PRELOAD_ENGINES_DELAY_TIME,
                        () => {
                            if (!m_bus.is_connected())
                                return false;
                            m_bus.preload_engines_async.begin(names,
                                                              -1,
                                                              null);
                            return false;
                        });
    }

    private void update_engines(string[]? unowned_engine_names,
                                string[]? order_names) {
        string[]? engine_names = unowned_engine_names;

        if (engine_names == null || engine_names.length == 0)
            engine_names = {"xkb:us::eng"};

        string[] names = {};

        foreach (var name in order_names) {
            if (name in engine_names)
                names += name;
        }

        foreach (var name in engine_names) {
            if (name in names)
                continue;
            names += name;
        }

        var engines = m_bus.get_engines_by_names(names);

        /* Fedora internal patch could save engines not in simple.xml
         * likes 'xkb:cn::chi'.
         */
        if (engines.length < names.length) {
            string message1;
            if (engines.length == 0) {
                string[] fallback_names = {"xkb:us::eng"};
                m_settings_general.set_strv("preload-engines", fallback_names);
                engines = m_bus.get_engines_by_names(fallback_names);
                message1 = _("Your configured input method %s does not exist " +
                             "in IBus input methods so \"US\" layout was " +
                             "configured instead of your input method."
                            ).printf(names[0]);
            } else {
                message1 = _("At least one of your configured input methods " +
                             "does not exist in IBus input methods.");
            }
            var message2 = _("Please run `ibus-setup` command, open \"Input " +
                             "Method\" tab, and configure your input methods " +
                             "again.");
            var dialog = new Gtk.MessageDialog(
                    null,
                    Gtk.DialogFlags.DESTROY_WITH_PARENT,
                    Gtk.MessageType.WARNING,
                    Gtk.ButtonsType.CLOSE,
                    "%s %s", message1, message2);
            dialog.response.connect((id) => {
                    dialog.destroy();
            });
            dialog.show_all();
	}

        if (m_engines.length == 0) {
            m_engines = engines;
            switch_engine(0, true);
            run_preload_engines(engines, 1);
        } else {
            var current_engine = m_engines[0];
            m_engines = engines;
            int i;
            for (i = 0; i < m_engines.length; i++) {
                if (current_engine.get_name() == engines[i].get_name()) {
                    switch_engine(i);
                    if (i != 0) {
                        run_preload_engines(engines, 0);
                    } else {
                        run_preload_engines(engines, 1);
                    }
                    return;
                }
            }
            switch_engine(0, true);
            run_preload_engines(engines, 1);
        }

    }

    private void context_render_string(Cairo.Context cr,
                                       string        symbol,
                                       int           image_width,
                                       int           image_height) {
        int lwidth = 0;
        int lheight = 0;
        var desc = Pango.FontDescription.from_string("Monospace Bold 22");
        var layout = Pango.cairo_create_layout(cr);

        if (symbol.length >= 3)
            desc = Pango.FontDescription.from_string("Monospace Bold 18");

        layout.set_font_description(desc);
        layout.set_text(symbol, -1);
        layout.get_size(out lwidth, out lheight);
        cr.move_to((image_width - lwidth / Pango.SCALE) / 2,
                   (image_height - lheight / Pango.SCALE) / 2);
        cr.set_source_rgba(m_xkb_icon_rgba.red,
                           m_xkb_icon_rgba.green,
                           m_xkb_icon_rgba.blue,
                           m_xkb_icon_rgba.alpha);
        Pango.cairo_show_layout(cr, layout);
    }

    private Cairo.ImageSurface
    create_cairo_image_surface_with_string(string symbol, bool cache) {
        Cairo.ImageSurface image = null;

        if (cache) {
            image = m_xkb_icon_image[symbol];

            if (image != null)
                return image;
        }

        image = new Cairo.ImageSurface(Cairo.Format.ARGB32, 48, 48);
        var cr = new Cairo.Context(image);
        int width = image.get_width();
        int height = image.get_height();
        int stride = image.get_stride();

        cr.set_source_rgba(0.0, 0.0, 0.0, 0.0);
        cr.set_operator(Cairo.Operator.SOURCE);
        cr.paint();
        cr.set_operator(Cairo.Operator.OVER);
        context_render_string(cr, symbol, width, height);
        image.flush();

        if (m_icon_type == IconType.INDICATOR) {
            if (GLib.BYTE_ORDER == GLib.ByteOrder.LITTLE_ENDIAN) {
                unowned uint[] data = (uint[]) image.get_data();
                int length = stride * height / (int) sizeof(uint);
                for (int i = 0; i < length; i++)
                    data[i] = data[i].to_big_endian();
            }
        }

        if (cache)
            m_xkb_icon_image.insert(symbol, image);

        return image;
    }

    private Gdk.Pixbuf create_icon_pixbuf_with_string(string symbol) {
        Gdk.Pixbuf pixbuf = m_xkb_icon_pixbufs[symbol];

        if (pixbuf != null)
            return pixbuf;

        var image = create_cairo_image_surface_with_string(symbol, false);
        int width = image.get_width();
        int height = image.get_height();
        pixbuf = Gdk.pixbuf_get_from_surface(image, 0, 0, width, height);
        m_xkb_icon_pixbufs.insert(symbol, pixbuf);
        return pixbuf;
    }

    private void show_setup_dialog() {
        if (m_setup_pid != 0) {
            if (Posix.kill(m_setup_pid, Posix.Signal.USR1) == 0)
                return;
            m_setup_pid = 0;
        }

        string binary = GLib.Path.build_filename(Config.BINDIR, "ibus-setup");
        try {
            GLib.Process.spawn_async(null,
                                     {binary, "ibus-setup"},
                                     null,
                                     GLib.SpawnFlags.DO_NOT_REAP_CHILD,
                                     null,
                                     out m_setup_pid);
        } catch (GLib.SpawnError e) {
            warning("Execute %s failed! %s", binary, e.message);
            m_setup_pid = 0;
        }

        GLib.ChildWatch.add(m_setup_pid, (pid, state) => {
            if (pid != m_setup_pid)
                return;
            m_setup_pid = 0;
            GLib.Process.close_pid(pid);
        });
    }

    private void show_about_dialog() {
        if (m_about_dialog == null) {
            m_about_dialog = new Gtk.AboutDialog();
            m_about_dialog.set_program_name("IBus");
            m_about_dialog.set_version(Config.PACKAGE_VERSION);

            string copyright =
                "Copyright © 2007-2015 Peng Huang\n" +
                "Copyright © 2015-2019 Takao Fujiwara\n" +
                "Copyright © 2007-2015 Red Hat, Inc.\n";

            m_about_dialog.set_copyright(copyright);
            m_about_dialog.set_license("LGPL");
            m_about_dialog.set_comments(_("IBus is an intelligent input bus for Linux/Unix."));
            m_about_dialog.set_website("https://github.com/ibus/ibus/wiki");
            m_about_dialog.set_authors({"Peng Huang <shawn.p.huang@gmail.com>"});
            m_about_dialog.set_documenters({"Peng Huang <shawn.p.huang@gmail.com>"});
            m_about_dialog.set_translator_credits(_("translator-credits"));
            m_about_dialog.set_logo_icon_name("ibus");
            m_about_dialog.set_icon_name("ibus");
        }

        if (!m_about_dialog.get_visible()) {
            m_about_dialog.run();
            m_about_dialog.hide();
        } else {
            m_about_dialog.present();
        }
    }

    private Gtk.Menu create_context_menu() {
        // Show system menu
        if (m_sys_menu == null) {
            Gtk.MenuItem item;
            m_sys_menu = new Gtk.Menu();

            item = new Gtk.MenuItem.with_label(_("Preferences"));
            item.activate.connect((i) => show_setup_dialog());
            m_sys_menu.append(item);

#if EMOJI_DICT
            item = new Gtk.MenuItem.with_label(_("Emoji Choice"));
            item.activate.connect((i) => {
                IBus.ExtensionEvent event = new IBus.ExtensionEvent(
                        "name", "emoji", "is-enabled", true,
                        "params", "category-list");
                /* new GLib.Variant("(sv)", "emoji", event.serialize_object())
                 * will call g_variant_unref() for the child variant by vala.
                 * I have no idea not to unref the object so integrated
                 * the purpose to IBus.ExtensionEvent above.
                 */
                panel_extension(event);
            });
            m_sys_menu.append(item);
#endif

            item = new Gtk.MenuItem.with_label(_("About"));
            item.activate.connect((i) => show_about_dialog());
            m_sys_menu.append(item);

            m_sys_menu.append(new Gtk.SeparatorMenuItem());

            item = new Gtk.MenuItem.with_label(_("Restart"));
            item.activate.connect((i) => m_bus.exit(true));
            m_sys_menu.append(item);

            item = new Gtk.MenuItem.with_label(_("Quit"));
            item.activate.connect((i) => m_bus.exit(false));
            m_sys_menu.append(item);

            m_sys_menu.show_all();
        }

        return m_sys_menu;
    }

    private Gtk.Menu create_activate_menu() {
        m_ime_menu = new Gtk.Menu();

        // Show properties and IME switching menu
        m_property_manager.create_menu_items(m_ime_menu);

        m_ime_menu.append(new Gtk.SeparatorMenuItem());

        // Append IMEs
        foreach (var engine in m_engines) {
            var language = engine.get_language();
            var longname = engine.get_longname();
            var textdomain = engine.get_textdomain();
            var transname = GLib.dgettext(textdomain, longname);
            var item = new Gtk.MenuItem.with_label(
                "%s - %s".printf (IBus.get_language_name(language), transname));
            // Make a copy of engine to workaround a bug in vala.
            // https://bugzilla.gnome.org/show_bug.cgi?id=628336
            var e = engine;
            item.activate.connect((item) => {
                for (int i = 0; i < m_engines.length; i++) {
                    if (e == m_engines[i]) {
                        switch_engine(i);
                        break;
                    }
                }
            });
            m_ime_menu.add(item);
        }

        m_ime_menu.show_all();

        // Do not take focuse to avoid some focus related issues.
        m_ime_menu.set_take_focus(false);

        return m_ime_menu;
    }

    private void set_properties(IBus.PropList props) {
        int i = 0;
        while (true) {
            IBus.Property prop = props.get(i);
            if (prop == null)
                break;
            set_property(props.get(i), true);
            i++;
        }
    }

    private new void set_property(IBus.Property prop, bool all_update) {
        string symbol = prop.get_symbol().get_text();

        if (m_icon_prop_key != "" && prop.get_key() == m_icon_prop_key
            && symbol != "")
            animate_icon(symbol, all_update);
    }

    private void animate_icon(string symbol, bool all_update) {
        if (m_property_icon_delay_time < 0)
            return;

        uint timeout = 0;
        if (all_update)
            timeout = (uint) m_property_icon_delay_time;

        if (m_property_icon_delay_time_id > 0) {
            GLib.Source.remove(m_property_icon_delay_time_id);
            m_property_icon_delay_time_id = 0;
        }

        m_property_icon_delay_time_id = GLib.Timeout.add(timeout, () => {
            m_property_icon_delay_time_id = 0;

            if (m_icon_type == IconType.STATUS_ICON) {
                Gdk.Pixbuf pixbuf = create_icon_pixbuf_with_string(symbol);
                m_status_icon.set_from_pixbuf(pixbuf);
            }
            else if (m_icon_type == IconType.INDICATOR) {
                Cairo.ImageSurface image =
                        create_cairo_image_surface_with_string(symbol, true);
                m_indicator.set_cairo_image_surface_full(image, "");
            }

            return false;
        });
    }

    /* override virtual functions */
    public override void set_cursor_location(int x, int y,
                                             int width, int height) {
        m_candidate_panel.set_cursor_location(x, y, width, height);
        m_property_panel.set_cursor_location(x, y, width, height);
    }

    private bool switcher_focus_set_engine_real() {
        IBus.EngineDesc? selected_engine = m_switcher.get_selected_engine();
        string prev_context_path = m_switcher.get_input_context_path();
        if (selected_engine != null &&
            prev_context_path != "" &&
            prev_context_path == m_current_context_path) {
            set_engine(selected_engine);
            m_switcher.reset();
            return true;
        }

        return false;
    }

    private void switcher_focus_set_engine() {
        IBus.EngineDesc? selected_engine = m_switcher.get_selected_engine();
        string prev_context_path = m_switcher.get_input_context_path();
        if (selected_engine == null &&
            prev_context_path != "" &&
            m_switcher.is_running()) {
            var context = GLib.MainContext.default();
            if (m_switcher_focus_set_engine_id > 0 &&
                context.find_source_by_id(m_switcher_focus_set_engine_id)
                        != null) {
                GLib.Source.remove(m_switcher_focus_set_engine_id);
            }
            m_switcher_focus_set_engine_id = GLib.Timeout.add(100, () => {
                // focus_in is comming before switcher returns
                switcher_focus_set_engine_real();
                m_switcher_focus_set_engine_id = -1;
                return false;
            });
        } else {
            if (switcher_focus_set_engine_real()) {
                var context = GLib.MainContext.default();
                if (m_switcher_focus_set_engine_id > 0 &&
                    context.find_source_by_id(m_switcher_focus_set_engine_id)
                            != null) {
                    GLib.Source.remove(m_switcher_focus_set_engine_id);
                }
                m_switcher_focus_set_engine_id = -1;
            }
        }
    }

    public override void focus_in(string input_context_path) {
        m_current_context_path = input_context_path;

        /* 'fake' input context is named as 
         * '/org/freedesktop/IBus/InputContext_1' and always send in
         * focus-out events by ibus-daemon for the global engine mode.
         * Now ibus-daemon assumes to always use the global engine.
         * But this event should not be used for modal dialogs
         * such as Switcher.
         */
        if (!input_context_path.has_suffix("InputContext_1")) {
            m_real_current_context_path = m_current_context_path;
            m_property_panel.focus_in();
            this.switcher_focus_set_engine();
        }

        if (m_use_global_engine)
            return;

        var engine = m_engine_contexts[input_context_path];

        if (engine == null) {
            /* If engine == null, need to call set_engine(m_engines[0])
             * here and update m_engine_contexts[] to avoid the
             * following problem:
             *
             * If context1 is focused and does not set an engine and
             * return here, the current engine1 is used for context1.
             * When context2 is focused and switch engine1 to engine2,
             * the current engine becomes engine2.
             * And when context1 is focused again, context1 still
             * does not set an engine and return here,
             * engine2 is used for context2 instead of engine1. */
            engine = m_engines.length > 0 ? m_engines[0] : null;

            if (engine == null)
                return;
        } else {
            bool in_engines = false;

            foreach (var e in m_engines) {
                if (engine.get_name() == e.get_name()) {
                    in_engines = true;
                    break;
                }
            }

            /* The engine is deleted by ibus-setup before focus_in()
             * is called. */
            if (!in_engines)
                return;
        }

        set_engine(engine);
    }

    public override void focus_out(string input_context_path) {
        m_current_context_path = "";
    }

    public override void destroy_context(string input_context_path) {
        if (m_use_global_engine)
            return;

        m_engine_contexts.remove(input_context_path);
    }

    public override void register_properties(IBus.PropList props) {
        m_property_manager.set_properties(props);
        m_property_panel.set_properties(props);
        set_properties(props);
    }

    public override void update_property(IBus.Property prop) {
        m_property_manager.update_property(prop);
        m_property_panel.update_property(prop);
        set_property(prop, false);
    }

    public override void update_preedit_text(IBus.Text text,
                                             uint cursor_pos,
                                             bool visible) {
        if (visible) {
            m_candidate_panel.set_preedit_text(text, cursor_pos);
            m_property_panel.set_preedit_text(text, cursor_pos);
        } else {
            m_candidate_panel.set_preedit_text(null, 0);
            m_property_panel.set_preedit_text(null, 0);
        }
    }

    public override void hide_preedit_text() {
        m_candidate_panel.set_preedit_text(null, 0);
    }

    public override void update_auxiliary_text(IBus.Text text,
                                               bool visible) {
        m_candidate_panel.set_auxiliary_text(visible ? text : null);
        m_property_panel.set_auxiliary_text(visible ? text : null);
    }

    public override void hide_auxiliary_text() {
        m_candidate_panel.set_auxiliary_text(null);
    }

    public override void update_lookup_table(IBus.LookupTable table,
                                             bool visible) {
        m_candidate_panel.set_lookup_table(visible ? table : null);
        m_property_panel.set_lookup_table(visible ? table : null);
    }

    public override void hide_lookup_table() {
        m_candidate_panel.set_lookup_table(null);
    }

    public override void set_content_type(uint purpose, uint hints) {
        m_candidate_panel.set_content_type(purpose, hints);
    }

    public override void state_changed() {
        /* Do not change the order of m_engines during running switcher. */
        if (m_switcher.is_running())
            return;

        if (m_icon_type == IconType.INDICATOR) {
            // Wait for the callback of the session bus.
            if (m_indicator == null)
                return;
        }

        var icon_name = "ibus-keyboard";

        var engine = m_bus.get_global_engine();
        if (engine != null) {
            icon_name = engine.get_icon();
            m_icon_prop_key = engine.get_icon_prop_key();
        } else {
            m_icon_prop_key = "";
        }

        if (icon_name[0] == '/') {
            if (m_icon_type == IconType.STATUS_ICON)
                m_status_icon.set_from_file(icon_name);
            else if (m_icon_type == IconType.INDICATOR)
                m_indicator.set_icon_full(icon_name, "");
        } else {
            string language = null;

            if (engine != null) {
                var name = engine.get_name();
                if (name.length >= 4 && name[0:4] == "xkb:")
                    language = m_switcher.get_xkb_language(engine);
            }

            if (language != null) {
                if (m_icon_type == IconType.STATUS_ICON) {
                    Gdk.Pixbuf pixbuf =
                            create_icon_pixbuf_with_string(language);
                    m_status_icon.set_from_pixbuf(pixbuf);
                }
                else if (m_icon_type == IconType.INDICATOR) {
                    Cairo.ImageSurface image =
                            create_cairo_image_surface_with_string(language,
                                                                   true);
                    m_indicator.set_cairo_image_surface_full(image, "");
                }
            } else {
                var theme = Gtk.IconTheme.get_default();
                if (theme.lookup_icon(icon_name, 48, 0) != null) {
                    if (m_icon_type == IconType.STATUS_ICON)
                        m_status_icon.set_from_icon_name(icon_name);
                    else if (m_icon_type == IconType.INDICATOR)
                        m_indicator.set_icon_full(icon_name, "");
                } else {
                    if (m_icon_type == IconType.STATUS_ICON)
                        m_status_icon.set_from_icon_name("ibus-engine");
                    else if (m_icon_type == IconType.INDICATOR)
                        m_indicator.set_icon_full("ibus-engine", "");
                }
            }
        }

        if (engine == null)
            return;

        int i;
        for (i = 0; i < m_engines.length; i++) {
            if (m_engines[i].get_name() == engine.get_name())
                break;
        }

        // engine is first engine in m_engines.
        if (i == 0)
            return;

        // engine is not in m_engines.
        if (i >= m_engines.length)
            return;

        for (int j = i; j > 0; j--) {
            m_engines[j] = m_engines[j - 1];
        }
        m_engines[0] = engine;

        string[] names = {};
        foreach(var desc in m_engines) {
            names += desc.get_name();
        }
        m_settings_general.set_strv("engines-order", names);
    }
}
