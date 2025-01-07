/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2015 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2015-2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2008-2016 Red Hat, Inc.
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

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include "ibusbus.h"
#include "ibusmarshalers.h"
#include "ibusinternal.h"
#include "ibusshare.h"
#include "ibusenginedesc.h"
#include "ibusserializable.h"
#include "ibusconfig.h"

#define IBUS_BUS_GET_PRIVATE(o)  \
   ((IBusBusPrivate *)ibus_bus_get_instance_private (o))

enum {
    CONNECTED,
    DISCONNECTED,
    GLOBAL_ENGINE_CHANGED,
    NAME_OWNER_CHANGED,
    GLOBAL_SHORTCUT_KEY_RESPONDED,
    LAST_SIGNAL,
};

enum {
    PROP_0 = 0,
    PROP_CONNECT_ASYNC,
    PROP_CLIENT_ONLY,
};

/* IBusBusPriv */
struct _IBusBusPrivate {
    GFileMonitor *monitor;
    GDBusConnection *connection;
    gboolean connected;
    gboolean watch_dbus_signal;
    guint watch_dbus_signal_id;
    gboolean watch_ibus_signal;
    guint watch_global_engine_changed_id;
    guint watch_global_shortcut_key_responded_id;
    IBusConfig *config;
    gchar *unique_name;
    gboolean connect_async;
    gchar *bus_address;
    gboolean use_portal;
    gboolean client_only;
    GCancellable *cancellable;
    guint portal_name_watch_id;
};

static guint    bus_signals[LAST_SIGNAL] = { 0 };

static IBusBus *_bus = NULL;

/* functions prototype */
static GObject  *ibus_bus_constructor           (GType                   type,
                                                 guint                   n_params,
                                                 GObjectConstructParam  *params);
static void      ibus_bus_destroy               (IBusObject             *object);
static void      ibus_bus_connect_async         (IBusBus                *bus);
static void      ibus_bus_watch_dbus_signal     (IBusBus                *bus);
static void      ibus_bus_unwatch_dbus_signal   (IBusBus                *bus);
static void      ibus_bus_watch_ibus_signal     (IBusBus                *bus);
static void      ibus_bus_unwatch_ibus_signal   (IBusBus                *bus);
static GVariant *ibus_bus_call_sync             (IBusBus                *bus,
                                                 const gchar            *service,
                                                 const gchar            *path,
                                                 const gchar            *interface,
                                                 const gchar            *member,
                                                 GVariant               *parameters,
                                                 const GVariantType     *reply_type);
static void      ibus_bus_call_async             (IBusBus                *bus,
                                                  const gchar            *service,
                                                  const gchar            *path,
                                                  const gchar            *interface,
                                                  const gchar            *member,
                                                  GVariant               *parameters,
                                                  const GVariantType     *reply_type,
                                                  gpointer                source_tag,
                                                  gint                    timeout_msec,
                                                  GCancellable           *cancellable,
                                                  GAsyncReadyCallback     callback,
                                                  gpointer                user_data);
static void      ibus_bus_set_property           (IBusBus                *bus,
                                                  guint                   prop_id,
                                                  const GValue           *value,
                                                  GParamSpec             *pspec);
static void      ibus_bus_get_property           (IBusBus                *bus,
                                                  guint                   prop_id,
                                                  GValue                 *value,
                                                  GParamSpec             *pspec);

static void     ibus_bus_close_connection        (IBusBus                *bus);

G_DEFINE_TYPE_WITH_PRIVATE (IBusBus, ibus_bus, IBUS_TYPE_OBJECT)

