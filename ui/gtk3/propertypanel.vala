/* vim:set et sts=4 sw=4:
 *
 * ibus - The Input Bus
 *
 * Copyright(c) 2013-2016 Red Hat, Inc.
 * Copyright(c) 2013-2015 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright(c) 2013-2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
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

enum PanelShow {
    DO_NOT_SHOW,
    AUTO_HIDE,
    ALWAYS
}

public class PropertyPanel : Gtk.Box {
    private unowned Gdk.Window m_root_window;
#if ENABLE_XIM
    private unowned X.Display m_xdisplay;
#endif
    private Gtk.Window m_toplevel;
    private IBus.PropList m_props;
    private IPropToolItem[] m_items;
    private Gdk.Rectangle m_cursor_location = Gdk.Rectangle(){
            x = -1, y = -1, width = 0, height = 0 };
    private int m_show = PanelShow.DO_NOT_SHOW;
    private uint m_auto_hide_timeout = 10000;
    private uint m_auto_hide_timeout_id = 0;
    private bool m_follow_input_cursor_when_always_shown = false;
    // The timeout indicates milliseconds. 1000 msec == 1 sec
    private const uint MONITOR_NET_WORKAREA_TIMEOUT = 300000;
    private uint m_remove_filter_id;

    public PropertyPanel() {
        /* Chain up base class constructor */
        GLib.Object(orientation: Gtk.Orientation.HORIZONTAL,
                    spacing: 0);

        set_visible(true);

#if ENABLE_XIM
        var display = BindingCommon.get_xdisplay();
        if (display != null) {
            m_xdisplay = display.get_xdisplay();
            var screen = display.get_default_screen();
            m_root_window = screen.get_root_window();
        }
#endif

        m_toplevel = new Gtk.Window(Gtk.WindowType.POPUP);
        m_toplevel.add_events(Gdk.EventMask.BUTTON_PRESS_MASK);

        Handle handle = new Handle();
        handle.set_visible(true);
        pack_start(handle, false, false, 0);

        m_toplevel.add(this);

        m_toplevel.size_allocate.connect((w, a) => {
            if (!m_follow_input_cursor_when_always_shown &&
                m_show == PanelShow.ALWAYS && m_items.length > 0 &&
                m_cursor_location.x == -1 && m_cursor_location.y == -1) {
                set_default_location();
                m_cursor_location.x = 0;
                m_cursor_location.y = 0;
            }
        });

#if ENABLE_XIM
        // PropertyPanel runs before KDE5 panel runs and
        // monitor the desktop size.
        monitor_net_workarea_atom();
