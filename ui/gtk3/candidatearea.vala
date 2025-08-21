/* vim:set et sts=4 sw=4:
 *
 * ibus - The Input Bus
 *
 * Copyright(c) 2011-2015 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright(c) 2015-2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
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

class CandidateArea : Gtk.Box {
    private bool m_vertical;
    private Gtk.Widget m_text_view;
    private Gtk.Label[] m_labels;
    private Gtk.Label[] m_candidates;
    private Gtk.Widget[] m_widgets;

    private IBus.Text[] m_ibus_candidates;
    private uint m_focus_candidate;
    private bool m_show_cursor;
    private ThemedRGBA m_rgba;

    private Pango.Attribute m_language_attribute;

    private const string LABELS[] = {
        "1.", "2.", "3.", "4.", "5.", "6.", "7.", "8.",
        "9.", "0.", "a.", "b.", "c.", "d.", "e.", "f."
    };

    private const string PREV_PAGE_ICONS[] = {
        "go-previous",
        "go-up"
    };

    private const string NEXT_PAGE_ICONS[] = {
        "go-next",
        "go-down"
    };

    public signal void candidate_clicked(uint index, uint button, uint state);
    public signal void page_up();
    public signal void page_down();
    public signal void cursor_up();
    public signal void cursor_down();

    public CandidateArea(bool vertical) {
        GLib.Object();
        set_vertical(vertical, true);
        m_text_view = new Gtk.TextView();
        var style_context = m_text_view.get_style_context();
        m_rgba = new ThemedRGBA(style_context);
    }

    ~CandidateArea() {
        m_ibus_candidates = null;
        m_labels = null;
        m_candidates = null;
        m_widgets = null;
        m_rgba = null;
        m_text_view = null;
    }

    public bool candidate_scrolled(Gdk.EventScroll event) {
        switch (event.direction) {
        case Gdk.ScrollDirection.UP:
            cursor_up();
            break;
        case Gdk.ScrollDirection.DOWN:
            cursor_down();
            break;
        default: break;
        }
        return true;
    }

    public bool get_vertical() {
        return m_vertical;
    }

    public void set_vertical(bool vertical, bool force = false) {
        if (!force && m_vertical == vertical)
            return;
        m_vertical = vertical;
        orientation = vertical ?
            Gtk.Orientation.VERTICAL :
            Gtk.Orientation.HORIZONTAL;
        recreate_ui();

        if (m_ibus_candidates.length > 0) {
            // Workaround a vala issue
            // https://bugzilla.gnome.org/show_bug.cgi?id=661130
            set_candidates((owned)m_ibus_candidates,
                           m_focus_candidate,
                           m_show_cursor);
            show_all();
        }
    }

    public void set_labels(IBus.Text[] labels) {
        int i;
        for (i = 0; i < int.min(16, labels.length); i++)
            m_labels[i].set_text(labels[i].get_text());
        for (; i < 16; i++)
            m_labels[i].set_text(LABELS[i]);
    }

    public void set_language(Pango.Attribute language_attribute) {
        m_language_attribute = language_attribute.copy();
    }

    public void set_candidates(IBus.Text[] candidates,
                               uint focus_candidate = 0,
                               bool show_cursor = true) {
        m_ibus_candidates = candidates;
        m_focus_candidate = focus_candidate;
        m_show_cursor = show_cursor;

        assert(candidates.length <= 16);
        for (int i = 0 ; i < 16 ; i++) {
            Gtk.Label label = m_candidates[i];
            bool visible = false;
            if (i < candidates.length) {
                Pango.AttrList attrs = get_pango_attr_list_from_ibus_text(candidates[i]);
                attrs.change(m_language_attribute.copy());
                if (i == focus_candidate && show_cursor) {
                    Pango.Attribute pango_attr = Pango.attr_foreground_new(
                            (uint16)(m_rgba.selected_fg.red * uint16.MAX),
                            (uint16)(m_rgba.selected_fg.green * uint16.MAX),
                            (uint16)(m_rgba.selected_fg.blue * uint16.MAX));
                    pango_attr.start_index = 0;
                    pango_attr.end_index = candidates[i].get_text().length;
                    attrs.insert((owned)pango_attr);

                    pango_attr = Pango.attr_background_new(
                           (uint16)(m_rgba.selected_bg.red * uint16.MAX),
                           (uint16)(m_rgba.selected_bg.green * uint16.MAX),
                           (uint16)(m_rgba.selected_bg.blue * uint16.MAX));
                    pango_attr.start_index = 0;
                    pango_attr.end_index = candidates[i].get_text().length;
                    attrs.insert((owned)pango_attr);
                }
                label.set_text(candidates[i].get_text());
                label.set_attributes(attrs);
                visible = true;
            } else {
                label.set_text("");
                label.set_attributes(new Pango.AttrList());
            }
            if (m_vertical) {
                m_widgets[i * 2].set_visible(visible);
                m_widgets[i * 2 +1].set_visible(visible);
            } else {
                m_widgets[i].set_visible(visible);
            }
        }
    }

    private void recreate_ui() {
        foreach (Gtk.Widget w in get_children()) {
            w.destroy();
        }

        Gtk.Button prev_button = new Gtk.Button();
        prev_button.clicked.connect((b) => page_up());
        prev_button.set_image(new Gtk.Image.from_icon_name(
                                  PREV_PAGE_ICONS[orientation],
                                  Gtk.IconSize.MENU));
        prev_button.set_relief(Gtk.ReliefStyle.NONE);

        Gtk.Button next_button = new Gtk.Button();
        next_button.clicked.connect((b) => page_down());
        next_button.set_image(new Gtk.Image.from_icon_name(
                                  NEXT_PAGE_ICONS[orientation],
                                  Gtk.IconSize.MENU));
        next_button.set_relief(Gtk.ReliefStyle.NONE);

        if (m_vertical) {
            Gtk.EventBox container_ebox = new Gtk.EventBox();
            container_ebox.add_events(Gdk.EventMask.SCROLL_MASK);
            container_ebox.scroll_event.connect(candidate_scrolled);
            add(container_ebox);

            Gtk.Box vbox = new Gtk.Box(Gtk.Orientation.VERTICAL, 0);
            container_ebox.add(vbox);

            // Add Candidates
            Gtk.Box candidates_hbox = new Gtk.Box(Gtk.Orientation.HORIZONTAL, 0);
            vbox.pack_start(candidates_hbox, false, false, 0);
            Gtk.Box labels_vbox = new Gtk.Box(Gtk.Orientation.VERTICAL, 0);
            labels_vbox.set_homogeneous(true);
            Gtk.Box candidates_vbox = new Gtk.Box(Gtk.Orientation.VERTICAL, 0);
            candidates_vbox.set_homogeneous(true);
            candidates_hbox.pack_start(labels_vbox, false, false, 4);
            candidates_hbox.pack_start(new VSeparator(), false, false, 0);
            candidates_hbox.pack_start(candidates_vbox, true, true, 4);

            // Add HSeparator
            vbox.pack_start(new HSeparator(), false, false, 0);

            // Add buttons
            Gtk.Box buttons_hbox = new Gtk.Box(Gtk.Orientation.HORIZONTAL, 0);
            Gtk.Label state_label = new Gtk.Label(null);
            state_label.set_size_request(20, -1);
            buttons_hbox.pack_start(state_label, true, true, 0);
            buttons_hbox.pack_start(prev_button, false, false, 0);
            buttons_hbox.pack_start(next_button, false, false, 0);
            vbox.pack_start(buttons_hbox, false, false, 0);

            m_labels = {};
            m_candidates = {};
            m_widgets = {};
            for (int i = 0; i < 16; i++) {
                Gtk.Label label = new Gtk.Label(LABELS[i]);
                label.set_halign(Gtk.Align.START);
                label.set_valign(Gtk.Align.CENTER);
                label.show();
                m_labels += label;

                Gtk.Label candidate = new Gtk.Label("test");
                candidate.set_halign(Gtk.Align.START);
                candidate.set_valign(Gtk.Align.CENTER);
                candidate.show();
                m_candidates += candidate;

                label.set_margin_start (8);
                label.set_margin_end (8);
                candidate.set_margin_start (8);
                candidate.set_margin_end (8);

                // Make a copy of i to workaround a bug in vala.
                // https://bugzilla.gnome.org/show_bug.cgi?id=628336
                int index = i;
                Gtk.EventBox label_ebox = new Gtk.EventBox();
                label_ebox.set_no_show_all(true);
                label_ebox.button_press_event.connect((w, e) => {
                    candidate_clicked(i, e.button, e.state);
                    return true;
                });
                label_ebox.add(label);
                labels_vbox.pack_start(label_ebox, false, false, 2);
                m_widgets += label_ebox;

                Gtk.EventBox candidate_ebox = new Gtk.EventBox();
                candidate_ebox.set_no_show_all(true);
                candidate_ebox.button_press_event.connect((w, e) => {
                    candidate_clicked(index, e.button, e.state);
                    return true;
                });
                candidate_ebox.add(candidate);
                candidates_vbox.pack_start(candidate_ebox, false, false, 2);
                m_widgets += candidate_ebox;
            }
        } else {
            Gtk.EventBox container_ebox = new Gtk.EventBox();
            container_ebox.add_events(Gdk.EventMask.SCROLL_MASK);
            container_ebox.scroll_event.connect(candidate_scrolled);
            add(container_ebox);

            Gtk.Box hbox = new Gtk.Box(Gtk.Orientation.HORIZONTAL, 0);
            container_ebox.add(hbox);

            m_labels = {};
            m_candidates = {};
            m_widgets = {};
            for (int i = 0; i < 16; i++) {
                Gtk.Label label = new Gtk.Label(LABELS[i]);
                label.set_halign(Gtk.Align.START);
                label.set_valign(Gtk.Align.CENTER);
                label.show();
                m_labels += label;

                Gtk.Label candidate = new Gtk.Label("test");
                candidate.set_halign(Gtk.Align.START);
                candidate.set_valign(Gtk.Align.CENTER);
                candidate.show();
                m_candidates += candidate;

                Gtk.Box candidate_hbox = new Gtk.Box(Gtk.Orientation.HORIZONTAL, 0);
                candidate_hbox.show();
                candidate_hbox.pack_start(label, false, false, 2);
                candidate_hbox.pack_start(candidate, false, false, 2);

                // Make a copy of i to workaround a bug in vala.
                // https://bugzilla.gnome.org/show_bug.cgi?id=628336
                int index = i;
                Gtk.EventBox ebox = new Gtk.EventBox();
                ebox.set_no_show_all(true);
                ebox.button_press_event.connect((w, e) => {
                    candidate_clicked(index, e.button, e.state);
                    return true;
                });
                ebox.add(candidate_hbox);
                hbox.pack_start(ebox, false, false, 4);
                m_widgets += ebox;
            }
            hbox.pack_start(new VSeparator(), false, false, 0);
            hbox.pack_start(prev_button, false, false, 0);
            hbox.pack_start(next_button, false, false, 0);
        }
    }
}

