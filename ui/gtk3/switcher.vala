/* vim:set et sts=4 sw=4:
 *
 * ibus - The Input Bus
 *
 * Copyright(c) 2011-2016 Peng Huang <shawn.p.huang@gmail.com>
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

class Switcher : Gtk.Window {
    private class IBusEngineButton : Gtk.Button {
        public IBusEngineButton(IBus.EngineDesc engine, Switcher switcher) {
            GLib.Object();

            var longname = engine.get_longname();
            var textdomain = engine.get_textdomain();
            this.transname = GLib.dgettext(textdomain, longname);

            var name = engine.get_name();

            if (name.length < 4 || name[0:4] != "xkb:") {
                IconWidget icon = new IconWidget(engine.get_icon(),
                                                 Gtk.IconSize.DIALOG);
                icon.set_halign(Gtk.Align.CENTER);
                icon.set_valign(Gtk.Align.CENTER);
                add(icon);
            } else {
                var language = switcher.get_xkb_language(engine);

                Gtk.Label label = new Gtk.Label(language);
                label.set_halign(Gtk.Align.CENTER);
                label.set_valign(Gtk.Align.CENTER);
                string language_font = "Monospace Bold 16";
                string markup = "<span font=\"%s\">%s</span>".
                        printf(language_font, language);

                label.set_markup(markup);

                int fixed_width, fixed_height;
                Gtk.icon_size_lookup(Gtk.IconSize.DIALOG,
                                     out fixed_width,
                                     out fixed_height);
                label.set_size_request(fixed_width, fixed_height);

                add(label);
            }
        }

        public string transname { get; set; }

        public override bool draw(Cairo.Context cr) {
            base.draw(cr);
            if (is_focus) {
                cr.save();
                cr.rectangle(
                        0, 0, get_allocated_width(), get_allocated_height());
                cr.set_source_rgba(0.0, 0.0, 1.0, 0.1);
                cr.fill();
                cr.restore();
            }
            return true;
        }
    }

    private Gtk.Box m_box;
    private Gtk.Label m_label;
    private IBusEngineButton[] m_buttons = {};
    private IBus.EngineDesc[] m_engines;
    private uint m_selected_engine;
    private uint m_keyval;
    private uint m_modifiers;
    private Gdk.ModifierType m_primary_modifier;
    private bool m_is_running = false;
    private string m_input_context_path = "";
    private GLib.MainLoop m_loop;
    private int m_result = -1;
    private IBus.EngineDesc? m_result_engine = null;
    private uint m_popup_delay_time = 0;
    private uint m_popup_delay_time_id = 0;
    private int m_root_x;
    private int m_root_y;
    private double m_mouse_init_x;
    private double m_mouse_init_y;
    private bool   m_mouse_moved;
    private bool   m_is_wayland;
    private bool   m_no_wayland_panel;
    private GLib.HashTable<string, string> m_xkb_languages =
            new GLib.HashTable<string, string>(GLib.str_hash,
                                               GLib.str_equal);
#if USE_GDK_WAYLAND
    private bool m_hide_after_show;

    public signal void realize_surface(void *surface);
#endif

    public Switcher(bool is_wayland,
                    bool no_wayland_panel) {
        GLib.Object(
            type : Gtk.WindowType.POPUP,
            events : Gdk.EventMask.KEY_PRESS_MASK |
                     Gdk.EventMask.KEY_RELEASE_MASK,
            window_position : Gtk.WindowPosition.CENTER,
            accept_focus : true,
            decorated : false,
            modal : true,
            focus_visible : true
        );
        m_is_wayland = is_wayland;
        m_no_wayland_panel = no_wayland_panel;
        Gtk.Box vbox = new Gtk.Box(Gtk.Orientation.VERTICAL, 0);
        add(vbox);
        m_box = new Gtk.Box(Gtk.Orientation.HORIZONTAL, 0);
        m_box.set_halign(Gtk.Align.CENTER);
        m_box.set_valign(Gtk.Align.CENTER);
        vbox.pack_start(m_box, true, true, 0);
        m_label = new Gtk.Label("");

        /* Set the accessible role of the label to a status bar so it
         * will emit name changed events that can be used by screen
         * readers.
         */
        Atk.Object obj = m_label.get_accessible();
        obj.set_role (Atk.Role.STATUSBAR);

        m_label.set_margin_start(3);
        m_label.set_margin_end(3);
        m_label.set_margin_top(3);
        m_label.set_margin_bottom(3);
        vbox.pack_end(m_label, false, false, 0);

#if USE_GDK_WAYLAND
        if (m_is_wayland) {
            this.realize.connect((w) => {
                realize_window(true);
            });
            this.show.connect((w) => {
                if (m_hide_after_show)
                    realize_window(false);
                m_hide_after_show = false;
            });
            this.hide.connect((w) => {
                m_hide_after_show = true;
            });
        } else {
            grab_focus();
        }
