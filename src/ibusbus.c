/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2008-2010 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "ibusbus.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include "ibusmarshalers.h"
#include "ibusinternal.h"
#include "ibusshare.h"
#include "ibusenginedesc.h"
#include "ibusserializable.h"
#include "ibusconfig.h"

#define IBUS_BUS_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), IBUS_TYPE_BUS, IBusBusPrivate))

enum {
    CONNECTED,
    DISCONNECTED,
    GLOBAL_ENGINE_CHANGED,
    NAME_OWNER_CHANGED,
    LAST_SIGNAL,
};

enum {
    PROP_0 = 0,
    PROP_CONNECT_ASYNC,
};

/* IBusBusPriv */
struct _IBusBusPrivate {
    GFileMonitor *monitor;
    GDBusConnection *connection;
    gboolean watch_dbus_signal;
    guint watch_dbus_signal_id;
    gboolean watch_ibus_signal;
    guint watch_ibus_signal_id;
    IBusConfig *config;
    gchar *unique_name;
    gboolean connect_async;
    gchar *bus_address;
    GCancellable *cancellable;
};

static guint    bus_signals[LAST_SIGNAL] = { 0 };

static IBusBus *_bus = NULL;

/* functions prototype */
static GObject  *ibus_bus_constructor           (GType                   type,
                                                 guint                   n_params,
                                                 GObjectConstructParam  *params);
static void      ibus_bus_destroy               (IBusObject             *object);
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

G_DEFINE_TYPE (IBusBus, ibus_bus, IBUS_TYPE_OBJECT)

static void
ibus_bus_class_init (IBusBusClass *class)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (class);
    IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (class);

    gobject_class->constructor = ibus_bus_constructor;
    gobject_class->set_property = (GObjectSetPropertyFunc) ibus_bus_set_property;
    gobject_class->get_property = (GObjectGetPropertyFunc) ibus_bus_get_property;
    ibus_object_class->destroy = ibus_bus_destroy;

    /* install properties */
    /**
     * IBusBus:connect-async:
     *
     * Whether the #IBusBus object should connect asynchronously to the bus.
     *
     */
    g_object_class_install_property (gobject_class,
                                     PROP_CONNECT_ASYNC,
                                     g_param_spec_boolean ("connect-async",
                                                           "Connect Async",
                                                           "Connect asynchronously to the bus",
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

    g_type_class_add_private (class, sizeof (IBusBusPrivate));
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

    if (g_strcmp0 (signal_name, "GlobalEngineChanged") == 0) {
        gchar *engine_name = NULL;
        g_variant_get (parameters, "(&s)", &engine_name);
        g_signal_emit (IBUS_BUS (user_data),
                       bus_signals[GLOBAL_ENGINE_CHANGED], 0,
                       engine_name);
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
    bus->priv->watch_ibus_signal_id = 0;

    g_free (bus->priv->bus_address);
    bus->priv->bus_address = NULL;

    /* Cancel ongoing connect request. */
    g_cancellable_cancel (bus->priv->cancellable);
    g_cancellable_reset (bus->priv->cancellable);

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
        ibus_bus_connect_completed (bus);
    }
    else {
        g_free (bus->priv->bus_address);
        bus->priv->bus_address = NULL;
    }

    /* unref the ref from ibus_bus_connect */
    g_object_unref (bus);
}

static void
ibus_bus_connect_async (IBusBus *bus)
{
    const gchar *bus_address = ibus_get_address ();

    if (bus_address == NULL)
        return;

    if (g_strcmp0 (bus->priv->bus_address, bus_address) == 0)
        return;

    /* Close current connection and cancel ongoing connect request. */
    ibus_bus_close_connection (bus);

    bus->priv->bus_address = g_strdup (bus_address);
    g_object_ref (bus);
    g_dbus_connection_new_for_address (
            bus_address,
            G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
            G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
            NULL,
            bus->priv->cancellable,
            _bus_connect_async_cb, bus);
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
    gchar *path;
    GFile *file;

    bus->priv = IBUS_BUS_GET_PRIVATE (bus);

    bus->priv->config = NULL;
    bus->priv->connection = NULL;
    bus->priv->watch_dbus_signal = FALSE;
    bus->priv->watch_dbus_signal_id = 0;
    bus->priv->watch_ibus_signal = FALSE;
    bus->priv->watch_ibus_signal_id = 0;
    bus->priv->unique_name = NULL;
    bus->priv->connect_async = FALSE;
    bus->priv->bus_address = NULL;
    bus->priv->cancellable = g_cancellable_new ();

    path = g_path_get_dirname (ibus_get_socket_path ());

    g_mkdir_with_parents (path, 0700);
    g_chmod (path, 0700);

    if (stat (path, &buf) == 0) {
        if (buf.st_uid != getuid ()) {
            g_warning ("The owner of %s is not %s!", path, ibus_get_user_name ());
            return;
        }
    }

    file = g_file_new_for_path (ibus_get_socket_path ());
    bus->priv->monitor = g_file_monitor_file (file, 0, NULL, NULL);

    g_signal_connect (bus->priv->monitor, "changed", (GCallback) _changed_cb, bus);

    g_object_unref (file);
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
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (bus, prop_id, pspec);
    }
}

