/* vim:set et sts=4 sw=4:
 *
 * ibus - The Input Bus
 *
 * Copyright(c) 2013 Peng Huang <shawn.p.huang@gmail.com>
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

private const string IBUS_SCHEMAS_GENERAL = "org.freedesktop.ibus.general";
private const string IBUS_SCHEMAS_GENERAL_HOTKEY =
        "org.freedesktop.ibus.general.hotkey";
private const string IBUS_SCHEMAS_PANEL = "org.freedesktop.ibus.panel";
private const string IBUS_SCHEMAS_PANEL_EMOJI =
        "org.freedesktop.ibus.panel.emoji";
private const string SYSTEMD_SESSION_GNOME_FILE =
        "org.freedesktop.IBus.session.GNOME.service";

#if IBUS_WAYLAND
bool is_wayland_session = false;
#endif
bool name_only = false;
/* system() exists as a public API. */
bool is_system = false;
string cache_file = null;
string engine_id = null;
bool verbose = false;
string daemon_type = null;
string systemd_service_file = null;
GLib.MainLoop loop = null;


class EngineList {
    public IBus.EngineDesc[] data = {};
}


private bool is_kde(bool verbose) {
    unowned string? desktop = Environment.get_variable("XDG_CURRENT_DESKTOP");
    if (desktop == "KDE")
        return true;
    if (desktop == null || desktop == "(null)")
        desktop = Environment.get_variable("XDG_SESSION_DESKTOP");
    if (desktop == "plasma" || desktop == "KDE-wayland")
        return true;
    if (desktop == null) {
        if (verbose) {
            stderr.printf("XDG_CURRENT_DESKTOP is not exported in your " +
                          "desktop session.\n");
        }
    } else if (verbose) {
        stderr.printf("Your desktop session \"%s\" is not KDE\n.", desktop);
    }
    return false;
}


IBus.Bus? get_bus() {
    var bus = new IBus.Bus();
    if (!bus.is_connected ())
        return null;
    return bus;
}


private void
name_appeared_handler(GLib.DBusConnection connection,
                      string name,
                      string name_owner) {
    if (verbose)
        stderr.printf("NameAquired %s:%s\n", name, name_owner);
    loop.quit();
    loop = null;
}

private void
kde_virtual_keyboard_avail_cb(GLib.DBusConnection connection,
                              string?             sender_name,
                              string              object_path,
                              string              interface_name,
                              string              signal_name,
                              GLib.Variant        parameters)
{
    if (verbose) {
        stderr.printf("%s.%s %s returned.\n",
                      interface_name, signal_name, object_path);
    }
    loop.quit();
    loop = null;
}


GLib.DBusConnection? get_session_bus(bool verbose) {
    loop = new GLib.MainLoop();
    assert(loop != null);
    GLib.Bus.watch_name (GLib.BusType.SESSION,
                         DBus.SERVICE_DBUS,
                         GLib.BusNameWatcherFlags.NONE,
                         name_appeared_handler,
                         null);
    GLib.DBusConnection? connection = null;
    GLib.Bus.get.begin(GLib.BusType.SESSION, null,
                           (obj, res) => {
        try {
            connection = GLib.Bus.get.end(res);
            if (verbose)
                stderr.printf("The session bus is generated.\n");
        } catch(GLib.IOError e) {
            if (verbose)
                stderr.printf("The session bus error: %s\n", e.message);
            if (loop != null) {
                loop.quit();
                loop = null;
            }
        }
    });
    loop.run();
    if (connection.is_closed()) {
        if (verbose)
            stderr.printf("The session bus is closed.\n");
        return null;
    }
    return connection;
}


string?
get_ibus_systemd_object_path(GLib.DBusConnection connection,
                             bool                verbose) {
    string object_path = null;
    assert(systemd_service_file != null);
    try {
        var variant = connection.call_sync (
                "org.freedesktop.systemd1",
                "/org/freedesktop/systemd1",
                "org.freedesktop.systemd1.Manager",
                "GetUnit",
                new GLib.Variant("(s)", systemd_service_file),
                new GLib.VariantType("(o)"),
                GLib.DBusCallFlags.NONE,
                -1,
                null);
        variant.get("(o)", ref object_path);
        if (verbose) {
            stderr.printf("Succeed to get an object path \"%s\" for IBus " +
                          "systemd service file \"%s\".\n",
                          object_path, systemd_service_file);
        }
        return object_path;
    } catch (GLib.Error e) {
        if (verbose) {
            stderr.printf("IBus systemd service file \"%s\" is not installed " +
                          "in your system: %s\n",
                          systemd_service_file, e.message);
        }
    }
    return null;
}


