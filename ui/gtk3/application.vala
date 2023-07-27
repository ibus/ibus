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

static string program_name;
static IBus.Bus bus;
#if USE_GDK_WAYLAND
static IBus.WaylandIM wayland_im;
#endif


delegate void EntryFunc(string[] argv);

struct CommandEntry {
    unowned string name;
    unowned string description;
    unowned EntryFunc entry;
}
 
 
const CommandEntry commands[]  = {
#if USE_GDK_WAYLAND
    { "--enable-wayland-im", N_("Connect Wayland input method protocol"),
      make_wayland_im },
#endif
    { "--help", N_("Show this information"), print_help }
};


void print_help(string[] argv) {
    print_usage(stdout);
    Posix.exit(Posix.EXIT_SUCCESS);
}

void print_usage(FileStream stream) {
    stream.printf(_("Usage: %s COMMAND [OPTION...]\n\n"), program_name);
    stream.printf(_("Commands:\n"));
    for (int i = 0; i < commands.length; i++) {
        stream.printf("  %-12s    %s\n",
                      commands[i].name,
                      GLib.dgettext(null, commands[i].description));
    }
}


#if USE_GDK_WAYLAND
void make_wayland_im(string[] argv) {
    bus = new IBus.Bus();
    wayland_im = new IBus.WaylandIM(bus);
}
#endif


class Application {
    private Panel m_panel;

    public Application(string[] argv) {
        GLib.Intl.bindtextdomain(Config.GETTEXT_PACKAGE, Config.LOCALEDIR);
        GLib.Intl.bind_textdomain_codeset(Config.GETTEXT_PACKAGE, "UTF-8");
        IBus.init();
        Gtk.init(ref argv);

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
        //Gdk.set_allowed_backends("x11");

        program_name = Path.get_basename(argv[0]);
        string[] new_argv = {};
        foreach (var arg in argv) {
            int i = 0;
            for (i = 0; i < commands.length; i++) {
                if (commands[i].name == arg) {
                    commands[i].entry(argv);
                    break;
                }
            }
            if (i == commands.length)
                new_argv += arg;
        }
        Application app = new Application(new_argv);
        app.run();
    }
}
