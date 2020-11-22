/* vim:set et sts=4 sw=4:
 *
 * ibus - The Input Bus
 *
 * Copyright(c) 2011 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright(c) 2017-2020 Takao Fujiwara <takao.fujiwara1@gmail.com>
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

class Application {
    private IBus.Bus m_bus;
    private Panel m_panel;

    public Application(string[] argv) {
        GLib.Intl.bindtextdomain(Config.GETTEXT_PACKAGE, Config.LOCALEDIR);
        GLib.Intl.bind_textdomain_codeset(Config.GETTEXT_PACKAGE, "UTF-8");
        IBus.init();
        Gtk.init(ref argv);

        m_bus = new IBus.Bus();

        m_bus.connected.connect(bus_connected);
        m_bus.disconnected.connect(bus_disconnected);

        if (m_bus.is_connected()) {
            init();
        }
    }

    private void init() {
        DBusConnection connection = m_bus.get_connection();
        connection.signal_subscribe("org.freedesktop.DBus",
                                    "org.freedesktop.DBus",
                                    "NameAcquired",
                                    "/org/freedesktop/DBus",
                                    IBus.SERVICE_PANEL,
                                    DBusSignalFlags.NONE,
                                    bus_name_acquired_cb);
        connection.signal_subscribe("org.freedesktop.DBus",
                                    "org.freedesktop.DBus",
                                    "NameLost",
                                    "/org/freedesktop/DBus",
                                    IBus.SERVICE_PANEL,
                                    DBusSignalFlags.NONE,
                                    bus_name_lost_cb);
        var flags =
                IBus.BusNameFlag.ALLOW_REPLACEMENT |
                IBus.BusNameFlag.REPLACE_EXISTING;
        m_bus.request_name(IBus.SERVICE_PANEL, flags);
    }

    public int run() {
        Gtk.main();
        return 0;
    }

    private void bus_name_acquired_cb(DBusConnection connection,
                                      string?        sender_name,
                                      string         object_path,
                                      string         interface_name,
                                      string         signal_name,
                                      Variant        parameters) {
        debug("signal_name = %s", signal_name);
        m_panel = new Panel(m_bus);
        m_panel.load_settings();
    }

    private void bus_name_lost_cb(DBusConnection connection,
                                  string?        sender_name,
                                  string         object_path,
                                  string         interface_name,
                                  string         signal_name,
                                  Variant        parameters) {
        // "Destroy" dbus method was called before this callback is called.
        // "Destroy" dbus method -> ibus_service_destroy()
        // -> g_dbus_connection_unregister_object()
        // -> g_object_unref(m_panel) will be called later with an idle method,
        // which was assigned in the arguments of
        // g_dbus_connection_register_object()
        debug("signal_name = %s", signal_name);

        // unref m_panel
        m_panel.disconnect_signals();
        m_panel = null;
    }

    private void bus_disconnected(IBus.Bus bus) {
        debug("connection is lost.");
        Gtk.main_quit();
    }

    private void bus_connected(IBus.Bus bus) {
        init();
    }

    public static void main(string[] argv) {
        // https://bugzilla.redhat.com/show_bug.cgi?id=1226465#c20
        // In /etc/xdg/plasma-workspace/env/gtk3_scrolling.sh
        // Plasma deskop sets this variable and prevents Super-space,
        // and Ctrl-Shift-e when ibus-ui-gtk3 runs after the
        // desktop is launched.
        GLib.Environment.unset_variable("GDK_CORE_DEVICE_EVENTS");
        // for Gdk.X11.get_default_xdisplay()
        Gdk.set_allowed_backends("x11");

        Application app = new Application(argv);
        app.run();
    }
}
