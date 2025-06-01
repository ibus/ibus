/* vim:set et sts=4 sw=4:
 *
 * ibus - The Input Bus
 *
 * Copyright (c) 2017-2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
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

public class IBusEmojier : Gtk.ApplicationWindow {
    private class EListBox : Gtk.ListBox {
        public EListBox() {
            GLib.Object(
                vexpand : true,
                halign : Gtk.Align.FILL,
                valign : Gtk.Align.FILL
            );
            this.motion_notify_event.connect((e) => {
                Gdk.EventMotion pe = e;
                if (m_mouse_x == pe.x_root && m_mouse_y == pe.y_root)
                    return false;
                m_mouse_x = pe.x_root;
                m_mouse_y = pe.y_root;
                var row = this.get_row_at_y((int)e.y);
                if (row != null)
                    this.select_row(row);
                return false;
            });
            this.enter_notify_event.connect((e) => {
                // avoid gtk_button_update_state()
                return true;
            });
        }
    }
    private class EBoxRow : Gtk.ListBoxRow {
        public EBoxRow(string text) {
            this.text = text;
        }

        public string text { get; set; }
    }
    private class EScrolledWindow : Gtk.ScrolledWindow {
        public EScrolledWindow(Gtk.Adjustment? hadjustment=null,
                               Gtk.Adjustment? vadjustment=null) {
            GLib.Object(
                hscrollbar_policy : Gtk.PolicyType.NEVER,
                vscrollbar_policy : Gtk.PolicyType.NEVER,
                shadow_type : Gtk.ShadowType.IN,
                margin_start : 6,
                margin_end : 6,
                margin_top : 6,
                margin_bottom : 6
            );
            if (hadjustment != null)
                set_hadjustment(hadjustment);
            if (vadjustment != null)
                set_hadjustment(vadjustment);
        }
    }
    private class EGrid : Gtk.Grid {
        public EGrid() {
            GLib.Object(
                row_homogeneous : false,
                vexpand : true,
                halign : Gtk.Align.FILL,
                valign : Gtk.Align.FILL,
                row_spacing : 5,
                column_spacing : 5,
                border_width : 2
            );
        }
    }
    private class EWhiteLabel : Gtk.Label {
        private int m_minimum_width = 0;
        private int m_natural_width = 0;
        private int m_minimum_height = 0;
        private int m_natural_height = 0;
        public EWhiteLabel(string text) {
            GLib.Object(
                name : "IBusEmojierWhiteLabel"
            );
            set_label(text);
        }
        public override void get_preferred_width(out int minimum_width,
                                                 out int natural_width) {
            if (m_minimum_height == 0 && m_natural_height == 0) {
                base.get_preferred_height(out m_minimum_height,
                                          out m_natural_height);
            }
            var text = get_label();
            var ch = text.get_char();
            if (text.length == 1 && ch == '\t') {
                m_minimum_width = minimum_width = m_minimum_height;
                m_natural_width = natural_width = m_natural_height;
                return;
            }
            base.get_preferred_width(out minimum_width, out natural_width);
            if (text.length == 1 && (ch == '\n' || ch == '\r')) {
                minimum_width /= 2;
                natural_width /= 2;
                m_minimum_width = minimum_width;
                m_natural_width = natural_width;
                return;
            }
            if (minimum_width < m_minimum_height)
                minimum_width = m_minimum_height;
            if (natural_width < m_natural_height)
                natural_width = m_natural_height;
            m_minimum_width = minimum_width;
            m_natural_width = natural_width;
        }
        public override void get_preferred_height(out int minimum_height,
                                                  out int natural_height) {
            if (m_minimum_width == 0 && m_natural_width == 0) {
                base.get_preferred_width(out m_minimum_width,
                                         out m_natural_width);
            }
            var text = get_label();
            var ch = text.get_char();
            if (text.length == 1 && ch == '\v') {
                m_minimum_height = minimum_height = m_minimum_width;
                m_natural_height = natural_height = m_natural_width;
                return;
            }
            base.get_preferred_height(out minimum_height, out natural_height);
            if (text.length == 1 && (ch == '\n' || ch == '\r')) {
                minimum_height /= 2;
                natural_height /= 2;
                m_minimum_height = minimum_height;
                m_natural_height = natural_height;
                return;
            }
            m_minimum_height = minimum_height;
            m_natural_height = natural_height;
        }
    }
    private class ESelectedLabel : EWhiteLabel {
        public ESelectedLabel(string text) {
            GLib.Object(
                name : "IBusEmojierSelectedLabel"
            );
            if (text != "")
                set_label(text);
        }
    }
    private class EGoldLabel : EWhiteLabel {
        public EGoldLabel(string text) {
            GLib.Object(
                name : "IBusEmojierGoldLabel"
            );
            if (text != "")
                set_label(text);
        }
    }
    private class EPaddedLabel : Gtk.Label {
        public EPaddedLabel(string          text,
                            Gtk.Align       align) {
            GLib.Object(
                name : "IBusEmojierPaddedLabel",
                halign : align,
                valign : Gtk.Align.CENTER,
                margin_start : 20,
                margin_end : 20,
                margin_top : 6,
                margin_bottom : 6
            );
            set_text(text);
        }
    }
    private class EPaddedLabelBox : Gtk.Box {
        public EPaddedLabelBox(string          text,
                               Gtk.Align       align,
                               TravelDirection direction=TravelDirection.NONE,
                               string?         caption=null) {
            GLib.Object(
                name : "IBusEmojierPaddedLabelBox",
                orientation : Gtk.Orientation.HORIZONTAL,
                spacing : 0
            );
            if (direction == TravelDirection.BACKWARD) {
                IconWidget icon;
                if (Gtk.Widget.get_default_direction() ==
                    Gtk.TextDirection.RTL) {
                    icon = new IconWidget("go-previous-rtl-symbolic",
                                          Gtk.IconSize.MENU);
                } else {
                    icon = new IconWidget("go-previous-symbolic",
                                          Gtk.IconSize.MENU);
                }
                pack_start(icon, false, true, 0);
            }
            EPaddedLabel label = new EPaddedLabel(text, align);
            pack_start(label, true, true, 0);
            if (caption != null) {
                EPaddedLabel label_r = new EPaddedLabel(caption,
                                                        Gtk.Align.END);
                pack_end(label_r, true, true, 0);
            }
        }
    }
    private class LoadProgressObject : GLib.Object {
        public LoadProgressObject() {
        }
        public signal void deserialize_unicode(uint done, uint total);
    }


    private enum TravelDirection {
        NONE,
        BACKWARD,
    }

    public bool is_wayland { get; set; }

    public const uint BUTTON_CLOSE_BUTTON = 1000;

    private const uint EMOJI_GRID_PAGE = 10;
    private const string EMOJI_CATEGORY_FAVORITES = N_("Favorites");
    private const string EMOJI_CATEGORY_OTHERS = N_("Others");
    private const string EMOJI_CATEGORY_UNICODE = N_("Open Unicode choice");
    private const unichar[] EMOJI_VARIANT_LIST = {
            0x1f3fb, 0x1f3fc, 0x1f3fd, 0x1f3fe, 0x1f3ff, 0x200d };

    // Set the actual default values in the constructor
    // because these fields are used for class_init() and static functions,
    // e.g. set_emoji_font(), can be called before class_init() is called.
    private static string m_current_lang_id;
    private static string m_emoji_font_family;
    private static int m_emoji_font_size;
    private static bool m_emoji_font_changed = false;
    private static string[] m_favorites;
    private static string[] m_favorite_annotations;
    private static int m_emoji_max_seq_len;
    private static bool m_has_partial_match;
    private static uint m_partial_match_length;
    private static uint m_partial_match_condition;
    private static bool m_show_emoji_variant = false;
    private static int m_default_window_width;
    private static int m_default_window_height;
    private static GLib.HashTable<string, GLib.SList<string>>?
            m_annotation_to_emojis_dict;
    private static GLib.HashTable<string, IBus.EmojiData>?
            m_emoji_to_data_dict;
    private static GLib.HashTable<string, GLib.SList<string>>?
            m_category_to_emojis_dict;
    private static GLib.HashTable<string, GLib.SList<string>>?
            m_emoji_to_emoji_variants_dict;
    private static GLib.HashTable<unichar, IBus.UnicodeData>?
            m_unicode_to_data_dict;
    private static GLib.HashTable<string, GLib.SList<unichar>>?
            m_name_to_unicodes_dict;
    private static GLib.SList<IBus.UnicodeBlock> m_unicode_block_list;
    private static bool m_show_unicode = false;
    private static LoadProgressObject m_unicode_progress_object;
    private static bool m_loaded_unicode = false;
    private static string m_warning_message = "";

    private ThemedRGBA m_rgba;
    private Gtk.Box m_vbox;
    /* If emojier is emoji category list or Unicode category list,
     * m_annotation is "" and preedit is also "".
     * If emojier is candidate mode, m_annotation is an annotation and
     * get_current_candidate() returns the current emoji.
     * But the current preedit can be "" in candidate mode in case that
     * Unicode candidate window has U+0000.
     */
    private string m_annotation = "";
    private string? m_backward;
    private int m_backward_index = -1;
    private EScrolledWindow? m_scrolled_window = null;
    private EListBox m_list_box;
    private bool m_is_running = false;
    private string m_input_context_path = "";
    private GLib.MainLoop? m_loop;
    private string? m_result;
    /* If m_candidate_panel_is_visible is true, emojier is candidate mode and
     * the emoji lookup window is visible.
     * If m_candidate_panel_is_visible is false, the emoji lookup window is
     * not visible but the mode is not clear.
     */
    private bool m_candidate_panel_is_visible;
    /* If m_candidate_panel_mode is true, emojier is candidate mode and
     * it does not depend on whether the window is visible or not.
     * I.E. the first candidate does not show the lookup window and the
     * second one shows the window.
     * If m_candidate_panel_mode is false, emojier is emoji category list or
     * Unicode category list.
     */
    private bool m_candidate_panel_mode;
    private int m_category_active_index = -1;
    private IBus.LookupTable m_lookup_table;
    private Gtk.Label[] m_candidates;
    private bool m_enter_notify_enable = true;
    private uint m_entry_notify_show_id;
    private uint m_entry_notify_disable_id;
    protected static double m_mouse_x;
    protected static double m_mouse_y;
    private Gtk.ProgressBar m_unicode_progress_bar;
    private uint m_unicode_progress_id;
    private Gtk.Label m_unicode_percent_label;
    private double m_unicode_percent;
    private ulong m_unicode_deserialize_unicode_signal_id;
    private Gdk.Rectangle m_cursor_location;
    private bool m_is_up_side_down = false;
    private uint m_redraw_window_id;
    private bool m_rebuilding_gui = false;
    private uint m_rebuilding_gui_timeout_id;

    public signal void candidate_clicked(uint index, uint button, uint state);
    public signal void commit_text(string text);
    public signal void cancel();
    public signal void send_message(IBus.Message message);

    public IBusEmojier(bool is_wayland) {
        GLib.Object(
            type : Gtk.WindowType.POPUP
        );
        this.is_wayland = is_wayland;

        // GLib.ActionEntry accepts const variables only.
        var action = new GLib.SimpleAction.stateful(
                "variant",
                null,
                new GLib.Variant.boolean(m_show_emoji_variant));
        action.activate.connect(check_action_variant_cb);
        add_action(action);
        action = new GLib.SimpleAction("close", null);
        action.activate.connect(action_close_cb);
        add_action(action);
        if (m_current_lang_id == null)
            m_current_lang_id = "en";
        if (m_emoji_font_family == null) {
            m_emoji_font_family = "Monospace";
            m_emoji_font_changed = true;
        }
        if (m_emoji_font_size == 0) {
            m_emoji_font_size = 16;
            m_emoji_font_changed = true;
        }
        if (m_favorites == null)
            m_favorites = {};
        if (m_favorite_annotations == null)
            m_favorite_annotations = {};

        set_css_data();

        m_vbox = new Gtk.Box(Gtk.Orientation.VERTICAL, 0);
        add(m_vbox);

        // The constructor of IBus.LookupTable does not support more than
        // 16 pages.
        m_lookup_table = new IBus.LookupTable(1, 0, true, true);
        m_lookup_table.set_page_size(EMOJI_GRID_PAGE * EMOJI_GRID_PAGE);

        hide.connect(() => {
            if (m_loop != null && m_loop.is_running())
                m_loop.quit();
        });

        size_allocate.connect((w, a) => {
            adjust_window_position();
        });

        if (m_annotation_to_emojis_dict == null) {
            reload_emoji_dict();
        }

        get_load_progress_object();
    }


    private static void reload_emoji_dict() {
        init_emoji_dict();
        make_emoji_dict("en");
        if (m_current_lang_id != "en") {
            var lang_ids = m_current_lang_id.split("_");
            if (lang_ids.length > 1) {
                string sub_id = lang_ids[0];
                make_emoji_dict(sub_id);
            }
            make_emoji_dict(m_current_lang_id);
        }
        add_variants_to_component();

        GLib.List<unowned string> annotations =
                m_annotation_to_emojis_dict.get_keys();
        foreach (unowned string annotation in annotations) {
            if (m_emoji_max_seq_len < annotation.length)
                m_emoji_max_seq_len = annotation.length;
        }
        update_favorite_emoji_dict();
    }


    private static void init_emoji_dict() {
        m_annotation_to_emojis_dict =
                new GLib.HashTable<string, GLib.SList<string>>(GLib.str_hash,
                                                               GLib.str_equal);
        m_emoji_to_data_dict =
                new GLib.HashTable<string, IBus.EmojiData>(GLib.str_hash,
                                                           GLib.str_equal);
        m_category_to_emojis_dict =
                new GLib.HashTable<string, GLib.SList<string>>(GLib.str_hash,
                                                               GLib.str_equal);
        m_emoji_to_emoji_variants_dict =
                new GLib.HashTable<string, GLib.SList<string>>(GLib.str_hash,
                                                               GLib.str_equal);
        if (m_unicode_to_data_dict == null) {
            m_unicode_to_data_dict =
                new GLib.HashTable<unichar, IBus.UnicodeData>(
                        GLib.direct_hash,
                        GLib.direct_equal);
        }
        if (m_name_to_unicodes_dict == null) {
            m_name_to_unicodes_dict =
                new GLib.HashTable<string, GLib.SList<unichar>>(GLib.str_hash,
                                                                GLib.str_equal);
        }
    }


    private static void make_emoji_dict(string lang) {
        GLib.SList<IBus.EmojiData> emoji_list = IBus.EmojiData.load(
                    Config.PKGDATADIR + "/dicts/emoji-" + lang + ".dict");
        if (emoji_list == null)
            return;
        foreach (IBus.EmojiData data in emoji_list) {
            update_emoji_to_data_dict(data, lang);
            update_annotation_to_emojis_dict(data);
            update_category_to_emojis_dict(data, lang);
        }
    }


    private static void add_variants_to_component() {
        string category = "Component";
        unowned GLib.SList<string> hits =
                m_category_to_emojis_dict.lookup(category);
        if (hits == null) {
            category = "component";
            hits = m_category_to_emojis_dict.lookup(category);
        }
        if (hits == null)
            return;
        GLib.SList<IBus.EmojiData> emoji_list =
                new GLib.SList<IBus.EmojiData>();
        GLib.SList<string> annotations = new GLib.SList<string>();
        annotations.append("zero");
        IBus.EmojiData _data;
        _data = new IBus.EmojiData("emoji", "\u200d",
                                   "annotations", annotations,
                                   "description",
                                   "ZERO WIDTH JOINER",
                                   "category", category);
        emoji_list.append(_data);
        annotations = null;
        annotations.append("text");
        annotations.append("variation");
        annotations.append("selector");
        _data = new IBus.EmojiData("emoji", "\ufe0e",
                                   "annotations", annotations,
                                   "description",
                                   "VARIATION SELECTOR-15",
                                   "category", category);
        emoji_list.append(_data);
        annotations = null;
        annotations.append("emoji");
        annotations.append("variation");
        annotations.append("selector");
        _data = new IBus.EmojiData("emoji", "\ufe0f",
                                   "annotations", annotations,
                                   "description",
                                   "VARIATION SELECTOR-16",
                                   "category", category);
        emoji_list.append(_data);
        foreach (IBus.EmojiData data in emoji_list) {
            update_emoji_to_data_dict(data, "en");
            update_annotation_to_emojis_dict(data);
            update_category_to_emojis_dict(data, "en");
        }
    }


    private static void update_annotation_to_emojis_dict(IBus.EmojiData data) {
        string emoji = data.get_emoji();
        unowned GLib.SList<string> annotations = data.get_annotations();
        foreach (string annotation in annotations) {
            bool has_emoji = false;
            GLib.SList<string> hits =
                    m_annotation_to_emojis_dict.lookup(annotation).copy_deep(
                            GLib.strdup);
            foreach (string hit_emoji in hits) {
                if (hit_emoji == emoji) {
                    has_emoji = true;
                    break;
                }
            }
            if (!has_emoji) {
                hits.append(emoji);
                m_annotation_to_emojis_dict.replace(
                        annotation,
                        hits.copy_deep(GLib.strdup));
            }
        }
    }


    private static string utf8_code_point(string str) {
        var buff = new GLib.StringBuilder();
        int length = str.char_count();
        if (length == 0) {
            buff.append("U+%04X".printf(0));
            return buff.str;
        }
        for (int i = 0; i < length; i++) {
            unichar ch = str.get_char(0);
            if (i == 0)
                buff.append("U+%04X".printf(ch));
            else
                buff.append(" %04X".printf(ch));
            str = str.next_char();
        }
        return buff.str;
    }


    private static string utf8_entity(string str) {
        var buff = new GLib.StringBuilder();
        int length = str.char_count();
        for (int i = 0; i < length; i++) {
            unichar ch = str.get_char(0);
            switch(ch) {
            case '<':
                buff.append("&lt;");
                break;
            case '>':
                buff.append("&gt;");
                break;
            case '&':
                buff.append("&amp;");
                break;
            default:
                buff.append_unichar(ch);
                break;
            }
            str = str.next_char();
        }
        return buff.str;
    }


    private static void
    update_annotations_with_description (IBus.EmojiData data,
                                         string         description) {
        GLib.SList<string> annotations =
                data.get_annotations().copy_deep(GLib.strdup);
        bool update_annotations = false;
        string former = null;
        string later = null;
        int index = description.index_of(": ");
        if (index > 0) {
            former = description.substring(0, index);
            if (annotations.find_custom(former, GLib.strcmp) == null) {
                annotations.append(former);
                update_annotations = true;
            }
            later = description.substring(index + 2);
        } else {
            later = description.dup();
        }
        var words = later.split(" ");
        // If the description has less than 3 words, add it to annotations
        // FIXME: How to cast GLib.CompareFunc<string> to strcmp?
        if (words.length < 3 &&
            annotations.find_custom(
                    later,
                    GLib.strcmp) == null) {
            annotations.append(later);
            update_annotations = true;
        }
        if (update_annotations)
            data.set_annotations(annotations.copy_deep(GLib.strdup));
    }


    private static void update_emoji_to_data_dict(IBus.EmojiData data,
                                                  string         lang) {
        string emoji = data.get_emoji();
        if (lang == "en") {
            string description = data.get_description().down();
            update_annotations_with_description (data, description);
            m_emoji_to_data_dict.replace(emoji, data);
        } else {
            unowned IBus.EmojiData? en_data = null;
            en_data = m_emoji_to_data_dict.lookup(emoji);
            if (en_data == null) {
                m_emoji_to_data_dict.insert(emoji, data);
                return;
            }
            string trans_description = data.get_description();
            en_data.set_description(trans_description);
            trans_description = trans_description.down();
            update_annotations_with_description (data, trans_description);
            unowned GLib.SList<string> annotations = data.get_annotations();
            unowned GLib.SList<string> en_annotations
                = en_data.get_annotations();
            foreach (string annotation in en_annotations) {
                // FIXME: How to cast GLib.CompareFunc<string> to strcmp?
                if (annotations.find_custom(
                            annotation,
                            GLib.strcmp) == null) {
                    annotations.append(annotation.dup());
                }
            }
            en_data.set_annotations(annotations.copy_deep(GLib.strdup));
        }
    }


    private static void update_category_to_emojis_dict(IBus.EmojiData data,
                                                       string         lang) {
        string emoji = data.get_emoji();
        string category = data.get_category();
        if (category == "")
            category = EMOJI_CATEGORY_OTHERS;
        if (lang == "en") {
            bool has_variant = false;
            if (category.ascii_casecmp("component") != 0) {
                foreach (unichar ch in EMOJI_VARIANT_LIST) {
                    if (emoji.index_of_char(ch) >= 0) {
                        has_variant = true;
                        break;
                    }
                }
            }
            // If emoji includes variants (skin colors and items),
            // it's escaped in m_emoji_to_emoji_variants_dict and
            // not shown by default.
            if (has_variant) {
                unichar base_ch = emoji.get_char();
                string base_emoji  = base_ch.to_string();
                var buff = new GLib.StringBuilder();
                buff.append_unichar(base_ch);
                buff.append_unichar(0xfe0f);
                if (m_emoji_to_data_dict.lookup(buff.str) != null)
                    base_emoji = buff.str;
                GLib.SList<string>? variants =
                        m_emoji_to_emoji_variants_dict.lookup(
                                base_emoji).copy_deep(GLib.strdup);
                if (variants.find_custom(emoji, GLib.strcmp) == null) {
                    if (variants == null)
                        variants.append(base_emoji);
                    if (base_emoji != emoji)
                        variants.append(emoji);
                    m_emoji_to_emoji_variants_dict.replace(
                            base_emoji,
                            variants.copy_deep(GLib.strdup));
                }
                return;
            }
            bool has_emoji = false;
            GLib.SList<string> hits =
                    m_category_to_emojis_dict.lookup(category).copy_deep(
                            GLib.strdup);
            foreach (string hit_emoji in hits) {
                if (hit_emoji == emoji) {
                    has_emoji = true;
                    break;
                }
            }
            if (!has_emoji) {
                hits.append(emoji);
                m_category_to_emojis_dict.replace(category,
                                                  hits.copy_deep(GLib.strdup));
            }
        }
    }


    private static void make_unicode_block_dict() {
        m_unicode_block_list = IBus.UnicodeBlock.load(
                    Config.PKGDATADIR + "/dicts/unicode-blocks.dict");
        foreach (unowned IBus.UnicodeBlock block in m_unicode_block_list) {
            unowned string name = block.get_name();
            if (m_emoji_max_seq_len < name.length)
                m_emoji_max_seq_len = name.length;
        }
    }


    private static void make_unicode_name_dict(Object source_object) {
        IBus.UnicodeData.load_async(
                    Config.PKGDATADIR + "/dicts/unicode-names.dict",
                    source_object,
                    null,
        (IBus.UnicodeDataLoadAsyncFinish)make_unicode_name_dict_finish);
    }

    private static void
    make_unicode_name_dict_finish(GLib.SList<IBus.UnicodeData> unicode_list) {
        if (unicode_list == null)
            return;
        foreach (IBus.UnicodeData data in unicode_list) {
            update_unicode_to_data_dict(data);
            update_name_to_unicodes_dict(data);
        }
        GLib.List<unowned string> names =
                m_name_to_unicodes_dict.get_keys();
        foreach (unowned string name in names) {
            if (m_emoji_max_seq_len < name.length)
                m_emoji_max_seq_len = name.length;
        }
        m_loaded_unicode = true;
    }


    private static void update_unicode_to_data_dict(IBus.UnicodeData data) {
        unichar code = data.get_code();
        m_unicode_to_data_dict.replace(code, data);
    }


    private static void update_name_to_unicodes_dict(IBus.UnicodeData data) {
        unichar code = data.get_code();
        string[] names = {data.get_name().down(), data.get_alias().down()};
        foreach (unowned string name in names) {
            if (name == "")
                continue;
            bool has_code = false;
            GLib.SList<unichar> hits =
                    m_name_to_unicodes_dict.lookup(name).copy();
            foreach (unichar hit_code in hits) {
                if (hit_code == code) {
                    has_code = true;
                    break;
                }
            }
            if (!has_code) {
                hits.append(code);
                m_name_to_unicodes_dict.replace(name, hits.copy());
            }
        }
    }


    private void set_css_data() {
        Gdk.Display display = get_display();
        Gdk.Screen screen = (display != null) ?
                display.get_default_screen() : null;

        if (screen == null) {
            warning("Could not open display.");
            return;
        }
        // Set en locale because de_DE's decimal_point is ',' instead of '.'
        string? backup_locale =
            Intl.setlocale(LocaleCategory.NUMERIC, null).dup();
        if (Intl.setlocale(LocaleCategory.NUMERIC, "en_US.UTF-8") == null) {
          if (Intl.setlocale(LocaleCategory.NUMERIC, "C.UTF-8") == null) {
              if (Intl.setlocale(LocaleCategory.NUMERIC, "C") == null) {
                  warning("You don't install either en_US.UTF-8 or C.UTF-8 " +
                          "or C locale");
              }
          }
        }
        if (m_rgba == null)
            m_rgba = new ThemedRGBA(this);
        uint bg_red = (uint)(m_rgba.normal_bg.red * 255);
        uint bg_green = (uint)(m_rgba.normal_bg.green * 255);
        uint bg_blue = (uint)(m_rgba.normal_bg.blue * 255);
        double bg_alpha = m_rgba.normal_bg.alpha;
        string data =
                "#IBusEmojierWhiteLabel { background-color: " +
                        "rgba(%u, %u, %u, %lf); ".printf(
                        bg_red, bg_green, bg_blue, bg_alpha) +
                "font-family: %s; font-size: %dpt; ".printf(
                        m_emoji_font_family, m_emoji_font_size) +
                "border-width: 4px; border-radius: 3px; } ";

        uint fg_red = (uint)(m_rgba.selected_fg.red * 255);
        uint fg_green = (uint)(m_rgba.selected_fg.green * 255);
        uint fg_blue = (uint)(m_rgba.selected_fg.blue * 255);
        double fg_alpha = m_rgba.selected_fg.alpha;
        bg_red = (uint)(m_rgba.selected_bg.red * 255);
        bg_green = (uint)(m_rgba.selected_bg.green * 255);
        bg_blue = (uint)(m_rgba.selected_bg.blue * 255);
        bg_alpha = m_rgba.selected_bg.alpha;
        data += "#IBusEmojierSelectedLabel { color: " +
                        "rgba(%u, %u, %u, %lf); ".printf(
                        fg_red, fg_green, fg_blue, fg_alpha) +
                "font-family: %s; font-size: %dpt; ".printf(
                        m_emoji_font_family, m_emoji_font_size) +
                "background-color: " +
                        "rgba(%u, %u, %u, %lf); ".printf(
                        bg_red, bg_green, bg_blue, bg_alpha) +
                "border-width: 4px; border-radius: 3px; }";
        data += "#IBusEmojierGoldLabel { color: " +
                        "rgba(%u, %u, %u, %lf); ".printf(
                        fg_red, fg_green, fg_blue, fg_alpha) +
                "font-family: %s; font-size: %dpt; ".printf(
                        m_emoji_font_family, m_emoji_font_size) +
                "background-color: #b09c5f; " +
                "border-width: 4px; border-radius: 3px; }";

        Gtk.CssProvider css_provider = new Gtk.CssProvider();
        try {
            css_provider.load_from_data(data, -1);
        } catch (GLib.Error e) {
            warning("Failed css_provider_from_data: %s", e.message);
            return;
        }
        if (backup_locale != null)
            Intl.setlocale(LocaleCategory.NUMERIC, backup_locale);
        else
            Intl.setlocale(LocaleCategory.NUMERIC, "");

        Gtk.StyleContext.add_provider_for_screen(
                screen,
                css_provider,
                Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION);
    }


    private void set_fixed_size() {
        resize(20, 1);
    }


    private void remove_all_children() {
        if (m_list_box != null) {
            foreach (Gtk.Widget w in m_list_box.get_children()) {
                w.destroy();
            }
            m_list_box = null;
        }
        foreach (Gtk.Widget w in m_vbox.get_children()) {
            if (w.name == "IBusEmojierEntry" ||
                w.name == "IBusEmojierTitleLabelBox") {
                continue;
            }
            w.destroy();
        }
    }


    private void clamp_page() {
        Gtk.ListBoxRow row;
        if (m_category_active_index >= 0) {
            row = m_list_box.get_row_at_index(m_category_active_index);
            m_list_box.select_row(row);
        } else {
            row = m_list_box.get_row_at_index(0);
        }
        Gtk.Allocation alloc = { 0, 0, 0, 0 };
        row.get_allocation(out alloc);
        var adjustment = m_scrolled_window.get_vadjustment();
        adjustment.clamp_page(alloc.y, alloc.y + alloc.height);
        return_if_fail(m_category_active_index >= 0);
        m_lookup_table.set_cursor_pos((uint)m_category_active_index);
    }


    private void show_category_list() {
        // Do not call remove_all_children() to work adjustment.clamp_page()
        // with PageUp/Down.
        // After show_candidate_panel() is called, m_category_active_index
        // is saved for Escape key but m_list_box is null by
        // remove_all_children().
        if (m_category_active_index >= 0 && m_list_box != null) {
            var row = m_list_box.get_row_at_index(m_category_active_index);
            m_list_box.select_row(row);
            return;
        }
        if (m_category_active_index < 0)
            m_category_active_index = 0;
        remove_all_children();
        m_scrolled_window = new EScrolledWindow();
        set_fixed_size();

        m_vbox.add(m_scrolled_window);
        Gtk.Viewport viewport = new Gtk.Viewport(null, null);
        m_scrolled_window.add(viewport);

        m_list_box = new EListBox();
        viewport.add(m_list_box);
        Gtk.Adjustment adjustment = m_scrolled_window.get_vadjustment();
        m_list_box.set_adjustment(adjustment);
        m_list_box.row_activated.connect((box, gtkrow) => {
            m_category_active_index = gtkrow.get_index();
            EBoxRow row = gtkrow as EBoxRow;
            show_emoji_for_category(row.text);
            show_all();
        });

        uint ncandidates = m_lookup_table.get_number_of_candidates();
        for (uint i = 0; i < ncandidates; i++) {
            string category = m_lookup_table.get_candidate(i).text;
            EBoxRow row = new EBoxRow(category);
            EPaddedLabelBox widget =
                    new EPaddedLabelBox(_(category), Gtk.Align.CENTER);
            row.add(widget);
            m_list_box.add(row);
            if (i == m_category_active_index)
                m_list_box.select_row(row);
        }

        m_scrolled_window.show_all();
        if (m_category_active_index == -1)
            m_list_box.unselect_all();
        m_list_box.invalidate_filter();
        m_list_box.set_selection_mode(Gtk.SelectionMode.SINGLE);
    }


    private void show_emoji_for_category(string category) {
        if (category == EMOJI_CATEGORY_FAVORITES) {
            m_lookup_table.clear();
            m_candidate_panel_mode = true;
            foreach (unowned string favorate in m_favorites) {
                IBus.Text text = new IBus.Text.from_string(favorate);
                m_lookup_table.append_candidate(text);
            }
            m_backward = category;
        } else if (category == EMOJI_CATEGORY_UNICODE) {
            m_category_active_index = -1;
            m_show_unicode = true;
            update_unicode_blocks();
            return;
        } else {
            // Use copy_deep() since vala 0.43.4 does not allow to assign
            // a weak pointer to the full one in SList:
            // emojier.vala:885.48-886.62: error: Assignment: Cannot convert
            // from `GLib.SList<string>' to `GLib.SList<weak string>?'
            GLib.SList<string> emojis =
                    m_category_to_emojis_dict.lookup(category).copy_deep(
                            GLib.strdup);
            m_lookup_table.clear();
            m_candidate_panel_mode = true;
            foreach (unowned string emoji in emojis) {
                IBus.Text text = new IBus.Text.from_string(emoji);
                m_lookup_table.append_candidate(text);
            }
            m_backward = category;
        }
        m_annotation = m_lookup_table.get_candidate(0).text;
        // Restore the cursor position before the special table of
        // emoji variants is shown.
        if (m_backward_index >= 0) {
            m_lookup_table.set_cursor_pos((uint)m_backward_index);
            m_backward_index = -1;
        }
    }


    private void show_emoji_variants(GLib.SList<string>? emojis) {
        m_backward_index = (int)m_lookup_table.get_cursor_pos();
        m_lookup_table.clear();
        foreach (unowned string emoji in emojis) {
            IBus.Text text = new IBus.Text.from_string(emoji);
            m_lookup_table.append_candidate(text);
        }
    }


    private void show_unicode_blocks() {
        // Do not call remove_all_children() to work adjustment.clamp_page()
        // with PageUp/Down.
        // After show_candidate_panel() is called, m_category_active_index
        // is saved for Escape key but m_list_box is null by
        // remove_all_children().
        if (m_category_active_index >= 0 && m_list_box != null) {
            var row = m_list_box.get_row_at_index(m_category_active_index);
            m_list_box.select_row(row);
            return;
        }
        if (m_category_active_index < 0)
            m_category_active_index = 0;
        m_show_unicode = true;
        if (m_default_window_width == 0 && m_default_window_height == 0)
            get_size(out m_default_window_width, out m_default_window_height);
        remove_all_children();
        set_fixed_size();

        EPaddedLabelBox label =
                new EPaddedLabelBox(_("Bring back emoji choice"),
                                    Gtk.Align.CENTER,
                                    TravelDirection.BACKWARD);
        Gtk.Button button = new Gtk.Button();
        button.add(label);
        m_vbox.add(button);
        button.show_all();
        button.button_press_event.connect((w, e) => {
            m_category_active_index = -1;
            m_show_unicode = false;
            start_rebuild_gui(false);
            hide_candidate_panel();
            show_all();
            return true;
        });
        m_scrolled_window = new EScrolledWindow();
        m_vbox.add(m_scrolled_window);
        Gtk.Viewport viewport = new Gtk.Viewport(null, null);
        m_scrolled_window.add(viewport);

        m_list_box = new EListBox();
        viewport.add(m_list_box);
        Gtk.Adjustment adjustment = m_scrolled_window.get_vadjustment();
        m_list_box.set_adjustment(adjustment);
        m_list_box.row_activated.connect((box, gtkrow) => {
            m_category_active_index = gtkrow.get_index();
            EBoxRow row = gtkrow as EBoxRow;
            show_unicode_for_block(row.text);
            show_candidate_panel();
        });

        uint n = 0;
        foreach (unowned IBus.UnicodeBlock block in m_unicode_block_list) {
            string name = block.get_name();
            string caption = "U+%08X".printf(block.get_start());
            EBoxRow row = new EBoxRow(name);
            EPaddedLabelBox widget =
                    new EPaddedLabelBox(_(name),
                                        Gtk.Align.CENTER,
                                        TravelDirection.NONE,
                                        caption);
            row.add(widget);
            m_list_box.add(row);
            if (n++ == m_category_active_index) {
                m_list_box.select_row(row);
            }
        }

        set_size_request(-1, m_default_window_height + 100);
        m_scrolled_window.set_policy(Gtk.PolicyType.NEVER,
                                     Gtk.PolicyType.AUTOMATIC);
        m_scrolled_window.show_all();
        if (m_category_active_index == -1)
            m_list_box.unselect_all();
        m_list_box.invalidate_filter();
        m_list_box.set_selection_mode(Gtk.SelectionMode.SINGLE);
        Gtk.ListBoxRow row = m_list_box.get_row_at_index((int)n - 1);

        // If clamp_page() would be called without the allocation signal,
        // the jumping page could be failed when returns from 
        // show_unicode_for_block() with Escape key.
        row.size_allocate.connect((w, a) => {
            clamp_page();
        });
    }


    private void show_unicode_for_block(string block_name) {
        unichar start = 0;
        unichar end = 0;
        foreach (unowned IBus.UnicodeBlock block in m_unicode_block_list) {
            string name = block.get_name();
            if (block_name == name) {
                start = block.get_start();
                end = block.get_end();
            }
        }
        m_lookup_table.clear();
        m_candidate_panel_mode = true;
        for (unichar ch = start; ch < end; ch++) {
            unowned IBus.UnicodeData? data =
                    m_unicode_to_data_dict.lookup(ch);
            if (data == null)
                continue;
            IBus.Text text = new IBus.Text.from_unichar(ch);
            m_lookup_table.append_candidate(text);
        }
        m_backward = block_name;
        if (m_lookup_table.get_number_of_candidates() > 0)
            m_annotation = m_lookup_table.get_candidate(0).text;
    }


    private void show_arrow_buttons() {
        Gtk.Button next_button = new Gtk.Button();
        next_button.clicked.connect(() => {
            m_lookup_table.page_down();
            show_candidate_panel();
        });
        next_button.set_image(new Gtk.Image.from_icon_name(
                              "go-down",
                              Gtk.IconSize.MENU));
        next_button.set_relief(Gtk.ReliefStyle.NONE);
        next_button.set_tooltip_text(_("Page Down"));

        Gtk.Button prev_button = new Gtk.Button();
        prev_button.clicked.connect(() => {
            m_lookup_table.page_up();
            show_candidate_panel();
        });
        prev_button.set_image(new Gtk.Image.from_icon_name(
                              "go-up",
                              Gtk.IconSize.MENU));
        prev_button.set_relief(Gtk.ReliefStyle.NONE);
        prev_button.set_tooltip_text(_("Page Up"));

        var menu = new GLib.Menu();
        menu.append(_("Show emoji variants"), "win.variant");
        menu.append(_("Close"), "win.close");
        var menu_button = new Gtk.MenuButton();
        menu_button.set_direction(Gtk.ArrowType.NONE);
        menu_button.set_valign(Gtk.Align.CENTER);
        menu_button.set_menu_model(menu);
        menu_button.set_relief(Gtk.ReliefStyle.NONE);
        menu_button.set_tooltip_text(_("Menu"));

        IBus.Text text = this.get_title_text();
        Pango.AttrList attrs = get_pango_attr_list_from_ibus_text(text);
        Gtk.Label title_label = new Gtk.Label(text.get_text());
        title_label.set_attributes(attrs);

        Gtk.Button? warning_button = null;
        if (m_warning_message != "") { 
            warning_button = new Gtk.Button();
            warning_button.set_tooltip_text(
                    _("Click to view a warning message"));
            warning_button.set_image(new Gtk.Image.from_icon_name(
                                  "dialog-warning",
                                  Gtk.IconSize.MENU));
            warning_button.set_relief(Gtk.ReliefStyle.NONE);
            warning_button.clicked.connect(() => {
                Gtk.Label warning_label = new Gtk.Label(m_warning_message);
                warning_label.set_line_wrap(true);
                warning_label.set_max_width_chars(40);
                Gtk.Popover popover = new Gtk.Popover(warning_button);
                popover.add(warning_label);
                popover.show_all();
                popover.popup();
            });
        }

        Gtk.Box buttons_hbox = new Gtk.Box(Gtk.Orientation.HORIZONTAL, 0);
        Gtk.Label state_label = new Gtk.Label(null);
        state_label.set_size_request(10, -1);
        state_label.set_halign(Gtk.Align.CENTER);
        state_label.set_valign(Gtk.Align.CENTER);
        buttons_hbox.pack_start(state_label, false, true, 0);
        buttons_hbox.pack_start(prev_button, false, false, 0);
        buttons_hbox.pack_start(next_button, false, false, 0);
        buttons_hbox.pack_start(title_label, false, false, 0);
        if (warning_button != null)
            buttons_hbox.pack_start(warning_button, false, false, 0);
        buttons_hbox.pack_end(menu_button, false, false, 0);
        m_vbox.pack_start(buttons_hbox, false, false, 0);
        buttons_hbox.show_all();
    }


    private void show_unicode_progress_bar() {
        m_unicode_progress_bar = new Gtk.ProgressBar();
        m_unicode_progress_bar.set_ellipsize(Pango.EllipsizeMode.MIDDLE);
        m_unicode_progress_bar.set_halign(Gtk.Align.CENTER);
        m_unicode_progress_bar.set_valign(Gtk.Align.CENTER);
        m_vbox.add(m_unicode_progress_bar);
        m_unicode_progress_bar.show();
        var hbox = new Gtk.Box(Gtk.Orientation.HORIZONTAL, 5);
        hbox.set_halign(Gtk.Align.CENTER);
        hbox.set_valign(Gtk.Align.CENTER);
        m_vbox.add(hbox);
        var label = new Gtk.Label(_("Loading a Unicode dictionary:"));
        hbox.pack_start(label, false, true, 0);
        m_unicode_percent_label = new Gtk.Label("");
        hbox.pack_start(m_unicode_percent_label, false, true, 0);
        hbox.show_all();

        if (m_unicode_deserialize_unicode_signal_id == 0) {
            m_unicode_deserialize_unicode_signal_id =
                    m_unicode_progress_object.deserialize_unicode.connect(
                            (i, n) => {
                                m_unicode_percent = (double)i / n;
                    });
        }
        if (m_unicode_progress_id > 0) {
            GLib.Source.remove(m_unicode_progress_id);
        }
        m_unicode_progress_id = GLib.Timeout.add(100, () => {
            m_unicode_progress_id = 0;
            m_unicode_progress_bar.set_fraction(m_unicode_percent);
            m_unicode_percent_label.set_text(
                    "%.0f%%\n".printf(m_unicode_percent * 100));
            m_unicode_progress_bar.show();
            m_unicode_percent_label.show();
            if (m_loaded_unicode) {
                show_candidate_panel();
            }
            return !m_loaded_unicode;
        });
    }


    private void show_unicode_popup(int progress) {
        var message = new IBus.Message(IBus.MessageDomain.PANEL,
                                       IBus.PanelServiceMsgCode.LOADING_UNICODE,
                                       _("IBus Emoji initialization"),
                                       _("Loading a Unicode dictionary:"),
                                       "timeout", 3,
                                       "progress", progress,
                                       "serial", 50001);
        send_message(message);
    }


    private static string? check_unicode_point(string annotation) {
        string unicode_point = null;
        // Add "0x" because uint64.ascii_strtoull() is not accessible
        // and need to use uint64.parse()
        var buff = new GLib.StringBuilder("0x");
        var retval = new GLib.StringBuilder();
        for (int i = 0; i < annotation.char_count(); i++) {
            unichar ch = annotation.get_char(i);
            if (ch == 0)
                return null;
            if (ch.isspace()) {
                unichar code = (unichar)uint64.parse(buff.str);
                buff.assign("0x");
                if (!code.validate())
                    return null;
                retval.append(code.to_string());
                continue;
            }
            if (!ch.isxdigit())
                return null;
            buff.append_unichar(ch);
        }
        unichar code = (unichar)uint64.parse(buff.str);
        if (!code.validate())
            return null;
        retval.append(code.to_string());
        unicode_point = retval.str;
        if (unicode_point == null)
            return null;
        return unicode_point;
    }


    private GLib.SList<string>?
    lookup_emojis_from_annotation(string annotation) {
        GLib.SList<string>? total_emojis = null;
        unowned GLib.SList<string>? sub_emojis = null;
        unowned GLib.SList<unichar>? sub_exact_unicodes = null;
        unowned GLib.SList<unichar>? sub_unicodes = null;
        int length = annotation.length;
        if (m_has_partial_match && length >= m_partial_match_length) {
            GLib.SList<string>? sorted_emojis = null;
            foreach (unowned string key in
                     m_annotation_to_emojis_dict.get_keys()) {
                if (key.length < length)
                    continue;
                bool matched = false;
                switch(m_partial_match_condition) {
                case 0:
                    if (key.has_prefix(annotation))
                        matched = true;
                    break;
                case 1:
                    if (key.has_suffix(annotation))
                        matched = true;
                    break;
                case 2:
                    if (key.index_of(annotation) >= 0)
                        matched = true;
                    break;
                default:
                    break;
                }
                if (!matched)
                    continue;
                sub_emojis = m_annotation_to_emojis_dict.lookup(key);
                foreach (unowned string emoji in sub_emojis) {
                    if (total_emojis.find_custom(emoji, GLib.strcmp) == null) {
                        sorted_emojis.insert_sorted(emoji, GLib.strcmp);
                    }
                }
            }
            foreach (string emoji in sorted_emojis) {
                if (total_emojis.find_custom(emoji, GLib.strcmp) == null) {
                    total_emojis.append(emoji);
                }
            }
        } else {
            sub_emojis = m_annotation_to_emojis_dict.lookup(annotation);
            foreach (unowned string emoji in sub_emojis)
                total_emojis.append(emoji);
        }
        sub_exact_unicodes = m_name_to_unicodes_dict.lookup(annotation);
        foreach (unichar code in sub_exact_unicodes) {
            string ch = code.to_string();
            if (total_emojis.find_custom(ch, GLib.strcmp) == null) {
                total_emojis.append(ch);
            }
        }
        if (length >= m_partial_match_length) {
            GLib.SList<string>? sorted_unicodes = null;
            foreach (unowned string key in m_name_to_unicodes_dict.get_keys()) {
                bool matched = false;
                if (key.index_of(annotation) >= 0)
                        matched = true;
                if (!matched)
                    continue;
                sub_unicodes = m_name_to_unicodes_dict.lookup(key);
                foreach (unichar code in sub_unicodes) {
                    string ch = code.to_string();
                    if (sorted_unicodes.find_custom(ch, GLib.strcmp) == null) {
                        sorted_unicodes.insert_sorted(ch, GLib.strcmp);
                    }
                }
            }
            foreach (string ch in sorted_unicodes) {
                if (total_emojis.find_custom(ch, GLib.strcmp) == null) {
                    total_emojis.append(ch);
                }
            }
        }
        if (!m_loaded_unicode && m_unicode_deserialize_unicode_signal_id == 0) {
            m_unicode_deserialize_unicode_signal_id =
                    m_unicode_progress_object.deserialize_unicode.connect(
                            (i, n) => {
                                m_unicode_percent = (double)i / n;
                                show_unicode_popup(
                                        (int)(m_unicode_percent * 100));
                    });
        }
        if (!m_loaded_unicode)
            show_unicode_popup(0);
        return total_emojis;
    }


    private void update_candidate_window() {
        string annotation = m_annotation;
        if (annotation.length == 0) {
            m_backward = null;
            return;
        }
        m_lookup_table.clear();
        m_category_active_index = -1;
        if (annotation.length > m_emoji_max_seq_len) {
            return;
        }
        string? unicode_point = check_unicode_point(annotation);
        GLib.SList<string>? total_emojis = null;
        if (annotation.ascii_casecmp("history") == 0) {
            for (int i = 0; i < m_favorites.length; i++) {
                total_emojis.append(m_favorites[i].dup());
            }
        }
        if (total_emojis == null)
            total_emojis = lookup_emojis_from_annotation(annotation);
        if (total_emojis == null) {
            /* Users can type title strings against lower case.
             * E.g. "Smile" against "smile"
             * But the dictionary has the case sensitive annotations.
             * E.g. ":D" and ":q"
             * So need to call lookup_emojis_from_annotation() twice.
             */
            annotation = annotation.down();
            total_emojis = lookup_emojis_from_annotation(annotation);
        }
        if (total_emojis == null && unicode_point == null) {
            return;
        }
        if (unicode_point != null) {
            IBus.Text text = new IBus.Text.from_string(unicode_point);
            m_lookup_table.append_candidate(text);
        }
        foreach (unowned string emoji in total_emojis) {
            IBus.Text text = new IBus.Text.from_string(emoji);
            m_lookup_table.append_candidate(text);
        }
        m_candidate_panel_is_visible =
            (m_lookup_table.get_number_of_candidates() > 0) ? true : false;
        m_candidate_panel_mode = true;
    }


    private void update_category_list() {
        // Always update m_lookup_table even if the contents are same
        // because m_category_active_index needs to be kept after
        // bring back this API from show_emoji_for_category().
        reset_window_mode();
        m_lookup_table.clear();
        IBus.Text text;
        if (m_favorites.length > 0) {
            text = new IBus.Text.from_string(EMOJI_CATEGORY_FAVORITES);
            m_lookup_table.append_candidate(text);
        }
        GLib.List<unowned string> categories =
                m_category_to_emojis_dict.get_keys();
        // FIXME: How to cast GLib.CompareFunc<string> to strcmp?
        categories.sort((a, b) => {
            if (a == EMOJI_CATEGORY_OTHERS && b != EMOJI_CATEGORY_OTHERS)
                return 1;
            else if (a != EMOJI_CATEGORY_OTHERS && b == EMOJI_CATEGORY_OTHERS)
                return -1;
            return GLib.strcmp(_(a), _(b));
        });
        foreach (unowned string category in categories) {
            // "Others" category includes next unicode chars and fonts do not
            // support the base and varints yet.
            if (category == EMOJI_CATEGORY_OTHERS)
               continue;
            text = new IBus.Text.from_string(category);
            m_lookup_table.append_candidate(text);
        }
        if (m_unicode_block_list.length() > 0) {
            text = new IBus.Text.from_string(EMOJI_CATEGORY_UNICODE);
            m_lookup_table.append_candidate(text);
        }
        // Do not set m_category_active_index to 0 here so that
        // show_category_list() handles it.
    }


    private void update_unicode_blocks() {
        // Always update m_lookup_table even if the contents are same
        // because m_category_active_index needs to be kept after
        // bring back this API from show_emoji_for_category().
        reset_window_mode();
        m_lookup_table.clear();
        m_show_unicode = true;
        foreach (unowned IBus.UnicodeBlock block in m_unicode_block_list) {
            string name = block.get_name();
            IBus.Text text = new IBus.Text.from_string(name);
            m_lookup_table.append_candidate(text);
        }
        // Do not set m_category_active_index to 0 here so that
        // show_unicode_blocks() handles it.
    }


    private void show_code_point_description(string text) {
        EPaddedLabelBox widget_code = new EPaddedLabelBox(
                    _("Code point: %s").printf(utf8_code_point(text)),
                    Gtk.Align.START);
        m_vbox.add(widget_code);
        widget_code.show_all();
        if (m_emoji_to_emoji_variants_dict.lookup(text) != null) {
            EPaddedLabelBox widget_has_variant = new EPaddedLabelBox(
                    _("Has emoji variants"),
                    Gtk.Align.START);
            m_vbox.add(widget_has_variant);
            widget_has_variant.show_all();
        }
    }


    private void show_candidate_panel() {
        remove_all_children();
        set_fixed_size();
        if (m_emoji_font_changed) {
            set_css_data();
            m_emoji_font_changed = false;
        }
        uint page_size = m_lookup_table.get_page_size();
        uint ncandidates = m_lookup_table.get_number_of_candidates();
        uint cursor = m_lookup_table.get_cursor_pos();
        uint page_start_pos = cursor / page_size * page_size;
        uint page_end_pos = uint.min(page_start_pos + page_size, ncandidates);
        Gtk.Button? backward_button = null;
        if (m_backward != null) {
            string backward_desc = _(m_backward);
            EPaddedLabelBox label =
                    new EPaddedLabelBox(backward_desc,
                                        Gtk.Align.CENTER,
                                        TravelDirection.BACKWARD);
            backward_button = new Gtk.Button();
            backward_button.add(label);
            backward_button.button_press_event.connect((w, e) => {
                // Bring back to emoji candidate panel in case
                // m_show_emoji_variant is enabled and shows variants.
                if (m_backward_index >= 0 && m_backward != null) {
                    show_emoji_for_category(m_backward);
                    show_candidate_panel();
                } else {
                    start_rebuild_gui(false);
                    hide_candidate_panel();
                    show_all();
                }
                return true;
            });
        }
        EGrid grid = new EGrid();
        int n = 0;
        for (uint i = page_start_pos; i < page_end_pos; i++) {
            string text = m_lookup_table.get_candidate(i).text;
            bool has_variant =
                    (m_emoji_to_emoji_variants_dict.lookup(text) != null);
            Gtk.Label label;
            // If 'i' is the cursor position, use the selected color.
            // If the emoji has emoji variants, use the gold color.
            // Otherwise the white color.
            if (i == cursor) {
                label = new ESelectedLabel(text) as Gtk.Label;
            } else if (m_show_emoji_variant && has_variant &&
                       m_backward_index < 0) {
                label = new EGoldLabel(text) as Gtk.Label;
            } else {
                label = new EWhiteLabel(text) as Gtk.Label;
            }
            if (text.char_count() > 2) {
                string font_family = m_emoji_font_family;
                int font_size = m_emoji_font_size - 2;
                string emoji_font = "%s %d".printf(font_family, font_size);
                string markup = "<span font=\"%s\">%s</span>".
                        printf(emoji_font, utf8_entity(text));
                label.set_markup(markup);
            }
            label.set_halign(Gtk.Align.FILL);
            label.set_valign(Gtk.Align.FILL);
            Gtk.EventBox candidate_ebox = new Gtk.EventBox();
            candidate_ebox.add_events(Gdk.EventMask.POINTER_MOTION_MASK);
            candidate_ebox.add(label);
            // Make a copy of i to workaround a bug in vala.
            // https://bugzilla.gnome.org/show_bug.cgi?id=628336
            uint index = i;
            candidate_ebox.button_press_event.connect((w, e) => {
                candidate_clicked(index, e.button, e.state);
                return true;
            });
            candidate_ebox.motion_notify_event.connect((e) => {
                // m_enter_notify_enable is added because
                // enter_notify_event conflicts with keyboard operations.
                if (!m_enter_notify_enable)
                    return false;
                if (m_lookup_table.get_cursor_pos() == index)
                    return false;
                Gdk.EventMotion pe = e;
                if (m_mouse_x == pe.x_root && m_mouse_y == pe.y_root)
                    return false;
                m_mouse_x = pe.x_root;
                m_mouse_y = pe.y_root;

                m_lookup_table.set_cursor_pos(index);
                if (m_entry_notify_show_id > 0 &&
                    GLib.MainContext.default().find_source_by_id(
                            m_entry_notify_show_id) != null) {
                        GLib.Source.remove(m_entry_notify_show_id);
                }
                // If timeout is not added, memory leak happens and
                // button_press_event signal does not work above.
                m_entry_notify_show_id = GLib.Timeout.add(100, () => {
                        show_candidate_panel();
                        return false;
                });
                return false;
            });
            grid.attach(candidate_ebox,
                        n % (int)EMOJI_GRID_PAGE, n / (int)EMOJI_GRID_PAGE,
                        1, 1);
            n++;

            m_candidates += label;
        }
        m_candidate_panel_is_visible = true;
        if (!m_is_up_side_down) {
            show_arrow_buttons();
            if (backward_button != null) {
                m_vbox.add(backward_button);
                backward_button.show_all();
            }
            if (n > 0) {
                m_vbox.add(grid);
                grid.show_all();
                show_description();
            }
            if (!m_loaded_unicode)
                show_unicode_progress_bar();
        } else {
            if (!m_loaded_unicode)
                show_unicode_progress_bar();
            if (n > 0) {
                show_description();
                m_vbox.add(grid);
                grid.show_all();
            }
            if (backward_button != null) {
                m_vbox.add(backward_button);
                backward_button.show_all();
            }
            show_arrow_buttons();
        }
    }


    private void show_description() {
        uint cursor = m_lookup_table.get_cursor_pos();
        string text = m_lookup_table.get_candidate(cursor).text;
        unowned IBus.EmojiData? data = m_emoji_to_data_dict.lookup(text);
        if (data != null) {
            show_emoji_description(data, text);
            return;
        }
        if (text.char_count() <= 1) {
            unichar code = text.get_char();
            unowned IBus.UnicodeData? udata =
                    m_unicode_to_data_dict.lookup(code);
            if (udata != null) {
                show_unicode_description(udata, text);
                return;
            }
        }
        EPaddedLabelBox widget = new EPaddedLabelBox(
                _("Description: %s").printf(_("None")),
                Gtk.Align.START);
        m_vbox.add(widget);
        widget.show_all();
        show_code_point_description(text);
    }


    private void show_emoji_description(IBus.EmojiData data,
                                        string         text) {
        unowned string description = data.get_description();
        {
            EPaddedLabelBox widget = new EPaddedLabelBox(
                    _("Description: %s").printf(description),
                    Gtk.Align.START);
            m_vbox.add(widget);
            widget.show_all();
        }
        GLib.SList<string> annotations =
                data.get_annotations().copy_deep(GLib.strdup);
        var buff = new GLib.StringBuilder();
        int i = 0;
        foreach (unowned string annotation in annotations) {
            if (i++ == 0)
                buff.append_printf(_("Annotations: %s"), annotation);
            else
                buff.append_printf(" | %s", annotation);
            if (buff.str.char_count() > 30) {
                EPaddedLabelBox widget =
                        new EPaddedLabelBox(buff.str,
                                            Gtk.Align.START);
                m_vbox.add(widget);
                widget.show_all();
                buff.erase();
            }
        }
        if (buff.str != "") {
            EPaddedLabelBox widget = new EPaddedLabelBox(buff.str,
                                                         Gtk.Align.START);
            m_vbox.add(widget);
            widget.show_all();
        }
        show_code_point_description(text);
    }

    private void show_unicode_description(IBus.UnicodeData data,
                                          string           text) {
        unowned string name = data.get_name();
        {
            EPaddedLabelBox widget = new EPaddedLabelBox(
                    _("Name: %s").printf(name),
                    Gtk.Align.START);
            m_vbox.add(widget);
            widget.show_all();
        }
        unowned string alias = data.get_alias();
        {
            EPaddedLabelBox widget = new EPaddedLabelBox(
                    _("Alias: %s").printf(alias),
                    Gtk.Align.START);
            m_vbox.add(widget);
            widget.show_all();
        }
        show_code_point_description(text);
    }


    private void hide_candidate_panel() {
        hide();
        m_enter_notify_enable = true;
        m_annotation = "";
        // Call remove_all_children() instead of show_category_list()
        // so that show_category_list do not remove children with
        // PageUp/PageDown.
        remove_all_children();
        if (m_show_unicode)
            update_unicode_blocks();
        else
            update_category_list();
    }


    private void enter_notify_disable_with_timer() {
        // Enable keyboard operation and disable mouse operation.
        m_enter_notify_enable = false;
        if (m_entry_notify_disable_id > 0) {
            GLib.Source.remove(m_entry_notify_disable_id);
        }
        // Bring back the mouse operation after a timeout.
        m_entry_notify_show_id = GLib.Timeout.add(100, () => {
            m_enter_notify_enable = true;
            return false;
        });
    }


    private void candidate_panel_cursor_down() {
        enter_notify_disable_with_timer();
        uint ncandidates = m_lookup_table.get_number_of_candidates();
        uint cursor = m_lookup_table.get_cursor_pos();
        if ((cursor + EMOJI_GRID_PAGE) < ncandidates) {
            m_lookup_table.set_cursor_pos(cursor + EMOJI_GRID_PAGE);
        } else if (cursor % EMOJI_GRID_PAGE < ncandidates) {
                m_lookup_table.set_cursor_pos(cursor % EMOJI_GRID_PAGE);
        } else {
            m_lookup_table.set_cursor_pos(0);
        }
    }


    private void candidate_panel_cursor_up() {
        enter_notify_disable_with_timer();
        int ncandidates = (int)m_lookup_table.get_number_of_candidates();
        int cursor = (int)m_lookup_table.get_cursor_pos();
        int highest_pos =
            ((ncandidates - 1)/ (int)EMOJI_GRID_PAGE * (int)EMOJI_GRID_PAGE)
            + (cursor % (int)EMOJI_GRID_PAGE);
        if ((cursor - (int)EMOJI_GRID_PAGE) >= 0) {
            m_lookup_table.set_cursor_pos(cursor - (int)EMOJI_GRID_PAGE);
        } else if (highest_pos < ncandidates) {
            m_lookup_table.set_cursor_pos(highest_pos);
        } else {
            m_lookup_table.set_cursor_pos(0);
        }
    }


    private int get_page_num() {
        if (m_category_active_index < 0)
            m_category_active_index = 0;
        var row = m_list_box.get_row_at_index(m_category_active_index);
        Gtk.Allocation alloc = { 0, 0, 0, 0 };
        row.get_allocation(out alloc);
        var adjustment = m_scrolled_window.get_vadjustment();
        var page_size = (int)adjustment.get_page_size();
        int page_num = page_size / alloc.height;
        page_num += ((page_size % alloc.height) > 0) ? 1 : 0;
        return page_num;
    }


    private bool category_list_cursor_move(uint keyval) {
        return_val_if_fail (m_list_box != null, false);
        GLib.List<weak Gtk.Widget> list = m_list_box.get_children();
        int length = (int)list.length();
        if (length == 0)
            return false;
        switch(keyval) {
        case Gdk.Key.Down:
            if (++m_category_active_index == length)
                m_category_active_index = 0;
            break;
        case Gdk.Key.Up:
            if (--m_category_active_index < 0)
                    m_category_active_index = length - 1;
            break;
        case Gdk.Key.Home:
            m_category_active_index = 0;
            break;
        case Gdk.Key.End:
            m_category_active_index = length - 1;
            break;
        case Gdk.Key.Page_Down:
            var page_num = get_page_num();
            if (m_category_active_index + 1 == length)
                m_category_active_index = 0;
            else if (m_category_active_index + page_num >= length)
                m_category_active_index = length - 1;
            else
                m_category_active_index += page_num;
            break;
        case Gdk.Key.Page_Up:
            var page_num = get_page_num();
            if (m_category_active_index  == 0)
                m_category_active_index = length - 1;
            else if (m_category_active_index - page_num < 0)
                m_category_active_index = 0;
            else
                m_category_active_index -= page_num;
            break;
        }
        var row = m_list_box.get_selected_row();
        if (row != null)
            m_list_box.unselect_row(row);
        clamp_page ();
        return true;
    }


    private void start_rebuild_gui(bool initial_launching) {
        if (!this.is_wayland)
            return;
        if (!initial_launching && !base.get_visible())
            return;
        if (initial_launching && base.get_visible())
            return;
        if (m_rebuilding_gui_timeout_id != 0) {
            GLib.Source.remove(m_rebuilding_gui_timeout_id);
            m_rebuilding_gui_timeout_id = 0;
        }

        m_rebuilding_gui = true;
        m_rebuilding_gui_timeout_id =
                GLib.Timeout.add_seconds(5, () => {
                    if (!m_rebuilding_gui) {
                        m_rebuilding_gui_timeout_id = 0;
                        return false;
                    }
                    debug("Rebuilding GUI is time out.");
                    m_rebuilding_gui = false;
                    m_rebuilding_gui_timeout_id = 0;
                    return false;
                },
                GLib.Priority.DEFAULT_IDLE);
    }


    public bool has_variants(uint index,
                             bool need_commit_signal) {
        if (index >= m_lookup_table.get_number_of_candidates())
            return false;
        string text = m_lookup_table.get_candidate(index).text;
        unowned GLib.SList<string>? emojis =
                m_emoji_to_emoji_variants_dict.lookup(text);
        if (m_show_emoji_variant && emojis != null &&
            m_backward_index < 0) {
            show_emoji_variants(emojis);
            return true;
        }
        m_result = text;
        if (need_commit_signal)
            commit_text(text);
        return false;
    }


    public bool key_press_cursor_horizontal(uint keyval,
                                            uint modifiers) {
        assert (keyval == Gdk.Key.Left || keyval == Gdk.Key.Right);

        if (m_candidate_panel_mode &&
            m_lookup_table.get_number_of_candidates() > 0) {
            enter_notify_disable_with_timer();
            if (keyval == Gdk.Key.Left)
                m_lookup_table.cursor_up();
            else if (keyval == Gdk.Key.Right)
                m_lookup_table.cursor_down();
        } else {
            // For Gdk.Key.f and Gdk.Key.b
            if (keyval == Gdk.Key.Left)
                keyval = Gdk.Key.Up;
            else if (keyval == Gdk.Key.Right)
                keyval = Gdk.Key.Down;
            return category_list_cursor_move(keyval);
        }
        return true;
    }


    public bool key_press_cursor_vertical(uint keyval,
                                          uint modifiers) {
        assert (keyval == Gdk.Key.Down || keyval == Gdk.Key.Up ||
                keyval == Gdk.Key.Page_Down || keyval == Gdk.Key.Page_Up);

        if ((modifiers & Gdk.ModifierType.SHIFT_MASK) != 0) {
            if (keyval == Gdk.Key.Down)
                keyval = Gdk.Key.Page_Down;
            else if (keyval == Gdk.Key.Up)
                keyval = Gdk.Key.Page_Up;
        }
        if ((m_candidate_panel_is_visible || m_annotation.length > 0)
            && m_lookup_table.get_number_of_candidates() > 0) {
            switch (keyval) {
            case Gdk.Key.Down:
                candidate_panel_cursor_down();
                break;
            case Gdk.Key.Up:
                candidate_panel_cursor_up();
                break;
            case Gdk.Key.Page_Down:
                enter_notify_disable_with_timer();
                m_lookup_table.page_down();
                break;
            case Gdk.Key.Page_Up:
                enter_notify_disable_with_timer();
                m_lookup_table.page_up();
                break;
            }
        } else {
            return category_list_cursor_move(keyval);
        }
        return true;
    }


    public bool key_press_cursor_home_end(uint keyval,
                                          uint modifiers) {
        assert (keyval == Gdk.Key.Home || keyval == Gdk.Key.End);

        uint ncandidates = m_lookup_table.get_number_of_candidates();
        if (m_candidate_panel_mode && ncandidates > 0) {
            enter_notify_disable_with_timer();
            if (keyval == Gdk.Key.Home) {
                m_lookup_table.set_cursor_pos(0);
            } else if (keyval == Gdk.Key.End) {
                m_lookup_table.set_cursor_pos(ncandidates - 1);
            }
            return true;
        }
        return category_list_cursor_move(keyval);
    }


    public bool key_press_escape() {
        if (m_show_unicode) {
            if (!m_candidate_panel_is_visible) {
                m_show_unicode = false;
                m_category_active_index = -1;
            }
            start_rebuild_gui(false);
            hide_candidate_panel();
            return true;
        } else if (m_backward_index >= 0 && m_backward != null) {
            // Escape on Emoji variants window does not call focus-out events
            // because hide() is not called here so start_rebuild_gui()
            // is not called.
            show_emoji_for_category(m_backward);
            return true;
        } else if (m_candidate_panel_is_visible && m_backward != null) {
            start_rebuild_gui(false);
            hide_candidate_panel();
            return true;
        }
        hide();
        if (m_candidate_panel_mode &&
            m_lookup_table.get_number_of_candidates() > 0) {
            // Call remove_all_children() instead of show_category_list()
            // so that show_category_list do not remove children with
            // PageUp/PageDown.
            remove_all_children();
        }
        cancel();
        return false;
    }


    public bool key_press_enter(bool need_commit_signal) {
        if (m_candidate_panel_is_visible) {
            uint index = m_lookup_table.get_cursor_pos();
            return has_variants(index, need_commit_signal);
        } else if (m_category_active_index >= 0) {
            Gtk.ListBoxRow gtkrow = m_list_box.get_selected_row();
            EBoxRow row = gtkrow as EBoxRow;
            if (m_show_unicode)
                show_unicode_for_block(row.text);
            else
                show_emoji_for_category(row.text);
        }
        return true;
    }


    private Gdk.Rectangle get_monitor_geometry() {
        Gdk.Rectangle monitor_area = { 0, };

        // Use get_monitor_geometry() instead of get_monitor_area().
        // get_monitor_area() excludes docks, but the lookup window should be
        // shown over them.
        Gdk.Monitor monitor = Gdk.Display.get_default().get_monitor_at_point(
                m_cursor_location.x,
                m_cursor_location.y);
        monitor_area = monitor.get_geometry();
        return monitor_area;
    }


    private void adjust_window_position() {
        Gdk.Point cursor_right_bottom = {
                m_cursor_location.x + m_cursor_location.width,
                m_cursor_location.y + m_cursor_location.height
        };

        Gtk.Allocation allocation;
        get_allocation(out allocation);
        Gdk.Point window_right_bottom = {
            cursor_right_bottom.x + allocation.width,
            cursor_right_bottom.y + allocation.height
        };

        Gdk.Rectangle monitor_area = get_monitor_geometry();
        int monitor_right = monitor_area.x + monitor_area.width;
        int monitor_bottom = monitor_area.y + monitor_area.height;

        int x, y;
        if (window_right_bottom.x > monitor_right)
            x = monitor_right - allocation.width;
        else
            x = cursor_right_bottom.x;
        if (x < 0)
            x = 0;

        bool changed = false;
        // Do not up side down frequently.
        // The first pos does not show the lookup table yet but the
        // preedit only and the second pos shows the lookup table.
        if (m_lookup_table.get_cursor_pos() != 1) {
            if (m_is_up_side_down)
                y = m_cursor_location.y - allocation.height;
            else
                y = cursor_right_bottom.y;
        } else if (window_right_bottom.y > monitor_bottom) {
            y = m_cursor_location.y - allocation.height;
            // Do not up side down in Wayland
            if (m_input_context_path == "") {
                changed = (m_is_up_side_down == false);
                m_is_up_side_down = true;
            } else {
                changed = (m_is_up_side_down == true);
                m_is_up_side_down = false;
            }
        } else {
            y = cursor_right_bottom.y;
            changed = (m_is_up_side_down == true);
            m_is_up_side_down = false;
        }
        if (y < 0)
            y = 0;

        move(x, y);
        if (changed) {
            if (m_redraw_window_id > 0)
                GLib.Source.remove(m_redraw_window_id);
            m_redraw_window_id = GLib.Timeout.add(100, () => {
                m_redraw_window_id = 0;
                this.show_all();
                return false;
            });
        }
    }


