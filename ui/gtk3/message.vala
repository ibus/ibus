/* vim:set et sts=4 sw=4:
 *
 * ibus - The Input Bus
 *
 * Copyright(c) 2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
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

public class MessageDialog : Gtk.Box{
    private Gtk.Window m_toplevel;
    private Gtk.Box m_vbox;
    private Gtk.Box m_hbox;
    private Gtk.Label m_title_label;
    private Gtk.Label m_description_label;
    private Gtk.ProgressBar m_progress_bar;
    private Gtk.Label m_progress_label;
    private Gtk.Label m_timeout_label;
    private Gtk.Button m_button;
    private int m_timeout_time;
    private int m_elapsed_time;
    private uint m_timeout_id;

    private Gdk.Rectangle m_cursor_location;

    private bool m_is_wayland;
    private bool m_no_wayland_panel;

#if USE_GDK_WAYLAND
    private bool m_hide_after_show;
#endif

    public signal void close();

#if USE_GDK_WAYLAND
    public signal void realize_surface(void *surface);
#endif

    public MessageDialog (bool         is_wayland,
                          bool         no_wayland_panel,
                          IBus.Message message) {
        // Call base class constructor
        GLib.Object(
            name : "IBusMessageDialog",
            orientation: Gtk.Orientation.HORIZONTAL,
            visible: true
        );

        m_is_wayland = is_wayland;
        m_no_wayland_panel = no_wayland_panel;
        m_toplevel = new Gtk.Window(Gtk.WindowType.POPUP);
        m_toplevel.add_events(Gdk.EventMask.BUTTON_PRESS_MASK);
        m_toplevel.size_allocate.connect((w, a) => {
            adjust_window_position(w);
        });
#if USE_GDK_WAYLAND
        if (m_is_wayland) {
            m_toplevel.realize.connect((w) => {
                realize_window(true);
            });
            m_toplevel.show.connect((w) => {
                if (m_hide_after_show)
                    realize_window(false);
                m_hide_after_show = false;
            });
            m_toplevel.hide.connect((w) => {
                m_hide_after_show = true;
            });
        }
#endif

        m_vbox = new Gtk.Box(Gtk.Orientation.VERTICAL, 0);
        m_vbox.set_visible(true);
        pack_start(m_vbox, false, false, 0);

        m_toplevel.add(this);

        create_ui(message);
    }

    public void set_cursor_rect(Gdk.Rectangle location) {
        if (m_cursor_location == location)
            return;
        m_cursor_location = location;
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

    private void create_ui(IBus.Message message) {
        string? title = message.get_title();
        string description = message.get_description();

        if (title.length > 0) {
            m_title_label = new Gtk.Label(title);
        } else {
            m_title_label = new Gtk.Label(null);
            m_title_label.set_no_show_all(true);
        }
        m_title_label.set_halign(Gtk.Align.CENTER);
        m_title_label.set_valign(Gtk.Align.CENTER);
        m_title_label.set_margin_start(8);
        m_title_label.set_margin_end(8);
        m_title_label.set_line_wrap(true);
        m_title_label.set_width_chars(50);

        m_description_label = new Gtk.Label(description);
        m_description_label.set_halign(Gtk.Align.START);
        m_description_label.set_valign(Gtk.Align.CENTER);
        m_description_label.set_margin_start(8);
        m_description_label.set_margin_end(8);
        m_description_label.set_line_wrap(true);
        m_description_label.set_width_chars(50);

        m_hbox = new Gtk.Box(Gtk.Orientation.HORIZONTAL, 5);
        m_hbox.set_halign(Gtk.Align.CENTER);
        m_hbox.set_valign(Gtk.Align.CENTER);
        m_progress_bar = new Gtk.ProgressBar();
        m_progress_bar.set_ellipsize(Pango.EllipsizeMode.MIDDLE);
        m_progress_bar.set_halign(Gtk.Align.CENTER);
        m_progress_bar.set_valign(Gtk.Align.CENTER);
        m_progress_label = new Gtk.Label(null);

        int progress = message.get_progress();
        double progress_f = progress / 100.0;
        if (progress_f <= 1.0 && progress >= 0) {
            m_progress_bar.set_fraction(progress_f);
            m_progress_bar.set_text(
                    "%2u%%\n".printf(progress));
            m_progress_label.set_text(
                    "%2u%%\n".printf(progress));
        } else {
            if (progress_f > 1.0)
                warning ("Progress should be below 100.");
            m_hbox.set_no_show_all(true);
        }

        m_timeout_time = message.get_timeout();
        if (m_timeout_time >= 0) {
            m_timeout_label = new Gtk.Label(_("%5dSec").printf(m_timeout_time));
        } else {
            m_timeout_label = new Gtk.Label(null);
            m_timeout_label.set_no_show_all(true);
        }
        m_timeout_label.set_size_request(20, -1);
        m_timeout_label.set_halign(Gtk.Align.CENTER);
        m_timeout_label.set_valign(Gtk.Align.CENTER);
        m_timeout_label.set_margin_start(8);
        m_timeout_label.set_margin_end(8);

        if (message.get_serial() > 0 && m_timeout_time >= 0) {
            m_button = new Gtk.Button();
            m_button.set_no_show_all(true);
        } else {
            m_button = new Gtk.Button.with_label(_("Close"));
            m_button.get_style_context().add_class("text-button");
            m_button.set_can_default(true);
        }
        m_button.set_use_underline(true);
        m_button.set_valign(Gtk.Align.BASELINE);
        m_button.clicked.connect((w) => {
            if (m_timeout_id > 0) {
                GLib.Source.remove(m_timeout_id);
                m_timeout_id = 0;
            }
            close();
        });
        if (m_timeout_time >= 0) {
            m_elapsed_time = 1;
            m_timeout_id = Timeout.add_seconds(1,
                    () => {
                        if (m_elapsed_time >= m_timeout_time) {
                            close();
                            m_timeout_id = 0;
                            return Source.REMOVE;
                        }
                        m_timeout_label.set_text(_("%5dSec").printf(
                                m_timeout_time - m_elapsed_time++));
                        return Source.CONTINUE;
                    });
        }

        m_vbox.pack_start(m_title_label, false, false, 4);
        m_vbox.pack_start(m_description_label, false, false, 4);
        m_vbox.add(m_hbox);
        m_hbox.add(m_progress_bar);
        m_hbox.add(m_progress_label);
        m_vbox.pack_start(m_timeout_label, false, false, 4);
        m_vbox.pack_start(m_button, false, false, 0);

        m_toplevel.resize(1, 1);
    }

    public void update_message(IBus.Message message) {
        string? title = message.get_title();
        string description = message.get_description();

        if (title.length > 0) {
            m_title_label.set_text(title);
            m_title_label.show();
        } else {
            m_title_label.hide();
        }
        m_description_label.set_text(description);

        m_timeout_time = message.get_timeout();
        if (m_timeout_time >= 0) {
            m_timeout_label.set_text(_("%5dSec").printf(m_timeout_time));
            m_timeout_label.show();
            m_elapsed_time = 1;
        } else {
            m_timeout_label.hide();
        }

        int progress = message.get_progress();
        double progress_f = progress / 100.0;
        if (progress_f <= 1.0 && progress >= 0) {
            m_progress_bar.set_fraction(progress_f);
            m_progress_bar.set_text(
                    "%2u%%\n".printf(progress));
            m_progress_label.set_text(
                    "%2u%%\n".printf(progress));
            m_hbox.show();
        } else {
            if (progress_f > 1.0)
                warning ("Progress should be below 100.");
            m_hbox.hide();
        }

        m_toplevel.resize(1, 1);
        this.show();
    }

    public new void show() {
        m_toplevel.show_all();
    }

    public new void hide() {
#if USE_GDK_WAYLAND
        if (m_is_wayland)
            realize_surface(null);
#endif
        m_toplevel.hide();
    }

    /**
     * move:
     * @x: left position of the #MessageDialog
     * @y: top position of the #MessageDialog
     */
    private void move(int x, int y) {
        m_toplevel.move(x, y);
    }

    private void adjust_window_position(Gtk.Widget window) {
        adjust_window_position_horizontal(window);
    }

    /**
     * adjust_window_position_horizontal:
     * @window: A Gtk.Widget of the toplevel window.
     *
     * Horizontal writing mode but not the horizontal lookup table
     * when the allocation is emmitted.
     */
    private void adjust_window_position_horizontal(Gtk.Widget window) {
        Gdk.Point cursor_left_bottom = {
                m_cursor_location.x,
                m_cursor_location.y + m_cursor_location.height
        };

        Gtk.Allocation allocation;
        m_toplevel.get_allocation(out allocation);
        Gdk.Point window_right_bottom = {
            cursor_left_bottom.x + allocation.width,
            cursor_left_bottom.y + allocation.height
        };

        Gdk.Rectangle monitor_area = get_monitor_geometry(window);
        int monitor_right = monitor_area.x + monitor_area.width;
        int monitor_bottom = monitor_area.y + monitor_area.height;

        int x, y;
        if (window_right_bottom.x > monitor_right)
            x = monitor_right - allocation.width;
        else
            x = cursor_left_bottom.x;
        if (x < 0)
            x = 0;

        if (window_right_bottom.y > monitor_bottom)
            y = m_cursor_location.y - allocation.height;
        else
            y = cursor_left_bottom.y;
        if (y < 0)
            y = 0;

        move(x, y);
    }

    private Gdk.Rectangle get_monitor_geometry(Gtk.Widget window) {
        Gdk.Rectangle monitor_area = { 0, };

        // Use get_monitor_geometry() instead of get_monitor_area().
        // get_monitor_area() excludes docks, but the lookup window should be
        // shown over them.
        Gdk.Monitor monitor = window.get_display().get_monitor_at_point(
                m_cursor_location.x,
                m_cursor_location.y);
        monitor_area = monitor.get_geometry();
        return monitor_area;
    }

#if USE_GDK_WAYLAND
    private void realize_window(bool initial) {
        // The custom surface can be used when the Wayland input-method
        // is activated.
        if (m_no_wayland_panel)
            return;
        var window = m_toplevel.get_window();
        if (!window.ensure_native()) {
            warning("No native window.");
            return;
        }
        Type instance_type = window.get_type();
        Type wayland_type = typeof(GdkWayland.Window);
        if (!instance_type.is_a(wayland_type)) {
            warning("Not GdkWindowWayland.");
            return;
        }
        if (initial)
            ((GdkWayland.Window)window).set_use_custom_surface();
        var surface = ((GdkWayland.Window)window).get_wl_surface();
        realize_surface(surface);
    }
#endif
}
