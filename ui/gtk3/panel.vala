/* vim:set et sts=4 sw=4:
 *
 * ibus - The Input Bus
 *
 * Copyright(c) 2011 Peng Huang <shawn.p.huang@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or(at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA  02111-1307  USA
 */

class Panel : IBus.PanelService {
    private class Keybinding {
        public Keybinding(uint keysym,
                          Gdk.ModifierType modifiers,
                          bool reverse) {
            this.keysym = keysym;
            this.modifiers = modifiers;
            this.reverse = reverse;
        }

        public uint keysym { get; set; }
        public Gdk.ModifierType modifiers { get; set; }
        public bool reverse { get; set; }
    }

    private IBus.Bus m_bus;
    private IBus.Config m_config;
    private Gtk.StatusIcon m_status_icon;
    private Gtk.Menu m_ime_menu;
    private Gtk.Menu m_sys_menu;
    private IBus.EngineDesc[] m_engines = {};
    private IBus.EngineDesc current_engine = null;
    private CandidatePanel m_candidate_panel;
    private Switcher m_switcher;
    private PropertyManager m_property_manager;
    private GLib.Pid m_setup_pid = 0;
    private Gtk.AboutDialog m_about_dialog;
    private Gtk.CssProvider m_css_provider;
    private int m_switcher_delay_time = 400;
    private bool m_use_system_keyboard_layout = false;
    private const string ACCELERATOR_SWITCH_IME_FOREWARD = "<Super>space";

    private GLib.List<Keybinding> m_keybindings = new GLib.List<Keybinding>();

    public Panel(IBus.Bus bus) {
        GLib.assert(bus.is_connected());
        // Chain up base class constructor
        GLib.Object(connection : bus.get_connection(),
                    object_path : "/org/freedesktop/IBus/Panel");

        m_bus = bus;

        // init ui
        m_status_icon = new Gtk.StatusIcon();
        m_status_icon.set_name("ibus-ui-gtk");
        m_status_icon.set_title("IBus Panel");
        m_status_icon.popup_menu.connect(status_icon_popup_menu_cb);
        m_status_icon.activate.connect(status_icon_activate_cb);
        m_status_icon.set_from_icon_name("ibus-keyboard");

        m_candidate_panel = new CandidatePanel();
        m_candidate_panel.page_up.connect((w) => this.page_up());
        m_candidate_panel.page_down.connect((w) => this.page_down());

        m_switcher = new Switcher();
        // The initial shortcut is "<Super>space"
        bind_switch_shortcut(null);

        if (m_switcher_delay_time >= 0) {
            m_switcher.set_popup_delay_time((uint) m_switcher_delay_time);
        }

        m_property_manager = new PropertyManager();
        m_property_manager.property_activate.connect((k, s) => {
            property_activate(k, s);
        });

        state_changed();
    }

    ~Panel() {
        unbind_switch_shortcut();
    }

    private void keybinding_manager_bind(KeybindingManager keybinding_manager,
                                         string?           accelerator) {
        uint switch_keysym = 0;
        Gdk.ModifierType switch_modifiers = 0;
        Gdk.ModifierType reverse_modifier = Gdk.ModifierType.SHIFT_MASK;
        Keybinding keybinding;

        Gtk.accelerator_parse(accelerator,
                out switch_keysym, out switch_modifiers);

        // Map virtual modifiers to (i.e. Mod2, Mod3, ...)
        const Gdk.ModifierType VIRTUAL_MODIFIERS = (
                Gdk.ModifierType.SUPER_MASK |
                Gdk.ModifierType.HYPER_MASK |
                Gdk.ModifierType.META_MASK);
        if ((switch_modifiers & VIRTUAL_MODIFIERS) != 0) {
        // workaround a bug in gdk vapi vala > 0.18
        // https://bugzilla.gnome.org/show_bug.cgi?id=677559
#if VALA_0_18
            Gdk.Keymap.get_default().map_virtual_modifiers(
                    ref switch_modifiers);
#else
            if ((switch_modifiers & Gdk.ModifierType.SUPER_MASK) != 0)
                switch_modifiers |= Gdk.ModifierType.MOD4_MASK;
            if ((switch_modifiers & Gdk.ModifierType.HYPER_MASK) != 0)
                switch_modifiers |= Gdk.ModifierType.MOD4_MASK;
#endif
            switch_modifiers &= ~VIRTUAL_MODIFIERS;
        }

        if (switch_keysym == 0 && switch_modifiers == 0) {
            warning("Parse accelerator '%s' failed!", accelerator);
            return;
        }

        keybinding = new Keybinding(switch_keysym,
                                    switch_modifiers,
                                    false);
        m_keybindings.append(keybinding);

        keybinding_manager.bind(switch_keysym, switch_modifiers,
                (e) => handle_engine_switch(e, false));

        // accelerator already has Shift mask
        if ((switch_modifiers & reverse_modifier) != 0) {
            return;
        }

        switch_modifiers |= reverse_modifier;

        keybinding = new Keybinding(switch_keysym,
                                    switch_modifiers,
                                    true);
        m_keybindings.append(keybinding);

        keybinding_manager.bind(switch_keysym, switch_modifiers,
                (e) => handle_engine_switch(e, true));
    }