#else
        grab_focus();
#endif
    }

    public int run_popup(uint              keyval,
                         uint              state,
                         IBus.EngineDesc[] engines,
                         int               index) {
        m_keyval = keyval;
        m_modifiers = state & IBus.MODIFIER_FILTER;
        if (m_keyval >= IBus.KEY_A && m_keyval <= IBus.KEY_Z &&
            (m_modifiers & Gdk.ModifierType.SHIFT_MASK) != 0) {
            m_keyval = m_keyval - IBus.KEY_A + IBus.KEY_a;
        }
        m_primary_modifier = KeybindingManager.get_primary_modifier(
                m_modifiers);
        m_selected_engine = m_result = index;
        update_engines(engines);
        /* Let gtk recalculate the window size. */
        resize(1, 1);
        m_label.set_text(m_buttons[index].transname);
        show_all();
        set_focus(m_buttons[index]);
        m_result_engine = m_engines[m_result];
        return m_result;
    }

    public int run(uint              keyval,
                   uint              state,
                   Gdk.Event         event,
                   IBus.EngineDesc[] engines,
                   int               index,
                   string            input_context_path) {
        assert(m_loop == null);
        assert(index < engines.length);

        if (m_is_running)
            return index;
        m_is_running = true;
        m_keyval = keyval;
        m_modifiers = state;
        m_primary_modifier =
            KeybindingManager.get_primary_modifier(
                state & IBus.MODIFIER_FILTER);
        m_selected_engine = m_result = index;
        m_input_context_path = input_context_path;
        m_result_engine = null;

        update_engines(engines);
        /* Let gtk recalculate the window size. */
        resize(1, 1);

        m_label.set_text(m_buttons[index].transname);
        m_buttons[index].grab_focus();

        // Avoid regressions.
        if (m_popup_delay_time > 0) {
            get_position(out m_root_x, out m_root_y);
            // Pull the window from the screen so that the window gets
            // the key press and release events but mouse does not select
            // an IME unexpectedly.
            move(-1000, -1000);
        }

        show_all();

        if (m_popup_delay_time > 0) {
            // Restore the window position after m_popup_delay_time
            m_popup_delay_time_id = GLib.Timeout.add(m_popup_delay_time,
                                                     () => {
                restore_window_position("timeout");
                return false;
            });
        }

        Gdk.Device pointer;
#if VALA_0_34
        Gdk.Seat seat = event.get_seat();
        if (seat == null) {
            var display = get_display();
            seat = display.get_default_seat();
        }
        //keyboard = seat.get_keyboard();
        pointer = seat.get_pointer();

        Gdk.GrabStatus status;
        // Grab all keyboard events
        status = seat.grab(get_window(),
                           Gdk.SeatCapabilities.KEYBOARD,
                           true,
                           null,
                           event,
                           null);
        if (status != Gdk.GrabStatus.SUCCESS)
            warning("Grab keyboard failed! status = %d", status);
        status = seat.grab(get_window(),
                           Gdk.SeatCapabilities.POINTER,
                           true,
                           null,
                           event,
                           null);
        if (status != Gdk.GrabStatus.SUCCESS)
            warning("Grab pointer failed! status = %d", status);
#else
        Gdk.Device device = event.get_device();
        if (device == null) {
            var display = get_display();
            var device_manager = display.get_device_manager();
/* The macro VALA_X_Y supports even numbers.
 * http://git.gnome.org/browse/vala/commit/?id=294b374af6
 */
#if VALA_0_16
            device = device_manager.list_devices(Gdk.DeviceType.MASTER).data;
#else
            unowned GLib.List<Gdk.Device> devices =
                    device_manager.list_devices(Gdk.DeviceType.MASTER);
            device = devices.data;
#endif
        }

        Gdk.Device keyboard;
        if (device.get_source() == Gdk.InputSource.KEYBOARD) {
            keyboard = device;
            pointer = device.get_associated_device();
        } else {
            pointer = device;
            keyboard = device.get_associated_device();
        }

        Gdk.GrabStatus status;
        // Grab all keyboard events
        status = keyboard.grab(get_window(),
                               Gdk.GrabOwnership.NONE,
                               true,
                               Gdk.EventMask.KEY_PRESS_MASK |
                               Gdk.EventMask.KEY_RELEASE_MASK,
                               null,
                               Gdk.CURRENT_TIME);
        if (status != Gdk.GrabStatus.SUCCESS)
            warning("Grab keyboard failed! status = %d", status);
        // Grab all pointer events
        status = pointer.grab(get_window(),
                              Gdk.GrabOwnership.NONE,
                              true,
                              Gdk.EventMask.BUTTON_PRESS_MASK |
                              Gdk.EventMask.BUTTON_RELEASE_MASK,
                              null,
                              Gdk.CURRENT_TIME);
        if (status != Gdk.GrabStatus.SUCCESS)
            warning("Grab pointer failed! status = %d", status);
#endif

        // Probably we can delete m_popup_delay_time in 1.6
        pointer.get_position_double(null,
                                    out m_mouse_init_x,
                                    out m_mouse_init_y);
        m_mouse_moved = false;


        m_loop = new GLib.MainLoop();
        m_loop.run();
        m_loop = null;

#if VALA_0_34
        seat.ungrab();
#else
        keyboard.ungrab(Gdk.CURRENT_TIME);
        pointer.ungrab(Gdk.CURRENT_TIME);
#endif

        hide();
        // Make sure the switcher is hidden before returning from this function.
        while (Gtk.events_pending())
            Gtk.main_iteration ();

        GLib.assert(m_result < m_engines.length);
        m_result_engine = m_engines[m_result];
        m_is_running = false;

        return m_result;
    }

    /* Based on metacity/src/ui/tabpopup.c:meta_ui_tab_popup_new */
    private void update_engines(IBus.EngineDesc[] engines) {
        foreach (var button in m_buttons) {
            button.destroy();
        }
        m_buttons = {};

        if (engines == null) {
            m_engines = {};
            return;
        }

        m_engines = engines;
        int max_label_width = 0;

        for (int i = 0; i < m_engines.length; i++) {
            var index = i;
            var engine = m_engines[i];
            var button = new IBusEngineButton(engine, this);
            var longname = engine.get_longname();
            var textdomain = engine.get_textdomain();
            var transname = GLib.dgettext(textdomain, longname);
            button.set_relief(Gtk.ReliefStyle.NONE);
            button.add_events(Gdk.EventMask.POINTER_MOTION_MASK);
            button.show();

            button.enter_notify_event.connect((e) => {
                // avoid gtk_button_update_state()
                return true;
            });
            button.motion_notify_event.connect((e) => {
#if VALA_0_24
                Gdk.EventMotion pe = e;
#else
                Gdk.EventMotion *pe = &e;
#endif
                if (m_selected_engine == index)
                    return false;
                if (!m_mouse_moved &&
                    m_mouse_init_x == pe.x_root &&
                    m_mouse_init_y == pe.y_root) {
                    return false;
                }
                m_mouse_moved = true;
                if (!m_is_wayland)
                    button.grab_focus();
                m_selected_engine = index;
                return false;
            });

            button.button_press_event.connect((e) => {
                m_selected_engine = index;
                m_result = (int)m_selected_engine;
                m_loop.quit();
                return true;
            });

            button.transname = transname;
            m_label.set_label(transname);

            int width;
            m_label.get_preferred_width(null, out width);
            max_label_width = int.max(max_label_width, width);

            m_box.pack_start(button, true, true);
            m_buttons += button;
        }

        m_label.set_text(m_buttons[0].transname);
        m_label.set_ellipsize(Pango.EllipsizeMode.END);

        Gdk.Display display = Gdk.Display.get_default();
        int screen_width = 0;
#if VALA_0_34
        // display.get_monitor_at_window() is null because of unrealized window
        Gdk.Monitor monitor = display.get_primary_monitor();
        if (monitor == null)
            return;
        Gdk.Rectangle area = monitor.get_geometry();
        screen_width = area.width;
#else
        Gdk.Screen screen = (display != null) ?
                display.get_default_screen() : null;

        if (screen != null) {
            screen_width = screen.get_width();
        }
#endif

        if (screen_width > 0 && max_label_width > (screen_width / 4)) {
            max_label_width = screen_width / 4;
        }

        /* add random padding */
        max_label_width += 20;
        set_default_size(max_label_width, -1);
    }

    private void next_engine() {
        if (m_selected_engine == m_engines.length - 1)
            m_selected_engine = 0;
        else
            m_selected_engine ++;
        m_label.set_text(m_buttons[m_selected_engine].transname);
        set_focus(m_buttons[m_selected_engine]);
    }

    private void previous_engine() {
        if (m_selected_engine == 0)
            m_selected_engine = m_engines.length - 1;
        else
            m_selected_engine --;
        m_label.set_text(m_buttons[m_selected_engine].transname);
        set_focus(m_buttons[m_selected_engine]);
    }

    private void restore_window_position(string debug_str) {
        debug("restore_window_position %s: (%ld, %ld)\n",
                debug_str, m_root_x, m_root_y);

        if (m_popup_delay_time_id == 0) {
            return;
        }

        GLib.Source.remove(m_popup_delay_time_id);
        m_popup_delay_time_id = 0;
        move(m_root_x, m_root_y);
    }

    /* override virtual functions */
    public override void show() {
        base.show();
        set_focus_visible(true);
    }