#endif
    }

    public void set_properties(IBus.PropList props) {
        debug("set_properties()\n");

        // When click PropMenuToolButton, the focus is changed and
        // set_properties() is called here while the menu button is active.
        // Ignore that case here not to remove items.
        bool has_active = false;
        foreach (var item in m_items) {
            Type type = item.get_type();
            if (type == typeof(PropMenuToolButton) ||
                type == typeof(PropToggleToolButton)) {
                if (((Gtk.ToggleToolButton)item).get_active()) {
                    has_active = true;
                    break;
                }
            }
        }
        if (has_active)
            return;

        foreach (var item in m_items)
            remove((item as Gtk.Widget));
        m_items = {};

        m_props = props;

        create_menu_items();

        /* show_with_auto_hide_timer() needs to call here because
         * if the event order is, focus_in(), set_cursor_location() and
         * set_properties(), m_items.length can be 0 when
         * set_cursor_location() is called and Property Panel is not shown.
         */
        show_with_auto_hide_timer();
    }

    public void update_property(IBus.Property prop) {
        GLib.assert(prop != null);

        debug("update_property(prop.key = %s)\n", prop.get_key());

        if (m_props != null)
            m_props.update_property(prop);

        /* Need to update GUI since panel buttons are not redrawn. */
        foreach (var item in m_items)
            item.update_property(prop);

        show_with_auto_hide_timer();
    }

    public void set_cursor_location(int x, int y, int width, int height) {
        if (!m_follow_input_cursor_when_always_shown &&
            m_show == PanelShow.ALWAYS)
            return;

        /* FIXME: set_cursor_location() has a different behavior
         * in embedded preedit by applications.
         * GtkTextView applications, e.g. gedit, always call
         * set_cursor_location() with and without preedit
         * but VTE applications, e.g. gnome-terminal, and xterm
         * do not call set_cursor_location() with preedit.
         * firefox and thunderbird do not call set_cursor_location()
         * without preedit.
         * This may treat GtkIMContext and XIM with different ways.
         * Maybe get_preedit_string() class method.
         */

        /* FIXME: When the cursor is at the bottom of the screen,
         * gedit returns the right cursor position but terminal applications
         * such as gnome-terminal, xfce4-terminal and etc, the position is
         * not accurate and the cursor and panel could be overlapped slightly.
         * Maybe it's a bug in vte.
         */
        Gdk.Rectangle location = Gdk.Rectangle(){
            x = x, y = y, width = width, height = height };

        if (m_cursor_location == location)
            return;

        debug("set_cursor_location(x = %d, y = %d, width = %d, height = %d)\n",
              x, y, width, height);

        /* Hide the panel in AUTO_HIDE mode when the cursor position is
         * chagned on the same input context by typing keyboard or
         * clicking mouse. (But not focus change or property change)
         */
        if (m_show == PanelShow.AUTO_HIDE)
            if (m_cursor_location.x != -1 || m_cursor_location.y != -1) {
                m_cursor_location = location;
                hide_if_necessary();
                adjust_window_position();
                return;
            }

        m_cursor_location = location;
        adjust_window_position();
        show_with_auto_hide_timer();
    }

    public void set_preedit_text(IBus.Text? text, uint cursor) {
        if (text == null && cursor == 0)
            return;

        debug("set_preedit_text(text, cursor = %u)\n", cursor);

        /* Hide the panel in AUTO_HIDE mode when embed-preedit-text value
         * is disabled and the preedit is changed on the same input context.
         */
        hide_if_necessary();
    }

    public void set_auxiliary_text(IBus.Text? text) {
        if (text == null)
            return;

        debug("set_auxiliary_text(text)\n");

        hide_if_necessary();
    }

    public void set_lookup_table(IBus.LookupTable? table) {
        if (table == null)
            return;

        debug("set_lookup_table(table)\n");

        hide_if_necessary();
    }

    public new void show() {
        /* m_items.length is not checked here because set_properties()
         * is not called yet when set_show() is called. */
        if (m_show == PanelShow.DO_NOT_SHOW) {
            m_toplevel.hide();
            return;
        }
        else if (m_show == PanelShow.ALWAYS) {
            m_toplevel.show_all();
            return;
        }

        /* Do not change the state here if m_show == AUTO_HIDE. */
    }

    public new void hide() {
        m_toplevel.hide();
    }

    public void focus_in() {
        debug("focus_in()\n");

        /* Reset m_auto_hide_timeout_id in previous focus-in */
        hide_if_necessary();

        /* Invalidate m_cursor_location before set_cursor_location()
         * is called because the position can be same even if the input
         * focus is changed.
         * E.g. Two tabs on gnome-terminal can keep the cursor position.
         */
        if (m_follow_input_cursor_when_always_shown ||
            m_show != PanelShow.ALWAYS)
            m_cursor_location = { -1, -1, 0, 0 };

       /* set_cursor_location() will be called later. */
    }

    public void set_show(int _show) {
        m_show = _show;
        show();
    }

    public void set_auto_hide_timeout(uint timeout) {
        m_auto_hide_timeout = timeout;
    }

    public void set_follow_input_cursor_when_always_shown(bool is_follow) {
        m_follow_input_cursor_when_always_shown = is_follow;
    }

    private void create_menu_items() {
        int i = 0;
        while (true) {
            IBus.Property prop = m_props.get(i);
            if (prop == null)
                break;

            i++;
            IPropToolItem item = null;
            switch(prop.get_prop_type()) {
                case IBus.PropType.NORMAL:
                    item = new PropToolButton(prop);
                    break;
                case IBus.PropType.TOGGLE:
                    item = new PropToggleToolButton(prop);
                    break;
                case IBus.PropType.MENU:
                    item = new PropMenuToolButton(prop);
                    break;
                case IBus.PropType.SEPARATOR:
                    item = new PropSeparatorToolItem(prop);
                    break;
                default:
                    warning("unknown property type %d",
                            (int) prop.get_prop_type());
                    break;
            }
            if (item != null) {
                pack_start(item as Gtk.Widget, false, false, 0);
                m_items += item;
                item.property_activate.connect((w, k, s) =>
                                               property_activate(k, s));
            }
        }
    }

    private void move(int x, int y) {
        m_toplevel.move(x, y);
    }

    private void adjust_window_position() {
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

        int root_width = 0;
        int root_height = 0;
        if (m_root_window != null) {
            root_width = m_root_window.get_width();
            root_height = m_root_window.get_height();
        }

        int x, y;
        if (window_right_bottom.x > root_width && root_width > 0)
            x = root_width - allocation.width;
        else
            x = cursor_right_bottom.x;

        if (window_right_bottom.y > root_height && root_height > 0)
            y = m_cursor_location.y - allocation.height;
        else
            y = cursor_right_bottom.y;

        move(x, y);
    }

    private bool is_bottom_panel() {
        string desktop = Environment.get_variable("XDG_CURRENT_DESKTOP");
        // LXDE has not implemented DesktopNames yet.
        if (desktop == null)
            desktop = Environment.get_variable("XDG_SESSION_DESKTOP");
        switch (desktop) {
            case "KDE":         return true;
            case "LXDE":        return true;
            default:            return false;
        }
    }

    private void set_default_location() {
        Gtk.Allocation allocation;
        m_toplevel.get_allocation(out allocation);

        Gdk.Rectangle monitor_area;
#if VALA_0_34
        // gdk_screen_get_monitor_workarea() no longer return the correct
        // area from "_NET_WORKAREA" atom in GTK 3.22
        Gdk.Monitor monitor = Gdk.Display.get_default().get_monitor(0);
        monitor_area = monitor.get_workarea();
#else
        Gdk.Screen screen = Gdk.Screen.get_default();
        monitor_area = screen.get_monitor_workarea(0);
#endif
        int monitor_right = monitor_area.x + monitor_area.width;
        int monitor_bottom = monitor_area.y + monitor_area.height;
        int x, y;
        if (is_bottom_panel()) {
            /* Translators: If your locale is RTL, the msgstr is "default:RTL".
             * Otherwise the msgstr is "default:LTR". */
            if (_("default:LTR") != "default:RTL") {
                x = monitor_right - allocation.width;
                y = monitor_bottom - allocation.height;
            } else {
                x = monitor_area.x;
                y = monitor_bottom - allocation.height;
            }
        } else {
            if (_("default:LTR") != "default:RTL") {
                x = monitor_right - allocation.width;
                y = monitor_area.y;
            } else {
                x = monitor_area.x;
                y = monitor_area.y;
            }
        }

        move(x, y);
    }