bool
is_running_daemon_via_systemd(GLib.DBusConnection connection,
                              string              object_path,
                              bool                verbose) {
    string? state = null;
    try {
        while (true) {
            var variant = connection.call_sync (
                    "org.freedesktop.systemd1",
                    object_path,
                    "org.freedesktop.DBus.Properties",
                    "Get",
                    new GLib.Variant("(ss)",
                                     "org.freedesktop.systemd1.Unit",
                                     "ActiveState"),
                    new GLib.VariantType("(v)"),
                    GLib.DBusCallFlags.NONE,
                    -1,
                    null);
            GLib.Variant child = null;
            variant.get("(v)", ref child);
            state = child.dup_string();
            if (verbose) {
                stderr.printf("systemd state is \"%s\" for an object " +
                              "path \"%s\".\n", state, object_path);
            }
            if (state != "activating")
                break;
            Posix.sleep(1);
        }
    } catch (GLib.Error e) {
        if (verbose)
            stderr.printf("%s\n", e.message);
        return false;
    }
    if (state == "active")
        return true;
    return false;
}


#if IBUS_WAYLAND
void registry_global_cb(void       *data,
                        Wl.Registry wl_registry,
                        uint32      name,
                        string     @interface,
                        uint32      version) {
    if (verbose) {
        stderr.printf("Wl.RegistryListenerGlobal name: %3u interface: %s " +
                      "version: %u.\n", name, interface, version);
    }
    if (interface == "zwp_input_method_manager_v2") {
        is_wayland_session = true;
    } else if (interface == "zwp_input_method_v1") {
        stderr.printf("Wayland input-method protocol version 1 does not " +
                      "work with the forked application.\n");
        is_wayland_session = true;
    }
}


void registry_global_remove_cb(void       *data,
                               Wl.Registry wl_registry,
                               uint32      name) {
}


bool check_wayland_protocols() {
    var display = new Wl.Display.connect(null);
    if (display == null)
        return false;
    var registry = display.get_registry();
    var listener = Wl.RegistryListener() {
        global = registry_global_cb,
        global_remove = registry_global_remove_cb
    };
    registry.add_listener (listener, null);
    display.roundtrip();
    return true;
}


bool start_daemon_in_wayland(bool     restart,
                             string[] _argv) {

    check_wayland_protocols();
    if (!is_wayland_session) {
        stderr.printf("Cannot find the Wayland input-method protocol.\n");
        return false;
    }

    var bus = get_bus();
    if (restart) {
        if (bus == null) {
            stderr.printf(_("Can't connect to IBus.\n"));
            return false;
        }
        bus.exit(false);
        if (verbose) {
            stderr.printf("Succeed to exit ibus-daemon with an IBus API " +
                          "directly.\n");
        }
        bus = null;
    } else if (bus != null) {
        stderr.printf("%s\n".printf(_("IBus is running.")));
        Posix.exit(Posix.EXIT_FAILURE);
        return true;
    }

    string cmd = Config.LIBEXECDIR + "/ibus-ui-gtk3";
    var file = File.new_for_path(cmd);
    if (!file.query_exists())
        cmd = "../ui/gtk3/ibus-ui-gtk3";

    string daemon_args = "--xim --panel disable";
    if (_argv.length > 1)
        daemon_args = string.joinv(" ", _argv[1:]);
    string[] argv = { cmd };
    argv += "--enable-wayland-im";
    argv += "--exec-daemon";
    argv += "--daemon-args";
    argv += "%s".printf(daemon_args);
    if (verbose)
        argv += "--verbose";

    string[] env = Environ.get();

    try {
        // Non-blocking
        Process.spawn_async(null, argv, env,
                            SpawnFlags.SEARCH_PATH,
                            null, null);
    } catch (SpawnError e) {
        stderr.printf("%s\n", e.message);
        Posix.exit(Posix.EXIT_FAILURE);
        return true;
    }
    if (verbose) {
        stderr.printf("Succeed to %s ibus-daemon with a Wayland " +
                      "input-method version 2.\n",
                      restart ? "restart" : "start");
    }
    return true;
}
#endif

