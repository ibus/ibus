/* vim:set et sts=4 sw=4:
 *
 * ibus - The Input Bus
 *
 * Copyright(c) 2015-2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright(c) 2015 Red Hat, Inc.
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

/* This class extends AppIndicator because
 * AppIndicator misses "Activate" dbus method in the definition
 * for left click on the indicator.
 */

// I have no idea to exempt this file to make ibus_ui_gtk3_vala.stamp
#if INDICATOR
public extern string _notification_item;
public extern string _notification_watcher;

class Indicator : IBus.Service
{
    public string id { get; construct; }
    public string category_s { get; construct; }
    public string status_s { get; set; }
    public string icon_name { get; set; }
    public string icon_desc { get; set; }
    public string attention_icon_name { get; set; }
    public string attention_icon_desc { get; set; }
    public string title { get; set; }
    public string icon_theme_path { get; set; }
    public bool   connected { get; set; }
    public string label_s { get; set; }
    public string label_guide_s { get; set; }
    public uint32 ordering_index { get; set; }
    public GLib.Variant icon_vector { get; set; }

    public enum Category {
        APPLICATION_STATUS,
        COMMUNICATIONS,
        SYSTEM_SERVICES,
        HARDWARE,
        OTHER;

        public string to_nick() {
            switch(this) {
            case APPLICATION_STATUS: return "ApplicationStatus";
            case COMMUNICATIONS: return "Communications";
            case SYSTEM_SERVICES: return "SystemServices";
            case HARDWARE: return "Hardware";
            case OTHER: return "Other";
            default: assert_not_reached();
            }
        }
    }

    public enum Status {
        PASSIVE,
        ACTIVE,
        ATTENTION;

        public string to_nick() {
            switch(this) {
            case PASSIVE: return "Passive";
            case ACTIVE: return "Active";
            case ATTENTION: return "NeedsAttention";
            default: assert_not_reached();
            }
        }
    }

    private const string DEFAULT_ITEM_PATH = "/org/ayatana/NotificationItem";
    private const string NOTIFICATION_ITEM_DBUS_IFACE =
            "org.kde.StatusNotifierItem";
    private const string NOTIFICATION_WATCHER_DBUS_IFACE =
            "org.kde.StatusNotifierWatcher";
    private const string NOTIFICATION_WATCHER_DBUS_ADDR =
            "org.kde.StatusNotifierWatcher";
    private const string NOTIFICATION_WATCHER_DBUS_OBJ =
            "/StatusNotifierWatcher";

    private GLib.DBusNodeInfo m_watcher_node_info;
    private unowned GLib.DBusInterfaceInfo m_watcher_interface_info;
    private bool m_registered;
    private GLib.DBusProxy m_proxy;
    private int m_context_menu_x;
    private int m_context_menu_y;
    private int m_activate_menu_x;
    private int m_activate_menu_y;
    private Gdk.Window m_indicator_window;
    private Gtk.Menu m_menu;
    private Dbusmenu.Server m_dbusmenu_server;


    public Indicator(string id,
                     Category category = Category.OTHER) {
        string path = DEFAULT_ITEM_PATH + "/" + id;
        path = path.delimit("-", '_');

        // AppIndicator.set_category() converts enum value to string internally.
        // If connection is not assigned in the constructor, register() is
        // not called.
        GLib.Object(object_path: path,
                    id: id,
                    category_s: category.to_nick());
        this.status_s = Status.PASSIVE.to_nick();
        this.icon_name = "ibus";
        this.icon_desc = "";
        this.title = "";
        this.icon_theme_path = "";
        this.attention_icon_name = "";
        this.attention_icon_desc = "";
        this.label_s = "";
        this.label_guide_s = "";
        var n = free_interfaces(0);
        free_interfaces(-n);
        add_interfaces(_notification_item);

        try {
            m_watcher_node_info =
                    new GLib.DBusNodeInfo.for_xml(_notification_watcher);
        } catch (GLib.Error e) {
            warning("Failed to create dbus node info: " + e.message);
            return;
        }
        m_watcher_interface_info =
                m_watcher_node_info.lookup_interface(
                        NOTIFICATION_WATCHER_DBUS_IFACE);
        GLib.Bus.watch_name (GLib.BusType.SESSION,
                             NOTIFICATION_WATCHER_DBUS_ADDR,
                             GLib.BusNameWatcherFlags.NONE,
                             name_appeared_handler,
                             name_vanished_handler);
        GLib.Bus.get.begin(GLib.BusType.SESSION, null, (obj, res) => {
            try {
                this.connection = GLib.Bus.get.end(res);
                check_connect();
            } catch (GLib.IOError e) {
                warning("Failed to get the session bus: %s", e.message);
            }
        });
    }