#if ENABLE_XIM
    private Gdk.FilterReturn root_window_filter(Gdk.XEvent gdkxevent,
                                                Gdk.Event  event) {
        X.Event *xevent = (X.Event*) gdkxevent;
        if (xevent.type == X.EventType.PropertyNotify) {
            if (m_xdisplay == null) {
                if (m_remove_filter_id > 0) {
                    GLib.Source.remove(m_remove_filter_id);
                    m_remove_filter_id = 0;
                }
                return Gdk.FilterReturn.CONTINUE;
            }
            string aname = m_xdisplay.get_atom_name(xevent.xproperty.atom);
            if (aname == "_NET_WORKAREA" && xevent.xproperty.state == 0) {
                set_default_location();
                m_root_window.remove_filter(root_window_filter);
                if (m_remove_filter_id > 0) {
                    GLib.Source.remove(m_remove_filter_id);
                    m_remove_filter_id = 0;
                }
                return Gdk.FilterReturn.CONTINUE;
            }
        }
        return Gdk.FilterReturn.CONTINUE;
    }

    private void monitor_net_workarea_atom() {
        Gdk.EventMask events = m_root_window.get_events();
        if ((events & Gdk.EventMask.PROPERTY_CHANGE_MASK) == 0)
            m_root_window.set_events (events |
                                      Gdk.EventMask.PROPERTY_CHANGE_MASK);

        m_root_window.add_filter(root_window_filter);

        m_remove_filter_id = GLib.Timeout.add(MONITOR_NET_WORKAREA_TIMEOUT,
                                              () => {
            m_remove_filter_id = 0;
            m_root_window.remove_filter(root_window_filter);
            return false;
        },
        GLib.Priority.DEFAULT_IDLE);
    }
