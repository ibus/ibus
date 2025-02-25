/* vim:set et sts=4 sw=4:
 *
 * ibus - The Input Bus
 *
 * Copyright(c) 2018 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright(c) 2018-2025 Takao Fujwiara <takao.fujiwara1@gmail.com>
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

/* This file depends on keybindingmanager.vala */

class BindingCommon {
#if ENABLE_XIM
    public static Gdk.X11.Display m_xdisplay;
#endif
    public static bool m_default_is_xdisplay;
    public enum KeyEventFuncType {
        ANY,
        IME_SWITCHER,
        EMOJI_TYPING,
    }

    public class Keybinding : GLib.Object {
        public Keybinding(uint             keysym,
                          Gdk.ModifierType modifiers,
                          bool             reverse,
                          KeyEventFuncType ftype) {
            this.keysym = keysym;
            this.modifiers = modifiers;
            this.reverse = reverse;
            this.ftype = ftype;
        }

        public uint keysym { get; set; }
        public Gdk.ModifierType modifiers { get; set; }
        public bool reverse { get; set; }
        public KeyEventFuncType ftype { get; set; }
    }

    public delegate void KeybindingHandlerFunc(Gdk.Event event);

    public static void
    keybinding_manager_bind(KeybindingManager           keybinding_manager,
                            ref GLib.List<Keybinding>   keybindings,
                            string?                     accelerator,
                            KeyEventFuncType            ftype,
                            KeybindingManager.KeybindingHandlerFunc
                                                        handler_normal,
                            KeybindingManager.KeybindingHandlerFunc?
                                                        handler_reverse) {
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
            Gdk.Keymap.get_for_display(Gdk.Display.get_default()
                    ).map_virtual_modifiers(ref switch_modifiers);
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
                                    false,
                                    ftype);
        keybindings.append(keybinding);

        bool is_wayland = false;
#if USE_GDK_WAYLAND
        if (!BindingCommon.default_is_xdisplay())
            is_wayland = true;
#endif
#if ENABLE_XIM
        if (!is_wayland) {
            keybinding_manager.bind(switch_keysym, switch_modifiers,
                                    handler_normal);
        }
#endif
        if (ftype == KeyEventFuncType.EMOJI_TYPING) {
            return;
        }

        // accelerator already has Shift mask
        if ((switch_modifiers & reverse_modifier) != 0) {
            return;
        }

        switch_modifiers |= reverse_modifier;

        keybinding = new Keybinding(switch_keysym,
                                    switch_modifiers,
                                    true,
                                    ftype);
        keybindings.append(keybinding);

#if ENABLE_XIM
        if (!is_wayland && ftype == KeyEventFuncType.IME_SWITCHER) {
            keybinding_manager.bind(switch_keysym, switch_modifiers,
                                    handler_reverse);
        }
#endif
        return;
    }

    public static void
    unbind_switch_shortcut(KeyEventFuncType      ftype,
                           GLib.List<Keybinding> keybindings) {
#if ENABLE_XIM
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
#endif
    }

    public static void set_custom_font(GLib.Settings?       settings_panel,
                                       GLib.Settings?       settings_emoji,
                                       ref Gtk.CssProvider? css_provider) {
        Gdk.Display display = Gdk.Display.get_default();
        Gdk.Screen screen = (display != null) ?
                display.get_default_screen() : null;

        if (screen == null) {
            warning("Could not open display.");
            return;
        }

        if (settings_emoji != null) {
            string emoji_font = settings_emoji.get_string("font");
            if (emoji_font == null) {
                warning("No config emoji:font.");
                return;
            }
            IBusEmojier.set_emoji_font(emoji_font);
        }

        if (settings_panel == null)
            return;

        bool use_custom_font = settings_panel.get_boolean("use-custom-font");

        if (css_provider != null) {
            Gtk.StyleContext.remove_provider_for_screen(screen,
                                                        css_provider);
            css_provider = null;
        }

        if (use_custom_font == false) {
            return;
        }

        string custom_font = settings_panel.get_string("custom-font");
        if (custom_font == null) {
            warning("No config panel:custom-font.");
            return;
        }

        Pango.FontDescription font_desc =
                Pango.FontDescription.from_string(custom_font);
        string font_family = font_desc.get_family();
        int font_size = font_desc.get_size() / Pango.SCALE;
        string data;

        if (Gtk.MAJOR_VERSION < 3 ||
            (Gtk.MAJOR_VERSION == 3 && Gtk.MINOR_VERSION < 20)) {
            data = "GtkLabel { font: %s; }".printf(custom_font);
        } else {
            data = "label { font-family: %s; font-size: %dpt; }"
                           .printf(font_family, font_size);
        }

        css_provider = new Gtk.CssProvider();

        try {
            css_provider.load_from_data(data, -1);
        } catch (GLib.Error e) {
            warning("Failed css_provider_from_data: %s: %s", custom_font,
                                                             e.message);
            return;
        }

        Gtk.StyleContext.add_provider_for_screen(
                screen,
                css_provider,
                Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

    public static void set_custom_theme(GLib.Settings? settings_panel) {
        if (settings_panel == null)
            return;

        bool use_custom_theme = settings_panel.get_boolean("use-custom-theme");
        string custom_theme = settings_panel.get_string("custom-theme");

        Gtk.Settings gtk_settings = Gtk.Settings.get_default();

        if (use_custom_theme == false)
            custom_theme = "";

        if (custom_theme == null || custom_theme == "")
            gtk_settings.reset_property("gtk-theme-name");
        else
            gtk_settings.gtk_theme_name = custom_theme;
    }

    public static void set_custom_icon(GLib.Settings? settings_panel) {
        if (settings_panel == null)
            return;

        bool use_custom_icon = settings_panel.get_boolean("use-custom-icon");
        string custom_icon = settings_panel.get_string("custom-icon");

        Gtk.Settings gtk_settings = Gtk.Settings.get_default();

        if (use_custom_icon == false)
            custom_icon = "";

        if (custom_icon == null || custom_icon == "")
            gtk_settings.reset_property("gtk-icon-theme-name");
        else
            gtk_settings.gtk_icon_theme_name = custom_icon;
    }

    public static bool default_is_xdisplay() {
#if ENABLE_XIM
        if (m_xdisplay == null)
            get_xdisplay(true);
#endif
        return m_default_is_xdisplay;
    }

#if ENABLE_XIM
    public static Gdk.X11.Display? get_xdisplay(bool check_only=false) {
        if (m_xdisplay != null)
            return m_xdisplay;
        var display = Gdk.Display.get_default();
        if (display == null) {
            error("You should open a display for IBus panel.");
        }
        Type instance_type = display.get_type();
        Type x11_type = typeof(Gdk.X11.Display);
        if (instance_type.is_a(x11_type)) {
            m_default_is_xdisplay = true;
            m_xdisplay = (Gdk.X11.Display)display;
            return m_xdisplay;
        }
        if (check_only)
            return null;
        Gdk.set_allowed_backends("x11");
        // Call _gdk_display_manager_add_display() internally.
        m_xdisplay =
                (Gdk.X11.Display)Gdk.DisplayManager.get().open_display(null);
        Gdk.set_allowed_backends("*");
        return m_xdisplay;
    }
#endif
}