#if 0
    private void check_action_variant_cb(Gtk.MenuItem item) {
        Gtk.CheckMenuItem check = item as Gtk.CheckMenuItem;
        m_show_emoji_variant = check.get_active();
        // Redraw emoji candidate panel for m_show_emoji_variant
        if (m_candidate_panel_is_visible) {
            // DOTO: queue_draw() does not effect at the real time.
            this.queue_draw();
        }
    }
#else
    private void check_action_variant_cb(GLib.SimpleAction action,
                                         GLib.Variant?     parameter) {
        m_show_emoji_variant = !action.get_state().get_boolean();
        action.set_state(new GLib.Variant.boolean(m_show_emoji_variant));
        // Redraw emoji candidate panel for m_show_emoji_variant
        if (m_candidate_panel_is_visible) {
            // DOTO: queue_draw() does not effect at the real time.
            this.queue_draw();
        }
    }
#endif


    private void action_close_cb(GLib.SimpleAction action,
                                 GLib.Variant?     parameter) {
        candidate_clicked(0, BUTTON_CLOSE_BUTTON, 0);
    }


    public static void update_favorite_emoji_dict() {
        if (m_emoji_to_data_dict == null ||
            m_annotation_to_emojis_dict == null)
            return;

        for(int i = 0; i < m_favorites.length; i++) {
            var favorite = m_favorites[i];

            string? annotation = "";
            if (i < m_favorite_annotations.length) {
                annotation = m_favorite_annotations[i];
            }
            if (annotation == "")
                continue;
            unowned IBus.EmojiData? data =
                    m_emoji_to_data_dict.lookup(favorite);
            if (data == null) {
                GLib.SList<string> new_annotations = new GLib.SList<string>();
                new_annotations.append(annotation);
                IBus.EmojiData new_data = GLib.Object.new(
                            typeof(IBus.EmojiData),
                            "emoji", favorite.dup(),
                            "annotations", new_annotations,
                            "description", annotation.dup()
                    ) as IBus.EmojiData;
                m_emoji_to_data_dict.insert(favorite, new_data);
            } else {
                GLib.SList<string> annotations =
                        data.get_annotations().copy_deep(GLib.strdup);
                if (annotations.find_custom(annotation, GLib.strcmp) == null) {
                    annotations.append(annotation);
                    data.set_annotations(annotations.copy_deep(GLib.strdup));
                }
            }
            unowned GLib.SList<string> emojis =
                    m_annotation_to_emojis_dict.lookup(annotation);
            if (emojis.find_custom(favorite, GLib.strcmp) == null) {
                emojis.append(favorite);
                m_annotation_to_emojis_dict.replace(
                        annotation,
                        emojis.copy_deep(GLib.strdup));
            }
        }
    }


    public void set_annotation(string annotation) {
        m_annotation = annotation;
        remove_all_children();
        if (annotation.length > 0) {
            update_candidate_window();
        } else {
            if (m_show_unicode)
                update_unicode_blocks();
            else
                update_category_list();
        }
    }


    public IBus.LookupTable get_one_dimension_lookup_table() {
        var lookup_table = new IBus.LookupTable(EMOJI_GRID_PAGE, 0, true, true);
        uint i = 0;
        for (; i < m_lookup_table.get_number_of_candidates(); i++) {
            IBus.Text text = new IBus.Text.from_string("");
            text.copy(m_lookup_table.get_candidate(i));
            lookup_table.append_candidate(text);
        }
        if (i > 0)
            lookup_table.set_cursor_pos(m_lookup_table.get_cursor_pos());
        return lookup_table;
    }


    public uint get_number_of_candidates() {
        return m_lookup_table.get_number_of_candidates();
    }


    public uint get_cursor_pos() {
        return m_lookup_table.get_cursor_pos();
    }


    public void set_cursor_pos(uint cursor_pos) {
        m_lookup_table.set_cursor_pos(cursor_pos);
    }


    public string get_current_candidate() {
        // If category_list mode, do not show the category name on preedit.
        // If candidate_panel mode, the first space key does not show the
        // lookup table but the first candidate is available on preedit.
        if (!m_candidate_panel_mode)
            return "";
        uint cursor = m_lookup_table.get_cursor_pos();
        return m_lookup_table.get_candidate(cursor).text;
    }


    public IBus.Text get_title_text() {
        var language = _(IBus.get_language_name(m_current_lang_id));
        uint ncandidates = this.get_number_of_candidates();
        string main_title = _("Emoji Choice");
        if (m_show_unicode)
            main_title = _("Unicode Choice");
        var text = new IBus.Text.from_string(
                "%s (%s) (%u / %u)".printf(
                        main_title,
                        language,
                        this.get_cursor_pos() + 1,
                        ncandidates));
        int char_count = text.text.char_count();
        int start_index = -1;
        unowned string title = text.text;
        for (int i = 0; i < char_count; i++) {
            if (title.has_prefix(language)) {
                start_index = i;
                break;
            }
            title = title.next_char();
        }
        if (start_index >= 0) {
            var attr = new IBus.Attribute(
                    IBus.AttrType.FOREGROUND,
                    0x808080,
                    start_index,
                    start_index + language.char_count());
            var attrs = new IBus.AttrList();
            attrs.append(attr);
            text.set_attributes(attrs);
        }
        return text;
    }