static void
ibus_bus_class_init (IBusBusClass *class)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (class);
    IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (class);

    gobject_class->constructor = ibus_bus_constructor;
    gobject_class->set_property =
            (GObjectSetPropertyFunc) ibus_bus_set_property;
    gobject_class->get_property =
            (GObjectGetPropertyFunc) ibus_bus_get_property;
    ibus_object_class->destroy = ibus_bus_destroy;

    /* install properties */
    /**
     * IBusBus:connect-async:
     *
     * Whether the #IBusBus object should connect asynchronously to the bus.
     *
     */
    g_object_class_install_property (
            gobject_class,
            PROP_CONNECT_ASYNC,
            g_param_spec_boolean ("connect-async",
                                  "Connect Async",
                                  "Connect asynchronously to the bus",
                                  FALSE,
                                  G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    /**
     * IBusBus:client-only:
     *
     * Whether the #IBusBus object is for client use only.
     *
     */
    g_object_class_install_property (
            gobject_class,
            PROP_CLIENT_ONLY,
            g_param_spec_boolean ("client-only",
                                  "ClientOnly",
                                  "Client use only",
                                  FALSE,
                                  G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /* install signals */
    /**
     * IBusBus::connected:
     * @bus: The #IBusBus object which recevied the signal
     *
     * Emitted when #IBusBus is connected to ibus-daemon.
     *
     */
    bus_signals[CONNECTED] =
        g_signal_new (I_("connected"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE,
            0);

    /**
     * IBusBus::disconnected:
     * @bus: The #IBusBus object which recevied the signal
     *
     * Emitted when #IBusBus is disconnected from ibus-daemon.
     *
     */
    bus_signals[DISCONNECTED] =
        g_signal_new (I_("disconnected"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE,
            0);

    /**
     * IBusBus::global-engine-changed:
     * @bus: The #IBusBus object which recevied the signal
     * @name: The name of the new global engine.
     *
     * Emitted when global engine is changed.
     *
     */
    bus_signals[GLOBAL_ENGINE_CHANGED] =
        g_signal_new (I_("global-engine-changed"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__STRING,
            G_TYPE_NONE,
            1,
            G_TYPE_STRING);

    /**
     * IBusBus::name-owner-changed:
     * @bus: The #IBusBus object which recevied the signal
     * @name: The name which ower is changed.
     * @old_owner: The unique bus name of the old owner.
     * @new_owner: The unique bus name of the new owner.
     *
     * Emitted when D-Bus name owner is changed.
     *
     */
    bus_signals[NAME_OWNER_CHANGED] =
        g_signal_new (I_("name-owner-changed"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__STRING_STRING_STRING,
            G_TYPE_NONE,
            3,
            G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

    /**
     * IBusBus::global-shortcut-key-responded:
     * @bus: The #IBusBus object which recevied the signal
     * @type: The type of the global shortcut key.
     * @keyval: Key symbol of the key press.
     * @keycode: KeyCode of the key press.
     * @state: Key modifier flags.
     * @is_backward: %TRUE if the backward key is pressed.
     *
     * Emitted when global shortcut key is responded.
     * Since 1.5.00
     *
     */
    bus_signals[GLOBAL_SHORTCUT_KEY_RESPONDED] =
        g_signal_new (I_("global-shortcut-key-responded"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__UCHAR_UINT_UINT_UINT_BOOLEAN,
            G_TYPE_NONE,
            5,
            G_TYPE_UCHAR,
            G_TYPE_UINT,
            G_TYPE_UINT,
            G_TYPE_UINT,
            G_TYPE_BOOLEAN);
}

static void
_connection_dbus_signal_cb (GDBusConnection *connection,
                            const gchar *sender_name,
                            const gchar *object_path,
                            const gchar *interface_name,
                            const gchar *signal_name,
                            GVariant *parameters,
                            gpointer user_data)
{
    g_return_if_fail (user_data != NULL);
    g_return_if_fail (IBUS_IS_BUS (user_data));

    if (g_strcmp0 (signal_name, "NameOwnerChanged") == 0) {
        gchar *name = NULL;
        gchar *old_owner = NULL;
        gchar *new_owner = NULL;
        g_variant_get (parameters, "(&s&s&s)", &name, &old_owner, &new_owner);
        g_signal_emit (IBUS_BUS (user_data),
                       bus_signals[NAME_OWNER_CHANGED], 0,
                       name, old_owner, new_owner);
    }
    /* FIXME handle other D-Bus signals if needed */
}

static void
_connection_ibus_signal_cb (GDBusConnection *connection,
                            const gchar *sender_name,
                            const gchar *object_path,
                            const gchar *interface_name,
                            const gchar *signal_name,
                            GVariant *parameters,
                            gpointer user_data)
{
    g_return_if_fail (user_data != NULL);
    g_return_if_fail (IBUS_IS_BUS (user_data));

    if (!g_strcmp0 (signal_name, "GlobalEngineChanged")) {
        gchar *engine_name = NULL;
        g_variant_get (parameters, "(&s)", &engine_name);
        g_signal_emit (IBUS_BUS (user_data),
                       bus_signals[GLOBAL_ENGINE_CHANGED], 0,
                       engine_name);
    } else if (!g_strcmp0 (signal_name, "GlobalShortcutKeyResponded")) {
        guchar type = (guchar)IBUS_BUS_GLOBAL_BINDING_TYPE_ANY;
        guint keyval = 0;
        guint keycode = 0;
        guint state = 0;
        gboolean is_backward = FALSE;
        g_variant_get (parameters, "(yuuub)",
                       &type, &keyval, &keycode, &state, &is_backward);
        g_signal_emit (IBUS_BUS (user_data),
                       bus_signals[GLOBAL_SHORTCUT_KEY_RESPONDED], 0,
                       type, keyval, keycode, state, is_backward);
    }
    /* FIXME handle org.freedesktop.IBus.RegistryChanged signal if needed */
}

static void
_connection_closed_cb (GDBusConnection  *connection,
                       gboolean          remote_peer_vanished,
                       GError           *error,
                       IBusBus          *bus)
{
    if (error) {
        /* We replaced g_warning with g_debug here because
         * currently when ibus-daemon restarts, GTK client calls this and
         * _g_dbus_worker_do_read_cb() sets the error message:
         * "Underlying GIOStream returned 0 bytes on an async read"
         * http://git.gnome.org/browse/glib/tree/gio/gdbusprivate.c#n693
         * However we think the error message is almost harmless. */
        g_debug ("_connection_closed_cb: %s", error->message);
    }
    ibus_bus_close_connection (bus);
}

static void
ibus_bus_close_connection (IBusBus *bus)
{
    g_free (bus->priv->unique_name);
    bus->priv->unique_name = NULL;

    bus->priv->watch_dbus_signal_id = 0;
    bus->priv->watch_global_engine_changed_id = 0;

    g_free (bus->priv->bus_address);
    bus->priv->bus_address = NULL;

    /* Cancel ongoing connect request. */
    g_cancellable_cancel (bus->priv->cancellable);
    g_cancellable_reset (bus->priv->cancellable);

    bus->priv->connected = FALSE;

    /* unref the old connection at first */
    if (bus->priv->connection != NULL) {
        g_signal_handlers_disconnect_by_func (bus->priv->connection,
                                              G_CALLBACK (_connection_closed_cb),
                                              bus);
        if (!g_dbus_connection_is_closed(bus->priv->connection))
            g_dbus_connection_close(bus->priv->connection, NULL, NULL, NULL);
        g_object_unref (bus->priv->connection);
        bus->priv->connection = NULL;
        g_signal_emit (bus, bus_signals[DISCONNECTED], 0);
    }
}

static void
ibus_bus_connect_completed (IBusBus *bus)
{
    g_assert (bus->priv->connection);

    bus->priv->connected = TRUE;
    /* FIXME */
    ibus_bus_hello (bus);

    g_signal_connect (bus->priv->connection,
                      "closed",
                      (GCallback) _connection_closed_cb,
                      bus);
    if (bus->priv->watch_dbus_signal) {
        ibus_bus_watch_dbus_signal (bus);
    }
    if (bus->priv->watch_ibus_signal) {
        ibus_bus_watch_ibus_signal (bus);
    }

    g_signal_emit (bus, bus_signals[CONNECTED], 0);
}

static void
_bus_connect_start_portal_cb (GObject      *source_object,
                              GAsyncResult *res,
                              gpointer      user_data)
{
    IBusBus *bus = IBUS_BUS (user_data);
    GVariant *result;
    GError *error = NULL;

    result = g_dbus_connection_call_finish (G_DBUS_CONNECTION (source_object),
                                            res,
                                            &error);
    if (result != NULL) {
        ibus_bus_connect_completed (bus);
        g_variant_unref (result);
    } else {
        g_error_free (error);

        g_dbus_connection_close (bus->priv->connection, NULL, NULL, NULL);
        g_object_unref (bus->priv->connection);
        bus->priv->connection = NULL;

        g_free (bus->priv->bus_address);
        bus->priv->bus_address = NULL;
    }

    g_object_unref (bus);
}

static void
_bus_connect_async_cb (GObject      *source_object,
                       GAsyncResult *res,
                       gpointer      user_data)
{
    g_return_if_fail (user_data != NULL);
    g_return_if_fail (IBUS_IS_BUS (user_data));

    IBusBus *bus   = IBUS_BUS (user_data);
    GError  *error = NULL;

    bus->priv->connection =
                g_dbus_connection_new_for_address_finish (res, &error);

    if (error != NULL) {
        g_warning ("Unable to connect to ibus: %s", error->message);
        g_error_free (error);
        error = NULL;
    }

    if (bus->priv->connection != NULL) {
        if (bus->priv->use_portal) {
            g_object_set_data (G_OBJECT (bus->priv->connection),
                               "ibus-portal-connection",
                               GINT_TO_POINTER (TRUE));
            g_dbus_connection_call (
                    bus->priv->connection,
                    IBUS_SERVICE_PORTAL,
                    IBUS_PATH_IBUS,
                    "org.freedesktop.DBus.Peer",
                    "Ping",
                    g_variant_new ("()"),
                    G_VARIANT_TYPE ("()"),
                    G_DBUS_CALL_FLAGS_NONE,
                    -1,
                    bus->priv->cancellable,
                    (GAsyncReadyCallback) _bus_connect_start_portal_cb,
                    g_object_ref (bus));
        } else {
            ibus_bus_connect_completed (bus);
        }
    }
    else {
        g_free (bus->priv->bus_address);
        bus->priv->bus_address = NULL;
    }

    /* unref the ref from ibus_bus_connect */
    g_object_unref (bus);
}

static gchar *
ibus_bus_get_bus_address (IBusBus *bus)
{
    if (_bus->priv->use_portal)
        return g_dbus_address_get_for_bus_sync (G_BUS_TYPE_SESSION, NULL, NULL);
    else
        return g_strdup (ibus_get_address ());
}

static void
ibus_bus_connect_async (IBusBus *bus)
{
    gchar *bus_address = ibus_bus_get_bus_address (bus);

    if (bus_address == NULL)
        return;

    if (g_strcmp0 (bus->priv->bus_address, bus_address) == 0) {
        g_free (bus_address);
        return;
    }

    /* Close current connection and cancel ongoing connect request. */
    ibus_bus_close_connection (bus);

    bus->priv->bus_address = bus_address;
    g_object_ref (bus);
    g_dbus_connection_new_for_address (
            bus_address,
            G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
            G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
            NULL,
            bus->priv->cancellable,
            _bus_connect_async_cb, bus);
}

static gboolean
is_in_flatpak (void)
{
    static gboolean flatpak_info_read;
    static gboolean in_flatpak;

    if (flatpak_info_read)
        return in_flatpak;

    flatpak_info_read = TRUE;
    if (g_file_test ("/.flatpak-info", G_FILE_TEST_EXISTS))
        in_flatpak = TRUE;
    return in_flatpak;
}

static gboolean
ibus_bus_should_connect_portal (IBusBus *bus)
{
    return bus->priv->client_only &&
        (is_in_flatpak () || g_getenv ("IBUS_USE_PORTAL") != NULL);
}

static void
ibus_bus_connect (IBusBus *bus)
{
    const gchar *bus_address = ibus_get_address ();

    if (bus_address == NULL)
        return;

    if (g_strcmp0 (bus_address, bus->priv->bus_address) == 0 &&
        bus->priv->connection != NULL)
        return;

    /* Close current connection and cancel ongoing connect request. */
    ibus_bus_close_connection (bus);

    bus->priv->connection = g_dbus_connection_new_for_address_sync (
            bus_address,
            G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
            G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
            NULL, NULL, NULL);
    if (bus->priv->connection) {
        bus->priv->bus_address = g_strdup (bus_address);
        ibus_bus_connect_completed (bus);
    }
}

static void
_changed_cb (GFileMonitor       *monitor,
             GFile              *file,
             GFile              *other_file,
             GFileMonitorEvent   event_type,
             IBusBus            *bus)
{
    if (event_type != G_FILE_MONITOR_EVENT_CHANGED &&
        event_type != G_FILE_MONITOR_EVENT_CREATED &&
        event_type != G_FILE_MONITOR_EVENT_DELETED)
        return;

    ibus_bus_connect_async (bus);
}

static void
ibus_bus_init (IBusBus *bus)
{
    struct stat buf;
    char *path;
    int fd;

    bus->priv = IBUS_BUS_GET_PRIVATE (bus);

    bus->priv->config = NULL;
    bus->priv->connection = NULL;
    bus->priv->watch_dbus_signal = FALSE;
    bus->priv->watch_dbus_signal_id = 0;
    bus->priv->watch_ibus_signal = FALSE;
    bus->priv->watch_global_engine_changed_id = 0;
    bus->priv->watch_global_shortcut_key_responded_id = 0;
    bus->priv->unique_name = NULL;
    bus->priv->connect_async = FALSE;
    bus->priv->client_only = FALSE;
    bus->priv->bus_address = NULL;
    bus->priv->cancellable = g_cancellable_new ();

    path = g_path_get_dirname (ibus_get_socket_path ());

    errno = 0;
    if (g_mkdir_with_parents (path, 0700)) {
        g_warning ("Failed to mkdir %s: %s", path, g_strerror (errno));
        g_free (path);
        return;
    }

    errno = 0;
    if ((fd = open (path, O_RDONLY | O_DIRECTORY, S_IRWXU)) == -1) {
        g_warning ("open %s failed: %s", path, g_strerror (errno));
        g_free (path);
        return;
    }
    /* TOCTOU: Use fstat() and fchmod() but not stat() and chmod().
     * because it can cause a time-of-check, time-of-use race condition.
     */
    if (fstat (fd, &buf) == 0) {
        if (buf.st_uid != getuid ()) {
            g_warning ("The owner of %s is not %s!",
                       path, ibus_get_user_name ());
            close (fd);
            g_free (path);
            return;
        }
        if (buf.st_mode != (S_IFDIR | S_IRWXU)) {
            errno = 0;
            if (fchmod (fd, S_IRWXU))
                g_warning ("chmod failed: %s", g_strerror (errno));
        }
    }

    close (fd);
    g_free (path);
}

static void
ibus_bus_set_property (IBusBus      *bus,
                       guint         prop_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
    switch (prop_id) {
    case PROP_CONNECT_ASYNC:
        bus->priv->connect_async = g_value_get_boolean (value);
        break;
    case PROP_CLIENT_ONLY:
        bus->priv->client_only = g_value_get_boolean (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (bus, prop_id, pspec);
    }
}

static void
ibus_bus_get_property (IBusBus    *bus,
                       guint       prop_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
    switch (prop_id) {
    case PROP_CONNECT_ASYNC:
        g_value_set_boolean (value, bus->priv->connect_async);
        break;
    case PROP_CLIENT_ONLY:
        g_value_set_boolean (value, bus->priv->client_only);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (bus, prop_id, pspec);
    }
}

static void
portal_name_appeared (GDBusConnection *connection,
                      const gchar     *name,
                      const gchar     *owner,
                      gpointer         user_data)
{
    IBusBus *bus = IBUS_BUS (user_data);

    if (bus->priv->connection == NULL)
        ibus_bus_connect_async (bus);
}

static void
portal_name_vanished (GDBusConnection *connection,
                      const gchar     *name,
                      gpointer         user_data)
{
    IBusBus *bus = IBUS_BUS (user_data);

    if (bus->priv->connection)
        g_dbus_connection_close (bus->priv->connection, NULL, NULL, NULL);
}


static GObject*
ibus_bus_constructor (GType                  type,
                      guint                  n_params,
                      GObjectConstructParam *params)
{
    GObject *object;
    GFile *file;

    /* share one IBusBus instance in whole application */
    if (_bus == NULL) {
        object = G_OBJECT_CLASS (ibus_bus_parent_class)->constructor (
                type, n_params, params);
        /* make bus object sink */
        g_object_ref_sink (object);
        _bus = IBUS_BUS (object);

        _bus->priv->use_portal = ibus_bus_should_connect_portal (_bus);

        if (!_bus->priv->use_portal) {
            file = g_file_new_for_path (ibus_get_socket_path ());
            _bus->priv->monitor = g_file_monitor_file (file, 0, NULL, NULL);
            g_signal_connect (_bus->priv->monitor, "changed",
                              (GCallback) _changed_cb, _bus);
            g_object_unref (file);
        } else {
            _bus->priv->portal_name_watch_id =
                g_bus_watch_name (G_BUS_TYPE_SESSION,
                                  IBUS_SERVICE_PORTAL,
                                  G_BUS_NAME_WATCHER_FLAGS_NONE,
                                  portal_name_appeared,
                                  portal_name_vanished,
                                  _bus, NULL);
        }


        if (_bus->priv->connect_async)
            ibus_bus_connect_async (_bus);
        else
            ibus_bus_connect (_bus);
    }
    else {
        object = g_object_ref (G_OBJECT (_bus));
    }

    return object;
}

static void
ibus_bus_destroy (IBusObject *object)
{
    g_assert (_bus == (IBusBus *)object);

    IBusBus * bus = _bus;
    _bus = NULL;

    if (bus->priv->monitor) {
        g_object_unref (bus->priv->monitor);
        bus->priv->monitor = NULL;
    }

    if (bus->priv->config) {
        ibus_proxy_destroy ((IBusProxy *) bus->priv->config);
        bus->priv->config = NULL;
    }

    if (bus->priv->connection) {
        g_signal_handlers_disconnect_by_func (bus->priv->connection,
                                              G_CALLBACK (_connection_closed_cb),
                                              bus);
        /* FIXME should use async close function */
        g_dbus_connection_close_sync (bus->priv->connection, NULL, NULL);
        g_object_unref (bus->priv->connection);
        bus->priv->connection = NULL;
    }

    g_free (bus->priv->unique_name);
    bus->priv->unique_name = NULL;

    g_free (bus->priv->bus_address);
    bus->priv->bus_address = NULL;

    g_cancellable_cancel (bus->priv->cancellable);
    g_object_unref (bus->priv->cancellable);
    bus->priv->cancellable = NULL;

    if (bus->priv->portal_name_watch_id) {
        g_bus_unwatch_name (bus->priv->portal_name_watch_id);
        bus->priv->portal_name_watch_id = 0;
    }

    IBUS_OBJECT_CLASS (ibus_bus_parent_class)->destroy (object);
}

static gboolean
_async_finish_void (GTask   *task,
                    GError **error)
{
    /* g_task_propagate_error() is not a public API and 
     * g_task_had_error() needs to be called before
     * g_task_propagate_pointer() clears task->error.
     */
    gboolean had_error = g_task_had_error (task);
    g_task_propagate_pointer (task, error);
    if (had_error)
        return FALSE;
    return TRUE;
}

static gchar *
_async_finish_object_path (GTask   *task,
                           GError **error)
{
    gboolean had_error = g_task_had_error (task);
    GVariant *result = g_task_propagate_pointer (task, error);
    GVariant *variant = NULL;
    gchar *path = NULL;

    if (had_error) {
        g_assert (result == NULL);
        return NULL;
    }
    g_return_val_if_fail (result != NULL, NULL);
    g_variant_get (result, "(v)", &variant);
    path = g_variant_dup_string (variant, NULL);
    g_variant_unref (variant);
    g_variant_unref (result);
    return path;
}

static gchar *
_async_finish_string (GTask   *task,
                      GError **error)
{
    gboolean had_error = g_task_had_error (task);
    GVariant *variant = g_task_propagate_pointer (task, error);
    gchar *s = NULL;

    if (had_error) {
        g_assert (variant == NULL);
        return NULL;
    }
    g_return_val_if_fail (variant != NULL, NULL);
    g_variant_get (variant, "(&s)", &s);
    g_variant_unref (variant);
    return s;
}

static gboolean
_async_finish_gboolean (GTask   *task,
                        GError **error)
{
    gboolean had_error = g_task_had_error (task);
    GVariant *variant = g_task_propagate_pointer (task, error);
    gboolean retval = FALSE;

    if (had_error) {
        g_assert (variant == NULL);
        return FALSE;
    }
    g_return_val_if_fail (variant != NULL, FALSE);
    g_variant_get (variant, "(b)", &retval);
    g_variant_unref (variant);
    return retval;
}

static guint
_async_finish_guint (GTask   *task,
                     GError **error)
{
    gboolean had_error = g_task_had_error (task);
    GVariant *variant = g_task_propagate_pointer (task, error);
    static const guint bad_id = 0;
    guint id = 0;

    if (had_error) {
        g_assert (variant == NULL);
        return bad_id;
    }
    g_return_val_if_fail (variant != NULL, bad_id);
    g_variant_get (variant, "(u)", &id);
    g_variant_unref (variant);
    return id;
}

IBusBus *
ibus_bus_new (void)
{
    IBusBus *bus = IBUS_BUS (g_object_new (IBUS_TYPE_BUS,
                                           "connect-async", FALSE,
                                           "client-only", FALSE,
                                           NULL));

    return bus;
}

IBusBus *
ibus_bus_new_async (void)
{
    IBusBus *bus = IBUS_BUS (g_object_new (IBUS_TYPE_BUS,
                                           "connect-async", TRUE,
                                           "client-only", FALSE,
                                           NULL));

    return bus;
}

IBusBus *
ibus_bus_new_async_client (void)
{
    IBusBus *bus = IBUS_BUS (g_object_new (IBUS_TYPE_BUS,
                                           "connect-async", TRUE,
                                           "client-only", TRUE,
                                           NULL));

    return bus;
}

gboolean
ibus_bus_is_connected (IBusBus *bus)
{
    g_return_val_if_fail (IBUS_IS_BUS (bus), FALSE);

    if (bus->priv->connection == NULL || g_dbus_connection_is_closed (bus->priv->connection))
        return FALSE;

    return bus->priv->connected;
}

IBusInputContext *
ibus_bus_create_input_context (IBusBus      *bus,
                               const gchar  *client_name)
{
    g_return_val_if_fail (IBUS_IS_BUS (bus), NULL);
    g_return_val_if_fail (client_name != NULL, NULL);

    gchar *path;
    IBusInputContext *context = NULL;
    GVariant *result;
    result = ibus_bus_call_sync (bus,
                                 IBUS_SERVICE_IBUS,
                                 IBUS_PATH_IBUS,
                                 IBUS_INTERFACE_IBUS,
                                 "CreateInputContext",
                                 g_variant_new ("(s)", client_name),
                                 G_VARIANT_TYPE ("(o)"));

    if (result != NULL) {
        GError *error = NULL;
        g_variant_get (result, "(&o)", &path);
        context = ibus_input_context_new (path, bus->priv->connection, NULL, &error);
        g_variant_unref (result);
        if (context == NULL) {
            g_warning ("ibus_bus_create_input_context: %s", error->message);
            g_error_free (error);
        }
    }

    return context;
}

static void
_create_input_context_async_step_two_done (GObject      *source_object,
                                           GAsyncResult *res,
                                           GTask        *task)
{
    GError *error = NULL;
    IBusInputContext *context =
            ibus_input_context_new_async_finish (res, &error);
    if (context == NULL)
        g_task_return_error (task, error);
    else
        g_task_return_pointer (task, context, NULL);
    g_object_unref (task);
}

static void
_create_input_context_async_step_one_done (GDBusConnection *connection,
                                           GAsyncResult    *res,
                                           GTask           *task)
{
    GError *error = NULL;
    GVariant *variant = g_dbus_connection_call_finish (connection, res, &error);
    const gchar *path = NULL;
    IBusBus *bus;
    GCancellable *cancellable;

    if (variant == NULL) {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    if (g_dbus_connection_is_closed (connection)) {
        /*
         * The connection is closed, can not continue next steps, so complete
         * the asynchronous request with error.
         */
        g_variant_unref(variant);
        g_task_return_new_error (task,
                G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "Connection is closed.");
        return;
    }

    g_variant_get (variant, "(&o)", &path);
    g_variant_unref(variant);

    bus = (IBusBus *)g_task_get_source_object (task);
    g_assert (IBUS_IS_BUS (bus));

    cancellable = g_task_get_cancellable (task);

    ibus_input_context_new_async (path,
            bus->priv->connection,
            cancellable,
            (GAsyncReadyCallback)_create_input_context_async_step_two_done,
            task);
}

void
ibus_bus_create_input_context_async (IBusBus            *bus,
                                     const gchar        *client_name,
                                     gint                timeout_msec,
                                     GCancellable       *cancellable,
                                     GAsyncReadyCallback callback,
                                     gpointer            user_data)
{
    GTask *task;

    g_return_if_fail (IBUS_IS_BUS (bus));
    g_return_if_fail (client_name != NULL);
    g_return_if_fail (callback != NULL);

    task = g_task_new (bus, cancellable, callback, user_data);
    g_task_set_source_tag (task, ibus_bus_create_input_context_async);

    /* do not use ibus_bus_call_async, instead use g_dbus_connection_call
     * directly, because we need two async steps for create an IBusInputContext.
     * 1. Call CreateInputContext to request ibus-daemon create a remote IC.
     * 2. New local IBusInputContext proxy of the remote IC
     */
    g_dbus_connection_call (bus->priv->connection,
            ibus_bus_get_service_name (bus),
            IBUS_PATH_IBUS,
            bus->priv->use_portal ? IBUS_INTERFACE_PORTAL : IBUS_INTERFACE_IBUS,
            "CreateInputContext",
            g_variant_new ("(s)", client_name),
            G_VARIANT_TYPE("(o)"),
            G_DBUS_CALL_FLAGS_NO_AUTO_START,
            timeout_msec,
            cancellable,
            (GAsyncReadyCallback)_create_input_context_async_step_one_done,
            task);
}

IBusInputContext *
ibus_bus_create_input_context_async_finish (IBusBus      *bus,
                                            GAsyncResult *res,
                                            GError      **error)
{
    GTask *task;
    gboolean had_error;
    IBusInputContext *context = NULL;

    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_task_is_valid (res, bus));

    task = G_TASK (res);
    g_assert (g_task_get_source_tag (task) == 
                      ibus_bus_create_input_context_async);
    had_error = g_task_had_error (task);
    context = g_task_propagate_pointer (task, error);
    if (had_error) {
        g_assert (context == NULL);
        return NULL;
    }
    g_assert (IBUS_IS_INPUT_CONTEXT (context));
    return context;
}

gchar *
ibus_bus_current_input_context (IBusBus      *bus)
{
    g_return_val_if_fail (IBUS_IS_BUS (bus), NULL);

    gchar *path = NULL;
    GVariant *result;
    result = ibus_bus_call_sync (bus,
                                 IBUS_SERVICE_IBUS,
                                 IBUS_PATH_IBUS,
                                 "org.freedesktop.DBus.Properties",
                                 "Get",
                                 g_variant_new ("(ss)",
                                                IBUS_INTERFACE_IBUS,
                                                "CurrentInputContext"),
                                 G_VARIANT_TYPE ("(v)"));

    if (result != NULL) {
        GVariant *variant = NULL;
        g_variant_get (result, "(v)", &variant);
        path = g_variant_dup_string (variant, NULL);
        g_variant_unref (variant);
        g_variant_unref (result);
    }

    return path;
}

void
ibus_bus_current_input_context_async (IBusBus            *bus,
                                      gint                timeout_msec,
                                      GCancellable       *cancellable,
                                      GAsyncReadyCallback callback,
                                      gpointer            user_data)
{
    g_return_if_fail (IBUS_IS_BUS (bus));

    ibus_bus_call_async (bus,
                         IBUS_SERVICE_IBUS,
                         IBUS_PATH_IBUS,
                         "org.freedesktop.DBus.Properties",
                         "Get",
                         g_variant_new ("(ss)",
                                        IBUS_INTERFACE_IBUS,
                                        "CurrentInputContext"),
                         G_VARIANT_TYPE ("(v)"),
                         ibus_bus_current_input_context_async,
                         timeout_msec,
                         cancellable,
                         callback,
                         user_data);
}

gchar *
ibus_bus_current_input_context_async_finish (IBusBus      *bus,
                                             GAsyncResult *res,
                                             GError      **error)
{
    GTask *task;

    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_task_is_valid (res, bus));

    task = G_TASK (res);
    g_assert(g_task_get_source_tag (task) ==
                     ibus_bus_current_input_context_async);
    return _async_finish_object_path (task, error);
}

static void
ibus_bus_watch_dbus_signal (IBusBus *bus)
{
    g_return_if_fail (bus->priv->connection != NULL);
    g_return_if_fail (bus->priv->watch_dbus_signal_id == 0);

    /* Subscribe to dbus signals such as NameOwnerChanged. */
    bus->priv->watch_dbus_signal_id
        = g_dbus_connection_signal_subscribe (bus->priv->connection,
                                              DBUS_SERVICE_DBUS,
                                              DBUS_INTERFACE_DBUS,
                                              "NameOwnerChanged",
                                              DBUS_PATH_DBUS,
                                              NULL /* arg0 */,
                                              (GDBusSignalFlags) 0,
                                              _connection_dbus_signal_cb,
                                              bus,
                                              NULL /* user_data_free_func */);
    /* FIXME handle other D-Bus signals if needed */
}

static void
ibus_bus_unwatch_dbus_signal (IBusBus *bus)
{
    g_return_if_fail (bus->priv->watch_dbus_signal_id != 0);
    g_dbus_connection_signal_unsubscribe (bus->priv->connection,
                                          bus->priv->watch_dbus_signal_id);
    bus->priv->watch_dbus_signal_id = 0;
}

void
ibus_bus_set_watch_dbus_signal (IBusBus        *bus,
                                gboolean        watch)
{
    g_return_if_fail (IBUS_IS_BUS (bus));

    if (bus->priv->watch_dbus_signal == watch)
        return;

    bus->priv->watch_dbus_signal = watch;

    if (ibus_bus_is_connected (bus)) {
        if (watch) {
            ibus_bus_watch_dbus_signal (bus);
        }
        else {
            ibus_bus_unwatch_dbus_signal (bus);
        }
    }
}

static void
ibus_bus_watch_ibus_signal (IBusBus *bus)
{
    g_return_if_fail (bus->priv->connection != NULL);
    g_return_if_fail (bus->priv->watch_global_engine_changed_id == 0);

    /* Subscribe to ibus signals such as GlboalEngineChanged. */
    bus->priv->watch_global_engine_changed_id
        = g_dbus_connection_signal_subscribe (bus->priv->connection,
                                              "org.freedesktop.IBus",
                                              IBUS_INTERFACE_IBUS,
                                              "GlobalEngineChanged",
                                              IBUS_PATH_IBUS,
                                              NULL /* arg0 */,
                                              (GDBusSignalFlags) 0,
                                              _connection_ibus_signal_cb,
                                              bus,
                                              NULL /* user_data_free_func */);
    bus->priv->watch_global_shortcut_key_responded_id
        = g_dbus_connection_signal_subscribe (bus->priv->connection,
                                              "org.freedesktop.IBus",
                                              IBUS_INTERFACE_IBUS,
                                              "GlobalShortcutKeyResponded",
                                              IBUS_PATH_IBUS,
                                              NULL /* arg0 */,
                                              (GDBusSignalFlags) 0,
                                              _connection_ibus_signal_cb,
                                              bus,
                                              NULL /* user_data_free_func */);
    /* FIXME handle org.freedesktop.IBus.RegistryChanged signal if needed */
}

static void
ibus_bus_unwatch_ibus_signal (IBusBus *bus)
{
    g_return_if_fail (bus->priv->watch_global_engine_changed_id != 0);
    g_dbus_connection_signal_unsubscribe (
            bus->priv->connection,
            bus->priv->watch_global_engine_changed_id);
    bus->priv->watch_global_engine_changed_id = 0;
    g_dbus_connection_signal_unsubscribe (
            bus->priv->connection,
            bus->priv->watch_global_shortcut_key_responded_id);
    bus->priv->watch_global_shortcut_key_responded_id = 0;
}

void
ibus_bus_set_watch_ibus_signal (IBusBus        *bus,
                                gboolean        watch)
{
    g_return_if_fail (IBUS_IS_BUS (bus));

    if (bus->priv->watch_ibus_signal == watch)
        return;

    bus->priv->watch_ibus_signal = watch;

    if (ibus_bus_is_connected (bus)) {
        if (watch) {
            ibus_bus_watch_ibus_signal (bus);
        }
        else {
            ibus_bus_unwatch_ibus_signal (bus);
        }
    }
}

const gchar *
ibus_bus_hello (IBusBus *bus)
{
    g_return_val_if_fail (IBUS_IS_BUS (bus), NULL);
    g_return_val_if_fail (ibus_bus_is_connected (bus), NULL);

    /* gdbus connection will say hello by self. */
    if (bus->priv->connection)
        return g_dbus_connection_get_unique_name (bus->priv->connection);
    return NULL;
}

guint32
ibus_bus_request_name (IBusBus      *bus,
                       const gchar  *name,
                       guint32       flags)
{
    g_return_val_if_fail (IBUS_IS_BUS (bus), 0);
    g_return_val_if_fail (name != NULL, 0);

    guint32 retval = 0;
    GVariant *result;
    result = ibus_bus_call_sync (bus,
                                 DBUS_SERVICE_DBUS,
                                 DBUS_PATH_DBUS,
                                 DBUS_INTERFACE_DBUS,
                                 "RequestName",
                                 g_variant_new ("(su)", name, flags),
                                 G_VARIANT_TYPE ("(u)"));

    if (result) {
        g_variant_get (result, "(u)", &retval);
        g_variant_unref (result);
    }

    return retval;
}

void
ibus_bus_request_name_async (IBusBus            *bus,
                             const gchar        *name,
                             guint               flags,
                             gint                timeout_msec,
                             GCancellable       *cancellable,
                             GAsyncReadyCallback callback,
                             gpointer            user_data)
{
    g_return_if_fail (IBUS_IS_BUS (bus));
    g_return_if_fail (name != NULL);

    ibus_bus_call_async (bus,
                         DBUS_SERVICE_DBUS,
                         DBUS_PATH_DBUS,
                         DBUS_INTERFACE_DBUS,
                         "RequestName",
                         g_variant_new ("(su)", name, flags),
                         G_VARIANT_TYPE ("(u)"),
                         ibus_bus_request_name_async,
                         timeout_msec,
                         cancellable,
                         callback,
                         user_data);
}

guint
ibus_bus_request_name_async_finish (IBusBus      *bus,
                                    GAsyncResult *res,
                                    GError      **error)
{
    GTask *task;

    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_task_is_valid (res, bus));

    task = G_TASK (res);
    g_assert(g_task_get_source_tag (task) == ibus_bus_request_name_async);
    return _async_finish_guint (task, error);
}

guint
ibus_bus_release_name (IBusBus      *bus,
                       const gchar  *name)
{
    g_return_val_if_fail (IBUS_IS_BUS (bus), 0);
    g_return_val_if_fail (name != NULL, 0);

    guint retval = 0;
    GVariant *result;
    result = ibus_bus_call_sync (bus,
                                 DBUS_SERVICE_DBUS,
                                 DBUS_PATH_DBUS,
                                 DBUS_INTERFACE_DBUS,
                                 "ReleaseName",
                                 g_variant_new ("(s)", name),
                                 G_VARIANT_TYPE ("(u)"));

    if (result) {
        g_variant_get (result, "(u)", &retval);
        g_variant_unref (result);
    }

    return retval;
}

void
ibus_bus_release_name_async (IBusBus            *bus,
                             const gchar        *name,
                             gint                timeout_msec,
                             GCancellable       *cancellable,
                             GAsyncReadyCallback callback,
                             gpointer            user_data)
{
    g_return_if_fail (IBUS_IS_BUS (bus));
    g_return_if_fail (name != NULL);

    ibus_bus_call_async (bus,
                         DBUS_SERVICE_DBUS,
                         DBUS_PATH_DBUS,
                         DBUS_INTERFACE_DBUS,
                         "ReleaseName",
                         g_variant_new ("(s)", name),
                         G_VARIANT_TYPE ("(u)"),
                         ibus_bus_release_name_async,
                         timeout_msec,
                         cancellable,
                         callback,
                         user_data);
}

guint
ibus_bus_release_name_async_finish (IBusBus      *bus,
                                    GAsyncResult *res,
                                    GError      **error)
{
    GTask *task;

    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_task_is_valid (res, bus));

    task = G_TASK (res);
    g_assert(g_task_get_source_tag (task) == ibus_bus_release_name_async);
    return _async_finish_guint (task, error);
}

GList *
ibus_bus_list_queued_owners (IBusBus      *bus,
                             const gchar  *name)
{
    GList *retval = NULL;
    GVariant *result;

    g_return_val_if_fail (IBUS_IS_BUS (bus), NULL);
    g_return_val_if_fail (name != NULL, NULL);

    result = ibus_bus_call_sync (bus,
                                 DBUS_SERVICE_DBUS,
                                 DBUS_PATH_DBUS,
                                 DBUS_INTERFACE_DBUS,
                                 "ListQueuedOwners",
                                 g_variant_new ("(s)", name),
                                 G_VARIANT_TYPE ("(as)"));

    if (result) {
        GVariantIter *iter = NULL;
        const gchar *name = NULL;
        g_variant_get (result, "(as)", &iter);
        while (g_variant_iter_loop (iter, "&s", &name)) {
            if (name == NULL) {
                continue;
            }
            retval = g_list_append (retval, g_strdup (name));
        }
        g_variant_iter_free (iter);
        g_variant_unref (result);
    }

    return retval;
}

gboolean
ibus_bus_name_has_owner (IBusBus        *bus,
                         const gchar    *name)
{
    g_return_val_if_fail (IBUS_IS_BUS (bus), FALSE);
    g_return_val_if_fail (name != NULL, FALSE);

    gboolean retval = FALSE;
    GVariant *result;
    result = ibus_bus_call_sync (bus,
                                 DBUS_SERVICE_DBUS,
                                 DBUS_PATH_DBUS,
                                 DBUS_INTERFACE_DBUS,
                                 "NameHasOwner",
                                 g_variant_new ("(s)", name),
                                 G_VARIANT_TYPE ("(b)"));

    if (result) {
        g_variant_get (result, "(b)", &retval);
        g_variant_unref (result);
    }

    return retval;
}

void
ibus_bus_name_has_owner_async (IBusBus            *bus,
                               const gchar        *name,
                               gint                timeout_msec,
                               GCancellable       *cancellable,
                               GAsyncReadyCallback callback,
                               gpointer            user_data)
{
    g_return_if_fail (IBUS_IS_BUS (bus));
    g_return_if_fail (name != NULL);

    ibus_bus_call_async (bus,
                         DBUS_SERVICE_DBUS,
                         DBUS_PATH_DBUS,
                         DBUS_INTERFACE_DBUS,
                         "NameHasOwner",
                         g_variant_new ("(s)", name),
                         G_VARIANT_TYPE ("(b)"),
                         ibus_bus_name_has_owner_async,
                         timeout_msec,
                         cancellable,
                         callback,
                         user_data);
}

gboolean
ibus_bus_name_has_owner_async_finish (IBusBus      *bus,
                                      GAsyncResult *res,
                                      GError      **error)
{
    GTask *task;

    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_task_is_valid (res, bus));

    task = G_TASK (res);
    g_assert(g_task_get_source_tag (task) == ibus_bus_name_has_owner_async);
    return _async_finish_gboolean (task, error);
}

GList *
ibus_bus_list_names (IBusBus    *bus)
{
    g_return_val_if_fail (IBUS_IS_BUS (bus), NULL);
    return NULL;
}

gboolean
ibus_bus_add_match (IBusBus     *bus,
                    const gchar *rule)
{
    g_return_val_if_fail (IBUS_IS_BUS (bus), FALSE);
    g_return_val_if_fail (rule != NULL, FALSE);

    GVariant *result;
    result = ibus_bus_call_sync (bus,
                                 DBUS_SERVICE_DBUS,
                                 DBUS_PATH_DBUS,
                                 DBUS_INTERFACE_DBUS,
                                 "AddMatch",
                                 g_variant_new ("(s)", rule),
                                 NULL);

    if (result) {
        g_variant_unref (result);
        return TRUE;
    }
    return FALSE;
}

void
ibus_bus_add_match_async (IBusBus            *bus,
                          const gchar        *rule,
                          gint                timeout_msec,
                          GCancellable       *cancellable,
                          GAsyncReadyCallback callback,
                          gpointer            user_data)
{
    g_return_if_fail (IBUS_IS_BUS (bus));
    g_return_if_fail (rule != NULL);

    ibus_bus_call_async (bus,
                         DBUS_SERVICE_DBUS,
                         DBUS_PATH_DBUS,
                         DBUS_INTERFACE_DBUS,
                         "AddMatch",
                         g_variant_new ("(s)", rule),
                         NULL,
                         ibus_bus_add_match_async,
                         timeout_msec,
                         cancellable,
                         callback,
                         user_data);
}

gboolean
ibus_bus_add_match_async_finish (IBusBus      *bus,
                                 GAsyncResult *res,
                                 GError      **error)
{
    GTask *task;

    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_task_is_valid (res, bus));

    task = G_TASK (res);
    g_assert(g_task_get_source_tag (task) == ibus_bus_add_match_async);
    return _async_finish_void (task, error);
}

gboolean
ibus_bus_remove_match (IBusBus      *bus,
                       const gchar  *rule)
{
    g_return_val_if_fail (IBUS_IS_BUS (bus), FALSE);
    g_return_val_if_fail (rule != NULL, FALSE);

    GVariant *result;
    result = ibus_bus_call_sync (bus,
                                 DBUS_SERVICE_DBUS,
                                 DBUS_PATH_DBUS,
                                 DBUS_INTERFACE_DBUS,
                                 "RemoveMatch",
                                 g_variant_new ("(s)", rule),
                                 NULL);

    if (result) {
        g_variant_unref (result);
        return TRUE;
    }
    return FALSE;
}

void
ibus_bus_remove_match_async (IBusBus            *bus,
                             const gchar        *rule,
                             gint                timeout_msec,
                             GCancellable       *cancellable,
                             GAsyncReadyCallback callback,
                             gpointer            user_data)
{
    g_return_if_fail (IBUS_IS_BUS (bus));
    g_return_if_fail (rule != NULL);

    ibus_bus_call_async (bus,
                         DBUS_SERVICE_DBUS,
                         DBUS_PATH_DBUS,
                         DBUS_INTERFACE_DBUS,
                         "RemoveMatch",
                         g_variant_new ("(s)", rule),
                         NULL,
                         ibus_bus_remove_match_async,
                         timeout_msec,
                         cancellable,
                         callback,
                         user_data);
}

gboolean
ibus_bus_remove_match_async_finish (IBusBus      *bus,
                                    GAsyncResult *res,
                                    GError      **error)
{
    GTask *task;

    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_task_is_valid (res, bus));

    task = G_TASK (res);
    g_assert(g_task_get_source_tag (task) == ibus_bus_remove_match_async);
    return _async_finish_void (task, error);
}

gchar *
ibus_bus_get_name_owner (IBusBus        *bus,
                         const gchar    *name)
{
    g_return_val_if_fail (IBUS_IS_BUS (bus), NULL);
    g_return_val_if_fail (name != NULL, NULL);

    gchar *retval = NULL;
    GVariant *result;
    result = ibus_bus_call_sync (bus,
                                 DBUS_SERVICE_DBUS,
                                 DBUS_PATH_DBUS,
                                 DBUS_INTERFACE_DBUS,
                                 "GetNameOwner",
                                 g_variant_new ("(s)", name),
                                 G_VARIANT_TYPE ("(s)"));

    if (result) {
        g_variant_get (result, "(s)", &retval);
        g_variant_unref (result);
    }

    return retval;
}

void
ibus_bus_get_name_owner_async (IBusBus            *bus,
                               const gchar        *name,
                               gint                timeout_msec,
                               GCancellable       *cancellable,
                               GAsyncReadyCallback callback,
                               gpointer            user_data)
{
    g_return_if_fail (IBUS_IS_BUS (bus));
    g_return_if_fail (name != NULL);

    ibus_bus_call_async (bus,
                         DBUS_SERVICE_DBUS,
                         DBUS_PATH_DBUS,
                         DBUS_INTERFACE_DBUS,
                         "GetNameOwner",
                         g_variant_new ("(s)", name),
                         G_VARIANT_TYPE ("(s)"),
                         ibus_bus_get_name_owner_async,
                         timeout_msec,
                         cancellable,
                         callback,
                         user_data);
}

gchar *
ibus_bus_get_name_owner_async_finish (IBusBus      *bus,
                                      GAsyncResult *res,
                                      GError      **error)
{
    GTask *task;

    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_task_is_valid (res, bus));

    task = G_TASK (res);
    g_assert(g_task_get_source_tag (task) == ibus_bus_get_name_owner_async);
    return g_strdup (_async_finish_string (task, error));
}

GDBusConnection *
ibus_bus_get_connection (IBusBus *bus)
{
    g_return_val_if_fail (IBUS_IS_BUS (bus), NULL);

    return bus->priv->connection;
}

const gchar *
ibus_bus_get_service_name (IBusBus *bus)
{
    if (bus->priv->use_portal)
        return IBUS_SERVICE_PORTAL;
    return IBUS_SERVICE_IBUS;
}

gboolean
ibus_bus_exit (IBusBus *bus,
               gboolean restart)
{
    g_return_val_if_fail (IBUS_IS_BUS (bus), FALSE);

    GVariant *result;
    result = ibus_bus_call_sync (bus,
                                 IBUS_SERVICE_IBUS,
                                 IBUS_PATH_IBUS,
                                 IBUS_INTERFACE_IBUS,
                                 "Exit",
                                 g_variant_new ("(b)", restart),
                                 NULL);

    ibus_bus_close_connection (bus);

    if (result) {
        g_variant_unref (result);
        return TRUE;
    }
    return FALSE;
}

void
ibus_bus_exit_async (IBusBus            *bus,
                     gboolean            restart,
                     gint                timeout_msec,
                     GCancellable       *cancellable,
                     GAsyncReadyCallback callback,
                     gpointer            user_data)
{
    g_return_if_fail (IBUS_IS_BUS (bus));

    ibus_bus_call_async (bus,
                         IBUS_SERVICE_IBUS,
                         IBUS_PATH_IBUS,
                         IBUS_INTERFACE_IBUS,
                         "Exit",
                         g_variant_new ("(b)", restart),
                         NULL,
                         ibus_bus_exit_async,
                         timeout_msec,
                         cancellable,
                         callback,
                         user_data);
}

gboolean
ibus_bus_exit_async_finish (IBusBus      *bus,
                            GAsyncResult *res,
                            GError      **error)
{
    GTask *task;

    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_task_is_valid (res, bus));

    task = G_TASK (res);
    g_assert (g_task_get_source_tag (task) == ibus_bus_exit_async);
    return _async_finish_void (task, error);
}

gboolean
ibus_bus_register_component (IBusBus       *bus,
                             IBusComponent *component)
{
    g_return_val_if_fail (IBUS_IS_BUS (bus), FALSE);
    g_return_val_if_fail (IBUS_IS_COMPONENT (component), FALSE);

    GVariant *variant = ibus_serializable_serialize ((IBusSerializable *)component);
    GVariant *result = ibus_bus_call_sync (bus,
                                           IBUS_SERVICE_IBUS,
                                           IBUS_PATH_IBUS,
                                           IBUS_INTERFACE_IBUS,
                                           "RegisterComponent",
                                           g_variant_new ("(v)", variant),
                                           NULL);
    if (result) {
        g_variant_unref (result);
        return TRUE;
    }
    return FALSE;
}

void
ibus_bus_register_component_async (IBusBus            *bus,
                                   IBusComponent      *component,
                                   gint                timeout_msec,
                                   GCancellable       *cancellable,
                                   GAsyncReadyCallback callback,
                                   gpointer            user_data)
{
    g_return_if_fail (IBUS_IS_BUS (bus));
    g_return_if_fail (IBUS_IS_COMPONENT (component));

    GVariant *variant = ibus_serializable_serialize ((IBusSerializable *)component);
    ibus_bus_call_async (bus,
                         IBUS_SERVICE_IBUS,
                         IBUS_PATH_IBUS,
                         IBUS_INTERFACE_IBUS,
                         "RegisterComponent",
                         g_variant_new ("(v)", variant),
                         NULL,
                         ibus_bus_register_component_async,
                         timeout_msec,
                         cancellable,
                         callback,
                         user_data);
}

gboolean
ibus_bus_register_component_async_finish (IBusBus      *bus,
                                          GAsyncResult *res,
                                          GError      **error)
{
    GTask *task;

    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_task_is_valid (res, bus));

    task = G_TASK (res);
    g_assert (
            g_task_get_source_tag (task) == ibus_bus_register_component_async);
    return _async_finish_void (task, error);
}

static GList *
ibus_bus_do_list_engines (IBusBus *bus, gboolean active_engines_only)
{
    g_return_val_if_fail (IBUS_IS_BUS (bus), NULL);

    GList *retval = NULL;
    GVariant *result;
    const gchar *property =
            active_engines_only ? "ActiveEngines" : "Engines";
    result = ibus_bus_call_sync (bus,
                                 IBUS_SERVICE_IBUS,
                                 IBUS_PATH_IBUS,
                                 "org.freedesktop.DBus.Properties",
                                 "Get",
                                 g_variant_new ("(ss)",
                                                IBUS_INTERFACE_IBUS,
                                                property),
                                 G_VARIANT_TYPE ("(v)"));

    if (result) {
        GVariant *variant = NULL;
        GVariantIter *iter = NULL;

        g_variant_get (result, "(v)", &variant);
        iter = g_variant_iter_new (variant);
        GVariant *var;
        while (g_variant_iter_loop (iter, "v", &var)) {
            IBusSerializable *serializable = ibus_serializable_deserialize (var);
            g_object_ref_sink (serializable);
            retval = g_list_append (retval, serializable);
        }
        g_variant_iter_free (iter);
        g_variant_unref (variant);
        g_variant_unref (result);
    }

    return retval;
}

GList *
ibus_bus_list_engines (IBusBus *bus)
{
    return ibus_bus_do_list_engines (bus, FALSE);
}

void
ibus_bus_list_engines_async (IBusBus            *bus,
                             gint                timeout_msec,
                             GCancellable       *cancellable,
                             GAsyncReadyCallback callback,
                             gpointer            user_data)
{
    g_return_if_fail (IBUS_IS_BUS (bus));

    ibus_bus_call_async (bus,
                         IBUS_SERVICE_IBUS,
                         IBUS_PATH_IBUS,
                         "org.freedesktop.DBus.Properties",
                         "Get",
                         g_variant_new ("(ss)",
                                        IBUS_INTERFACE_IBUS,
                                        "Engines"),
                         G_VARIANT_TYPE ("(v)"),
                         ibus_bus_list_engines_async,
                         timeout_msec,
                         cancellable,
                         callback,
                         user_data);
}

GList *
ibus_bus_list_engines_async_finish (IBusBus      *bus,
                                    GAsyncResult *res,
                                    GError      **error)
{
    GTask *task;
    gboolean had_error;
    GVariant *result = NULL;
    GVariant *variant = NULL;
    GList *retval = NULL;
    GVariantIter *iter = NULL;
    GVariant *var;

    g_assert (g_task_is_valid (res, bus));

    task = G_TASK (res);
    had_error = g_task_had_error (task);
    result = g_task_propagate_pointer (task, error);
    if (had_error) {
        g_assert (result == NULL);
        return NULL;
    }
    g_return_val_if_fail (result != NULL, NULL);

    g_variant_get (result, "(v)", &variant);
    iter = g_variant_iter_new (variant);
    while (g_variant_iter_loop (iter, "v", &var)) {
        IBusSerializable *serializable = ibus_serializable_deserialize (var);
        g_object_ref_sink (serializable);
        retval = g_list_append (retval, serializable);
    }
    g_variant_iter_free (iter);
    g_variant_unref (variant);
    g_variant_unref (result);
    return retval;
}

#ifndef IBUS_DISABLE_DEPRECATED
GList *
ibus_bus_list_active_engines (IBusBus *bus)
{
    return ibus_bus_do_list_engines (bus, TRUE);
}

void
ibus_bus_list_active_engines_async (IBusBus            *bus,
                                    gint                timeout_msec,
                                    GCancellable       *cancellable,
                                    GAsyncReadyCallback callback,
                                    gpointer            user_data)
{
    g_return_if_fail (IBUS_IS_BUS (bus));

    ibus_bus_call_async (bus,
                         IBUS_SERVICE_IBUS,
                         IBUS_PATH_IBUS,
                         "org.freedesktop.DBus.Properties",
                         "Get",
                         g_variant_new ("(ss)",
                                        IBUS_INTERFACE_IBUS,
                                        "ActiveEngines"),
                         G_VARIANT_TYPE ("(v)"),
                         ibus_bus_list_active_engines_async,
                         timeout_msec,
                         cancellable,
                         callback,
                         user_data);
}

GList *
ibus_bus_list_active_engines_async_finish (IBusBus      *bus,
                                           GAsyncResult *res,
                                           GError      **error)
{
    return ibus_bus_list_engines_async_finish (bus, res, error);
}
#endif /* IBUS_DISABLE_DEPRECATED */

IBusEngineDesc **
ibus_bus_get_engines_by_names (IBusBus             *bus,
                               const gchar * const *names)
{
    g_return_val_if_fail (IBUS_IS_BUS (bus), NULL);

    GVariant *result;
    result = ibus_bus_call_sync (bus,
                                 IBUS_SERVICE_IBUS,
                                 IBUS_PATH_IBUS,
                                 IBUS_INTERFACE_IBUS,
                                 "GetEnginesByNames",
                                 g_variant_new("(^as)", names),
                                 G_VARIANT_TYPE ("(av)"));
    if (result == NULL)
        return NULL;

    GArray *array = g_array_new (TRUE, TRUE, sizeof (IBusEngineDesc *));
    GVariantIter *iter = NULL;
    g_variant_get (result, "(av)", &iter);
    GVariant *var;
    while (g_variant_iter_loop (iter, "v", &var)) {
        IBusEngineDesc *desc = (IBusEngineDesc *) ibus_serializable_deserialize (var);
        g_object_ref_sink (desc);
        g_array_append_val (array, desc);
    }
    g_variant_iter_free (iter);
    g_variant_unref (result);

    return (IBusEngineDesc **)g_array_free (array, FALSE);
}

static void
_config_destroy_cb (IBusConfig *config,
                    IBusBus    *bus)
{
    IBusBusPrivate *priv;
    priv = IBUS_BUS_GET_PRIVATE (bus);

    g_assert (priv->config == config);

    g_object_unref (config);
    priv->config = NULL;
}

IBusConfig *
ibus_bus_get_config (IBusBus *bus)
{
    g_assert (IBUS_IS_BUS (bus));
    g_return_val_if_fail (ibus_bus_is_connected (bus), NULL);

    IBusBusPrivate *priv;
    priv = IBUS_BUS_GET_PRIVATE (bus);

    if (priv->config == NULL && priv->connection) {
        priv->config = ibus_config_new (priv->connection, NULL, NULL);
        if (priv->config) {
            g_signal_connect (priv->config, "destroy", G_CALLBACK (_config_destroy_cb), bus);
        }
    }

    return priv->config;
}

#ifndef IBUS_DISABLE_DEPRECATED
gboolean
ibus_bus_get_use_sys_layout (IBusBus *bus)
{
    g_return_val_if_fail (IBUS_IS_BUS (bus), FALSE);

    gboolean retval = FALSE;
    GVariant *result;
    result = ibus_bus_call_sync (bus,
                                 IBUS_SERVICE_IBUS,
                                 IBUS_PATH_IBUS,
                                 IBUS_INTERFACE_IBUS,
                                 "GetUseSysLayout",
                                 NULL,
                                 G_VARIANT_TYPE ("(b)"));

    if (result) {
        g_variant_get (result, "(b)", &retval);
        g_variant_unref (result);
    }

    return retval;
}

void
ibus_bus_get_use_sys_layout_async (IBusBus            *bus,
                                   gint                timeout_msec,
                                   GCancellable       *cancellable,
                                   GAsyncReadyCallback callback,
                                   gpointer            user_data)
{
    g_return_if_fail (IBUS_IS_BUS (bus));

    ibus_bus_call_async (bus,
                         IBUS_SERVICE_IBUS,
                         IBUS_PATH_IBUS,
                         IBUS_INTERFACE_IBUS,
                         "GetUseSysLayout",
                         NULL,
                         G_VARIANT_TYPE ("(b)"),
                         ibus_bus_get_use_sys_layout_async,
                         timeout_msec,
                         cancellable,
                         callback,
                         user_data);
}

gboolean
ibus_bus_get_use_sys_layout_async_finish (IBusBus      *bus,
                                          GAsyncResult *res,
                                          GError      **error)
{
    GTask *task;

    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_task_is_valid (res, bus));

    task = G_TASK (res);
    g_assert(g_task_get_source_tag (task) == ibus_bus_get_use_sys_layout_async);
    return _async_finish_gboolean (task, error);
}