    private void name_appeared_handler(GLib.DBusConnection connection,
                                       string name,
                                       string name_owner) {
        // FIXME: https://discourse.gnome.org/t/how-to-write-vala-glib-dbusproxy-async/2059
        GLib.DBusProxy.new.begin(
                connection,
                GLib.DBusProxyFlags.DO_NOT_LOAD_PROPERTIES |
                        GLib.DBusProxyFlags.DO_NOT_CONNECT_SIGNALS,
                m_watcher_interface_info,
                NOTIFICATION_WATCHER_DBUS_ADDR,
                NOTIFICATION_WATCHER_DBUS_OBJ,
                NOTIFICATION_WATCHER_DBUS_IFACE,
                null,
                (obj, res) => {
                        bus_watcher_ready(obj, res);
                });
    }


    private void name_vanished_handler(GLib.DBusConnection connection,
                                       string name) {
        m_proxy = null;
    }


    private void check_connect() {
        if (this.connection == null)
            return;
        if (m_menu == null)
            return;
        try {
            if (!m_registered) {
                if (!register(this.connection))
                    return;
                else
                    m_registered = true;
            }
        } catch (GLib.Error e) {
            warning("Failed to register the application indicator xml: " +
                    e.message);
            return;
        }
    }


    private void bus_watcher_ready(GLib.Object? obj, GLib.AsyncResult? res) {
        if (res != null) {
            try {
                m_proxy = GLib.DBusProxy.new.end(res);
            } catch (GLib.IOError e) {
                warning("Failed to call dbus proxy: " + e.message);
                return;
            }

            m_proxy.notify["g-name-owner"].connect((obj, pspec) => {
                    var name = m_proxy.get_name_owner();
                    if (name != null)
                        check_connect();
            });
        }

        var name = m_proxy.get_name_owner();
        // KDE panel does not run yet if name == null
        if (name == null)
            return;

        m_proxy.call.begin("RegisterStatusNotifierItem",
                           new GLib.Variant("(s)", this.object_path),
                           GLib.DBusCallFlags.NONE,
                           -1,
                           null,
                           (p_obj, p_res) => {
                                   try {
                                       m_proxy.call.end(p_res);
                                       registered_status_notifier_item();
                                   } catch (GLib.Error e) {
                        warning("Failed to call " +
                                "RegisterStatusNotifierItem: " +
                                e.message);
                                   }
                           });
    }


    private void _context_menu_cb(GLib.DBusConnection       connection,
                                  GLib.Variant              parameters,
                                  GLib.DBusMethodInvocation invocation) {
        GLib.Variant var_x = parameters.get_child_value(0);
        GLib.Variant var_y = parameters.get_child_value(1);
        m_context_menu_x = var_x.get_int32();
        m_context_menu_y = var_y.get_int32();
        Gdk.Window window = query_gdk_window();
        context_menu(m_context_menu_x, m_context_menu_y, window, 2, 0);
    }


    private void _activate_menu_cb(GLib.DBusConnection       connection,
                                   GLib.Variant              parameters,
                                   GLib.DBusMethodInvocation invocation) {
        GLib.Variant var_x = parameters.get_child_value(0);
        GLib.Variant var_y = parameters.get_child_value(1);
        m_activate_menu_x = var_x.get_int32();
        m_activate_menu_y = var_y.get_int32();
        Gdk.Window window = query_gdk_window();
        activate(m_activate_menu_x, m_activate_menu_y, window);
    }