    private void bind_switch_shortcut(Variant? variant) {
        string[] accelerators = {};
        Variant var_trigger = variant;

        if (var_trigger == null && m_config != null) {
            var_trigger = m_config.get_value("general/hotkey",
                                             "triggers");
        }

        if (var_trigger != null) {
            accelerators = var_trigger.dup_strv();
        } else {
            accelerators += ACCELERATOR_SWITCH_IME_FOREWARD;
        }

        var keybinding_manager = KeybindingManager.get_instance();

        foreach (var accelerator in accelerators) {
            keybinding_manager_bind(keybinding_manager, accelerator);
        }
    }

    private void unbind_switch_shortcut() {
        var keybinding_manager = KeybindingManager.get_instance();

        unowned GLib.List<Keybinding> keybindings = m_keybindings;

        while (keybindings != null) {
            Keybinding keybinding = keybindings.data;

            keybinding_manager.unbind(keybinding.keysym,
                                      keybinding.modifiers);
            keybindings = keybindings.next;
        }

        m_keybindings = null;
    }

    private void set_custom_font() {
        Gdk.Display display = Gdk.Display.get_default();
        Gdk.Screen screen = (display != null) ?
                display.get_default_screen() : null;

        if (screen == null) {
            warning("Could not open display.");
            return;
        }

        bool use_custom_font = false;
        GLib.Variant var_use_custom_font = m_config.get_value("panel",
                                                              "use_custom_font");

        if (var_use_custom_font != null) {
            use_custom_font = var_use_custom_font.get_boolean();
        }

        if (m_css_provider != null) {
            Gtk.StyleContext.remove_provider_for_screen(screen,
                                                        m_css_provider);
            m_css_provider = null;
        }

        if (use_custom_font == false) {
            return;
        }

        string font_name = null;
        GLib.Variant var_custom_font = m_config.get_value("panel",
                                                          "custom_font");
        if (var_custom_font != null) {
            font_name = var_custom_font.dup_string();
        }

        if (font_name == null) {
            warning("No config panel:custom_font.");
            return;
        }

        string data_format = "GtkLabel { font: %s; }";
        string data = data_format.printf(font_name);
        m_css_provider = new Gtk.CssProvider();

        try {
            m_css_provider.load_from_data(data, -1);
        } catch (GLib.Error e) {
            warning("Failed css_provider_from_data: %s: %s", font_name,
                                                             e.message);
            return;
        }

        Gtk.StyleContext.add_provider_for_screen(screen,
                                                 m_css_provider,
                                                 Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

    private void set_switcher_delay_time(Variant? variant) {
        Variant var_switcher_delay_time = variant;

        if (var_switcher_delay_time == null) {
            var_switcher_delay_time = m_config.get_value("general",
                                                         "switcher-delay-time");
        }

        if (var_switcher_delay_time == null) {
            return;
        }

        m_switcher_delay_time = var_switcher_delay_time.get_int32();

        if (m_switcher_delay_time >= 0) {
            m_switcher.set_popup_delay_time((uint) m_switcher_delay_time);
        }
    }

    private void set_use_system_keyboard_layout(Variant? variant) {
        Variant var_use_system_kbd_layout = variant;

        if (var_use_system_kbd_layout == null) {
            var_use_system_kbd_layout = m_config.get_value(
                    "general",
                    "use_system_keyboard_layout");
        }

        if (var_use_system_kbd_layout == null) {
            return;
        }

        m_use_system_keyboard_layout = var_use_system_kbd_layout.get_boolean();
    }

    private void set_embed_preedit_text(Variant? variant) {
        Variant var_embed_preedit = variant;

        if (var_embed_preedit == null) {
            var_embed_preedit = m_config.get_value("general",
                                                   "embed_preedit_text");
        }

        if (var_embed_preedit == null) {
            return;
        }

        m_bus.set_ibus_property("EmbedPreeditText",
                                var_embed_preedit);
    }

    private void set_use_global_engine(Variant? variant) {
        Variant var_use_global_engine = variant;

        if (var_use_global_engine == null) {
            var_use_global_engine = m_config.get_value("general",
                                                       "use_global_engine");
        }

        if (var_use_global_engine == null) {
            return;
        }

        m_bus.set_ibus_property("UseGlobalEngine",
                                var_use_global_engine);

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

    private void update_version_1_5_3() {
#if ENABLE_LIBNOTIFY
        if (!Notify.is_initted()) {
            Notify.init ("ibus");
        }

        var notification = new Notify.Notification(
                _("IBus Update"),
                _("Super+space is now the default hotkey."),
                "ibus");
        notification.set_timeout(30 * 1000);
        notification.set_category("hotkey");

        try {
            notification.show();
        } catch (GLib.Error e){
            warning ("Notification is failed for IBus 1.5.3: %s", e.message);
        }
#else
        warning(_("Super+space is now the default hotkey."));
#endif
    }

    private void set_version() {
        Variant var_prev_version = m_config.get_value("general", "version");
        Variant var_current_version = null;
        string prev_version = "".dup();
        string current_version = null;

        if (var_prev_version != null) {
            prev_version = var_prev_version.dup_string();
        }

        if (compare_versions(prev_version, "1.5.3") < 0) {
            update_version_1_5_3();
        }

        current_version = "%d.%d.%d".printf(IBus.MAJOR_VERSION,
                                            IBus.MINOR_VERSION,
                                            IBus.MICRO_VERSION);

        if (prev_version == current_version) {
            return;
        }

        var_current_version = new Variant.string(current_version);
        m_config.set_value("general", "version", var_current_version);
    }

    public void set_config(IBus.Config config) {
        if (m_config != null) {
            m_config.value_changed.disconnect(config_value_changed_cb);
            m_config.watch(null, null);
            m_config = null;
        }

        m_config = config;
        if (m_config != null) {
            m_config.value_changed.connect(config_value_changed_cb);
            m_config.watch("general", "preload_engines");
            m_config.watch("general", "embed_preedit_text");
            m_config.watch("general", "engines_order");
            m_config.watch("general", "switcher_delay_time");
            m_config.watch("general", "use_global_engine");
            m_config.watch("general", "use_system_keyboard_layout");
            m_config.watch("general/hotkey", "triggers");
            m_config.watch("panel", "custom_font");
            m_config.watch("panel", "use_custom_font");
            // Update m_use_system_keyboard_layout before update_engines()
            // is called.
            set_use_system_keyboard_layout(null);
            if (m_config.get_value("general", "use_global_engine") == true) {
                update_engines(m_config.get_value("general", "preload_engines"),
                               m_config.get_value("general", "engines_order"));
            } else {
                update_engines(m_config.get_value("general", "preload_engines"),
                               null);
            }
            unbind_switch_shortcut();
            bind_switch_shortcut(null);
            set_switcher_delay_time(null);
            set_embed_preedit_text(null);
            set_use_global_engine(null);
            set_custom_font();

            set_version();
        } else {
            update_engines(null, null);
        }
    }

    private void exec_setxkbmap(IBus.EngineDesc engine) {
        string layout = engine.get_layout();
        string variant = engine.get_layout_variant();
        string option = engine.get_layout_option();
        string standard_error = null;
        int exit_status = 0;
        string[] args = { "setxkbmap" };

        if (layout != null && layout != "" && layout != "default") {
            args += "-layout";
            args += layout;
        }
        if (variant != null && variant != "" && variant != "default") {
            args += "-variant";
            args += variant;
        }
        if (option != null && option != "" && option != "default") {
            /*TODO: Need to get the session XKB options */
            args += "-option";
            args += "-option";
            args += option;
        }

        if (args.length == 1) {
            return;
        }

        try {
            if (!GLib.Process.spawn_sync(null, args, null,
                                         GLib.SpawnFlags.SEARCH_PATH,
                                         null, null,
                                         out standard_error,
                                         out exit_status)) {
                warning("Switch xkb layout to %s failed.",
                        engine.get_layout());
            }
        } catch (GLib.SpawnError e) {
            warning("Execute setxkbmap failed: %s", e.message);
        }

        if (exit_status != 0) {
            warning("Execute setxkbmap failed: %s", standard_error ?? "(null)");
        }
    }

    private void switch_engine(int i, bool force = false) {
        GLib.assert(i >= 0 && i < m_engines.length);

        // Do not need switch
        if (i == 0 && !force && m_bus.get_use_global_engine())
            return;

        IBus.EngineDesc engine = m_engines[i];

        if (!m_bus.set_current_engine(engine.get_name())) {
            warning("Switch engine to %s failed.", engine.get_name());
            return;
        }

        // set xkb layout
        if (!m_use_system_keyboard_layout) {
            exec_setxkbmap(engine);
        }
    }

    private void config_value_changed_cb(IBus.Config config,
                                         string section,
                                         string name,
                                         Variant variant) {
        if (section == "general" && name == "preload_engines") {
            update_engines(variant, null);
            return;
        }

        if (section == "general/hotkey" && name == "triggers") {
            unbind_switch_shortcut();
            bind_switch_shortcut(variant);
            return;
        }

        if (section == "panel" && (name == "custom_font" ||
                                   name == "use_custom_font")) {
            set_custom_font();
            return;
        }

        if (section == "general" && name == "switcher_delay_time") {
            set_switcher_delay_time(variant);
            return;
        }

        if (section == "general" && name == "use_system_keyboard_layout") {
            set_use_system_keyboard_layout(variant);
            return;
        }

        if (section == "general" && name == "embed_preedit_text") {
            set_embed_preedit_text(variant);
            return;
        }

        if (section == "general" && name == "use_global_engine") {
            set_use_global_engine(variant);
            if (variant == false) {
                update_engines(m_config.get_value("general", "preload_engines"),
                               null);
            }
            return;
        }
    }

    private int get_current_engine_index() {
        if (current_engine == null) return 0;

        for (int i = 0; i < m_engines.length; ++i) {
            if (m_engines[i].get_name() == current_engine.get_name())
                return i;
        }

        return 0;
    }

    private void handle_engine_switch(Gdk.Event event, bool revert) {
        // Do not need switch IME
        if (m_engines.length <= 1)
            return;

        uint keyval = event.key.keyval;
        uint modifiers = KeybindingManager.MODIFIER_FILTER & event.key.state;

        uint primary_modifiers =
            KeybindingManager.get_primary_modifier(event.key.state);

        bool pressed = KeybindingManager.primary_modifier_still_pressed(
                event, primary_modifiers);

        if (revert) {
            modifiers &= ~Gdk.ModifierType.SHIFT_MASK;
        }

        int i = get_current_engine_index() + (revert ? - 1 : 1) + m_engines.length;
        i %= m_engines.length;
        if (pressed && m_switcher_delay_time >= 0) {
            i = m_switcher.run(keyval, modifiers, event, m_engines, i);
            if (i < 0) {
                debug("switch cancelled");
            } else {
                GLib.assert(i < m_engines.length);
                switch_engine(i);
            }
        } else {
            switch_engine(i);
        }
    }

    private void run_preload_engines(IBus.EngineDesc[] engines, int index) {
        string[] names = {};

        if (engines.length <= index) {
            return;
        }

        names += engines[index].get_name();
        m_bus.preload_engines_async(names, -1, null);
    }

    private void update_engines(GLib.Variant? var_engines,
                                GLib.Variant? var_order) {
        string[] engine_names = null;

        if (var_engines != null)
            engine_names = var_engines.dup_strv();
        if (engine_names == null || engine_names.length == 0)
            engine_names = {"xkb:us::eng"};

        string[] order_names =
            (var_order != null) ? var_order.dup_strv() : null;

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

    private void show_setup_dialog() {
        if (m_setup_pid != 0) {
            if (Posix.kill(m_setup_pid, Posix.SIGUSR1) == 0)
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

            string copyright = _(
                "Copyright (c) 2007-2012 Peng Huang\n" +
                "Copyright (c) 2007-2010 Red Hat, Inc.\n");

            m_about_dialog.set_copyright(copyright);
            m_about_dialog.set_license("LGPL");
            m_about_dialog.set_comments(_("IBus is an intelligent input bus for Linux/Unix."));
            m_about_dialog.set_website("http://code.google.com/p/ibus");
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

    private void status_icon_popup_menu_cb(Gtk.StatusIcon status_icon,
                                           uint button,
                                           uint activate_time) {
        // Show system menu
        if (m_sys_menu == null) {
            Gtk.ImageMenuItem item;
            m_sys_menu = new Gtk.Menu();

            item = new Gtk.ImageMenuItem.from_stock(Gtk.Stock.PREFERENCES, null);
            item.activate.connect((i) => show_setup_dialog());
            m_sys_menu.append(item);

            item = new Gtk.ImageMenuItem.from_stock(Gtk.Stock.ABOUT, null);
            item.activate.connect((i) => show_about_dialog());
            m_sys_menu.append(item);

            m_sys_menu.append(new Gtk.SeparatorMenuItem());

            item = new Gtk.ImageMenuItem.from_stock(Gtk.Stock.REFRESH, null);
            item.set_label(_("Restart"));
            item.activate.connect((i) => m_bus.exit(true));
            m_sys_menu.append(item);

            item = new Gtk.ImageMenuItem.from_stock(Gtk.Stock.QUIT, null);
            item.activate.connect((i) => m_bus.exit(false));
            m_sys_menu.append(item);

            m_sys_menu.show_all();
        }

        m_sys_menu.popup(null,
                         null,
                         m_status_icon.position_menu,
                         0,
                         Gtk.get_current_event_time());
    }

    private void status_icon_activate_cb(Gtk.StatusIcon status_icon) {
        m_ime_menu = new Gtk.Menu();

        // Show properties and IME switching menu
        m_property_manager.create_menu_items(m_ime_menu);

        m_ime_menu.append(new Gtk.SeparatorMenuItem());

        // Append IMEs
        foreach (var engine in m_engines) {
            var language = engine.get_language();
            var longname = engine.get_longname();
            var item = new Gtk.ImageMenuItem.with_label(
                "%s - %s".printf (IBus.get_language_name(language), longname));
            if (engine.get_icon() != "") {
                var icon = new IconWidget(engine.get_icon(), Gtk.IconSize.MENU);
                 item.set_image(icon);
            }
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
        m_ime_menu.popup(null,
                         null,
                         m_status_icon.position_menu,
                         0,
                         Gtk.get_current_event_time());
    }

    /* override virtual functions */
    public override void set_cursor_location(int x, int y,
                                             int width, int height) {
        m_candidate_panel.set_cursor_location(x, y, width, height);
    }

    public override void focus_in(string input_context_path) {
        state_changed();
    }

    public override void focus_out(string input_context_path) {
    }

    public override void register_properties(IBus.PropList props) {
        m_property_manager.set_properties(props);
    }

    public override void update_property(IBus.Property prop) {
        m_property_manager.update_property(prop);
    }

    public override void update_preedit_text(IBus.Text text,
                                             uint cursor_pos,
                                             bool visible) {
        if (visible)
            m_candidate_panel.set_preedit_text(text, cursor_pos);
        else
            m_candidate_panel.set_preedit_text(null, 0);
    }

    public override void hide_preedit_text() {
        m_candidate_panel.set_preedit_text(null, 0);
    }

    public override void update_auxiliary_text(IBus.Text text,
                                               bool visible) {
        m_candidate_panel.set_auxiliary_text(visible ? text : null);
    }

    public override void hide_auxiliary_text() {
        m_candidate_panel.set_auxiliary_text(null);
    }

    public override void update_lookup_table(IBus.LookupTable table,
                                             bool visible) {
        m_candidate_panel.set_lookup_table(visible ? table : null);
    }

    public override void hide_lookup_table() {
        m_candidate_panel.set_lookup_table(null);
    }

    public override void state_changed() {
        var icon_name = "ibus-keyboard";

        var engine = m_bus.get_current_engine();

        if (engine != null) {
            icon_name = engine.get_icon();
        }

        if (icon_name[0] == '/')
            m_status_icon.set_from_file(icon_name);
        else {
            var theme = Gtk.IconTheme.get_default();
            if (theme.lookup_icon(icon_name, 48, 0) != null) {
                m_status_icon.set_from_icon_name(icon_name);
            } else {
                m_status_icon.set_from_icon_name("ibus-engine");
            }
        }

        if (engine == null)
            return;

        if (m_bus.get_use_global_engine()) {
            adjust_engines_order(engine);
            current_engine = null;
        } else {
            current_engine = engine;
        }
    }

    private void adjust_engines_order(IBus.EngineDesc first_engine) {
        int i;
        for (i = 0; i < m_engines.length; i++) {
            if (m_engines[i].get_name() == first_engine.get_name())
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
        m_engines[0] = first_engine;

        string[] names = {};
        foreach(var desc in m_engines) {
            names += desc.get_name();
        }
        if (m_config != null)
            m_config.set_value("general",
                               "engines_order",
                               new GLib.Variant.strv(names));
    }
}