gboolean
ibus_bus_get_use_global_engine (IBusBus *bus)
{
    g_return_val_if_fail (IBUS_IS_BUS (bus), FALSE);

    gboolean retval = FALSE;
    GVariant *result;
    result = ibus_bus_call_sync (bus,
                                 IBUS_SERVICE_IBUS,
                                 IBUS_PATH_IBUS,
                                 IBUS_INTERFACE_IBUS,
                                 "GetUseGlobalEngine",
                                 NULL,
                                 G_VARIANT_TYPE ("(b)"));

    if (result) {
        g_variant_get (result, "(b)", &retval);
        g_variant_unref (result);
    }

    return retval;
}

void
ibus_bus_get_use_global_engine_async (IBusBus            *bus,
                                      gint                timeout_msec,
                                      GCancellable       *cancellable,
                                      GAsyncReadyCallback callback,
                                      gpointer            user_data)
{
    g_return_if_fail (IBUS_IS_BUS (bus));

    ibus_bus_call_async (bus,
                         IBUS_SERVICE_IBUS,
                         IBUS_PATH_IBUS,
                         IBUS_INTERFACE_IBUS,
                         "GetUseGlobalEngine",
                         NULL,
                         G_VARIANT_TYPE ("(b)"),
                         ibus_bus_get_use_global_engine_async,
                         timeout_msec,
                         cancellable,
                         callback,
                         user_data);
}