    private Gdk.Window? query_gdk_window() {
#if ENABLE_XIM
        if (m_indicator_window != null)
            return m_indicator_window;

        var display = BindingCommon.get_xdisplay();
        if (display == null)
            return null;
        unowned X.Display xdisplay = display.get_xdisplay();
        X.Window current = xdisplay.default_root_window();
        X.Window parent = 0;
        X.Window child = 0;
        int root_x, root_y, win_x, win_y;
        uint mask = 0;
        root_x = root_y = win_x = win_y = 0;
        bool retval;
        // Need XSetErrorHandler for BadWindow?
        while ((retval = xdisplay.query_pointer(current,
                                      out parent, out child,
                                      out root_x, out root_y,
                                      out win_x, out win_y,
                                      out mask))) {
            if (child == 0)
                break;
            current = child;
        }
        if (!retval) {
            string format =
                    "XQueryPointer is failed: current: %x root: %x " +
                    "child: %x (%d, %d), (%d, %d), %u";
            string message = format.printf((uint)current,
                                           (uint)xdisplay.default_root_window(),
                                           (uint)child,
                                           root_x, root_y, win_x, win_y,
                                           mask);
            warning("XQueryPointer is failed: %s", message);
            return null;
        }
        if (current == xdisplay.default_root_window())
            warning("The query window is root window");
        m_indicator_window = Gdk.X11.Window.lookup_for_display(
                display as Gdk.X11.Display,
                current);
        if (m_indicator_window != null)
            return m_indicator_window;
        m_indicator_window = new Gdk.X11.Window.foreign_for_display(
                display as Gdk.X11.Display,
                current);
        return m_indicator_window;
#else
        return null;
#endif
    }


    private GLib.Variant? _get_id(GLib.DBusConnection connection) {
        return new GLib.Variant.string(this.id);
    }


    private GLib.Variant? _get_category(GLib.DBusConnection connection) {
        return new GLib.Variant.string(this.category_s);
    }


    private GLib.Variant? _get_status(GLib.DBusConnection connection) {
        return new GLib.Variant.string(this.status_s);
    }


    private GLib.Variant? _get_icon_name(GLib.DBusConnection connection) {
        return new GLib.Variant.string(this.icon_name);
    }


    private GLib.Variant? _get_icon_vector(GLib.DBusConnection connection) {
        return this.icon_vector;
    }


    private GLib.Variant? _get_icon_desc(GLib.DBusConnection connection) {
        return new GLib.Variant.string(this.icon_desc);
    }


    private GLib.Variant? _get_attention_icon_name(GLib.DBusConnection
                                                             connection) {
        return new GLib.Variant.string(this.attention_icon_name);
    }


    private GLib.Variant? _get_attention_icon_desc(GLib.DBusConnection
                                                             connection) {
        return new GLib.Variant.string(this.attention_icon_desc);
    }


    private GLib.Variant? _get_title(GLib.DBusConnection connection) {
        return new GLib.Variant.string(this.title);
    }


    private GLib.Variant? _get_icon_theme_path(GLib.DBusConnection connection) {
        return new GLib.Variant.string(this.icon_theme_path);
    }


    private GLib.Variant? _get_menu(GLib.DBusConnection connection) {
        if (m_dbusmenu_server != null) {
            string o;
            m_dbusmenu_server.get(Dbusmenu.SERVER_PROP_DBUS_OBJECT, out o);
            var variant = new GLib.Variant("o", o);
            return variant;
        }
        return new GLib.Variant("o", "/");;
    }


    private GLib.Variant? _get_xayatana_label(GLib.DBusConnection connection) {
        return new GLib.Variant.string(this.label_s);
    }


    private GLib.Variant? _get_xayatana_label_guide(GLib.DBusConnection
                                                              connection) {
        return new GLib.Variant.string(this.label_guide_s);
    }


    private GLib.Variant? _get_xayatana_ordering_index(GLib.DBusConnection
                                                              connection) {
        return new GLib.Variant.uint32(this.ordering_index);
    }