bool
start_daemon_with_dbus_systemd(GLib.DBusConnection connection,
                               bool                restart,
                               bool                verbose) {
    string object_path = null;
    string method = "StartUnit";
    assert(systemd_service_file != null);
    if (restart)
        method = "RestartUnit";
    try {
        var variant = connection.call_sync (
                "org.freedesktop.systemd1",
                "/org/freedesktop/systemd1",
                "org.freedesktop.systemd1.Manager",
                method,
                new GLib.Variant("(ss)", systemd_service_file, "fail"),
                new GLib.VariantType("(o)"),
                GLib.DBusCallFlags.NONE,
                -1,
                null);
        variant.get("(o)", ref object_path);
        if (verbose) {
            stderr.printf("Succeed to restart IBus daemon via IBus systemd " +
                          "service file \"%s\": \"%s\"\n",
                          systemd_service_file, object_path);
        }
        return true;
    } catch (GLib.Error e) {
        if (verbose) {
            stderr.printf("Failed to %s IBus daemon via IBus systemd " +
                          "service file \"%s\": %s\n",
                          restart ? "restart" : "start",
                          systemd_service_file, e.message);
        }
    }
    return false;
}


bool
start_daemon_with_dbus_kde(GLib.DBusConnection connection,
                           bool                verbose) {
    loop = new GLib.MainLoop();
    assert(loop != null);
    uint subscripion_id = connection.signal_subscribe(
            null,
            "org.kde.kwin.VirtualKeyboard",
            "availableChanged",
            "/VirtualKeyboard",
            null,
            DBusSignalFlags.NONE,
            kde_virtual_keyboard_avail_cb);

    string wayland_values = "InputMethod";
    var bytes = new GLib.VariantBuilder(new GLib.VariantType("ay"));
    for (int i = 0; i < wayland_values.length; i++) {
        bytes.add("y", wayland_values.get(i));
    }
    var bytes2 = new GLib.VariantBuilder(new GLib.VariantType("aay"));
    bytes2.add("ay", bytes);
    var array = new GLib.VariantBuilder(new GLib.VariantType("a{saay}"));
    array.add("{saay}", "Wayland", bytes2);
    try {
        if (!connection.emit_signal(null,
                                    "/kwinrc",
                                    "org.kde.kconfig.notify",
                                    "ConfigChanged",
                                    new GLib.Variant("(a{saay})", array))) {
            if (verbose)
                stderr.printf("Failed to emit a KDE D-Bus signal.\n");
            connection.signal_unsubscribe(subscripion_id);
            if (loop != null) {
                loop.quit();
                loop = null;
            }
            return false;
        }
    } catch (GLib.Error e) {
        stderr.printf("%s\n", e.message);
        connection.signal_unsubscribe(subscripion_id);
        if (loop != null) {
            loop.quit();
            loop = null;
        }
        return false;
    }
    // Wait for "/VirtualKeyboard" on "org.kde.kwin.VirtualKeyboard" signal
    // so that "/kwinrc" on "org.kde.kconfig.notify" signal is reached to
    // kwin.
    loop.run();
    connection.signal_unsubscribe(subscripion_id);
    return true;
}


int list_engine(string[] argv) {
    const OptionEntry[] options = {
        { "name-only", 0, 0, OptionArg.NONE, out name_only,
          N_("List engine name only"), null },
        { null }
    };

    var option = new OptionContext();
    option.add_main_entries(options, Config.GETTEXT_PACKAGE);

    try {
        option.parse(ref argv);
    } catch (OptionError e) {
        stderr.printf("%s\n", e.message);
        return Posix.EXIT_FAILURE;
    }

    var bus = get_bus();
    if (bus == null) {
        stderr.printf(_("Can't connect to IBus.\n"));
        return Posix.EXIT_FAILURE;
    }

    var engines = bus.list_engines();

    if (name_only) {
        foreach (var engine in engines) {
            print("%s\n", engine.get_name());
        }
        return Posix.EXIT_SUCCESS;
    }

    var map = new HashTable<string, EngineList>(GLib.str_hash, GLib.str_equal);

    foreach (var engine in engines) {
        var list = map.get(engine.get_language());
        if (list == null) {
            list = new EngineList();
            map.insert(engine.get_language(), list);
        }
        list.data += engine;
    }

    foreach (var language in map.get_keys()) {
        var list = map.get(language);
        print(_("language: %s\n"), IBus.get_language_name(language));
        foreach (var engine in list.data) {
            print("  %s - %s\n", engine.get_name(), engine.get_longname());
        }
    }

    return Posix.EXIT_SUCCESS;
}