gboolean
ibus_bus_get_use_global_engine_async_finish (IBusBus      *bus,
                                             GAsyncResult *res,
                                             GError      **error)
{
    GTask *task;

    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_task_is_valid (res, bus));

    task = G_TASK (res);
    g_assert(g_task_get_source_tag (task) ==
                     ibus_bus_get_use_global_engine_async);
    return _async_finish_gboolean (task, error);
}

gboolean
ibus_bus_is_global_engine_enabled (IBusBus *bus)
{
    g_return_val_if_fail (IBUS_IS_BUS (bus), FALSE);

    gboolean retval = FALSE;
    GVariant *result;
    result = ibus_bus_call_sync (bus,
                                 IBUS_SERVICE_IBUS,
                                 IBUS_PATH_IBUS,
                                 IBUS_INTERFACE_IBUS,
                                 "IsGlobalEngineEnabled",
                                 NULL,
                                 G_VARIANT_TYPE ("(b)"));

    if (result) {
        g_variant_get (result, "(b)", &retval);
        g_variant_unref (result);
    }

    return retval;
}

void ibus_bus_is_global_engine_enabled_async (IBusBus            *bus,
                                              gint                timeout_msec,
                                              GCancellable       *cancellable,
                                              GAsyncReadyCallback callback,
                                              gpointer            user_data)
{
    g_return_if_fail (IBUS_IS_BUS (bus));

    ibus_bus_call_async (bus,
                         IBUS_SERVICE_IBUS,
                         IBUS_PATH_IBUS,
                         IBUS_INTERFACE_IBUS,
                         "IsGlobalEngineEnabled",
                         NULL,
                         G_VARIANT_TYPE ("(b)"),
                         ibus_bus_is_global_engine_enabled_async,
                         timeout_msec,
                         cancellable,
                         callback,
                         user_data);
}

