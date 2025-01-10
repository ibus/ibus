/* vim:set et sts=4 sw=4:
 *
 * ibus - The Input Bus
 *
 * Copyright(c) 2011 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright(c) 2017-2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
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

const string IBUS_WAYLAND_VERSION="1.1";
const ulong G_USEC_PER_SEC=1000000L;
const ulong SLEEP_DIV_PER_SEC = 100L;
const ulong MAX_DISPLAY_IDLE_TIME =
        G_USEC_PER_SEC * SLEEP_DIV_PER_SEC * 60 * 3;

static string prgname;
static IBus.Bus bus;


class Application {
    private Panel m_panel;
    private static FileStream m_log;
    private static bool m_verbose;
    private static bool m_enable_wayland_im;
#if USE_GDK_WAYLAND
    private static ulong m_realize_surface_id;
    private static ulong m_ibus_focus_in_id;
    private static ulong m_ibus_focus_out_id;
    private static string m_user;
    private static IBus.WaylandIM m_wayland_im;
    private static bool m_exec_daemon;
    private static string m_daemon_args;
#endif

    public Application() {
        GLib.Intl.bindtextdomain(Config.GETTEXT_PACKAGE, Config.LOCALEDIR);
        GLib.Intl.bind_textdomain_codeset(Config.GETTEXT_PACKAGE, "UTF-8");

        if (bus == null)
            bus = new IBus.Bus();

        bus.connected.connect(bus_connected);
        bus.disconnected.connect(bus_disconnected);
        bus.set_watch_ibus_signal(true);
        bus.global_shortcut_key_responded.connect(bus_global_shortcut_key_cb);

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
        m_panel = new Panel(bus, m_enable_wayland_im);
        if (m_log != null)
            m_panel.set_log(m_log, m_verbose);
#if USE_GDK_WAYLAND
        m_realize_surface_id = m_panel.realize_surface.connect(
                (w, s) => this.set_wayland_surface(s));
        m_ibus_focus_in_id = m_wayland_im.ibus_focus_in.connect(
                (w, o) => m_panel.set_wayland_object_path(o));
        m_ibus_focus_out_id = m_wayland_im.ibus_focus_out.connect(
                (w, o) => m_panel.set_wayland_object_path(null));
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
        if (m_ibus_focus_in_id != 0) {
            GLib.SignalHandler.disconnect(m_wayland_im, m_ibus_focus_in_id);
            m_ibus_focus_in_id = 0;
        }
        if (m_ibus_focus_out_id != 0) {
            GLib.SignalHandler.disconnect(m_wayland_im, m_ibus_focus_out_id);
            m_ibus_focus_out_id = 0;
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

    private void bus_global_shortcut_key_cb(IBus.Bus bus,
                                            uint8    type,
                                            uint     keyval,
                                            uint     keycode,
                                            uint     state,
                                            bool     is_backward) {
        if (m_panel == null)
            return;
        if (m_verbose) {
            m_log.printf("Global shortcut key %u keyval %X keycode %u " +
                         "state %X pressed %s backward %s\n",
                         type,
                         keyval, keycode, state,
                         (state & IBus.ModifierType.RELEASE_MASK) != 0
                                 ? "FALSE" : "TRUE",
                         is_backward ? "TRUE" : "FALSE");
            m_log.flush();
        }
        IBus.BusGlobalBindingType gtype = (IBus.BusGlobalBindingType)type;
        m_panel.set_global_shortcut_key_state(gtype,
                                              keyval,
                                              keycode,
                                              state,
                                              is_backward);
    }

#if USE_GDK_WAYLAND
    private static bool open_log() {
        var directory =
                Path.build_filename(GLib.Environment.get_user_cache_dir(),
                                    "ibus");
        return_val_if_fail(directory != null, false);
        Posix.errno = 0;
        if (GLib.DirUtils.create_with_parents(directory, 0700) != 0) {
            warning("mkdir is failed in %s: %s",
                    directory, Posix.strerror(Posix.errno));
            return false;
        }
        var path =
                Path.build_filename(directory, "wayland.log");
        m_log = FileStream.open(path, "w");
        unowned Posix.Passwd? pw = Posix.getpwuid(Posix.getuid());
        m_user = pw.pw_name.substring(0, 6);
        if (m_user == null)
            m_user = GLib.Environment.get_variable("USER").substring(0, 6);
        if (m_user == null)
            m_user = "UNKNOW";
        GLib.DateTime now = new GLib.DateTime.now_local();
        var msec = now.get_microsecond() / 1000;
        m_log.printf("Start %02d:%02d:%02d:%06d\n",
                     now.get_hour(), now.get_minute(), now.get_second(), msec);
        m_log.flush();
        return true;
    }

    private static void check_ps() {
        string standard_output = null;
        string standard_error = null;
        int wait_status = 0;
        try {
            GLib.Process.spawn_command_line_sync ("ps -ef",
                                                  out standard_output,
                                                  out standard_error,
                                                  out wait_status);
        } catch (GLib.SpawnError e) {
            m_log.printf("Failed ps %s: %s\n", e.message, standard_error);
            m_log.flush();
            return;
        }
        var lines = standard_output.split("\n", -1);
        m_log.printf("ps -ef\n");
        foreach (var line in lines) {
            if (line.index_of(m_user) >= 0 && line.index_of("wayland") >= 0)
                m_log.printf("  %s\n", line);
        }
        m_log.flush();
    }

    private static bool run_ibus_daemon() {
        string[] args = { "ibus-daemon" };
        foreach (var arg in m_daemon_args.split(" "))
            args += arg;
        GLib.Pid child_pid = 0;
        try {
            GLib.Process.spawn_async (null, args, null,
                                      GLib.SpawnFlags.DO_NOT_REAP_CHILD
                                      | GLib.SpawnFlags.SEARCH_PATH,
                                      null,
                                      out child_pid);
        } catch (GLib.SpawnError e) {
            m_log.printf("ibus-daemon error: %s\n", e.message);
            warning("%s\n", e.message);
            return false;
        }
        return true;
    }

    private static void make_wayland_im() {
        if (BindingCommon.default_is_xdisplay())
            return;
        assert (open_log());
        void *wl_display = null;
        ulong i = 0;
        while (true) {
            var display = Gdk.Display.get_default();
            if (display != null)
                wl_display = ((GdkWayland.Display)display).get_wl_display();
            if (wl_display != null)
                break;
            if (i == MAX_DISPLAY_IDLE_TIME)
                break;
            Thread.usleep(G_USEC_PER_SEC / SLEEP_DIV_PER_SEC);
            if (m_verbose) {
                m_log.printf("Spend %lu/%lu secs\n", i, SLEEP_DIV_PER_SEC);
                m_log.flush();
            }
            ++i;
        }
        int _errno = Posix.errno;
        if (m_verbose)
            check_ps();
        if (wl_display == null) {
            m_log.printf("Failed to connect to Wayland server: %s\n",
                         Posix.strerror(_errno));
            m_log.flush();
            assert_not_reached();
        }
        bus = new IBus.Bus();
        GLib.MainLoop? loop = null;
        if (!bus.is_connected() && m_exec_daemon) {
            ulong handler_id = bus.connected.connect((bus) => {
                if (loop != null)
                    loop.quit();
            });
            if (run_ibus_daemon()) {
                if (!bus.is_connected()) {
                    loop = new GLib.MainLoop();
                    loop.run();
                }
            }
            bus.disconnect(handler_id);
        }
        if (!bus.is_connected()) {
            m_log.printf("Failed to connect to ibus-daemon\n");
            m_log.flush();
            assert_not_reached();
        }
        m_wayland_im = new IBus.WaylandIM("bus", bus,
                                          "wl_display", wl_display,
                                          "log", m_log,
                                          "verbose", m_verbose);
    }

    private void set_wayland_surface(void *surface) {
        m_wayland_im.set_surface(surface);
    }
#endif

    public static void show_version() {
        print("%s %s Wayland %s\n", prgname,
                                    Config.PACKAGE_VERSION,
                                    IBUS_WAYLAND_VERSION);
    }

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
            { "version", 'V', GLib.OptionFlags.NO_ARG, GLib.OptionArg.CALLBACK,
              (void *)show_version,
              N_("Show version"),
              null },
#if USE_GDK_WAYLAND
            { "enable-wayland-im", 'i', 0, GLib.OptionArg.NONE,
              &m_enable_wayland_im,
              N_("Connect Wayland input method protocol"),
              null },
            { "exec-daemon", 'd', 0, GLib.OptionArg.NONE,
              &m_exec_daemon,
              N_("Execute ibus-daemon if it's not running"),
              null },
            { "daemon-args", 'g', 0, GLib.OptionArg.STRING,
              &m_daemon_args,
              N_("ibus-daemon's arguments"),
              null },
            { "verbose", 'v', 0, GLib.OptionArg.NONE,
              &m_verbose,
              N_("Verbose logging"),
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

        IBus.init();
#if USE_GDK_WAYLAND
        if (m_daemon_args == null)
            m_daemon_args = "--xim";
        // Should Make IBusWaylandIM after Gtk.init()
        if (m_enable_wayland_im)
            make_wayland_im();
#endif
        Application app = new Application();
        app.run();
    }
}