private int exec_setxkbmap(IBus.EngineDesc engine) {
    string layout = engine.get_layout();
    string variant = engine.get_layout_variant();
    string option = engine.get_layout_option();
    string standard_error = null;
    int exit_status = 0;
    string[] args = { "setxkbmap" };

    if (layout != null && layout != "" && layout != "default") {
        args += "-layout";
        args += layout;
    }
    if (variant != null && variant != "" && variant != "default") {
        args += "-variant";
        args += variant;
    }
    if (option != null && option != "" && option != "default") {
        /*TODO: Need to get the session XKB options */
        args += "-option";
        args += "-option";
        args += option;
    }

    if (args.length == 1) {
        return Posix.EXIT_FAILURE;
    }

    try {
        if (!GLib.Process.spawn_sync(null, args, null,
                                     GLib.SpawnFlags.SEARCH_PATH,
                                     null, null,
                                     out standard_error,
                                     out exit_status)) {
            warning("Switch xkb layout to %s failed.",
                    engine.get_layout());
            return Posix.EXIT_FAILURE;
        }
    } catch (GLib.SpawnError e) {
        warning("Execute setxkbmap failed: %s", e.message);
        return Posix.EXIT_FAILURE;
    }

    if (exit_status != 0) {
        warning("Execute setxkbmap failed: %s", standard_error ?? "(null)");
        return Posix.EXIT_FAILURE;
    }

    return Posix.EXIT_SUCCESS;
}


int get_set_engine(string[] argv) {
    var bus = get_bus();
    string engine = null;
    if (argv.length > 1)
        engine = argv[1];

    if (engine == null) {
        var desc = bus.get_global_engine();
        if (desc == null) {
            stderr.printf(_("No engine is set.\n"));
            return Posix.EXIT_FAILURE;
        }
        print("%s\n", desc.get_name());
        return Posix.EXIT_SUCCESS;
    }

    if(!bus.set_global_engine(engine)) {
        stderr.printf(_("Set global engine failed.\n"));
        return Posix.EXIT_FAILURE;
    }
    var desc = bus.get_global_engine();
    if (desc == null) {
        stderr.printf(_("Get global engine failed.\n"));
        return Posix.EXIT_FAILURE;
    }

    var settings = new GLib.Settings(IBUS_SCHEMAS_GENERAL);
    if (!settings.get_boolean("use-system-keyboard-layout"))
        return exec_setxkbmap(desc);

    return Posix.EXIT_SUCCESS;
}


int message_watch(string[] argv) {
    return Posix.EXIT_SUCCESS;
}


bool operate_daemon_in_kde_wayland(GLib.DBusConnection connection,
                                   bool                start,
                                   bool                verbose) {
    if (!is_kde(verbose))
        return false;
    if (Environment.get_variable("WAYLAND_DISPLAY") == null) {
        if (verbose)
            stderr.printf("Your KDE session is not Wayland\n.");
        return false;
    }
    var kwinrc = GLib.Path.build_filename(
        GLib.Environment.get_user_config_dir(), "kwinrc");
    var key_file = new KRcFile();
    try {
        key_file.load_from_file(kwinrc, GLib.KeyFileFlags.KEEP_COMMENTS);
    } catch (GLib.KeyFileError e) {
        stderr.printf("Error in %s: %s\n", kwinrc, e.message);
        return false;
    } catch (GLib.FileError e) {
        stderr.printf("Error in %s: %s\n", kwinrc, e.message);
        return false;
    }
    try {
        bool is_updated = false;
        if (start) {
            string ibus_value =
                    GLib.Path.build_filename(Config.DATADIR,
                                             "applications",
                                             Config.UI_WAYLAND_DESKTOP);
            string? value = null;
            if (key_file.has_group("Wayland") &&
                key_file.has_key("Wayland", "InputMethod[$e]")) {
                value = key_file.get_value("Wayland", "InputMethod[$e]");
            }
            if (value != ibus_value) {
                key_file.set_value("Wayland", "InputMethod[$e]", ibus_value);
                key_file.save_to_file(kwinrc);
                is_updated = true;
            }
        } else {
            if (key_file.has_group("Wayland")) {
                var keys = key_file.get_keys("Wayland");
                if (key_file.has_key("Wayland", "InputMethod[$e]")) {
                    if (keys.length == 1)
                        key_file.remove_group("Wayland");
                    else
                        key_file.remove_key("Wayland", "InputMethod[$e]");
                    key_file.save_to_file(kwinrc);
                    is_updated = true;
                }
            }
        }
        if (verbose) {
            if (is_updated) {
                stderr.printf("\"InputMethod\" was %s in kwinrc for %s.\n",
                              start ? "added" : "deleted",
                              start ? "start" : "exit");
            } else {
                if (start) {
                    stderr.printf(
                            "kwinrc is not updated for start. \"ibus exit\" " +
                            "command may be needed in case kwin_wayland " +
                            "restarted.\n");
                } else {
                    stderr.printf("kwinrc is not updated for exit.\n");
                    return true;
                }
            }
        }
    } catch (GLib.KeyFileError e) {
        stderr.printf("%s\n", e.message);
        return false;
    } catch (GLib.FileError e) {
        stderr.printf("%s\n", e.message);
        return false;
    }
    if (!start_daemon_with_dbus_kde(connection, verbose))
        return false;
    if (verbose) {
        stderr.printf("Succeed to %s ibus-daemon with a " +
                      "KDE Wayland method.\n", start ? "start" : "exit");
    }
    return true;
}