gboolean ibus_bus_is_global_engine_enabled_async_finish (IBusBus      *bus,
                                                         GAsyncResult *res,
                                                         GError      **error)
{
    GTask *task;

    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_task_is_valid (res, bus));

    task = G_TASK (res);
    g_assert(g_task_get_source_tag (task) ==
                     ibus_bus_is_global_engine_enabled_async);
    return _async_finish_gboolean (task, error);
}
#endif /* IBUS_DISABLE_DEPRECATED */

IBusEngineDesc *
ibus_bus_get_global_engine (IBusBus *bus)
{
    g_return_val_if_fail (IBUS_IS_BUS (bus), NULL);

    GVariant *result;
    IBusEngineDesc *engine = NULL;
    result = ibus_bus_call_sync (bus,
                                 IBUS_SERVICE_IBUS,
                                 IBUS_PATH_IBUS,
                                 "org.freedesktop.DBus.Properties",
                                 "Get",
                                 g_variant_new ("(ss)",
                                                IBUS_INTERFACE_IBUS,
                                                "GlobalEngine"),
                                 G_VARIANT_TYPE ("(v)"));

    if (result) {
        GVariant *variant = NULL;
        g_variant_get (result, "(v)", &variant);
        if (variant) {
            GVariant *obj = g_variant_get_variant (variant);
            engine = IBUS_ENGINE_DESC (ibus_serializable_deserialize (obj));
            g_variant_unref (obj);
            g_variant_unref (variant);
        }
        g_variant_unref (result);
    }

    return engine;
}