static GObject*
ibus_bus_constructor (GType                  type,
                      guint                  n_params,
                      GObjectConstructParam *params)
{
    GObject *object;

    /* share one IBusBus instance in whole application */
    if (_bus == NULL) {
        object = G_OBJECT_CLASS (ibus_bus_parent_class)->constructor (type, n_params, params);
        /* make bus object sink */
        g_object_ref_sink (object);
        _bus = IBUS_BUS (object);

        if (_bus->priv->connect_async)
            ibus_bus_connect_async (_bus);
        else
            ibus_bus_connect (_bus);
    }
    else {
        object = g_object_ref (_bus);
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

    IBUS_OBJECT_CLASS (ibus_bus_parent_class)->destroy (object);
}

static gboolean
_async_finish_void (GAsyncResult *res,
                    GError      **error)
{
    GSimpleAsyncResult *simple = (GSimpleAsyncResult *) res;
    if (g_simple_async_result_propagate_error (simple, error))
        return FALSE;
    return TRUE;
}

static gchar *
_async_finish_object_path (GAsyncResult *res,
                           GError      **error)
{
    GSimpleAsyncResult *simple = (GSimpleAsyncResult *) res;
    if (g_simple_async_result_propagate_error (simple, error))
        return NULL;
    GVariant *variant = g_simple_async_result_get_op_res_gpointer (simple);
    g_return_val_if_fail (variant != NULL, NULL);
    gchar *path = NULL;
    g_variant_get (variant, "(&o)", &path);
    return path;
}

static gchar *
_async_finish_string (GAsyncResult *res,
                      GError      **error)
{
    GSimpleAsyncResult *simple = (GSimpleAsyncResult *) res;
    if (g_simple_async_result_propagate_error (simple, error))
        return NULL;
    GVariant *variant = g_simple_async_result_get_op_res_gpointer (simple);
    g_return_val_if_fail (variant != NULL, NULL);
    gchar *s = NULL;
    g_variant_get (variant, "(&s)", &s);
    return s;
}

static gboolean
_async_finish_gboolean (GAsyncResult *res,
                        GError      **error)
{
    GSimpleAsyncResult *simple = (GSimpleAsyncResult *) res;
    if (g_simple_async_result_propagate_error (simple, error))
        return FALSE;
    GVariant *variant = g_simple_async_result_get_op_res_gpointer (simple);
    g_return_val_if_fail (variant != NULL, FALSE);
    gboolean retval = FALSE;
    g_variant_get (variant, "(b)", &retval);
    return retval;
}

static guint
_async_finish_guint (GAsyncResult *res,
                     GError      **error)
{
    static const guint bad_id = 0;
    GSimpleAsyncResult *simple = (GSimpleAsyncResult *) res;
    if (g_simple_async_result_propagate_error (simple, error))
        return bad_id;
    GVariant *variant = g_simple_async_result_get_op_res_gpointer (simple);
    g_return_val_if_fail (variant != NULL, bad_id);

    guint id = 0;
    g_variant_get (variant, "(u)", &id);
    return id;
}

IBusBus *
ibus_bus_new (void)
{
    IBusBus *bus = IBUS_BUS (g_object_new (IBUS_TYPE_BUS,
                                           "connect-async", FALSE,
                                           NULL));

    return bus;
}

IBusBus *
ibus_bus_new_async (void)
{
    IBusBus *bus = IBUS_BUS (g_object_new (IBUS_TYPE_BUS,
                                           "connect-async", TRUE,
                                           NULL));

    return bus;
}

gboolean
ibus_bus_is_connected (IBusBus *bus)
{
    g_return_val_if_fail (IBUS_IS_BUS (bus), FALSE);

    if (bus->priv->connection == NULL || g_dbus_connection_is_closed (bus->priv->connection))
        return FALSE;

    return TRUE;
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
_create_input_context_async_step_two_done (GObject            *source_object,
                                           GAsyncResult       *res,
                                           GSimpleAsyncResult *simple)
{
    GError *error = NULL;
    IBusInputContext *context =
            ibus_input_context_new_async_finish (res, &error);
    if (context == NULL) {
        g_simple_async_result_set_from_error (simple, error);
        g_error_free (error);
    }
    else {
        g_simple_async_result_set_op_res_gpointer (simple, context, NULL);
    }
    g_simple_async_result_complete_in_idle (simple);
    g_object_unref (simple);
}

static void
_create_input_context_async_step_one_done (GDBusConnection    *connection,
                                           GAsyncResult       *res,
                                           GSimpleAsyncResult *simple)
{
    GError *error = NULL;
    GVariant *variant = g_dbus_connection_call_finish (connection, res, &error);

    if (variant == NULL) {
        g_simple_async_result_set_from_error (simple, error);
        g_error_free (error);
        g_simple_async_result_complete_in_idle (simple);
        g_object_unref (simple);
        return;
    }

    if (g_dbus_connection_is_closed (connection)) {
        /*
         * The connection is closed, can not contine next steps, so complete
         * the asynchronous request with error.
         */
        g_simple_async_result_set_error (simple,
                G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "Connection is closed.");
        g_simple_async_result_complete_in_idle (simple);
        return;
    }

    const gchar *path = NULL;
    g_variant_get (variant, "(&o)", &path);


    IBusBus *bus = (IBusBus *)g_async_result_get_source_object (
            (GAsyncResult *)simple);
    g_assert (IBUS_IS_BUS (bus));

    GCancellable *cancellable =
            (GCancellable *)g_object_get_data ((GObject *)simple,
                                               "cancellable");

    ibus_input_context_new_async (path,
            bus->priv->connection,
            cancellable,
            (GAsyncReadyCallback)_create_input_context_async_step_two_done,
            simple);
    /* release the reference from g_async_result_get_source_object() */
    g_object_unref (bus);
}

void
ibus_bus_create_input_context_async (IBusBus            *bus,
                                     const gchar        *client_name,
                                     gint                timeout_msec,
                                     GCancellable       *cancellable,
                                     GAsyncReadyCallback callback,
                                     gpointer            user_data)
{
    g_return_if_fail (IBUS_IS_BUS (bus));
    g_return_if_fail (client_name != NULL);
    g_return_if_fail (callback != NULL);

    GSimpleAsyncResult *simple = g_simple_async_result_new ((GObject *)bus,
            callback, user_data, ibus_bus_create_input_context_async);

    if (cancellable != NULL) {
        g_object_set_data_full ((GObject *)simple,
                                "cancellable",
                                g_object_ref (cancellable),
                                (GDestroyNotify)g_object_unref);
    }

    /* do not use ibus_bus_call_async, instread use g_dbus_connection_call
     * directly, because we need two async steps for create an IBusInputContext.
     * 1. Call CreateInputContext to request ibus-daemon create a remote IC.
     * 2. New local IBusInputContext proxy of the remote IC
     */
    g_dbus_connection_call (bus->priv->connection,
            IBUS_SERVICE_IBUS,
            IBUS_PATH_IBUS,
            IBUS_INTERFACE_IBUS,
            "CreateInputContext",
            g_variant_new ("(s)", client_name),
            G_VARIANT_TYPE("(o)"),
            G_DBUS_CALL_FLAGS_NO_AUTO_START,
            timeout_msec,
            cancellable,
            (GAsyncReadyCallback)_create_input_context_async_step_one_done,
            simple);
}

IBusInputContext *
ibus_bus_create_input_context_async_finish (IBusBus      *bus,
                                            GAsyncResult *res,
                                            GError      **error)
{
    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_simple_async_result_is_valid (res, (GObject *) bus,
                                              ibus_bus_create_input_context_async));

    GSimpleAsyncResult *simple = (GSimpleAsyncResult *) res;
    if (g_simple_async_result_propagate_error (simple, error))
        return NULL;
    IBusInputContext *context =
            g_simple_async_result_get_op_res_gpointer (simple);
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
                                 IBUS_INTERFACE_IBUS,
                                 "CurrentInputContext",
                                 NULL,
                                 G_VARIANT_TYPE ("(o)"));

    if (result != NULL) {
        g_variant_get (result, "(o)", &path);
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
                         IBUS_INTERFACE_IBUS,
                         "CurrentInputContext",
                         NULL,
                         G_VARIANT_TYPE ("(o)"),
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
    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_simple_async_result_is_valid (res, (GObject *) bus,
                                              ibus_bus_current_input_context_async));
    return g_strdup (_async_finish_object_path (res, error));
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
    g_return_if_fail (bus->priv->watch_ibus_signal_id == 0);

    /* Subscribe to ibus signals such as GlboalEngineChanged. */
    bus->priv->watch_ibus_signal_id
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
    /* FIXME handle org.freedesktop.IBus.RegistryChanged signal if needed */
}