bool start_daemon_with_systemd(bool restart, bool verbose) {
    GLib.DBusConnection? connection = get_session_bus(verbose);
    if (connection == null)
        return false;
    string? object_path = null;
    if (restart) {
        object_path = get_ibus_systemd_object_path(connection, verbose);
        if (object_path == null)
            return false;
        if (!is_running_daemon_via_systemd(connection,
                                           object_path,
                                           verbose)) {
            return false;
        }
    }
    if (!start_daemon_with_dbus_systemd(connection, restart, verbose))
        return false;
    // Do not check the systemd state in case of restart because
    // the systemd file validation is already done and also stopping
    // daemon and starting daemon take time and the state could be
    // "inactive" with the time lag.
    if (restart)
        return true;
    object_path = get_ibus_systemd_object_path(connection, verbose);
    if (object_path == null)
        return false;
    if (!is_running_daemon_via_systemd(connection, object_path, verbose))
        return false;
    return true;
}


int start_daemon_real(string[] argv,
                      bool     restart) {
    const OptionEntry[] options = {
        { "type", 0, 0, OptionArg.STRING, out daemon_type,
          N_("Start or restart daemon with \"direct\", \"systemd\", " +
             "\"kde-wayland\" or \"wayland\" TYPE."),
          "TYPE" },
        { "service-file", 0, 0, OptionArg.STRING, out systemd_service_file,
          N_("Start or restart daemon with SYSTEMD_SERVICE file."),
          "SYSTEMD_SERVICE" },
        { "verbose", 0, 0, OptionArg.NONE, out verbose,
          N_("Show debug messages."), null },
        { null }
    };

    var option = new OptionContext();
    option.add_main_entries(options, Config.GETTEXT_PACKAGE);
    option.set_ignore_unknown_options(true);

    try {
        option.parse(ref argv);
    } catch (OptionError e) {
        stderr.printf("%s\n", e.message);
        return Posix.EXIT_FAILURE;
    }
    if (daemon_type != null && daemon_type != "direct" &&
        daemon_type != "systemd" && daemon_type != "kde-wayland" &&
        daemon_type != "wayland") {
        stderr.printf("type argument must be \"direct\" or \"systemd\" " +
                      "or \"kde-wayland\" or \"wayland\"\n");
        return Posix.EXIT_FAILURE;
    }
    if (systemd_service_file == null)
        systemd_service_file = SYSTEMD_SESSION_GNOME_FILE;

    if (daemon_type == null || daemon_type == "wayland") {
#if IBUS_WAYLAND
        if (start_daemon_in_wayland(restart, argv))
            return Posix.EXIT_SUCCESS;
#else
        if (daemon_type == "wayland") {
            stderr.printf("Please build IBus with Wayland.\n");
            return Posix.EXIT_FAILURE;
        }
#endif
    }

    if (daemon_type == null || daemon_type == "kde-wayland") {
        do {
            GLib.DBusConnection? connection = get_session_bus(verbose);
            if (connection == null)
                break;
            if (restart) {
                if (operate_daemon_in_kde_wayland(connection, false, verbose))
                    Posix.sleep(3);
                else
                    break;
            }
            if (operate_daemon_in_kde_wayland(connection, true, verbose))
                return Posix.EXIT_SUCCESS;
        } while (false);
    }

    if (daemon_type == null || daemon_type == "systemd") {
        if (start_daemon_with_systemd(restart, verbose))
            return Posix.EXIT_SUCCESS;
    }

    if (daemon_type == "systemd" || daemon_type == "kde-wayland")
        return Posix.EXIT_FAILURE;
    var bus = get_bus();
    if (restart) {
        if (bus == null) {
            stderr.printf(_("Can't connect to IBus.\n"));
            return Posix.EXIT_FAILURE;
        }
        bus.exit(true);
        if (verbose) {
            stderr.printf("Succeed to restart ibus-daemon with an IBus API " +
                          "directly.\n");
        }
    } else if (bus != null) {
        stderr.printf("%s\n".printf(_("IBus is running.")));
        return Posix.EXIT_FAILURE;
    } else {
        string startarg = "ibus-daemon";
        argv[0] = startarg;
        var paths = GLib.Environment.get_variable("PATH").split(":");
        foreach (unowned string path in paths) {
            var full_path = "%s/%s".printf(path, startarg);
            if (GLib.FileUtils.test(full_path, GLib.FileTest.IS_EXECUTABLE)) {
                startarg = full_path;
                break;
            }
        }
        // When ibus-daemon is launched by GLib.Process.spawn_async(),
        // the parent process will be systemd
        if (verbose) {
            stderr.printf("Running \"%s\" directly as a foreground " +
                          "process.\n", startarg);
        }
        Posix.execv(startarg, argv);
    }
    return Posix.EXIT_SUCCESS;
}


