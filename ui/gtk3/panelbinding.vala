/* vim:set et sts=4 sw=4:
 *
 * ibus - The Input Bus
 *
 * Copyright(c) 2018 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright(c) 2018-2020 Takao Fujwiara <takao.fujiwara1@gmail.com>
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

class Preedit : Gtk.Window {
    private Gtk.Label m_extension_preedit_text;
    private Gtk.Label m_extension_preedit_emoji;
    private IBus.Text? m_engine_preedit_text;
    private bool m_engine_preedit_text_show;
    private uint m_engine_preedit_cursor_pos;
    private string m_prefix = "@";
    private bool m_is_shown = true;


    public Preedit() {
        GLib.Object(
            name : "IBusPreedit",
            type: Gtk.WindowType.POPUP
        );
        m_extension_preedit_text  = new Gtk.Label("");
        m_extension_preedit_emoji  = new Gtk.Label("");
    }


    public new void hide() {
        reset();
        base.hide();
        m_is_shown = false;
    }


    public bool is_shown() {
        return m_is_shown;
    }


    public void reset() {
        set_emoji("");
        set_text("");
        resize(1, 1);
        m_is_shown = true;
    }

    public void append_text(string text) {
        if (text.length == 0)
            return;
        string total = m_extension_preedit_text.get_text();
        total += text;
        m_extension_preedit_text.set_text(total);
    }


    public string get_text() {
        return m_extension_preedit_text.get_text();
    }


    public void set_text(string text) {
        m_extension_preedit_text.set_text(text);
    }


    public string get_emoji() {
        return m_extension_preedit_emoji.get_text();
    }


    public void set_emoji(string text) {
        m_extension_preedit_emoji.set_text(text);
    }


    public bool backspace() {
        string total = m_extension_preedit_emoji.get_text();
        if (total.length > 0) {
            m_extension_preedit_emoji.set_text("");
            resize(1, 1);
            return false;
        }
        total = m_extension_preedit_text.get_text();
        int char_count = total.char_count();
        if (char_count == 0)
            return true;
        total = total[0:total.index_of_nth_char(char_count - 1)];
        resize(1, 1);
        m_extension_preedit_text.set_text(total);
        if (total.length == 0)
            resize(1, 1);
        return true;
    }


    private string get_extension_text () {
        string extension_text = m_extension_preedit_emoji.get_text();
        if (extension_text.length == 0)
            extension_text = m_extension_preedit_text.get_text();
        return m_prefix + extension_text;
    }


    private void set_preedit_color(IBus.Text text,
                                   uint start_index,
                                   uint end_index) {
        text.append_attribute(IBus.AttrType.UNDERLINE,
                              IBus.AttrUnderline.SINGLE,
                              start_index, (int)end_index);
    }


    public IBus.Text get_engine_preedit_text() {
        string extension_text = get_extension_text();
        uint char_count = extension_text.char_count();
        IBus.Text retval;
        if (m_engine_preedit_text == null || !m_engine_preedit_text_show) {
            retval = new IBus.Text.from_string(extension_text);
            set_preedit_color(retval, 0, char_count);
            return retval;
        }
        retval = new IBus.Text.from_string(
                extension_text + m_engine_preedit_text.get_text());
        set_preedit_color(retval, 0, char_count);

        unowned IBus.AttrList attrs = m_engine_preedit_text.get_attributes();

        if (attrs == null)
            return retval;

        int i = 0;
        while (true) {
            IBus.Attribute attr = attrs.get(i++);
            if (attr == null)
                break;
            long start_index = attr.start_index;
            long end_index = attr.end_index;
            if (start_index < 0)
                start_index = 0;
            if (end_index < 0)
                end_index = m_engine_preedit_text.get_length();
            retval.append_attribute(attr.type, attr.value,
                                    char_count + (uint)start_index,
                                    (int)char_count + (int)end_index);
        }
        return retval;
    }


    public void set_engine_preedit_text(IBus.Text? text) {
        m_engine_preedit_text = text;
    }


    public void show_engine_preedit_text() {
        m_engine_preedit_text_show = true;
    }


    public void hide_engine_preedit_text() {
        m_engine_preedit_text_show = false;
    }


    public uint get_engine_preedit_cursor_pos() {
        return get_extension_text().char_count() + m_engine_preedit_cursor_pos;
    }


    public void set_engine_preedit_cursor_pos(uint cursor_pos) {
        m_engine_preedit_cursor_pos = cursor_pos;
    }


    public IBus.Text get_commit_text() {
        string extension_text = m_extension_preedit_emoji.get_text();
        if (extension_text.length == 0 && m_prefix != "u")
            extension_text = m_extension_preedit_text.get_text();
        return new IBus.Text.from_string(extension_text);
    }


    public void set_extension_name(string extension_name) {
        if (extension_name.length == 0)
            m_prefix = "@";
        else
            m_prefix = extension_name[0:1];
    }
}


class PanelBinding : IBus.PanelService {
    private bool m_is_wayland;
    private bool m_wayland_lookup_table_is_visible;
    private IBus.Bus m_bus;
    private Gtk.Application m_application;
    private GLib.Settings m_settings_panel = null;
    private GLib.Settings m_settings_emoji = null;
    private string m_current_context_path = "";
    private string m_real_current_context_path = "";
    private IBusEmojier? m_emojier;
    private uint m_emojier_set_emoji_lang_id;
    private uint m_emojier_focus_commit_text_id;
    private string[] m_emojier_favorites = {};
    private Gtk.CssProvider m_css_provider;
    private const uint PRELOAD_ENGINES_DELAY_TIME = 30000;
    private bool m_load_emoji_at_startup;
    private bool m_loaded_emoji = false;
    private bool m_load_unicode_at_startup;
    private bool m_loaded_unicode = false;
    private bool m_enable_extension;
    private string m_extension_name = "";
    private Preedit m_preedit;
    private IBus.ProcessKeyEventData m_key_event_data =
            IBus.ProcessKeyEventData();

    public PanelBinding(IBus.Bus bus,
                        Gtk.Application application) {
        GLib.assert(bus.is_connected());
        // Chain up base class constructor
        GLib.Object(connection : bus.get_connection(),
                    object_path : IBus.PATH_PANEL_EXTENSION_EMOJI);

#if USE_GDK_WAYLAND
        Type instance_type = Gdk.Display.get_default().get_type();
        Type wayland_type = typeof(GdkWayland.Display);
        m_is_wayland = instance_type.is_a(wayland_type);
#else
        m_is_wayland = false;
        warning("Checking Wayland is disabled");
#endif

        m_bus = bus;
        m_application = application;

        init_settings();
        m_preedit = new Preedit();
    }


    private void init_settings() {
        m_settings_panel = new GLib.Settings("org.freedesktop.ibus.panel");
        m_settings_emoji = new GLib.Settings("org.freedesktop.ibus.panel.emoji");

        m_settings_panel.changed["custom-font"].connect((key) => {
                BindingCommon.set_custom_font(m_settings_panel,
                                              m_settings_emoji,
                                              ref m_css_provider);
        });

        m_settings_panel.changed["use-custom-font"].connect((key) => {
                BindingCommon.set_custom_font(m_settings_panel,
                                              m_settings_emoji,
                                              ref m_css_provider);
        });

        m_settings_panel.changed["custom-theme"].connect((key) => {
            BindingCommon.set_custom_theme(m_settings_panel);
        });

        m_settings_panel.changed["use-custom-theme"].connect((key) => {
                BindingCommon.set_custom_theme(m_settings_panel);
        });

        m_settings_emoji.changed["unicode-hotkey"].connect((key) => {
                set_emoji_hotkey();
        });

        m_settings_emoji.changed["font"].connect((key) => {
                BindingCommon.set_custom_font(m_settings_panel,
                                              m_settings_emoji,
                                              ref m_css_provider);
        });

        m_settings_emoji.changed["hotkey"].connect((key) => {
                set_emoji_hotkey();
        });

        m_settings_emoji.changed["favorites"].connect((key) => {
                set_emoji_favorites();
        });

        m_settings_emoji.changed["favorite-annotations"].connect((key) => {
                set_emoji_favorites();
        });

        m_settings_emoji.changed["lang"].connect((key) => {
                set_emoji_lang();
        });

        m_settings_emoji.changed["has-partial-match"].connect((key) => {
                set_emoji_partial_match();
        });

        m_settings_emoji.changed["partial-match-length"].connect((key) => {
                set_emoji_partial_match();
        });

        m_settings_emoji.changed["partial-match-condition"].connect((key) => {
                set_emoji_partial_match();
        });

        m_settings_emoji.changed["load-emoji-at-startup"].connect((key) => {
                set_load_emoji_at_startup();
        });

        m_settings_emoji.changed["load-unicode-at-startup"].connect((key) => {
                set_load_unicode_at_startup();
        });
    }


    // Returning unowned IBus.KeyEventData causes NULL with gcc optimization
    // and use m_key_event_data.
    private void parse_accelerator(string accelerator) {
        m_key_event_data = {};
        uint keysym = 0;
        IBus.ModifierType modifiers = 0;
        IBus.accelerator_parse(accelerator,
                out keysym, out modifiers);
        if (keysym == 0U && modifiers == 0) {
            warning("Failed to parse shortcut key '%s'".printf(accelerator));
            return;
        }
        if ((modifiers & IBus.ModifierType.SUPER_MASK) != 0) {
            modifiers ^= IBus.ModifierType.SUPER_MASK;
            modifiers |= IBus.ModifierType.MOD4_MASK;
        }
        m_key_event_data.keyval = keysym;
        m_key_event_data.state = modifiers;
    }


    private void set_emoji_hotkey() {
        IBus.ProcessKeyEventData[] emoji_keys = {};
        IBus.ProcessKeyEventData key;
        string[] accelerators = m_settings_emoji.get_strv("hotkey");
        foreach (var accelerator in accelerators) {
            parse_accelerator(accelerator);
            emoji_keys += m_key_event_data;
        }

        /* Since {} is not allocated, parse_accelerator() should be unowned. */
        key = {};
        emoji_keys += key;

        IBus.ProcessKeyEventData[] unicode_keys = {};
        accelerators = m_settings_emoji.get_strv("unicode-hotkey");
        foreach (var accelerator in accelerators) {
            parse_accelerator(accelerator);
            unicode_keys += m_key_event_data;
        }
        key = {};
        unicode_keys += key;

        panel_extension_register_keys("emoji", emoji_keys,
                                      "unicode", unicode_keys);
    }


    private void set_emoji_favorites() {
        m_emojier_favorites = m_settings_emoji.get_strv("favorites");
        IBusEmojier.set_favorites(
                m_emojier_favorites,
                m_settings_emoji.get_strv("favorite-annotations"));
    }


    private void set_emoji_lang() {
        if (m_emojier_set_emoji_lang_id > 0) {
            GLib.Source.remove(m_emojier_set_emoji_lang_id);
            m_emojier_set_emoji_lang_id = 0;
        }
        m_emojier_set_emoji_lang_id = GLib.Idle.add(() => {
            IBusEmojier.set_annotation_lang(
                    m_settings_emoji.get_string("lang"));
            m_emojier_set_emoji_lang_id = 0;
            m_loaded_emoji = true;
            if (m_load_unicode_at_startup && !m_loaded_unicode) {
                IBusEmojier.load_unicode_dict();
                m_loaded_unicode = true;
            }
            return false;
        });
    }


    private void set_emoji_partial_match() {
        IBusEmojier.set_partial_match(
                m_settings_emoji.get_boolean("has-partial-match"));
        IBusEmojier.set_partial_match_length(
                m_settings_emoji.get_int("partial-match-length"));
        IBusEmojier.set_partial_match_condition(
                m_settings_emoji.get_int("partial-match-condition"));
    }


    private void set_load_emoji_at_startup() {
        m_load_emoji_at_startup =
            m_settings_emoji.get_boolean("load-emoji-at-startup");
    }


    private void set_load_unicode_at_startup() {
        m_load_unicode_at_startup =
            m_settings_emoji.get_boolean("load-unicode-at-startup");
    }

    public void load_settings() {

        set_emoji_hotkey();
        set_load_emoji_at_startup();
        set_load_unicode_at_startup();
        BindingCommon.set_custom_font(m_settings_panel,
                                      m_settings_emoji,
                                      ref m_css_provider);
        BindingCommon.set_custom_theme(m_settings_panel);
        set_emoji_favorites();
        if (m_load_emoji_at_startup && !m_loaded_emoji)
            set_emoji_lang();
        set_emoji_partial_match();
    }


    /**
     * disconnect_signals:
     *
     * Call this API before m_panel = null so that the ref_count becomes 0
     */
    public void disconnect_signals() {
        if (m_emojier_set_emoji_lang_id > 0) {
            GLib.Source.remove(m_emojier_set_emoji_lang_id);
            m_emojier_set_emoji_lang_id = 0;
        }
        if (m_emojier != null) {
            m_application.remove_window(m_emojier);
            m_emojier = null;
        }
        m_application = null;
    }


    private void commit_text_update_favorites(IBus.Text text,
                                              bool      disable_extension) {
        commit_text(text);

        // If disable_extension is false, the extension event is already
        // sent before the focus-in is received.
        if (disable_extension) {
            IBus.ExtensionEvent event = new IBus.ExtensionEvent(
                    "name", m_extension_name,
                    "is-enabled", false,
                    "is-extension", true);
            panel_extension(event);
        }
        string committed_string = text.text;
        string preedit_string = m_preedit.get_text();
        m_preedit.hide();
        if (preedit_string == committed_string)
            return;
        bool has_favorite = false;
        foreach (unowned string favorite in m_emojier_favorites) {
            if (favorite == committed_string) {
                has_favorite = true;
                break;
            }
        }
        if (!has_favorite) {
            m_emojier_favorites += committed_string;
            m_settings_emoji.set_strv("favorites", m_emojier_favorites);
        }
    }


    private bool emojier_focus_commit_real() {
        if (m_emojier == null)
            return true;
        string selected_string = m_emojier.get_selected_string();
        string prev_context_path = m_emojier.get_input_context_path();
        if (selected_string != null &&
            prev_context_path != "" &&
            prev_context_path == m_current_context_path) {
            IBus.Text text = new IBus.Text.from_string(selected_string);
            commit_text_update_favorites(text, false);
            m_emojier.reset();
            return true;
        }

        return false;
    }


    private void emojier_focus_commit() {
        if (m_emojier == null)
            return;
        string selected_string = m_emojier.get_selected_string();
        string prev_context_path = m_emojier.get_input_context_path();
        if (selected_string == null &&
            prev_context_path != "") {
            var context = GLib.MainContext.default();
            if (m_emojier_focus_commit_text_id > 0 &&
                context.find_source_by_id(m_emojier_focus_commit_text_id)
                        != null) {
                GLib.Source.remove(m_emojier_focus_commit_text_id);
            }
            m_emojier_focus_commit_text_id = GLib.Timeout.add(100, () => {
                // focus_in is comming before switcher returns
                emojier_focus_commit_real();
                m_emojier_focus_commit_text_id = -1;
                return false;
            });
        } else {
            if (emojier_focus_commit_real()) {
                var context = GLib.MainContext.default();
                if (m_emojier_focus_commit_text_id > 0 &&
                    context.find_source_by_id(m_emojier_focus_commit_text_id)
                            != null) {
                    GLib.Source.remove(m_emojier_focus_commit_text_id);
                }
                m_emojier_focus_commit_text_id = -1;
            }
        }
    }


    private bool key_press_escape() {
        if (is_emoji_lookup_table()) {
            bool show_candidate = m_emojier.key_press_escape();
            convert_preedit_text();
            return show_candidate;
        }
        if (m_preedit.get_emoji() != "") {
            m_preedit.set_emoji("");
            string annotation = m_preedit.get_text();
            m_emojier.set_annotation(annotation);
            return false;
        }
        m_enable_extension = false;
        hide_emoji_lookup_table();
        m_preedit.hide();
        IBus.ExtensionEvent event = new IBus.ExtensionEvent(
                "name", m_extension_name,
                "is-enabled", false,
                "is-extension", true);
        panel_extension(event);
        return false;
    }


    private bool key_press_keyval(uint keyval) {
        unichar ch = IBus.keyval_to_unicode(keyval);
        if (m_extension_name == "unicode" && !ch.isxdigit())
            return false;
        if (ch.iscntrl())
            return false;
        string str = ch.to_string();
        m_preedit.append_text(str);
        string annotation = m_preedit.get_text();
        m_emojier.set_annotation(annotation);
        m_preedit.set_emoji("");
        return true;
    }


    private bool key_press_enter() {
        if (m_extension_name != "unicode" && is_emoji_lookup_table()) {
            // Check if variats exist
            if (m_emojier.key_press_enter(false)) {
                convert_preedit_text();
                return true;
            }
        }
        IBus.Text text = m_preedit.get_commit_text();
        commit_text_update_favorites(text, true);
        return false;
    }


    private void convert_preedit_text() {
        if (m_emojier.get_number_of_candidates() > 0)
            m_preedit.set_emoji(m_emojier.get_current_candidate());
        else
            m_preedit.set_emoji("");
    }


    private bool key_press_space() {
        bool show_candidate = false;
        if (m_preedit.get_emoji() != "") {
            m_emojier.key_press_cursor_horizontal(Gdk.Key.Right, 0);
            show_candidate = true;
        } else {
            string annotation = m_preedit.get_text();
            if (annotation.length == 0) {
                show_candidate = true;
                if (is_emoji_lookup_table())
                    m_emojier.key_press_cursor_horizontal(Gdk.Key.Right, 0);
            } else {
                m_emojier.set_annotation(annotation);
            }
        }
        convert_preedit_text();
        return show_candidate;
    }


    private bool key_press_cursor_horizontal(uint keyval,
                                             uint modifiers) {
        if (is_emoji_lookup_table()) {
            m_emojier.key_press_cursor_horizontal(keyval, modifiers);
            convert_preedit_text();
            return true;
        }
        return false;
    }


    private bool key_press_cursor_vertical(uint keyval,
                                           uint modifiers) {
        if (is_emoji_lookup_table()) {
            m_emojier.key_press_cursor_vertical(keyval, modifiers);
            convert_preedit_text();
            return true;
        }
        return false;
    }


    private bool key_press_cursor_home_end(uint keyval,
                                           uint modifiers) {
        if (is_emoji_lookup_table()) {
            m_emojier.key_press_cursor_home_end(keyval, modifiers);
            convert_preedit_text();
            return true;
        }
        return false;
    }


    private bool key_press_control_keyval(uint keyval,
                                          uint modifiers) {
        bool show_candidate = false;
        switch(keyval) {
        case Gdk.Key.f:
            show_candidate = key_press_cursor_horizontal(Gdk.Key.Right,
                                                         modifiers);
            break;
        case Gdk.Key.b:
            show_candidate = key_press_cursor_horizontal(Gdk.Key.Left,
                                                         modifiers);
            break;
        case Gdk.Key.n:
        case Gdk.Key.N:
            show_candidate = key_press_cursor_vertical(Gdk.Key.Down, modifiers);
            break;
        case Gdk.Key.p:
        case Gdk.Key.P:
            show_candidate = key_press_cursor_vertical(Gdk.Key.Up, modifiers);
            break;
        case Gdk.Key.h:
            show_candidate = key_press_cursor_home_end(Gdk.Key.Home, modifiers);
            break;
        case Gdk.Key.e:
            show_candidate = key_press_cursor_home_end(Gdk.Key.End, modifiers);
            break;
        case Gdk.Key.u:
            m_preedit.reset();
            m_emojier.set_annotation("");
            hide_emoji_lookup_table();
            break;
        case Gdk.Key.C:
        case Gdk.Key.c:
            if ((modifiers & Gdk.ModifierType.SHIFT_MASK) != 0) {
                if (!m_is_wayland && m_emojier != null &&
                    m_emojier.get_number_of_candidates() > 0) {
                    var text = m_emojier.get_current_candidate();
                    Gtk.Clipboard clipboard =
                            Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD);
                    clipboard.set_text(text, -1);
                    clipboard.store();
                }
                show_candidate = is_emoji_lookup_table();
            }
            break;
        default:
            show_candidate = is_emoji_lookup_table();
            break;
        }
        return show_candidate;
    }


    private void hide_wayland_lookup_table() {
        m_wayland_lookup_table_is_visible = false;
        var text = new IBus.Text.from_string("");
        update_auxiliary_text_received(text, false);
        update_lookup_table_received(
                new IBus.LookupTable(1, 0, false, true),
                false);
    }


    private void show_wayland_lookup_table(IBus.Text text) {
        m_wayland_lookup_table_is_visible = true;
        var table = m_emojier.get_one_dimension_lookup_table();
        uint ncandidates = table.get_number_of_candidates();
        update_auxiliary_text_received(
                text,
                ncandidates > 0 ? true : false);
        update_lookup_table_received(
                table,
                ncandidates > 0 ? true : false);
    }


    private void hide_emoji_lookup_table() {
        if (m_emojier == null)
            return;
        if (m_wayland_lookup_table_is_visible)
            hide_wayland_lookup_table();
        else
            m_emojier.hide();
    }


    private void show_emoji_lookup_table() {
        /* Emojier category_list is shown in both Xorg and Wayland
         * because the annotation information is useful but the Wayland lookup
         * window is alway one dimension. So the category_list is shown
         * when the user annotation is null.
         */
        if (m_is_wayland && m_preedit.get_text() != "") {
            var text = m_emojier.get_title_text();
            show_wayland_lookup_table(text);
        } else {
            // POPUP window takes the focus in Wayland.
            if (m_is_wayland)
                m_emojier.set_input_context_path(m_real_current_context_path);
            m_emojier.show_all();
        }
    }


    private bool is_emoji_lookup_table() {
        if (m_is_wayland)
            return m_wayland_lookup_table_is_visible;
        else
            return m_emojier.get_visible();
    }


    private void show_preedit_and_candidate(bool show_candidate) {
        uint cursor_pos = 0;
        if (!show_candidate)
            cursor_pos = m_preedit.get_engine_preedit_cursor_pos();
        update_preedit_text_received(
                m_preedit.get_engine_preedit_text(),
                cursor_pos,
                true);
        if (!show_candidate) {
            hide_emoji_lookup_table();
            return;
        }
        if (m_emojier == null)
            return;
        /* Wayland gives the focus on Emojir which is a GTK popup window
         * and move the focus fom the current input context to Emojier.
         * This forwards the lookup table to gnome-shell's lookup table
         * but it enables one dimension lookup table only.
         */
        show_emoji_lookup_table();
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
            if (m_is_wayland)
                this.emojier_focus_commit();
        }
    }


    public override void focus_out(string input_context_path) {
        m_current_context_path = "";
    }


    public override void panel_extension_received(IBus.ExtensionEvent event) {
        m_extension_name = event.get_name();
        if (m_extension_name != "emoji" && m_extension_name != "unicode") {
            string format = "The name %s is not implemented in PanelExtension";
            warning (format.printf(m_extension_name));
            m_extension_name = "";
            return;
        }
        m_enable_extension = event.is_enabled;
        if (!m_enable_extension) {
            hide_emoji_lookup_table();
            return;
        }
        if (!m_loaded_emoji)
            set_emoji_lang();
        if (!m_loaded_unicode && m_loaded_emoji) {
            IBusEmojier.load_unicode_dict();
            m_loaded_unicode = true;
        }
        if (m_emojier == null) {
            m_emojier = new IBusEmojier();
            // For title handling in gnome-shell
            m_application.add_window(m_emojier);
            m_emojier.candidate_clicked.connect((i, b, s) => {
                candidate_clicked_lookup_table_real(i, b, s, true);
            });
            m_emojier.commit_text.connect((s) => {
                if (!m_is_wayland)
                    return;
                // Currently emojier has a focus but the text input focus
                // does not and commit the text later.
                IBus.ExtensionEvent close_event = new IBus.ExtensionEvent(
                        "name", m_extension_name,
                        "is-enabled", false,
                        "is-extension", true);
                panel_extension(close_event);
            });
        }
        m_emojier.reset();
        m_emojier.set_annotation("");
        m_preedit.set_extension_name(m_extension_name);
        m_preedit.reset();
        update_preedit_text_received(
                m_preedit.get_engine_preedit_text(),
                m_preedit.get_engine_preedit_cursor_pos(),
                true);
        string params = event.get_params();
        if (params == "category-list") {
            key_press_space();
            show_preedit_and_candidate(true);
        }
    }


    public override void set_cursor_location(int x,
                                             int y,
                                             int width,
                                             int height) {
        if (m_emojier != null)
            m_emojier.set_cursor_location(x, y, width, height);
    }


    public override void update_preedit_text(IBus.Text text,
                                             uint      cursor_pos,
                                             bool      visible) {
        m_preedit.set_engine_preedit_text(text);
        if (visible)
            m_preedit.show_engine_preedit_text();
        else
            m_preedit.hide_engine_preedit_text();
        m_preedit.set_engine_preedit_cursor_pos(cursor_pos);
        update_preedit_text_received(m_preedit.get_engine_preedit_text(),
                                     m_preedit.get_engine_preedit_cursor_pos(),
                                     visible);
    }


    public override void show_preedit_text() {
        m_preedit.show_engine_preedit_text();
        show_preedit_and_candidate(false);
    }


    public override void hide_preedit_text() {
        m_preedit.hide_engine_preedit_text();
        show_preedit_and_candidate(false);
    }


    public override bool process_key_event(uint keyval,
                                           uint keycode,
                                           uint state) {
        if ((state & IBus.ModifierType.RELEASE_MASK) != 0)
            return false;
        uint modifiers = state;
        bool show_candidate = false;
        switch(keyval) {
        case Gdk.Key.Escape:
            show_candidate = key_press_escape();
            if (!m_preedit.is_shown())
                return true;
            break;
        case Gdk.Key.Return:
        case Gdk.Key.KP_Enter:
            if (m_extension_name == "unicode")
                key_press_space();
            show_candidate = key_press_enter();
            if (!m_preedit.is_shown()) {
                hide_emoji_lookup_table();
                return true;
            }
            break;
        case Gdk.Key.BackSpace:
            m_preedit.backspace();
            string annotation = m_preedit.get_text();
            if (annotation == "" && m_extension_name == "unicode") {
                key_press_escape();
                return true;
            }
            m_emojier.set_annotation(annotation);
            break;
        case Gdk.Key.space:
        case Gdk.Key.KP_Space:
            if ((modifiers & Gdk.ModifierType.SHIFT_MASK) != 0) {
                if (!key_press_keyval(keyval))
                    return true;
                show_candidate = is_emoji_lookup_table();
                break;
            }
            show_candidate = key_press_space();
            if (m_extension_name == "unicode") {
                key_press_enter();
                return true;
            }
            break;
        case Gdk.Key.Right:
        case Gdk.Key.KP_Right:
            /* one dimension in Wayland, two dimensions in X11 */
            if (m_is_wayland) {
                show_candidate = key_press_cursor_vertical(Gdk.Key.Down,
                                                           modifiers);
            } else {
                show_candidate = key_press_cursor_horizontal(Gdk.Key.Right,
                                                             modifiers);
            }
            break;
        case Gdk.Key.Left:
        case Gdk.Key.KP_Left:
            if (m_is_wayland) {
                show_candidate = key_press_cursor_vertical(Gdk.Key.Up,
                                                           modifiers);
            } else {
                show_candidate = key_press_cursor_horizontal(Gdk.Key.Left,
                                                             modifiers);
            }
            break;
        case Gdk.Key.Down:
        case Gdk.Key.KP_Down:
            if (m_is_wayland) {
                show_candidate = key_press_cursor_horizontal(Gdk.Key.Right,
                                                             modifiers);
            } else {
                show_candidate = key_press_cursor_vertical(Gdk.Key.Down,
                                                           modifiers);
            }
            break;
        case Gdk.Key.Up:
        case Gdk.Key.KP_Up:
            if (m_is_wayland) {
                show_candidate = key_press_cursor_horizontal(Gdk.Key.Left,
                                                             modifiers);
            } else {
                show_candidate = key_press_cursor_vertical(Gdk.Key.Up,
                                                           modifiers);
            }
            break;
        case Gdk.Key.Page_Down:
        case Gdk.Key.KP_Page_Down:
            if (m_is_wayland) {
                show_candidate = key_press_cursor_vertical(Gdk.Key.Down,
                                                           modifiers);
            } else {
                show_candidate = key_press_cursor_vertical(Gdk.Key.Page_Down,
                                                           modifiers);
            }
            break;
        case Gdk.Key.Page_Up:
        case Gdk.Key.KP_Page_Up:
            if (m_is_wayland) {
                show_candidate = key_press_cursor_vertical(Gdk.Key.Up,
                                                           modifiers);
            } else {
                show_candidate = key_press_cursor_vertical(Gdk.Key.Page_Up,
                                                           modifiers);
            }
            break;
        case Gdk.Key.Home:
        case Gdk.Key.KP_Home:
            show_candidate = key_press_cursor_home_end(Gdk.Key.Home, modifiers);
            break;
        case Gdk.Key.End:
        case Gdk.Key.KP_End:
            show_candidate = key_press_cursor_home_end(Gdk.Key.End, modifiers);
            break;
        default:
            if ((modifiers & Gdk.ModifierType.CONTROL_MASK) != 0) {
                show_candidate = key_press_control_keyval(keyval, modifiers);
                break;
            }
            if (!key_press_keyval(keyval))
                return true;
            show_candidate = is_emoji_lookup_table();
            break;
        }
        show_preedit_and_candidate(show_candidate);
        return true;
    }

    public override void commit_text_received(IBus.Text text) {
        unowned string? str = text.text;
        if (str == null)
            return;
        /* Do not call convert_preedit_text() because it depends on
         * each IME whether process_key_event() receives Shift-space or not.
         */
        m_preedit.append_text(str);
        m_preedit.set_emoji("");
        string annotation = m_preedit.get_text();
        m_emojier.set_annotation(annotation);
        show_preedit_and_candidate(false);
    }

    public override void page_up_lookup_table() {
        bool show_candidate = key_press_cursor_vertical(Gdk.Key.Up, 0);
        show_preedit_and_candidate(show_candidate);
    }

    public override void page_down_lookup_table() {
        bool show_candidate = key_press_cursor_vertical(Gdk.Key.Down, 0);
        show_preedit_and_candidate(show_candidate);
    }

    public override void cursor_up_lookup_table() {
        bool show_candidate = key_press_cursor_horizontal(Gdk.Key.Left, 0);
        show_preedit_and_candidate(show_candidate);
    }

    public override void cursor_down_lookup_table() {
        bool show_candidate = key_press_cursor_horizontal(Gdk.Key.Right, 0);
        show_preedit_and_candidate(show_candidate);
    }

    private void candidate_clicked_lookup_table_real(uint index,
                                                     uint button,
                                                     uint state,
                                                     bool is_emojier) {
        if (button == IBusEmojier.BUTTON_CLOSE_BUTTON) {
            m_enable_extension = false;
            hide_emoji_lookup_table();
            m_preedit.hide();
            IBus.ExtensionEvent event = new IBus.ExtensionEvent(
                    "name", m_extension_name,
                    "is-enabled", false,
                    "is-extension", true);
            panel_extension(event);
            return;
        }
        if (m_emojier == null)
            return;
        bool show_candidate = false;
        uint ncandidates = m_emojier.get_number_of_candidates();
        if (ncandidates > 0 && ncandidates >= index) {
            m_emojier.set_cursor_pos(index);
            bool need_commit_signal = m_is_wayland && is_emojier;
            show_candidate = m_emojier.has_variants(index, need_commit_signal);
            if (!m_is_wayland)
                m_preedit.set_emoji(m_emojier.get_current_candidate());
        } else {
            return;
        }
        if (!show_candidate) {
            IBus.Text text = m_preedit.get_commit_text();
            hide_emoji_lookup_table();
            if (!is_emojier || !m_is_wayland)
                commit_text_update_favorites(text, true);
            return;
        }
        show_preedit_and_candidate(show_candidate);
    }

    public override void candidate_clicked_lookup_table(uint index,
                                                        uint button,
                                                        uint state) {
        candidate_clicked_lookup_table_real(index, button, state, false);
    }
}