void
ibus_bus_get_global_engine_async (IBusBus            *bus,
                                  gint                timeout_msec,
                                  GCancellable       *cancellable,
                                  GAsyncReadyCallback callback,
                                  gpointer            user_data)
{
    g_return_if_fail (IBUS_IS_BUS (bus));

    ibus_bus_call_async (bus,
                         IBUS_SERVICE_IBUS,
                         IBUS_PATH_IBUS,
                         "org.freedesktop.DBus.Properties",
                         "Get",
                         g_variant_new ("(ss)",
                                        IBUS_INTERFACE_IBUS,
                                        "GlobalEngine"),
                         G_VARIANT_TYPE ("(v)"),
                         ibus_bus_get_global_engine_async,
                         timeout_msec,
                         cancellable,
                         callback,
                         user_data);
}

IBusEngineDesc *
ibus_bus_get_global_engine_async_finish (IBusBus      *bus,
                                         GAsyncResult *res,
                                         GError      **error)
{
    GTask *task;
    gboolean had_error;
    GVariant *result = NULL;

    g_assert (g_task_is_valid (res, bus));

    task = G_TASK (res);
    had_error = g_task_had_error (task);
    result = g_task_propagate_pointer (task, error);
    if (had_error) {
        g_assert (result == NULL);
        return NULL;
    }
    g_return_val_if_fail (result != NULL, NULL);
    GVariant *variant = NULL;
    g_variant_get (result, "(v)", &variant);

    IBusEngineDesc *engine = NULL;
    if (variant) {
        GVariant *obj = g_variant_get_variant (variant);
        engine = IBUS_ENGINE_DESC (ibus_serializable_deserialize (obj));
        g_variant_unref (obj);
        g_variant_unref (variant);
    }
    g_variant_unref (result);
    return engine;
}