int restart_daemon(string[] argv) {
    return start_daemon_real(argv, true);
}

int start_daemon(string[] argv) {
    return start_daemon_real(argv, false);
}

int exit_daemon(string[] argv) {
    const OptionEntry[] options = {
        { "type", 0, 0, OptionArg.STRING, out daemon_type,
          N_("Exit daemon with \"direct\" or \"kde-wayland\" " +
             "TYPE."),
          "TYPE" },
        { "verbose", 0, 0, OptionArg.NONE, out verbose,
          N_("Show debug messages."), null },
        { null }
    };

    var option = new OptionContext();
    option.add_main_entries(options, Config.GETTEXT_PACKAGE);
    option.set_ignore_unknown_options(true);

    try {
        option.parse(ref argv);
    } catch (OptionError e) {
        stderr.printf("%s\n", e.message);
        return Posix.EXIT_FAILURE;
    }
    if (daemon_type != null && daemon_type != "direct" &&
        daemon_type != "kde-wayland") {
        stderr.printf("type argument must be \"direct\" or \"kde-wayland\"\n");
        return Posix.EXIT_FAILURE;
    }
    if (daemon_type == null || daemon_type == "kde-wayland") {
        GLib.DBusConnection? connection = get_session_bus(verbose);
        if (connection != null &&
            operate_daemon_in_kde_wayland(connection, false, verbose)) {
            return Posix.EXIT_SUCCESS;
        }
    }

    if (daemon_type == "kde-wayland")
        return Posix.EXIT_FAILURE;
    var bus = get_bus();
    if (bus == null) {
        stderr.printf(_("Can't connect to IBus.\n"));
        return Posix.EXIT_FAILURE;
    }
    bus.exit(false);
    return Posix.EXIT_SUCCESS;
}


int print_version(string[] argv) {
    print("IBus %s\n", Config.PACKAGE_VERSION);
    return Posix.EXIT_SUCCESS;
}


int read_cache (string[] argv) {
    const OptionEntry[] options = {
        { "system", 0, 0, OptionArg.NONE, out is_system,
          N_("Read the system registry cache."), null },
        { "file", 0, 0, OptionArg.STRING, out cache_file,
          N_("Read the registry cache FILE."), "FILE" },
        { null }
    };

    var option = new OptionContext();
    option.add_main_entries(options, Config.GETTEXT_PACKAGE);

    try {
        option.parse(ref argv);
    } catch (OptionError e) {
        stderr.printf("%s\n", e.message);
        return Posix.EXIT_FAILURE;
    }

    var registry = new IBus.Registry();

    if (cache_file != null) {
        if (!registry.load_cache_file(cache_file)) {
            stderr.printf(_("The registry cache is invalid.\n"));
            return Posix.EXIT_FAILURE;
        }
    } else {
        if (!registry.load_cache(!is_system)) {
            stderr.printf(_("The registry cache is invalid.\n"));
            return Posix.EXIT_FAILURE;
        }
    }

    var output = new GLib.StringBuilder();
    registry.output(output, 1);

    print ("%s\n", output.str);
    return Posix.EXIT_SUCCESS;
}


