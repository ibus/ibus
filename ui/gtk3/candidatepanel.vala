/* vim:set et sts=4 sw=4:
 *
 * ibus - The Input Bus
 *
 * Copyright(c) 2011-2015 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright(c) 2015 Takao Fujiwara <takao.fujiwara1@gmail.com>
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

public class CandidatePanel : Gtk.Box{
    private bool m_vertical_panel_system = true;
    private bool m_vertical_writing;
    private Gtk.Window m_toplevel;
    private Gtk.Box m_vbox;

    private Gtk.Label m_preedit_label;
    private Gtk.Label m_aux_label;
    private CandidateArea m_candidate_area;
    private HSeparator m_hseparator;

    private Gdk.Rectangle m_cursor_location;

    public signal void cursor_up();
    public signal void cursor_down();
    public signal void page_up();
    public signal void page_down();
    public signal void candidate_clicked(uint index,
                                         uint button,
                                         uint state);

    public CandidatePanel() {
        // Call base class constructor
        GLib.Object(
            name : "IBusCandidate",
            orientation: Gtk.Orientation.HORIZONTAL,
            visible: true
        );

        m_toplevel = new Gtk.Window(Gtk.WindowType.POPUP);
        m_toplevel.add_events(Gdk.EventMask.BUTTON_PRESS_MASK);
        m_toplevel.button_press_event.connect((w, e) => {
            if (e.button != 1 || (e.state & Gdk.ModifierType.CONTROL_MASK) == 0)
                return false;
            set_vertical(!m_vertical_panel_system);
            return true;
        });
        m_toplevel.size_allocate.connect((w, a) => {
            adjust_window_position();
        });

        Handle handle = new Handle();
        handle.set_visible(true);
        pack_start(handle, false, false, 0);

        m_vbox = new Gtk.Box(Gtk.Orientation.VERTICAL, 0);
        m_vbox.set_visible(true);
        pack_start(m_vbox, false, false, 0);

        m_toplevel.add(this);

        create_ui();
    }

    public void set_vertical(bool vertical) {
        if (m_vertical_panel_system == vertical)
            return;
        m_vertical_panel_system = vertical;
        m_candidate_area.set_vertical(vertical);
    }

    private void set_orientation(IBus.Orientation orientation) {
        switch (orientation) {
        case IBus.Orientation.VERTICAL:
            m_candidate_area.set_vertical(true);
            break;
        case IBus.Orientation.HORIZONTAL:
            m_candidate_area.set_vertical(false);
            break;
        case IBus.Orientation.SYSTEM:
            m_candidate_area.set_vertical(m_vertical_panel_system);
            break;
        }
    }

    public void set_cursor_location(int x, int y, int width, int height) {
        Gdk.Rectangle location = Gdk.Rectangle(){
            x = x, y = y, width = width, height = height };
        if (m_cursor_location == location)
            return;
        m_cursor_location = location;

        /* Do not call adjust_window_position() here because
         * m_toplevel is not shown yet and
         * m_toplevel.get_allocation() returns height = width = 1 */
    }

    private void set_labels(IBus.Text[] labels) {
        m_candidate_area.set_labels(labels);
    }

    private void set_attributes(Gtk.Label label, IBus.Text text) {
        Pango.AttrList attrs = get_pango_attr_list_from_ibus_text(text);

        Gtk.StyleContext context = label.get_style_context();
        Gdk.RGBA color;

        if (context.lookup_color("placeholder_text_color", out color)) {
            Pango.Attribute pango_attr = Pango.attr_foreground_new(
                        (uint16)(color.red * uint16.MAX),
                        (uint16)(color.green * uint16.MAX),
                        (uint16)(color.blue * uint16.MAX));
            pango_attr.start_index = 0;
            pango_attr.end_index = label.get_text().length;
            attrs.insert((owned)pango_attr);
        }

        label.set_attributes(attrs);
    }

    public void set_preedit_text(IBus.Text? text, uint cursor) {
        if (text != null) {
            var str = text.get_text();

            if (str.length > 0) {
                m_preedit_label.set_text(str);
                m_preedit_label.show();
                set_attributes(m_preedit_label, text);
            } else {
                m_preedit_label.set_text("");
                m_preedit_label.hide();
            }
        } else {
            m_preedit_label.set_text("");
            m_preedit_label.hide();
        }
        update();
    }

    public void set_auxiliary_text(IBus.Text? text) {
        if (text != null) {
            m_aux_label.set_text(text.get_text());
            m_aux_label.show();
        } else {
            m_aux_label.set_text("");
            m_aux_label.hide();
        }
        update();
    }

    public void set_lookup_table(IBus.LookupTable? table) {
        IBus.Text[] candidates = {};
        uint cursor_in_page = 0;
        bool show_cursor = true;
        IBus.Text[] labels = {};
        IBus.Orientation orientation = IBus.Orientation.SYSTEM;

        if (table != null) {
            uint page_size = table.get_page_size();
            uint ncandidates = table.get_number_of_candidates();
            uint cursor = table.get_cursor_pos();
            cursor_in_page = table.get_cursor_in_page();
            show_cursor = table.is_cursor_visible();

            uint page_start_pos = cursor / page_size * page_size;
            uint page_end_pos = uint.min(page_start_pos + page_size, ncandidates);
            for (uint i = page_start_pos; i < page_end_pos; i++)
                candidates += table.get_candidate(i);

            for (uint i = 0; i < page_size; i++) {
                IBus.Text? label = table.get_label(i);
                if (label != null)
                    labels += label;
            }

            orientation = (IBus.Orientation)table.get_orientation();
        }

        m_candidate_area.set_candidates(candidates, cursor_in_page, show_cursor);
        set_labels(labels);

        if (table != null) {
            // Do not change orientation if table is null to avoid recreate
            // candidates area.
            set_orientation(orientation);
        }

        if (candidates.length != 0)
            m_candidate_area.show_all();
        else
            m_candidate_area.hide();

        update();
    }

    public void set_content_type(uint purpose, uint hints) {
        m_vertical_writing =
                ((hints & IBus.InputHints.VERTICAL_WRITING) != 0);
    }

    private void update() {
        if (m_candidate_area.get_visible() ||
            m_preedit_label.get_visible() ||
            m_aux_label.get_visible())
            m_toplevel.show();
        else
            m_toplevel.hide();

        if (m_aux_label.get_visible() &&
            (m_candidate_area.get_visible() || m_preedit_label.get_visible()))
            m_hseparator.show();
        else
            m_hseparator.hide();
    }

    public override void get_preferred_width(out int minimum_width, out int natural_width) {
        base.get_preferred_width(out minimum_width, out natural_width);
        m_toplevel.resize(1, 1);
    }

    public override void get_preferred_height(out int minimum_width, out int natural_width) {
        base.get_preferred_height(out minimum_width, out natural_width);
        m_toplevel.resize(1, 1);
    }

    private void create_ui() {
        m_preedit_label = new Gtk.Label(null);
        m_preedit_label.set_size_request(20, -1);
        m_preedit_label.set_halign(Gtk.Align.START);
        m_preedit_label.set_valign(Gtk.Align.CENTER);
        /* Use Gtk.Widget.set_margin_start() since gtk 3.12 */
        m_preedit_label.set_padding(8, 0);
        m_preedit_label.set_no_show_all(true);

        m_aux_label = new Gtk.Label(null);
        m_aux_label.set_size_request(20, -1);
        m_aux_label.set_halign(Gtk.Align.START);
        m_aux_label.set_valign(Gtk.Align.CENTER);
        /* Use Gtk.Widget.set_margin_start() since gtk 3.12 */
        m_aux_label.set_padding(8, 0);
        m_aux_label.set_no_show_all(true);

        m_candidate_area = new CandidateArea(m_vertical_panel_system);
        m_candidate_area.candidate_clicked.connect(
                (w, i, b, s) => candidate_clicked(i, b, s));
        m_candidate_area.page_up.connect((c) => page_up());
        m_candidate_area.page_down.connect((c) => page_down());
        m_candidate_area.cursor_up.connect((c) => cursor_up());
        m_candidate_area.cursor_down.connect((c) => cursor_down());

        m_hseparator = new HSeparator();
        m_hseparator.set_visible(true);

        pack_all_widgets();
    }

    private void pack_all_widgets() {
        m_vbox.pack_start(m_preedit_label, false, false, 4);
        m_vbox.pack_start(m_aux_label, false, false, 4);
        m_vbox.pack_start(m_hseparator, false, false, 0);
        m_vbox.pack_start(m_candidate_area, false, false, 0);
    }

    public new void show() {
        m_toplevel.show_all();
    }

    public new void hide() {
        m_toplevel.hide();
    }

    private void move(int x, int y) {
        m_toplevel.move(x, y);
    }

    private void adjust_window_position() {
        if (!m_vertical_writing)
            adjust_window_position_horizontal();
        else
            adjust_window_position_vertical();
    }

    private void adjust_window_position_horizontal() {
        Gdk.Point cursor_right_bottom = {
                m_cursor_location.x + m_cursor_location.width,
                m_cursor_location.y + m_cursor_location.height
        };

        Gtk.Allocation allocation;
        m_toplevel.get_allocation(out allocation);
        Gdk.Point window_right_bottom = {
            cursor_right_bottom.x + allocation.width,
            cursor_right_bottom.y + allocation.height
        };

        Gdk.Screen screen = Gdk.Screen.get_default();
        int monitor_num = screen.get_monitor_at_point(m_cursor_location.x,
                                                      m_cursor_location.y);
        Gdk.Rectangle monitor_area = screen.get_monitor_workarea(monitor_num);
        int monitor_right = monitor_area.x + monitor_area.width;
        int monitor_bottom = monitor_area.y + monitor_area.height;

        int x, y;
        if (window_right_bottom.x > monitor_right)
            x = monitor_right - allocation.width;
        else
            x = cursor_right_bottom.x;
        if (x < 0)
            x = 0;

        if (window_right_bottom.y > monitor_bottom)
            y = m_cursor_location.y - allocation.height;
        else
            y = cursor_right_bottom.y;
        if (y < 0)
            y = 0;

        move(x, y);
    }

    private void adjust_window_position_vertical() {
        /* Not sure in which top or left cursor appears
         * in the vertical writing mode.
         * Max (m_cursor_location.width, m_cursor_location.height)
         * can be considered as a char size.
         */
        int char_size = int.max(m_cursor_location.width,
                                m_cursor_location.height);
        Gdk.Point cursor_right_bottom = {
                m_cursor_location.x + char_size,
                m_cursor_location.y + char_size
        };

        Gtk.Allocation allocation;
        m_toplevel.get_allocation(out allocation);
        Gdk.Point hwindow_right_bottom = {
            m_cursor_location.x + allocation.width,
            cursor_right_bottom.y + allocation.height
        };
        Gdk.Point vwindow_left_bottom = {
            m_cursor_location.x - allocation.width,
            m_cursor_location.y + allocation.height
        };

        Gdk.Screen screen = Gdk.Screen.get_default();
        int monitor_num = screen.get_monitor_at_point(m_cursor_location.x,
                                                      m_cursor_location.y);
        Gdk.Rectangle monitor_area = screen.get_monitor_workarea(monitor_num);
        int monitor_right = monitor_area.x + monitor_area.width;
        int monitor_bottom = monitor_area.y + monitor_area.height;

        int x, y;
        if (!m_candidate_area.get_vertical()) {
            if (hwindow_right_bottom.x > monitor_right)
                x = monitor_right - allocation.width;
            else
                x = m_cursor_location.x;

            if (hwindow_right_bottom.y > monitor_bottom)
                y = m_cursor_location.y - allocation.height;
            else
                y = cursor_right_bottom.y;
        } else {
            if (vwindow_left_bottom.x > monitor_right)
                x = monitor_right - allocation.width;
            else if (vwindow_left_bottom.x < 0)
                x = cursor_right_bottom.x;
            else
                x = vwindow_left_bottom.x;

            if (vwindow_left_bottom.y > monitor_bottom)
                y = monitor_bottom - allocation.height;
            else
                y = m_cursor_location.y;
        }

        if (x < 0)
            x = 0;
        if (y < 0)
            y = 0;

        move(x, y);
    }
}