#if USE_GDK_WAYLAND
    private void realize_window(bool initial) {
        // The custom surface can be used when the Wayland input-method
        // is activated.
        if (m_no_wayland_panel)
            return;
        var window = get_window();
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

    public override bool key_press_event(Gdk.EventKey e) {
        bool retval = Gdk.EVENT_STOP;

/* Gdk.EventKey is changed to the pointer.
 * https://git.gnome.org/browse/vala/commit/?id=598942f1
 */
#if VALA_0_24
        Gdk.EventKey pe = e;
#else
        Gdk.EventKey *pe = &e;
#endif

        if (m_popup_delay_time > 0) {
            restore_window_position("pressed");
        }

        do {
            uint modifiers = IBus.MODIFIER_FILTER & pe.state;
            uint keyval = pe.keyval;

            if ((modifiers != m_modifiers) &&
                (modifiers != (m_modifiers | Gdk.ModifierType.SHIFT_MASK))) {
                break;
            }

            if (keyval == m_keyval) {
                if (modifiers == m_modifiers)
                    next_engine();
                else // modififers == m_modifiers | SHIFT_MASK
                    previous_engine();
                break;
            }

            switch (keyval) {
                case 0x08fb: /* leftarrow */
                case 0xff51: /* Left */
                    previous_engine();
                    break;
                case 0x08fc: /* uparrow */
                case 0xff52: /* Up */
                    break;
                case 0x08fd: /* rightarrow */
                case 0xff53: /* Right */
                    next_engine();
                    break;
                case 0x08fe: /* downarrow */
                case 0xff54: /* Down */
                    break;
                default:
                    debug("0x%04x", keyval);
                    break;
            }
        } while (false);
        return retval;
    }

    public override bool key_release_event(Gdk.EventKey e) {
#if VALA_0_24
        Gdk.EventKey pe = e;
#else
        Gdk.EventKey *pe = &e;
#endif

        if (KeybindingManager.primary_modifier_still_pressed((Gdk.Event) pe,
            m_primary_modifier)) {
            return Gdk.EVENT_STOP;
        }

        // if e.type == Gdk.EventType.KEY_RELEASE, m_loop is already null.
        // m_loop is always null in Wayland but this signal is not emitted
        // when the Wayland panel protocol is enabled.
        if (m_loop == null && !m_is_wayland)
            return Gdk.EVENT_PROPAGATE;

        if (m_popup_delay_time > 0) {
            if (m_popup_delay_time_id != 0) {
                GLib.Source.remove(m_popup_delay_time_id);
                m_popup_delay_time_id = 0;
            }
        }

        m_result = (int)m_selected_engine;
        if (!m_is_wayland) {
            m_loop.quit();
        } else if (m_no_wayland_panel) {
            // Switcher without the Wayland panel protocol can return
            // the m_result_engine immediately in Wayland without waiting
            // for the focus-in event due to a virtual input context in
            // ibus-daemon.
            GLib.assert(m_result < m_engines.length);
            m_result_engine = m_engines[m_result];
            hide();
        } else {
            // Switcher should not get focus with the Wayland panel protocol.
            GLib.assert_not_reached();
        }
        return Gdk.EVENT_STOP;
    }

    public void set_popup_delay_time(uint popup_delay_time) {
        m_popup_delay_time = popup_delay_time;
    }

    public string get_xkb_language(IBus.EngineDesc engine) {
        var name = engine.get_name();

        assert(name[0:4] == "xkb:");

        var language = m_xkb_languages[name];

        if (language != null)
            return language;

        language = engine.get_language();
        int length = language.length;

        /* Maybe invalid layout */
        if (length < 2)
            return language;

        language = language.up();

        int index = 0;

        foreach (var saved_language in m_xkb_languages.get_values()) {
            // Duplicated langauge is EN, EN_2 for E.G.
            if (length <= saved_language.length &&
                language == saved_language[0:length])
                index++;
        }

        if (index > 0) {
            unichar u = 0x2081 + index;
            language = "%s%s".printf(language, u.to_string());
        }

        m_xkb_languages.insert(name, language);
        return language;
    }

    public bool is_running() {
        return m_is_running;
    }

    public string get_input_context_path() {
        return m_input_context_path;
    }

    public IBus.EngineDesc? get_selected_engine() {
        return m_result_engine;
    }

    public void reset() {
        m_input_context_path = "";
        m_result = -1;
        m_result_engine = null;
        m_keyval = 0;
    }
}