static void
ibus_bus_unwatch_ibus_signal (IBusBus *bus)
{
    g_return_if_fail (bus->priv->watch_ibus_signal_id != 0);
    g_dbus_connection_signal_unsubscribe (bus->priv->connection,
                                          bus->priv->watch_ibus_signal_id);
    bus->priv->watch_ibus_signal_id = 0;
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
    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_simple_async_result_is_valid (res, (GObject *) bus,
                                              ibus_bus_request_name_async));
    return _async_finish_guint (res, error);
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
    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_simple_async_result_is_valid (res, (GObject *) bus,
                                              ibus_bus_release_name_async));
    return _async_finish_guint (res, error);
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
    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_simple_async_result_is_valid (res, (GObject *) bus,
                                              ibus_bus_name_has_owner_async));
    return _async_finish_gboolean (res, error);
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
    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_simple_async_result_is_valid (res, (GObject *) bus,
                                              ibus_bus_add_match_async));
    return _async_finish_void (res, error);
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
    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_simple_async_result_is_valid (res, (GObject *) bus,
                                              ibus_bus_remove_match_async));
    return _async_finish_void (res, error);
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
    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_simple_async_result_is_valid (res, (GObject *) bus,
                                              ibus_bus_get_name_owner_async));
    return g_strdup (_async_finish_string (res, error));
}