#endif

    private void show_with_auto_hide_timer() {
        /* Do not call gtk_window_resize() in
         * GtkWidgetClass->get_preferred_width()
         * because the following warning is shown in GTK 3.20:
         * "Allocating size to GtkWindow %x without calling
         * gtk_widget_get_preferred_width/height(). How does the code
         * know the size to allocate?"
         * in gtk_widget_size_allocate_with_baseline() */
        m_toplevel.resize(1, 1);

        if (m_items.length == 0) {
            /* Do not blink the panel with focus-in in case the panel
             * is always shown. */
            if (m_follow_input_cursor_when_always_shown ||
                m_show != PanelShow.ALWAYS)
                m_toplevel.hide();

            return;
        }

        if (m_show != PanelShow.AUTO_HIDE) {
            show();
            return;
        }

        /* If all windows are closed, desktop background is focused and
         * focus_in() and set_properties() are called but
         * set_cursor_location() is not called.
         * Then we should not show Property panel when m_cursor_location
         * is (-1, -1) because of no windows.
         */
        if (m_cursor_location.x == -1 && m_cursor_location.y == -1)
            return;

        if (m_auto_hide_timeout_id != 0)
            GLib.Source.remove(m_auto_hide_timeout_id);

        m_toplevel.show_all();

        /* Change the priority because IME typing sometimes freezes. */
        m_auto_hide_timeout_id = GLib.Timeout.add(m_auto_hide_timeout, () => {
            m_toplevel.hide();
            m_auto_hide_timeout_id = 0;
            return false;
        },
        GLib.Priority.DEFAULT_IDLE);
    }

    private void hide_if_necessary() {
        if (m_show == PanelShow.AUTO_HIDE && m_auto_hide_timeout_id != 0) {
            GLib.Source.remove(m_auto_hide_timeout_id);
            m_auto_hide_timeout_id = 0;
            m_toplevel.hide();
        }
    }

    public signal void property_activate(string key, int state);
}

public interface IPropToolItem : GLib.Object {
    public abstract void update_property(IBus.Property prop);
    public signal void property_activate(string key, int state);
}

public class PropMenu : Gtk.Menu, IPropToolItem {
    private Gtk.Widget m_parent_button;
    private IPropItem[] m_items;

    public PropMenu(IBus.Property prop) {
        /* Chain up base class constructor */
        GLib.Object();

        set_take_focus(false);
        create_items(prop.get_sub_props());
        show_all();
        set_sensitive(prop.get_sensitive());
    }

    public void update_property(IBus.Property prop) {
        foreach (var item in m_items)
            item.update_property(prop);
    }

    public new void popup(uint       button,
                          uint32     activate_time,
                          Gtk.Widget widget) {
#if VALA_0_34
        base.popup_at_widget(widget,
                             Gdk.Gravity.SOUTH_WEST,
                             Gdk.Gravity.NORTH_WEST,
                             null);
#else
        m_parent_button = widget;
        base.popup(null, null, menu_position, button, activate_time);
#endif
    }