    public override void service_method_call(GLib.DBusConnection
                                                                connection,
                                             string             sender,
                                             string             object_path,
                                             string             interface_name,
                                             string             method_name,
                                             GLib.Variant       parameters,
                                             GLib.DBusMethodInvocation
                                                                invocation) {
        GLib.return_if_fail (object_path == this.object_path);
        GLib.return_if_fail (interface_name == NOTIFICATION_ITEM_DBUS_IFACE);

        if (method_name == "Activate") {
            _activate_menu_cb(connection, parameters, invocation);
            return;
        }
        if (method_name == "ContextMenu") {
            _context_menu_cb(connection, parameters, invocation);
            return;
        }
        if (method_name == "SecondaryActivate") {
            secondary_activate();
            return;
        }

        warning("service_method_call() does not handle the method: " +
                method_name);
    }


    public override GLib.Variant? service_get_property(GLib.DBusConnection
                                                                connection,
                                                       string   sender,
                                                       string   object_path,
                                                       string   interface_name,
                                                       string   property_name)
    throws GLib.Error {
        if (object_path != this.object_path) {
            throw new GLib.DBusError.FAILED(
                    "%s != %s".printf(object_path, this.object_path));
        }
        if (interface_name != NOTIFICATION_ITEM_DBUS_IFACE) {
            throw new GLib.DBusError.FAILED(
                    "%s != %s".printf(interface_name,
                                      NOTIFICATION_ITEM_DBUS_IFACE));
        }

        GLib.Variant? result = null;
        if (property_name == "Id") {
            result = _get_id(connection);
        } else if (property_name == "Category") {
            result = _get_category(connection);
        } else if (property_name == "Status") {
            result = _get_status(connection);
        } else if (property_name == "IconName") {
            result = _get_icon_name(connection);
        } else if (property_name == "IconPixmap") {
            result = _get_icon_vector(connection);
        } else if (property_name == "IconAccessibleDesc") {
            result = _get_icon_desc(connection);
        } else if (property_name == "AttentionIconName") {
            result = _get_attention_icon_name(connection);
        } else if (property_name == "AttentionAccessibleDesc") {
            result = _get_attention_icon_desc(connection);
        } else if (property_name == "Title") {
            result = _get_title(connection);
        } else if (property_name == "IconThemePath") {
            result = _get_icon_theme_path(connection);
        } else if (property_name == "Menu") {
            result = _get_menu(connection);
        } else if (property_name == "XAyatanaLabel") {
            result = _get_xayatana_label(connection);
        } else if (property_name == "XAyatanaLabelGuide") {
            result = _get_xayatana_label_guide(connection);
        } else if (property_name == "XAyatanaOrderingIndex") {
            result = _get_xayatana_ordering_index(connection);
        } else {
            throw new GLib.DBusError.UNKNOWN_PROPERTY(
                "IBus.Indicator does not handle the property: "
                + property_name);
        }

        if (result == null) {
            throw new GLib.DBusError.FAILED(
                "IBus.Indicator returns null for the property "
                + property_name);
        }
        return result;
    }


    public override bool service_set_property(GLib.DBusConnection
                                                           connection,
                                              string       sender,
                                              string       object_path,
                                              string       interface_name,
                                              string       property_name,
                                              GLib.Variant value) {
        return false;
    }


    // AppIndicator.set_status() converts enum value to string internally.
    public void set_status(Status status) {
        string status_s = status.to_nick();
        if (this.status_s == status_s)
            return;
        this.status_s = status_s;

        /* This API does not require (this.connection != null)
         * because service_get_property() can be called when
         * this.connection emits the "NewStatus" signal or
         * or m_proxy calls the "RegisterStatusNotifierItem" signal.
         */
        if (this.connection == null)
            return;
        try {
            this.connection.emit_signal(null,
                                        this.object_path,
                                        NOTIFICATION_ITEM_DBUS_IFACE,
                                        "NewStatus",
                                        new GLib.Variant("(s)", status_s));
        } catch(GLib.Error e) {
            warning("Unable to send signal for NewIcon: %s", e.message);
        }
    }