GDBusConnection *
ibus_bus_get_connection (IBusBus *bus)
{
    g_return_val_if_fail (IBUS_IS_BUS (bus), NULL);

    return bus->priv->connection;
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
    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_simple_async_result_is_valid (res, (GObject *) bus,
                                              ibus_bus_exit_async));
    return _async_finish_void (res, error);
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
    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_simple_async_result_is_valid (res, (GObject *) bus,
                                              ibus_bus_register_component_async));
    return _async_finish_void (res, error);
}

static GList *
ibus_bus_do_list_engines (IBusBus *bus, gboolean active_engines_only)
{
    g_return_val_if_fail (IBUS_IS_BUS (bus), NULL);

    GList *retval = NULL;
    GVariant *result;
    result = ibus_bus_call_sync (bus,
                                 IBUS_SERVICE_IBUS,
                                 IBUS_PATH_IBUS,
                                 IBUS_INTERFACE_IBUS,
                                 active_engines_only ? "ListActiveEngines" : "ListEngines",
                                 NULL,
                                 G_VARIANT_TYPE ("(av)"));

    if (result) {
        GVariantIter *iter = NULL;
        g_variant_get (result, "(av)", &iter);
        GVariant *var;
        while (g_variant_iter_loop (iter, "v", &var)) {
            IBusSerializable *serializable = ibus_serializable_deserialize (var);
            g_object_ref_sink (serializable);
            retval = g_list_append (retval, serializable);
        }
        g_variant_iter_free (iter);
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
                         IBUS_INTERFACE_IBUS,
                         "ListEngines",
                         NULL,
                         G_VARIANT_TYPE ("(av)"),
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
    GSimpleAsyncResult *simple = (GSimpleAsyncResult *) res;
    if (g_simple_async_result_propagate_error (simple, error))
        return NULL;
    GVariant *variant = g_simple_async_result_get_op_res_gpointer (simple);
    g_return_val_if_fail (variant != NULL, NULL);

    GList *retval = NULL;
    GVariantIter *iter = NULL;
    g_variant_get (variant, "(av)", &iter);
    GVariant *var;
    while (g_variant_iter_loop (iter, "v", &var)) {
        IBusSerializable *serializable = ibus_serializable_deserialize (var);
        g_object_ref_sink (serializable);
        retval = g_list_append (retval, serializable);
    }
    g_variant_iter_free (iter);
    return retval;
}

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
                         IBUS_INTERFACE_IBUS,
                         "ListActiveEngines",
                         NULL,
                         G_VARIANT_TYPE ("(av)"),
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
    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_simple_async_result_is_valid (res, (GObject *) bus,
                                              ibus_bus_get_use_sys_layout_async));
    return _async_finish_gboolean (res, error);
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
    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_simple_async_result_is_valid (res, (GObject *) bus,
                                              ibus_bus_get_use_global_engine_async));
    return _async_finish_gboolean (res, error);
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
    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_simple_async_result_is_valid (res, (GObject *) bus,
                                              ibus_bus_is_global_engine_enabled_async));
    return _async_finish_gboolean (res, error);
}