    public override void destroy() {
        m_parent_button = null;
        foreach (var item in m_items)
            remove((item as Gtk.Widget));
        m_items = {};
        base.destroy();
    }

    private void create_items(IBus.PropList props) {
        int i = 0;
        PropRadioMenuItem last_radio = null;

        while (true) {
            IBus.Property prop = props.get(i);
            if (prop == null)
                break;

            i++;
            IPropItem item = null;
            switch(prop.get_prop_type()) {
                case IBus.PropType.NORMAL:
                    item = new PropImageMenuItem(prop);
                    break;
                case IBus.PropType.TOGGLE:
                    item = new PropCheckMenuItem(prop);
                    break;
                case IBus.PropType.RADIO:
                    last_radio = new PropRadioMenuItem(prop, last_radio);
                    item = last_radio;
                    break;
                case IBus.PropType.MENU:
                    {
                        var menuitem = new PropImageMenuItem(prop);
                        menuitem.set_submenu(new PropMenu(prop));
                        item = menuitem;
                    }
                    break;
                case IBus.PropType.SEPARATOR:
                    item = new PropSeparatorMenuItem(prop);
                    break;
                default:
                    warning("Unknown property type: %d",
                            (int) prop.get_prop_type());
                    break;
            }
            if (prop.get_prop_type() != IBus.PropType.RADIO)
                last_radio = null;
            if (item != null) {
                append(item as Gtk.MenuItem);
                item.property_activate.connect((w, k, s) =>
                                               property_activate(k, s));
                m_items += item;
            }
        }
    }

#if !VALA_0_34
    private void menu_position(Gtk.Menu menu,
                               out int  x,
                               out int  y,
                               out bool push_in) {
        var button = m_parent_button;
        var screen = button.get_screen();
        var monitor = screen.get_monitor_at_window(button.get_window());

        Gdk.Rectangle monitor_location;
        screen.get_monitor_geometry(monitor, out monitor_location);

        button.get_window().get_origin(out x, out y);

        Gtk.Allocation button_allocation;
        button.get_allocation(out button_allocation);

        x += button_allocation.x;
        y += button_allocation.y;

        int menu_width;
        int menu_height;
        menu.get_size_request(out menu_width, out menu_height);

        if (x + menu_width >= monitor_location.width)
            x -= menu_width - button_allocation.width;
        else if (x - menu_width <= 0)
            ;
        else {
            if (x <= monitor_location.width * 3 / 4)
                ;
            else
                x -= menu_width - button_allocation.width;
        }

        if (y + button_allocation.height + menu_width
                >= monitor_location.height)
            y -= menu_height;
        else if (y - menu_height <= 0)
            y += button_allocation.height;
        else {
            if (y <= monitor_location.height * 3 / 4)
                y += button_allocation.height;
            else
                y -= menu_height;
        }

        push_in = false;
    }
#endif
}

public class PropToolButton : Gtk.ToolButton, IPropToolItem {
    private IBus.Property m_prop = null;

    public PropToolButton(IBus.Property prop) {
        /* Chain up base class constructor
         *
         * If the constructor sets "label" property, "halign" property
         * does not work in KDE5 so use sync() for the label.
         */
        GLib.Object(halign: Gtk.Align.START);

        m_prop = prop;

        set_homogeneous(false);
        sync();
    }

    public void update_property(IBus.Property prop) {
        if (m_prop.get_key() != prop.get_key())
            return;
        m_prop.set_symbol(prop.get_symbol());
        m_prop.set_tooltip(prop.get_tooltip());
        m_prop.set_sensitive(prop.get_sensitive());
        m_prop.set_icon(prop.get_icon());
        m_prop.set_state(prop.get_state());
        m_prop.set_visible(prop.get_visible());
        sync();
    }

    private void sync() {
        set_label(m_prop.get_symbol().get_text());
        set_tooltip_text(m_prop.get_tooltip().get_text());
        set_sensitive(m_prop.get_sensitive());
        set_icon_name(m_prop.get_icon());

        if (m_prop.get_visible())
            show();
        else
            hide();
    }