int write_cache (string[] argv) {
    const OptionEntry[] options = {
        { "system", 0, 0, OptionArg.NONE, out is_system,
          N_("Write the system registry cache."), null },
        { "file", 0, 0, OptionArg.STRING, out cache_file,
          N_("Write the registry cache FILE."),
          "FILE" },
        { null }
    };

    var option = new OptionContext();
    option.add_main_entries(options, Config.GETTEXT_PACKAGE);

    try {
        option.parse(ref argv);
    } catch (OptionError e) {
        stderr.printf("%s\n", e.message);
        return Posix.EXIT_FAILURE;
    }

    var registry = new IBus.Registry();
    registry.load();

    if (cache_file != null) {
        return registry.save_cache_file(cache_file) ?
                Posix.EXIT_SUCCESS : Posix.EXIT_FAILURE;
    }

    return registry.save_cache(!is_system) ?
            Posix.EXIT_SUCCESS : Posix.EXIT_FAILURE;
}


int print_address(string[] argv) {
    string address = IBus.get_address();
    print("%s\n", address != null ? address : "(null)");
    return Posix.EXIT_SUCCESS;
}


private int read_config_options(string[] argv) {
    const OptionEntry[] options = {
        { "engine-id", 0, 0, OptionArg.STRING, out engine_id,
          N_("Use engine schema paths instead of ibus core, " +
             "which can be comma-separated values."), "ENGINE_ID" },
        { null }
    };

    var option = new OptionContext();
    option.add_main_entries(options, Config.GETTEXT_PACKAGE);

    try {
        option.parse(ref argv);
    } catch (OptionError e) {
        stderr.printf("%s\n", e.message);
        return Posix.EXIT_FAILURE;
    }
    return Posix.EXIT_SUCCESS;
}


private GLib.SList<string> get_ibus_schemas() {
    string[] ids = {};
    if (engine_id != null) {
        ids = engine_id.split(",");
    }
    GLib.SList<string> ibus_schemas = new GLib.SList<string>();
    GLib.SettingsSchemaSource schema_source =
            GLib.SettingsSchemaSource.get_default();
    string[] list_schemas = {};
    schema_source.list_schemas(true, out list_schemas, null);
    foreach (string schema in list_schemas) {
        if (ids.length != 0) {
            foreach (unowned string id in ids) {
                if (id == schema ||
                    schema.has_prefix("org.freedesktop.ibus.engine." + id)) {
                    ibus_schemas.prepend(schema);
                    break;
                }
            }
        } else if (schema.has_prefix("org.freedesktop.ibus") &&
            !schema.has_prefix("org.freedesktop.ibus.engine")) {
            ibus_schemas.prepend(schema);
        }
    }
    if (ibus_schemas.length() == 0) {
        printerr("Not found schemas of \"org.freedesktop.ibus\"\n");
        return ibus_schemas;
    }
    ibus_schemas.sort(GLib.strcmp);

    return ibus_schemas;
}


int read_config(string[] argv) {
    if (read_config_options(argv) == Posix.EXIT_FAILURE)
        return Posix.EXIT_FAILURE;

    GLib.SList<string> ibus_schemas = get_ibus_schemas();
    if (ibus_schemas.length() == 0)
        return Posix.EXIT_FAILURE;

    GLib.SettingsSchemaSource schema_source =
            GLib.SettingsSchemaSource.get_default();
    var output = new GLib.StringBuilder();
    foreach (string schema in ibus_schemas) {
        GLib.SettingsSchema settings_schema = schema_source.lookup(schema,
                                                                   false);
        GLib.Settings settings = new GLib.Settings(schema);

        output.append_printf("SCHEMA: %s\n", schema);

        foreach (string key in settings_schema.list_keys()) {
            GLib.Variant variant = settings.get_value(key);
            output.append_printf("  %s: %s\n", key, variant.print(true));
        }
    }
    print("%s", output.str);

    return Posix.EXIT_SUCCESS;
}