IBusEngineDesc *
ibus_bus_get_current_engine (IBusBus *bus)
{
    g_return_val_if_fail (IBUS_IS_BUS (bus), NULL);

    GVariant *result;
    IBusEngineDesc *engine = NULL;
    result = ibus_bus_call_sync (bus,
                                 IBUS_SERVICE_IBUS,
                                 IBUS_PATH_IBUS,
                                 IBUS_INTERFACE_IBUS,
                                 "GetCurrentEngine",
                                 NULL,
                                 G_VARIANT_TYPE ("(v)"));

    if (result) {
        GVariant *variant = NULL;
        g_variant_get (result, "(v)", &variant);
        if (variant) {
            engine = IBUS_ENGINE_DESC (ibus_serializable_deserialize (variant));
            g_variant_unref (variant);
        }
        g_variant_unref (result);
    }

    return engine;
}

void
ibus_bus_get_current_engine_async (IBusBus            *bus,
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
                         "GetCurrentEngine",
                         NULL,
                         G_VARIANT_TYPE ("(v)"),
                         ibus_bus_get_current_engine_async,
                         timeout_msec,
                         cancellable,
                         callback,
                         user_data);
}

IBusEngineDesc *
ibus_bus_get_current_engine_async_finish (IBusBus      *bus,
                                          GAsyncResult *res,
                                          GError      **error)
{
    GSimpleAsyncResult *simple = (GSimpleAsyncResult *) res;
    if (g_simple_async_result_propagate_error (simple, error))
        return NULL;
    GVariant *variant = g_simple_async_result_get_op_res_gpointer (simple);
    g_return_val_if_fail (variant != NULL, NULL);
    GVariant *inner_variant = NULL;
    g_variant_get (variant, "(v)", &inner_variant);

    IBusEngineDesc *engine = NULL;
    if (inner_variant) {
        engine = IBUS_ENGINE_DESC (ibus_serializable_deserialize (inner_variant));
        g_variant_unref (inner_variant);
    }
    return engine;
}

gboolean
ibus_bus_set_current_engine (IBusBus     *bus,
                             const gchar *current_engine)
{
    g_return_val_if_fail (IBUS_IS_BUS (bus), FALSE);
    g_return_val_if_fail (current_engine != NULL, FALSE);

    GVariant *result;
    result = ibus_bus_call_sync (bus,
                                 IBUS_SERVICE_IBUS,
                                 IBUS_PATH_IBUS,
                                 IBUS_INTERFACE_IBUS,
                                 "SetCurrentEngine",
                                 g_variant_new ("(s)", current_engine),
                                 NULL);

    if (result) {
        g_variant_unref (result);
        return TRUE;
    }
    return FALSE;
}