    public override void clicked() {
        property_activate(m_prop.get_key(), m_prop.get_state());
    }

    public new void set_icon_name(string icon_name) {
        string label = m_prop.get_symbol().get_text();
        IconWidget icon_widget = null;

        if (label == "") {
            label = null;
            icon_widget = new IconWidget(icon_name, Gtk.IconSize.BUTTON);
            set_is_important(false);
        } else {
            set_is_important(true);
        }

        set_icon_widget(icon_widget);
    }
}

public class PropToggleToolButton : Gtk.ToggleToolButton, IPropToolItem {
    private IBus.Property m_prop = null;

    public PropToggleToolButton(IBus.Property prop) {
        /* Chain up base class constructor
         *
         * Need to set halign for KDE5
         */
        GLib.Object(halign: Gtk.Align.START);

        m_prop = prop;

        set_homogeneous(false);
        sync();
    }

    public new void set_property(IBus.Property prop) {
        m_prop = prop;
        sync();
    }

    public void update_property(IBus.Property prop) {
        if (m_prop.get_key() != prop.get_key())
            return;
        m_prop.set_symbol(prop.get_symbol());
        m_prop.set_tooltip(prop.get_tooltip());
        m_prop.set_sensitive(prop.get_sensitive());
        m_prop.set_icon(prop.get_icon());
        m_prop.set_state(prop.get_state());
        m_prop.set_visible(prop.get_visible());
        sync();
    }

    private void sync() {
        set_label(m_prop.get_symbol().get_text());
        set_tooltip_text(m_prop.get_tooltip().get_text());
        set_sensitive(m_prop.get_sensitive());
        set_icon_name(m_prop.get_icon());
        set_active(m_prop.get_state() == IBus.PropState.CHECKED);

        if (m_prop.get_visible())
            show();
        else
            hide();
    }

    public override void toggled() {
        /* Do not send property-activate to engine in case the event is
         * sent from engine. */

        bool do_emit = false;

        if (get_active()) {
            if (m_prop.get_state() != IBus.PropState.CHECKED)
                do_emit = true;
            m_prop.set_state(IBus.PropState.CHECKED);
        } else {
            if (m_prop.get_state() != IBus.PropState.UNCHECKED)
                do_emit = true;
            m_prop.set_state(IBus.PropState.UNCHECKED);
        }

        if (do_emit)
            property_activate(m_prop.get_key(), m_prop.get_state());
    }

    public new void set_icon_name(string icon_name) {
        string label = m_prop.get_symbol().get_text();
        IconWidget icon_widget = null;

        if (label == "") {
            label = null;
            icon_widget = new IconWidget(icon_name, Gtk.IconSize.BUTTON);
            set_is_important(false);
        } else {
            set_is_important(true);
        }

        set_icon_widget(icon_widget);
    }
}

public class PropMenuToolButton : PropToggleToolButton, IPropToolItem {
    private PropMenu m_menu = null;

    public PropMenuToolButton(IBus.Property prop) {
        /* Chain up base class constructor
         *
         * Need to set halign for KDE5
         */
        GLib.Object(halign: Gtk.Align.START);

        m_menu = new PropMenu(prop);
        m_menu.deactivate.connect((m) =>
                                  set_active(false));
        m_menu.property_activate.connect((k, s) =>
                                         property_activate(k, s));

        base.set_property(prop);
    }

    public new void update_property(IBus.Property prop) {
        base.update_property(prop);
        m_menu.update_property(prop);
    }

    public override void toggled() {
        if (get_active())
            m_menu.popup(0, Gtk.get_current_event_time(), this);
    }

    public override void destroy() {
        m_menu = null;
        base.destroy();
    }
}

public class PropSeparatorToolItem : Gtk.SeparatorToolItem, IPropToolItem {
    public PropSeparatorToolItem(IBus.Property prop) {
        /* Chain up base class constructor */
        GLib.Object();

        set_homogeneous(false);
    }

    public void update_property(IBus.Property prop) {
    }
}