gboolean
ibus_bus_set_global_engine (IBusBus     *bus,
                            const gchar *global_engine)
{
    g_return_val_if_fail (IBUS_IS_BUS (bus), FALSE);
    g_return_val_if_fail (global_engine != NULL, FALSE);

    GVariant *result;
    result = ibus_bus_call_sync (bus,
                                 IBUS_SERVICE_IBUS,
                                 IBUS_PATH_IBUS,
                                 IBUS_INTERFACE_IBUS,
                                 "SetGlobalEngine",
                                 g_variant_new ("(s)", global_engine),
                                 NULL);

    if (result) {
        g_variant_unref (result);
        return TRUE;
    }
    return FALSE;
}

void
ibus_bus_set_global_engine_async (IBusBus            *bus,
                                  const gchar        *global_engine,
                                  gint                timeout_msec,
                                  GCancellable       *cancellable,
                                  GAsyncReadyCallback callback,
                                  gpointer            user_data)
{
    g_return_if_fail (IBUS_IS_BUS (bus));
    g_return_if_fail (global_engine != NULL);

    ibus_bus_call_async (bus,
                         IBUS_SERVICE_IBUS,
                         IBUS_PATH_IBUS,
                         IBUS_INTERFACE_IBUS,
                         "SetGlobalEngine",
                         g_variant_new ("(s)", global_engine),
                         NULL, /* no return value */
                         ibus_bus_set_global_engine_async,
                         timeout_msec,
                         cancellable,
                         callback,
                         user_data);
}

gboolean
ibus_bus_set_global_engine_async_finish (IBusBus      *bus,
                                         GAsyncResult *res,
                                         GError      **error)
{
    GTask *task;

    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_task_is_valid (res, bus));

    task = G_TASK (res);
    g_assert (g_task_get_source_tag (task) == ibus_bus_set_global_engine_async);
    return _async_finish_void (task, error);
}

gboolean
ibus_bus_preload_engines (IBusBus             *bus,
                          const gchar * const *names)
{
    GVariant *result;
    GVariant *variant = NULL;

    g_return_val_if_fail (IBUS_IS_BUS (bus), FALSE);
    g_return_val_if_fail (names != NULL && names[0] != NULL, FALSE);

    variant = g_variant_new_strv(names, -1);
    result = ibus_bus_call_sync (bus,
                                 IBUS_SERVICE_IBUS,
                                 IBUS_PATH_IBUS,
                                 "org.freedesktop.DBus.Properties",
                                 "Set",
                                 g_variant_new ("(ssv)",
                                                IBUS_INTERFACE_IBUS,
                                                "PreloadEngines",
                                                variant),
                                 NULL);

    if (result) {
        g_variant_unref (result);
        return TRUE;
    }

    return FALSE;
}

void
ibus_bus_preload_engines_async (IBusBus             *bus,
                                const gchar * const *names,
                                gint                 timeout_msec,
                                GCancellable        *cancellable,
                                GAsyncReadyCallback  callback,
                                gpointer             user_data)
{
    GVariant *variant = NULL;

    g_return_if_fail (IBUS_IS_BUS (bus));
    g_return_if_fail (names != NULL && names[0] != NULL);

    variant = g_variant_new_strv(names, -1);
    ibus_bus_call_async (bus,
                         IBUS_SERVICE_IBUS,
                         IBUS_PATH_IBUS,
                         "org.freedesktop.DBus.Properties",
                         "Set",
                         g_variant_new ("(ssv)",
                                        IBUS_INTERFACE_IBUS,
                                        "PreloadEngines",
                                        variant),
                         NULL, /* no return value */
                         ibus_bus_preload_engines_async,
                         timeout_msec,
                         cancellable,
                         callback,
                         user_data);
}

gboolean
ibus_bus_preload_engines_async_finish (IBusBus       *bus,
                                       GAsyncResult  *res,
                                       GError       **error)
{
    GTask *task;

    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_task_is_valid (res, bus));

    task = G_TASK (res);
    g_assert (g_task_get_source_tag (task) == ibus_bus_preload_engines_async);
    return _async_finish_void (task, error);
}

static GVariant *
ibus_process_key_event_data_list_serialize (
        IBusBusGlobalBindingType       gtype,
        const IBusProcessKeyEventData *keys)
{
    GVariantBuilder builder, array;
    gsize i;

    g_variant_builder_init (&builder, G_VARIANT_TYPE_TUPLE);
    g_variant_builder_add (&builder, "y", gtype);
    g_variant_builder_init (&array, G_VARIANT_TYPE ("a(uuu)"));
    for (i = 0; keys[i].keyval; ++i) {
        g_variant_builder_add (&array, "(uuu)",
                               keys[i].keyval,
                               keys[i].keycode,
                               keys[i].state);
    }
    g_variant_builder_add (&builder, "a(uuu)", &array);
    return g_variant_builder_end (&builder);
}