#if 0
    public GLib.SList<string>? get_candidates() {
        if (m_annotation.length == 0) {
            return null;
        }
        if (m_annotation.length > m_emoji_max_seq_len) {
            return null;
        }
        string? unicode_point = check_unicode_point(m_annotation);
        GLib.SList<string>? total_emojis =
            lookup_emojis_from_annotation(m_annotation);
        if (total_emojis == null) {
            /* Users can type title strings against lower case.
             * E.g. "Smile" against "smile"
             * But the dictionary has the case sensitive annotations.
             * E.g. ":D" and ":q"
             * So need to call lookup_emojis_from_annotation() twice.
             */
            string lower_annotation = m_annotation.down();
            total_emojis = lookup_emojis_from_annotation(lower_annotation);
        }
        if (unicode_point != null)
            total_emojis.prepend(unicode_point);
        return total_emojis;
    }
#endif


#if 0
    public string run(string    input_context_path,
                      Gdk.Event event) {
        assert (m_loop == null);

        m_is_running = true;
        m_input_context_path = input_context_path;
        m_candidate_panel_is_visible = false;
        m_result = null;
        m_enter_notify_enable = true;
        m_show_unicode = false;

        /* Let gtk recalculate the window size. */
        resize(1, 1);

        show_category_list();
        show_all();

        /* Some window managers, e.g. MATE, GNOME, Plasma desktops,
         * does not give the keyboard focus when Emojier is lauched
         * twice with Ctrl-period via XIEvent, if present_with_time()
         * is not applied.
         * But XFCE4 desktop does not effect this bug.
         * Seems this is caused by the window manager's focus stealing
         * prevention logic:
         * https://mail.gnome.org/archives/gtk-devel-list/2017-May/msg00026.html
         */
        present_centralize(event);

        Gdk.Device pointer;
        Gdk.Seat seat = event.get_seat();
        if (seat == null) {
            var display = get_display();
            seat = display.get_default_seat();
        }
        pointer = seat.get_pointer();
        pointer.get_position_double(null,
                                    out m_mouse_x,
                                    out m_mouse_y);

        m_loop = new GLib.MainLoop();
        m_loop.run();
        m_loop = null;

        hide();
        // Make sure the switcher is hidden before returning from this function.
        while (Gtk.events_pending())
            Gtk.main_iteration();
        m_is_running = false;

        return m_result;
    }