void
ibus_bus_set_current_engine_async (IBusBus            *bus,
                                  const gchar        *current_engine,
                                  gint                timeout_msec,
                                  GCancellable       *cancellable,
                                  GAsyncReadyCallback callback,
                                  gpointer            user_data)
{
    g_return_if_fail (IBUS_IS_BUS (bus));
    g_return_if_fail (current_engine != NULL);

    ibus_bus_call_async (bus,
                         IBUS_SERVICE_IBUS,
                         IBUS_PATH_IBUS,
                         IBUS_INTERFACE_IBUS,
                         "SetCurrentEngine",
                         g_variant_new ("(s)", current_engine),
                         NULL, /* no return value */
                         ibus_bus_set_current_engine_async,
                         timeout_msec,
                         cancellable,
                         callback,
                         user_data);
}

IBusEngineDesc *
ibus_bus_get_global_engine (IBusBus *bus)
{
    g_return_val_if_fail (IBUS_IS_BUS (bus), NULL);

    GVariant *result;
    IBusEngineDesc *engine = NULL;
    result = ibus_bus_call_sync (bus,
                                 IBUS_SERVICE_IBUS,
                                 IBUS_PATH_IBUS,
                                 IBUS_INTERFACE_IBUS,
                                 "GetGlobalEngine",
                                 NULL,
                                 G_VARIANT_TYPE ("(v)"));

    if (result) {
        GVariant *variant = NULL;
        g_variant_get (result, "(v)", &variant);
        if (variant) {
            engine = IBUS_ENGINE_DESC (ibus_serializable_deserialize (variant));
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
                         IBUS_INTERFACE_IBUS,
                         "GetGlobalEngine",
                         NULL,
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
    GSimpleAsyncResult *simple = (GSimpleAsyncResult *) res;
    if (g_simple_async_result_propagate_error (simple, error))
        return NULL;
    GVariant *variant = g_simple_async_result_get_op_res_gpointer (simple);
    g_return_val_if_fail (variant != NULL, NULL);
    GVariant *inner_variant = NULL;
    g_variant_get (variant, "(v)", &inner_variant);

    IBusEngineDesc *engine = NULL;
    if (inner_variant) {
        engine = IBUS_ENGINE_DESC (ibus_serializable_deserialize (inner_variant));
        g_variant_unref (inner_variant);
    }
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
    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_simple_async_result_is_valid (res, (GObject *) bus,
                                              ibus_bus_set_global_engine_async));
    return _async_finish_void (res, error);
}

gboolean
ibus_bus_preload_engines (IBusBus             *bus,
                          const gchar * const *names)
{
    GVariant *result;

    g_return_val_if_fail (IBUS_IS_BUS (bus), FALSE);
    g_return_val_if_fail (names != NULL && names[0] != NULL, FALSE);

    result = ibus_bus_call_sync (bus,
                                 IBUS_SERVICE_IBUS,
                                 IBUS_PATH_IBUS,
                                 IBUS_INTERFACE_IBUS,
                                 "PreloadEngines",
                                 g_variant_new("(^as)", names),
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
    g_return_if_fail (IBUS_IS_BUS (bus));
    g_return_if_fail (names != NULL && names[0] != NULL);

    ibus_bus_call_async (bus,
                         IBUS_SERVICE_IBUS,
                         IBUS_PATH_IBUS,
                         IBUS_INTERFACE_IBUS,
                         "PreloadEngines",
                         g_variant_new("(^as)", names),
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
    g_assert (IBUS_IS_BUS (bus));
    g_assert (g_simple_async_result_is_valid (
            res, (GObject *) bus,
            ibus_bus_preload_engines_async));
    return _async_finish_void (res, error);
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
    g_assert (G_IS_DBUS_CONNECTION (connection));

    GSimpleAsyncResult *simple = (GSimpleAsyncResult *) user_data;
    GError *error = NULL;
    GVariant *variant = g_dbus_connection_call_finish (connection, res, &error);

    if (variant == NULL) {
        /* Replace with g_simple_async_result_take_error in glib 2.28 */
        g_simple_async_result_set_from_error (simple, error);
        g_error_free (error);
    }
    else {
        g_simple_async_result_set_op_res_gpointer (simple, variant,
                                                   (GDestroyNotify) g_variant_unref);
    }
    g_simple_async_result_complete (simple);
    g_object_unref (simple);
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
    g_assert (IBUS_IS_BUS (bus));
    g_assert (member != NULL);
    g_return_if_fail (ibus_bus_is_connected (bus));

    GSimpleAsyncResult *simple = g_simple_async_result_new ((GObject*) bus,
                                                            callback,
                                                            user_data,
                                                            source_tag);

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
                            simple);
}
