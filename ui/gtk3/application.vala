/* vim:set et sts=4 sw=4:
 *
 * ibus - The Input Bus
 *
 * Copyright(c) 2011 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright(c) 2017-2023 Takao Fujiwara <takao.fujiwara1@gmail.com>
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

static string prgname;
static IBus.Bus bus;


class Application {
    private Panel m_panel;
#if USE_GDK_WAYLAND
    private static ulong m_realize_surface_id;
    private static bool m_enable_wayland_im;
    private static IBus.WaylandIM m_wayland_im;
#endif

    public Application() {
        GLib.Intl.bindtextdomain(Config.GETTEXT_PACKAGE, Config.LOCALEDIR);
        GLib.Intl.bind_textdomain_codeset(Config.GETTEXT_PACKAGE, "UTF-8");
        IBus.init();

        if (bus == null)
            bus = new IBus.Bus();

        bus.connected.connect(bus_connected);
        bus.disconnected.connect(bus_disconnected);

        if (bus.is_connected()) {
            init();
        }
    }

    private void init() {
        DBusConnection connection = bus.get_connection();
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
        bus.request_name(IBus.SERVICE_PANEL, flags);
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
        m_panel = new Panel(bus);
#if USE_GDK_WAYLAND
        m_realize_surface_id = m_panel.realize_surface.connect(
                (w, s) => this.set_wayland_surface(s));
#endif
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
#if USE_GDK_WAYLAND
        if (m_realize_surface_id != 0) {
            GLib.SignalHandler.disconnect(m_panel, m_realize_surface_id);
            m_realize_surface_id = 0;
        }
#endif
        m_panel = null;
    }

    private void bus_disconnected(IBus.Bus bus) {
        debug("connection is lost.");
        Gtk.main_quit();
    }

    private void bus_connected(IBus.Bus bus) {
        init();
    }

#if USE_GDK_WAYLAND
    private static void make_wayland_im() {
        bus = new IBus.Bus();
        var display = Gdk.Display.get_default();
        var wl_display = ((GdkWayland.Display)display).get_wl_display();
        m_wayland_im = new IBus.WaylandIM("bus", bus, "wl_display", wl_display);
    }

    private void set_wayland_surface(void *surface) {
        m_wayland_im.set_surface(surface);
    }
#endif

    public static void main(string[] argv) {
        // https://bugzilla.redhat.com/show_bug.cgi?id=1226465#c20
        // In /etc/xdg/plasma-workspace/env/gtk3_scrolling.sh
        // Plasma deskop sets this variable and prevents Super-space,
        // and Ctrl-Shift-e when ibus-ui-gtk3 runs after the
        // desktop is launched.
        // GDK_CORE_DEVICE_EVENTS is referred by GdkX11DeviceManager
        // but not GdkWaylandDeviceManager and should be defined
        // before Gdk.Display.get_default() is called.
        GLib.Environment.unset_variable("GDK_CORE_DEVICE_EVENTS");

        OptionEntry entries[]  = {
#if USE_GDK_WAYLAND
            { "enable-wayland-im", 'i', 0, GLib.OptionArg.NONE,
              &m_enable_wayland_im,
              N_("Connect Wayland input method protocol"),
              null },
#endif
            { null }
        };

        prgname = Path.get_basename(argv[0]);
        var parameter_string = "- %s".printf(prgname);
        GLib.OptionContext context = new GLib.OptionContext(parameter_string);
        context.add_main_entries(entries, prgname);
        context.add_group(Gtk.get_option_group (true));
        try {
            context.parse(ref argv);
        } catch (OptionError e) {
            warning(e.message);
        }

#if USE_GDK_WAYLAND
        // Should Make IBusWaylandIM after Gtk.init()
        if (m_enable_wayland_im)
            make_wayland_im();
#endif
        Application app = new Application();
        app.run();
    }
}