#endif


    // override virtual functions
    public override void show_all() {
        // Ctrl-period, space keys causes focus-out/in events in GNOME Wayland.
        start_rebuild_gui(true);
        base.show_all();
        if (m_candidate_panel_mode)
            show_candidate_panel();
        else if (m_show_unicode)
            show_unicode_blocks();
        else
            show_category_list();
    }


    public override void hide() {
        base.hide();
        m_candidate_panel_is_visible = false;
        // m_candidate_panel_mode is not false in when you type something
        // during enabling the candidate panel.
        if (m_redraw_window_id > 0) {
            GLib.Source.remove(m_redraw_window_id);
            m_redraw_window_id = 0;
        }
        if (m_unicode_progress_id > 0) {
            GLib.Source.remove(m_unicode_progress_id);
            m_unicode_progress_id = 0;
        }
    }


    public override bool key_press_event(Gdk.EventKey event) {
        uint keyval = event.keyval;
        uint modifiers = event.state;

        /* Need to handle events in key_press_event() instead of
         * key_release_event() so that this can know if the event
         * was handled by IME.
         */
        switch (keyval) {
        case Gdk.Key.Escape:
            if (key_press_escape())
                show_all();
            return true;
        case Gdk.Key.Return:
        case Gdk.Key.KP_Enter:
            if (key_press_enter(true))
                show_all();
            else
                hide();
            return true;
        case Gdk.Key.space:
        case Gdk.Key.KP_Space:
            if (m_candidate_panel_is_visible) {
                enter_notify_disable_with_timer();
                m_lookup_table.cursor_down();
                show_candidate_panel();
            }
            else {
                category_list_cursor_move(Gdk.Key.Down);
                show_all();
            }
            return true;
        case Gdk.Key.Right:
        case Gdk.Key.KP_Right:
            key_press_cursor_horizontal(Gdk.Key.Right, modifiers);
            show_all();
            return true;
        case Gdk.Key.Left:
        case Gdk.Key.KP_Left:
            key_press_cursor_horizontal(Gdk.Key.Left, modifiers);
            show_all();
            return true;
        case Gdk.Key.Down:
        case Gdk.Key.KP_Down:
            key_press_cursor_vertical(Gdk.Key.Down, modifiers);
            show_all();
            return true;
        case Gdk.Key.Up:
        case Gdk.Key.KP_Up:
            key_press_cursor_vertical(Gdk.Key.Up, modifiers);
            show_all();
            return true;
        case Gdk.Key.Page_Down:
        case Gdk.Key.KP_Page_Down:
            key_press_cursor_vertical(Gdk.Key.Page_Down, modifiers);
            show_all();
            return true;
        case Gdk.Key.Page_Up:
        case Gdk.Key.KP_Page_Up:
            key_press_cursor_vertical(Gdk.Key.Page_Up, modifiers);
            show_all();
            return true;
        case Gdk.Key.Home:
        case Gdk.Key.KP_Home:
            key_press_cursor_home_end(Gdk.Key.Home, modifiers);
            show_all();
            return true;
        case Gdk.Key.End:
        case Gdk.Key.KP_End:
            key_press_cursor_home_end(Gdk.Key.End, modifiers);
            show_all();
            return true;
        }

        if ((modifiers & Gdk.ModifierType.CONTROL_MASK) != 0) {
            switch (keyval) {
            case Gdk.Key.f:
                key_press_cursor_horizontal(Gdk.Key.Right, modifiers);
                show_all();
                return true;
            case Gdk.Key.b:
                key_press_cursor_horizontal(Gdk.Key.Left, modifiers);
                show_all();
                return true;
            case Gdk.Key.n:
            case Gdk.Key.N:
                key_press_cursor_vertical(Gdk.Key.Down, modifiers);
                show_all();
                return true;
            case Gdk.Key.p:
            case Gdk.Key.P:
                key_press_cursor_vertical(Gdk.Key.Up, modifiers);
                show_all();
                return true;
            case Gdk.Key.h:
                key_press_cursor_home_end(Gdk.Key.Home, modifiers);
                show_all();
                return true;
            case Gdk.Key.e:
                key_press_cursor_home_end(Gdk.Key.End, modifiers);
                show_all();
                return true;
            case Gdk.Key.C:
            case Gdk.Key.c:
                if ((modifiers & Gdk.ModifierType.SHIFT_MASK) != 0) {
                    if (m_candidate_panel_is_visible) {
                        uint index = m_lookup_table.get_cursor_pos();
                        var text = m_lookup_table.get_candidate(index).text;
                        Gtk.Clipboard clipboard =
                                Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD);
                        clipboard.set_text(text, -1);
                        clipboard.store();
                        return true;
                    }
                }
                break;
            }
            return false;
        }
        return true;
    }


    public override bool focus_in_event(Gdk.EventFocus event) {
        return base.focus_in_event(event);
    }


    public override bool focus_out_event(Gdk.EventFocus event) {
        return base.focus_out_event(event);
    }


    public bool is_running() {
        return m_is_running;
    }


    public string get_input_context_path() {
        return m_input_context_path;
    }


    public void set_input_context_path(string input_context_path) {
        m_input_context_path = input_context_path;
        if (input_context_path == "") {
            m_warning_message = _("" +
                "Failed to get the current text application. " +
                "Please re-focus your application. E.g. Press Esc key " +
                "several times to release the emoji typing mode, " +
                "click your desktop and click your text application again."
            );
        } else {
            m_warning_message = "";
        }
    }


    public string get_selected_string() {
        return m_result;
    }


    private void reset_window_mode() {
        m_backward_index = -1;
        m_backward = null;
        m_candidate_panel_is_visible = false;
        m_candidate_panel_mode = false;
        // Do not clear m_lookup_table to work with space key later.
    }


    public void reset() {
        reset_window_mode();
        m_input_context_path = "";
        m_result = null;
        m_category_active_index = -1;
        m_show_unicode = false;
    }


    public void present_centralize(Gdk.Event event) {
        Gtk.Allocation allocation;
        get_allocation(out allocation);
        Gdk.Rectangle monitor_area;
        Gdk.Rectangle work_area;
        Gdk.Display display = get_display();
        Gdk.Monitor monitor = display.get_monitor_at_window(this.get_window());
        monitor_area = monitor.get_geometry();
        work_area = monitor.get_workarea();
        int x = (monitor_area.x + monitor_area.width - allocation.width)/2;
        int y = (monitor_area.y + monitor_area.height
                 - allocation.height)/2;
        // Do not hide a bottom panel in XFCE4
        if (work_area.y < y)
            y = work_area.y;
        // FIXME: move() does not work in Wayland
        move(x, y);

        uint32 timestamp = event.get_time();
        present_with_time(timestamp);
    }


    public void set_cursor_location(int x,
                                    int y,
                                    int width,
                                    int height) {
        Gdk.Rectangle location = Gdk.Rectangle(){
            x = x, y = y, width = width, height = height };
        if (m_cursor_location == location)
            return;
        m_cursor_location = location;
    }


    public bool is_candidate_panel_mode() {
        return m_candidate_panel_mode;
    }


    public bool is_rebuilding_gui() {
        /* The candidate window and preedit text should not be closed
         * when the GUI is rebuilding.
         */
        return m_rebuilding_gui;
    }


    public static bool has_loaded_emoji_dict() {
        if (m_emoji_to_data_dict == null)
            return false;
        GLib.List keys = m_emoji_to_data_dict.get_keys();
        if (keys.length() == 0)
            return false;
        return true;
    }


    public static void set_annotation_lang(string? lang) {
        if (lang == null || lang == "")
            lang = "en";
        if (m_current_lang_id == lang)
            return;
        m_current_lang_id = lang;
        reload_emoji_dict();
    }


    public static string get_annotation_lang() {
        return m_current_lang_id;
    }

    public static void set_emoji_font(string? emoji_font) {
        return_if_fail(emoji_font != null && emoji_font != "");
        Pango.FontDescription font_desc =
                Pango.FontDescription.from_string(emoji_font);
        string font_family = font_desc.get_family();
        if (font_family != null) {
            m_emoji_font_family = font_family;
            m_emoji_font_changed = true;
        }
        int font_size = font_desc.get_size() / Pango.SCALE;
        if (font_size != 0) {
            m_emoji_font_size = font_size;
            m_emoji_font_changed = true;
        }
    }


    public static void set_partial_match(bool has_partial_match) {
        m_has_partial_match = has_partial_match;
    }


    public static void set_partial_match_length(int length) {
        if (length < 1)
            return;
        m_partial_match_length = length;
    }


    public static void set_partial_match_condition(int condition) {
        if (condition < 0)
            return;
        m_partial_match_condition = condition;
    }


    public static void set_favorites(string[]? unowned_favorites,
                                     string[]? unowned_favorite_annotations) {
        m_favorites = {};
        m_favorite_annotations = {};
        for(int i = 0; i < unowned_favorites.length; i++) {
            string? favorite = unowned_favorites[i];
            // Avoid gsetting value error by manual setting
            GLib.return_if_fail(favorite != null);
            m_favorites += favorite;
        }
        for(int i = 0; i < unowned_favorite_annotations.length; i++) {
            string? favorite_annotation = unowned_favorite_annotations[i];
            GLib.return_if_fail(favorite_annotation != null);
            m_favorite_annotations += favorite_annotation;
        }
        update_favorite_emoji_dict();
    }


    private static GLib.Object get_load_progress_object() {
            if (m_unicode_progress_object == null)
                m_unicode_progress_object = new LoadProgressObject();
            return m_unicode_progress_object as GLib.Object;
    }


    public static void load_unicode_dict() {
        if (m_unicode_block_list.length() == 0)
            make_unicode_block_dict();
        if (m_name_to_unicodes_dict.size() == 0)
            make_unicode_name_dict(IBusEmojier.get_load_progress_object());
    }
}