gboolean
ibus_bus_set_global_shortcut_keys (IBusBus                       *bus,
                                   IBusBusGlobalBindingType       gtype,
                                   const IBusProcessKeyEventData *keys)
{
    GVariant *result;
    GVariant *variant = NULL;

    g_return_val_if_fail (IBUS_IS_BUS (bus), FALSE);
    g_return_val_if_fail (keys != NULL && keys[0].keyval != 0, FALSE);

    variant = ibus_process_key_event_data_list_serialize (gtype, keys);
    result = ibus_bus_call_sync (bus,
                                 IBUS_SERVICE_IBUS,
                                 IBUS_PATH_IBUS,
                                 "org.freedesktop.DBus.Properties",
                                 "Set",
                                 g_variant_new ("(ssv)",
                                                IBUS_INTERFACE_IBUS,
                                                "GlobalShortcutKeys",
                                                variant),
                                 NULL);

    if (result) {
        g_variant_unref (result);
        return TRUE;
    }

    return FALSE;
}

void
ibus_bus_set_global_shortcut_keys_async (IBusBus                       *bus,
                                         IBusBusGlobalBindingType       gtype,
                                         const IBusProcessKeyEventData *keys,
                                         gint
                                                                   timeout_msec,
                                         GCancellable
                                                                  *cancellable,
                                         GAsyncReadyCallback
                                                                   callback,
                                         gpointer                  user_data)
{
    GVariant *variant = NULL;

    g_return_if_fail (IBUS_IS_BUS (bus));
    g_return_if_fail (keys != NULL && keys[0].keyval != 0);

    variant = ibus_process_key_event_data_list_serialize (gtype, keys);
    ibus_bus_call_async (bus,
                         IBUS_SERVICE_IBUS,
                         IBUS_PATH_IBUS,
                         "org.freedesktop.DBus.Properties",
                         "Set",
                         g_variant_new ("(ssv)",
                                        IBUS_INTERFACE_IBUS,
                                        "GlobalShortcutKeys",
                                        variant),
                         NULL, /* no return value */
                         ibus_bus_set_global_shortcut_keys_async,
                         timeout_msec,
                         cancellable,
                         callback,
                         user_data);
}

gboolean
ibus_bus_set_global_shortcut_keys_async_finish (IBusBus       *bus,
                                                GAsyncResult  *res,
                                                GError       **error)
{
    GTask *task;

    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_task_is_valid (res, bus));

    task = G_TASK (res);
    g_assert (g_task_get_source_tag (task) ==
              ibus_bus_set_global_shortcut_keys_async);
    return _async_finish_void (task, error);
}

GVariant *
ibus_bus_get_ibus_property (IBusBus     *bus,
                            const gchar *property_name)
{
    GVariant *result;
    GVariant *retval = NULL;

    g_return_val_if_fail (IBUS_IS_BUS (bus), NULL);
    g_return_val_if_fail (property_name != NULL, NULL);

    result = ibus_bus_call_sync (bus,
                                 IBUS_SERVICE_IBUS,
                                 IBUS_PATH_IBUS,
                                 "org.freedesktop.DBus.Properties",
                                 "Get",
                                 g_variant_new ("(ss)",
                                                IBUS_INTERFACE_IBUS,
                                                property_name),
                                 G_VARIANT_TYPE ("(v)"));

    if (result) {
        g_variant_get (result, "(v)", &retval);
        g_variant_unref (result);
    }

    return retval;
}

void
ibus_bus_get_ibus_property_async (IBusBus            *bus,
                                  const gchar        *property_name,
                                  gint                timeout_msec,
                                  GCancellable       *cancellable,
                                  GAsyncReadyCallback callback,
                                  gpointer            user_data)
{
    g_return_if_fail (IBUS_IS_BUS (bus));
    g_return_if_fail (property_name != NULL);

    ibus_bus_call_async (bus,
                         IBUS_SERVICE_IBUS,
                         IBUS_PATH_IBUS,
                         "org.freedesktop.DBus.Properties",
                         "Get",
                         g_variant_new ("(ss)",
                                        IBUS_INTERFACE_IBUS,
                                        property_name),
                         G_VARIANT_TYPE ("(v)"),
                         ibus_bus_get_ibus_property_async,
                         timeout_msec,
                         cancellable,
                         callback,
                         user_data);
}

GVariant *
ibus_bus_get_ibus_property_async_finish (IBusBus      *bus,
                                         GAsyncResult *res,
                                         GError      **error)
{
    GTask *task;
    gboolean had_error;
    GVariant *result = NULL;

    g_assert (g_task_is_valid (res, bus));

    task = G_TASK (res);
    had_error = g_task_had_error (task);
    result = g_task_propagate_pointer (task, error);
    if (had_error) {
        g_assert (result == NULL);
        return NULL;
    }
    g_return_val_if_fail (result != NULL, NULL);
    GVariant *retval = NULL;
    g_variant_get (result, "(v)", &retval);
    g_variant_unref (result);

    return retval;
}

void
ibus_bus_set_ibus_property (IBusBus     *bus,
                            const gchar *property_name,
                            GVariant    *value)
{
    GVariant *result;

    g_return_if_fail (IBUS_IS_BUS (bus));
    g_return_if_fail (property_name != NULL);

    result = ibus_bus_call_sync (bus,
                                 IBUS_SERVICE_IBUS,
                                 IBUS_PATH_IBUS,
                                 "org.freedesktop.DBus.Properties",
                                 "Set",
                                 g_variant_new ("(ssv)",
                                                IBUS_INTERFACE_IBUS,
                                                property_name,
                                                value),
                                 NULL);

    if (result) {
        g_variant_unref (result);
    }
}

void
ibus_bus_set_ibus_property_async (IBusBus             *bus,
                                  const gchar         *property_name,
                                  GVariant            *value,
                                  gint                 timeout_msec,
                                  GCancellable        *cancellable,
                                  GAsyncReadyCallback  callback,
                                  gpointer             user_data)
{
    g_return_if_fail (IBUS_IS_BUS (bus));
    g_return_if_fail (property_name != NULL);

    ibus_bus_call_async (bus,
                         IBUS_SERVICE_IBUS,
                         IBUS_PATH_IBUS,
                         "org.freedesktop.DBus.Properties",
                         "Set",
                         g_variant_new ("(ssv)",
                                        IBUS_INTERFACE_IBUS,
                                        property_name,
                                        value),
                         NULL, /* no return value */
                         ibus_bus_set_ibus_property_async,
                         timeout_msec,
                         cancellable,
                         callback,
                         user_data);
}

gboolean
ibus_bus_set_ibus_property_async_finish (IBusBus       *bus,
                                         GAsyncResult  *res,
                                         GError       **error)
{
    GTask *task;
    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_task_is_valid (res, bus));

    task = G_TASK (res);
    g_return_val_if_fail (
            g_task_get_source_tag (task) == ibus_bus_set_ibus_property_async,
            FALSE);
    return _async_finish_void (task, error);
}

static GVariant *
ibus_bus_call_sync (IBusBus            *bus,
                    const gchar        *bus_name,
                    const gchar        *path,
                    const gchar        *interface,
                    const gchar        *member,
                    GVariant           *parameters,
                    const GVariantType *reply_type)
{
    g_assert (IBUS_IS_BUS (bus));
    g_assert (member != NULL);
    g_return_val_if_fail (ibus_bus_is_connected (bus), NULL);

    if (bus->priv->use_portal &&
        g_strcmp0 (bus_name, IBUS_SERVICE_IBUS) == 0)  {
        bus_name = IBUS_SERVICE_PORTAL;
        if (g_strcmp0 (interface, IBUS_INTERFACE_IBUS) == 0)
            interface = IBUS_INTERFACE_PORTAL;
    }

    GError *error = NULL;
    GVariant *result;
    result = g_dbus_connection_call_sync (bus->priv->connection,
                                          bus_name,
                                          path,
                                          interface,
                                          member,
                                          parameters,
                                          reply_type,
                                          G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                          ibus_get_timeout (),
                                          NULL,
                                          &error);

    if (result == NULL) {
        g_warning ("ibus_bus_call_sync: %s.%s: %s", interface, member, error->message);
        g_error_free (error);
        return NULL;
    }

    return result;
}

static void
ibus_bus_call_async_done (GDBusConnection *connection,
                          GAsyncResult    *res,
                          gpointer         user_data)
{
    GTask *task;
    GError *error = NULL;
    GVariant *variant;

    g_assert (G_IS_DBUS_CONNECTION (connection));

    task = (GTask* ) user_data;
    variant = g_dbus_connection_call_finish (connection, res, &error);

    if (variant == NULL)
        g_task_return_error (task, error);
    else
        g_task_return_pointer (task, variant, (GDestroyNotify) g_variant_unref);
    g_object_unref (task);
}

static void
ibus_bus_call_async (IBusBus            *bus,
                     const gchar        *bus_name,
                     const gchar        *path,
                     const gchar        *interface,
                     const gchar        *member,
                     GVariant           *parameters,
                     const GVariantType *reply_type,
                     gpointer            source_tag,
                     gint                timeout_msec,
                     GCancellable       *cancellable,
                     GAsyncReadyCallback callback,
                     gpointer            user_data)
{
    GTask *task;

    g_assert (IBUS_IS_BUS (bus));
    g_assert (member != NULL);
    g_return_if_fail (ibus_bus_is_connected (bus));

    task = g_task_new (bus, cancellable, callback, user_data);
    g_task_set_source_tag (task, source_tag);

    if (bus->priv->use_portal &&
        g_strcmp0 (bus_name, IBUS_SERVICE_IBUS) == 0)  {
        bus_name = IBUS_SERVICE_PORTAL;
        if (g_strcmp0 (interface, IBUS_INTERFACE_IBUS) == 0)
            interface = IBUS_INTERFACE_PORTAL;
    }

    g_dbus_connection_call (bus->priv->connection,
                            bus_name,
                            path,
                            interface,
                            member,
                            parameters,
                            reply_type,
                            G_DBUS_CALL_FLAGS_NO_AUTO_START,
                            timeout_msec,
                            cancellable,
                            (GAsyncReadyCallback) ibus_bus_call_async_done,
                            task);
}