    // AppIndicator.set_icon() is deprecated.
    public void set_icon_full(string icon_name, string? icon_desc) {
        bool changed = false;
        if (this.icon_name != icon_name) {
            this.icon_name = icon_name;
            this.icon_vector = null;
            changed = true;
        }
        if (this.icon_desc != icon_desc) {
            this.icon_desc = icon_desc;
            changed = true;
        }
        if (!changed)
            return;

        /* This API does not require (this.connection != null)
         * because service_get_property() can be called when
         * this.connection emits the "NewIcon" signal or
         * or m_proxy calls the "RegisterStatusNotifierItem" signal.
         */
        if (this.connection == null || !m_registered)
            return;
        try {
            this.connection.emit_signal(null,
                                        this.object_path,
                                        NOTIFICATION_ITEM_DBUS_IFACE,
                                        "NewIcon",
                                        null);
        } catch(GLib.Error e) {
            warning("Unable to send signal for NewIcon: %s", e.message);
        }
    }


    public void set_cairo_image_surface_full(Cairo.ImageSurface image,
                                             string?            icon_desc) {
        int width = image.get_width();
        int height = image.get_height();
        int stride = image.get_stride();
        unowned uint8[] data = (uint8[]) image.get_data();
        data.length = stride * height;
        GLib.Bytes bytes = new GLib.Bytes(data);
        GLib.Variant bs =
                new GLib.Variant.from_bytes(GLib.VariantType.BYTESTRING,
                                            bytes,
                                            true);
        GLib.VariantBuilder builder = new GLib.VariantBuilder(
                new GLib.VariantType("a(iiay)"));
        builder.open(new GLib.VariantType("(iiay)"));
        builder.add("i", width);
        builder.add("i", height);
        builder.add_value(bs);
        builder.close();
        this.icon_vector = new GLib.Variant("a(iiay)", builder);
        this.icon_name = "";

        if (this.icon_desc != icon_desc)
            this.icon_desc = icon_desc;

        /* This API does not require (this.connection != null)
         * because service_get_property() can be called when
         * this.connection emits the "NewIcon" signal or
         * or m_proxy calls the "RegisterStatusNotifierItem" signal.
         */
        if (this.connection == null)
            return;
        try {
            this.connection.emit_signal(null,
                                        this.object_path,
                                        NOTIFICATION_ITEM_DBUS_IFACE,
                                        "NewIcon",
                                        null);
        } catch(GLib.Error e) {
            warning("Unable to send signal for NewIcon: %s", e.message);
        }
    }


    public void set_menu(Gtk.Menu menu) {
        if (this.object_path == null)
            return;
        m_menu = menu;
        if (m_dbusmenu_server == null) {
            var path = this.object_path + "/Menu";
            m_dbusmenu_server = new Dbusmenu.Server(path);
        }
        var root = DbusmenuGtk.gtk_parse_menu_structure(menu);
        m_dbusmenu_server.set_root(root);
        check_connect();
    }


    public void position_context_menu(Gtk.Menu menu,
                                      out int  x,
                                      out int  y,
                                      out bool push_in) {
        x = m_context_menu_x;
        y = m_context_menu_y;
        push_in = false;
    }


    public void position_activate_menu(Gtk.Menu menu,
                                       out int  x,
                                       out int  y,
                                       out bool push_in) {
        x = m_activate_menu_x;
        y = m_activate_menu_y;
        push_in = false;
    }


    /**
     * unregister_connection:
     *
     * "Destroy" dbus method is not called for the indicator's connection
     * when panel's connection is disconnected because the dbus connection
     * is a shared session bus so need to call
     * g_dbus_connection_unregister_object() by manual here
     * so that g_object_unref(m_panel) will be called later with an idle method,
     * which was assigned in the arguments of
     * g_dbus_connection_register_object()
     */
    public void unregister_connection() {
        unregister(get_connection());
    }


    public signal void context_menu(int        x,
                                    int        y,
                                    Gdk.Window window,
                                    uint       button,
                                    uint       activate_time);
    public signal void activate(int        x,
                                int        y,
                                Gdk.Window window);
    public signal void secondary_activate();
    public signal void registered_status_notifier_item();
}
#endif