int reset_config(string[] argv) {
    if (read_config_options(argv) == Posix.EXIT_FAILURE)
        return Posix.EXIT_FAILURE;

    GLib.SList<string> ibus_schemas = get_ibus_schemas();
    if (ibus_schemas.length() == 0)
        return Posix.EXIT_FAILURE;

    print("%s\n", _("Resetting…"));

    GLib.SettingsSchemaSource schema_source =
            GLib.SettingsSchemaSource.get_default();
    foreach (string schema in ibus_schemas) {
        GLib.SettingsSchema settings_schema = schema_source.lookup(schema,
                                                                   false);
        GLib.Settings settings = new GLib.Settings(schema);

        print("SCHEMA: %s\n", schema);

        foreach (string key in settings_schema.list_keys()) {
            print("  %s\n", key);
            settings.reset(key);
        }
    }

    GLib.Settings.sync();
    print("%s\n", _("Done"));

    return Posix.EXIT_SUCCESS;
}


#if EMOJI_DICT
int emoji_dialog(string[] argv) {
    string cmd = Config.LIBEXECDIR + "/ibus-ui-emojier";

    var file = File.new_for_path(cmd);
    if (!file.query_exists())
        cmd = "../ui/gtk3/ibus-ui-emojier";

    argv[0] = cmd;

    string[] env = Environ.get();

    try {
        // Non-blocking
        Process.spawn_async(null, argv, env,
                            SpawnFlags.SEARCH_PATH,
                            null, null);
    } catch (SpawnError e) {
        stderr.printf("%s\n", e.message);
        return Posix.EXIT_FAILURE;
    }

    return Posix.EXIT_SUCCESS;
}
#endif


int read_im_module(string[] argv) {
    string? im_module = IBusIMModule.im_module_get_id(argv);
    if (im_module == null)
        return Posix.EXIT_FAILURE;
    print("%s\n".printf(im_module));
    return Posix.EXIT_SUCCESS;
}


int print_help(string[] argv) {
    print_usage(stdout);
    return Posix.EXIT_SUCCESS;
}


delegate int EntryFunc(string[] argv);

struct CommandEntry {
    unowned string name;
    unowned string description;
    unowned EntryFunc entry;
}


const CommandEntry commands[]  = {
    { "engine", N_("Set or get engine"), get_set_engine },
    { "exit", N_("Exit ibus-daemon"), exit_daemon },
    { "list-engine", N_("Show available engines"), list_engine },
    { "watch", N_("(Not implemented)"), message_watch },
    { "restart", N_("Restart ibus-daemon"), restart_daemon },
    { "start", N_("Start ibus-daemon"), start_daemon },
    { "version", N_("Show version"), print_version },
    { "read-cache", N_("Show the content of registry cache"), read_cache },
    { "write-cache", N_("Create registry cache"), write_cache },
    { "address", N_("Print the D-Bus address of ibus-daemon"), print_address },
    { "read-config", N_("Show the configuration values"), read_config },
    { "reset-config", N_("Reset the configuration values"), reset_config },
#if EMOJI_DICT
    { "emoji", N_("Save emoji on dialog to clipboard"), emoji_dialog },
#endif
    { "im-module", N_("Retrieve im-module value from GTK instance"),
      read_im_module },
    { "help", N_("Show this information"), print_help }
};

static string program_name;


void print_usage(FileStream stream) {
    stream.printf(_("Usage: %s COMMAND [OPTION...]\n\n"), program_name);
    stream.printf(_("Commands:\n"));
    for (int i = 0; i < commands.length; i++) {
        stream.printf("  %-12s    %s\n",
                      commands[i].name,
                      GLib.dgettext(null, commands[i].description));
    }
}


public int main(string[] argv) {
    GLib.Intl.setlocale(GLib.LocaleCategory.ALL, "");
    GLib.Intl.bindtextdomain(Config.GETTEXT_PACKAGE, Config.LOCALEDIR);
    GLib.Intl.bind_textdomain_codeset(Config.GETTEXT_PACKAGE, "UTF-8");
    GLib.Intl.textdomain(Config.GETTEXT_PACKAGE);

    IBus.init();

    program_name = Path.get_basename(argv[0]);
    if (argv.length < 2) {
        print_usage(stderr);
        return Posix.EXIT_FAILURE;
    }

    string[] new_argv = argv[1:argv.length];
    new_argv[0] = "%s %s".printf(program_name, new_argv[0]);
    for (int i = 0; i < commands.length; i++) {
        if (commands[i].name == argv[1])
            return commands[i].entry(new_argv);
    }

    stderr.printf(_("%s is unknown command!\n"), argv[1]);
    print_usage(stderr);
    return Posix.EXIT_FAILURE;
}
