/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2008-2019 Red Hat, Inc.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
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
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 */
#include "ibusinternal.h"
#include "ibusmarshalers.h"
#include "ibusshare.h"
#include "ibusconfig.h"
#include "ibusbus.h"
#include "ibuserror.h"

#define IBUS_CONFIG_GET_PRIVATE(o)  \
   ((IBusConfigPrivate *)ibus_config_get_instance_private (o))

enum {
    VALUE_CHANGED,
    LAST_SIGNAL,
};


/* IBusConfigPriv */
struct _IBusConfigPrivate {
    GArray *watch_rules;
    guint watch_config_signal_id;
};

static guint    config_signals[LAST_SIGNAL] = { 0 };

static void      ibus_config_class_init     (IBusConfigClass    *class);
static void      ibus_config_init           (IBusConfig         *config);
static void      ibus_config_real_destroy   (IBusProxy          *proxy);

static void      ibus_config_g_signal       (GDBusProxy         *proxy,
                                             const gchar        *sender_name,
                                             const gchar        *signal_name,
                                             GVariant           *parameters);

static void      initable_iface_init        (GInitableIface     *initable_iface);
static void      async_initable_iface_init  (GAsyncInitableIface
                                                                *async_initable_iface);

static gchar    *_make_match_rule           (const gchar        *section,
                                             const gchar        *name);
static guint     _signal_subscribe          (GDBusProxy         *proxy);
static void      _signal_unsubscribe        (GDBusProxy         *proxy,
                                             guint               signal_id);

static void      _remove_all_match_rules    (IBusConfig         *config);

G_DEFINE_TYPE_WITH_CODE (IBusConfig, ibus_config, IBUS_TYPE_PROXY,
                         G_ADD_PRIVATE (IBusConfig)
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_iface_init)
                         G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE, async_initable_iface_init)
                         );

static void
ibus_config_class_init (IBusConfigClass *class)
{
    GDBusProxyClass *dbus_proxy_class = G_DBUS_PROXY_CLASS (class);
    IBusProxyClass *proxy_class = IBUS_PROXY_CLASS (class);

    dbus_proxy_class->g_signal = ibus_config_g_signal;
    proxy_class->destroy = ibus_config_real_destroy;


    /* install signals */
    /**
     * IBusConfig::value-changed:
     * @config: An IBusConfig.
     * @section: Section name.
     * @name: Name of the property.
     * @value: Value.
     *
     * Emitted when configuration value is changed.
     * <note><para>Argument @user_data is ignored in this function.</para></note>
     */
    config_signals[VALUE_CHANGED] =
        g_signal_new (I_("value-changed"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__STRING_STRING_VARIANT,
            G_TYPE_NONE,
            3,
            G_TYPE_STRING,
            G_TYPE_STRING,
            G_TYPE_VARIANT | G_SIGNAL_TYPE_STATIC_SCOPE);
}

static void
ibus_config_init (IBusConfig *config)
{
    config->priv = IBUS_CONFIG_GET_PRIVATE (config);
    config->priv->watch_rules = g_array_new (FALSE, FALSE, sizeof (gchar *));
}

static void
ibus_config_real_destroy (IBusProxy *proxy)
{
    IBusConfigPrivate *priv = IBUS_CONFIG_GET_PRIVATE (IBUS_CONFIG (proxy));

    _signal_unsubscribe (G_DBUS_PROXY (proxy), priv->watch_config_signal_id);
    _remove_all_match_rules (IBUS_CONFIG (proxy));
    g_array_free (priv->watch_rules, FALSE);

    IBUS_PROXY_CLASS(ibus_config_parent_class)->destroy (proxy);
}


static void
ibus_config_g_signal (GDBusProxy  *proxy,
                      const gchar *sender_name,
                      const gchar *signal_name,
                      GVariant    *parameters)
{
    if (g_strcmp0 (signal_name, "ValueChanged") == 0) {
        const gchar *section = NULL;
        const gchar *name = NULL;
        GVariant *value = NULL;

        g_variant_get (parameters, "(&s&sv)", &section, &name, &value);

        g_signal_emit (proxy,
                       config_signals[VALUE_CHANGED],
                       0,
                       section,
                       name,
                       value);
        g_variant_unref (value);
        return;
    }

    g_return_if_reached ();
}

static void
_connection_signal_cb (GDBusConnection *connection,
                       const gchar     *sender_name,
                       const gchar     *object_path,
                       const gchar     *interface_name,
                       const gchar     *signal_name,
                       GVariant        *parameters,
                       IBusConfig      *config)
{
    g_return_if_fail (IBUS_IS_CONFIG (config));

    ibus_config_g_signal (G_DBUS_PROXY (config),
                          sender_name,
                          signal_name,
                          parameters);
}

static gchar *
_make_match_rule (const gchar *section,
                  const gchar *name)
{
    GString *str = g_string_new ("type='signal',"
                                 "interface='" IBUS_INTERFACE_CONFIG "',"
                                 "path='" IBUS_PATH_CONFIG "',"
                                 "member='ValueChanged'");
    if (section != NULL) {
        g_string_append_printf (str, ",arg0='%s'", section);
        if (name != NULL)
            g_string_append_printf (str, ",arg1='%s'", name);
    }
    return g_string_free (str, FALSE);
}

static void
_remove_all_match_rules (IBusConfig *config)
{
    gint i;

    for (i = 0; i < config->priv->watch_rules->len; i++) {
        IBusBus *bus = ibus_bus_new ();
        gchar *rule = g_array_index (config->priv->watch_rules, gchar *, i);
        ibus_bus_remove_match (bus, rule);
        g_object_unref (bus);
        g_free (rule);
    }
    g_array_set_size (config->priv->watch_rules, 0);
}

gboolean
ibus_config_watch (IBusConfig  *config,
                   const gchar *section,
                   const gchar *name)
{
    g_return_val_if_fail (IBUS_IS_CONFIG (config), FALSE);
    g_assert ((section != NULL) || (section == NULL && name == NULL));

    IBusBus *bus = ibus_bus_new ();
    gchar *rule;
    gboolean retval;

    if (section == NULL && name == NULL) {
        _remove_all_match_rules (config);

        rule = _make_match_rule (NULL, NULL);
        retval = ibus_bus_add_match (bus, rule);
        g_object_unref (bus);
        g_free (rule);

        return retval;
    }

    if (config->priv->watch_rules->len == 0) {
        rule = _make_match_rule (NULL, NULL);
        retval = ibus_bus_remove_match (bus, rule);
        g_free (rule);
        if (!retval) {
            g_object_unref (bus);
            return FALSE;
        }
    }

    rule = _make_match_rule (section, name);
    retval = ibus_bus_add_match (bus, rule);
    g_object_unref (bus);
    if (!retval) {
        g_free (rule);
        return FALSE;
    }

    g_array_append_val (config->priv->watch_rules, rule);
    return TRUE;
}

gboolean
ibus_config_unwatch (IBusConfig  *config,
                     const gchar *section,
                     const gchar *name)
{
    g_return_val_if_fail (IBUS_IS_CONFIG (config), FALSE);
    g_assert ((section != NULL) || (section == NULL && name == NULL));

    IBusBus *bus = ibus_bus_new ();
    gchar *rule = _make_match_rule (section, name);
    gboolean retval;

    retval = ibus_bus_remove_match (bus, rule);
    g_object_unref (bus);
    if (retval && (section != NULL || name != NULL)) {
        /* Remove the previously registered match rule from
           config->priv->watch_rules. */
        gint i;
        for (i = 0; i < config->priv->watch_rules->len; i++) {
            gchar *_rule = g_array_index (config->priv->watch_rules, gchar *,
                                          i);
            if (g_strcmp0 (_rule, rule) == 0) {
                config->priv->watch_rules =
                    g_array_remove_index_fast (config->priv->watch_rules, i);
                g_free (_rule);
                break;
            }
        }
    }
    g_free (rule);

    return TRUE;
}

IBusConfig *
ibus_config_new (GDBusConnection  *connection,
                 GCancellable     *cancellable,
                 GError          **error)
{
    g_assert (G_IS_DBUS_CONNECTION (connection));

    GInitable *initable;
    char *owner;

    GDBusProxyFlags flags = G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START |
                            G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
                            G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS;

    initable = g_initable_new (IBUS_TYPE_CONFIG,
                               cancellable,
                               error,
                               "g-connection",      connection,
                               "g-flags",           flags,
                               "g-name",            IBUS_SERVICE_CONFIG,
                               "g-interface-name",  IBUS_INTERFACE_CONFIG,
                               "g-object-path",     IBUS_PATH_CONFIG,
                               "g-default-timeout", ibus_get_timeout (),
                               NULL);
    if (initable == NULL)
        return NULL;

    owner = g_dbus_proxy_get_name_owner (G_DBUS_PROXY (initable));
    if (owner == NULL) {
        /* The configuration daemon, which is usually ibus-gconf, is not started yet. */
        g_set_error (error,
                     IBUS_ERROR,
                     IBUS_ERROR_NO_CONFIG,
                     "Configuration daemon is not running.");
        g_object_unref (initable);
        return NULL;
    }
    g_free (owner);

    /* clients should not destroy the config service. */
    IBUS_PROXY (initable)->own = FALSE;

    return IBUS_CONFIG (initable);
}

void
ibus_config_new_async (GDBusConnection     *connection,
                       GCancellable        *cancellable,
                       GAsyncReadyCallback  callback,
                       gpointer             user_data)
{
    g_assert (G_IS_DBUS_CONNECTION (connection));
    g_assert (callback != NULL);

    GDBusProxyFlags flags = G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START |
                            G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
                            G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS;

    g_async_initable_new_async (IBUS_TYPE_CONFIG,
                                G_PRIORITY_DEFAULT,
                                cancellable,
                                callback,
                                user_data,
                                "g-connection",      connection,
                                "g-flags",           flags,
                                "g-name",            IBUS_SERVICE_CONFIG,
                                "g-interface-name",  IBUS_INTERFACE_CONFIG,
                                "g-object-path",     IBUS_PATH_CONFIG,
                                "g-default-timeout", ibus_get_timeout (),
                                NULL);
}

IBusConfig *
ibus_config_new_async_finish (GAsyncResult  *res,
                              GError       **error)
{
    g_assert (G_IS_ASYNC_RESULT (res));
    g_assert (error == NULL || *error == NULL);

    GObject *object = NULL;
    GObject *source_object = NULL;

    source_object = g_async_result_get_source_object (res);
    g_assert (source_object != NULL);

    object = g_async_initable_new_finish (G_ASYNC_INITABLE (source_object),
                                          res,
                                          error);
    g_object_unref (source_object);

    if (object != NULL) {
        char *owner;
        owner = g_dbus_proxy_get_name_owner (G_DBUS_PROXY (object));
        if (owner == NULL) {
            /* The configuration daemon, which is usually ibus-gconf, 
             * is not started yet. */
            g_set_error (error,
                         IBUS_ERROR,
                         IBUS_ERROR_NO_CONFIG,
                         "Configuration daemon is not running.");
            g_object_unref (object);
            return NULL;
        }
        g_free (owner);
        /* clients should not destroy the config service. */
        IBUS_PROXY (object)->own = FALSE;
        return IBUS_CONFIG (object);
    }
    else {
        return NULL;
    }
}

GVariant *
ibus_config_get_value (IBusConfig  *config,
                       const gchar *section,
                       const gchar *name)
{
    g_assert (IBUS_IS_CONFIG (config));
    g_assert (section != NULL);
    g_assert (name != NULL);

    GError *error = NULL;
    GVariant *result;
    result = g_dbus_proxy_call_sync ((GDBusProxy *) config,
                                     "GetValue",                /* method_name */
                                     g_variant_new ("(ss)",
                                        section, name),         /* parameters */
                                     G_DBUS_CALL_FLAGS_NONE,    /* flags */
                                     -1,                        /* timeout */
                                     NULL,                      /* cancellable */
                                     &error                     /* error */
                                     );
    if (result == NULL) {
        g_warning ("%s.GetValue: %s", IBUS_INTERFACE_CONFIG, error->message);
        g_error_free (error);
        return NULL;
    }

    GVariant *value = NULL;
    g_variant_get (result, "(v)", &value);
    g_variant_unref (result);

    return value;
}

void
ibus_config_get_value_async (IBusConfig         *config,
                             const gchar        *section,
                             const gchar        *name,
                             gint                timeout_ms,
                             GCancellable       *cancellable,
                             GAsyncReadyCallback callback,
                             gpointer            user_data)
{
    g_assert (IBUS_IS_CONFIG (config));
    g_assert (section != NULL);
    g_assert (name != NULL);

    g_dbus_proxy_call ((GDBusProxy *)config,
                       "GetValue",
                       g_variant_new ("(ss)", section, name),
                       G_DBUS_CALL_FLAGS_NONE,
                       timeout_ms,
                       cancellable,
                       callback,
                       user_data);
}

GVariant *
ibus_config_get_value_async_finish (IBusConfig    *config,
                                    GAsyncResult  *result,
                                    GError       **error)
{
    g_assert (IBUS_IS_CONFIG (config));
    g_assert (G_IS_ASYNC_RESULT (result));
    g_assert (error == NULL || *error == NULL);

    GVariant *value = NULL;
    GVariant *retval = g_dbus_proxy_call_finish ((GDBusProxy *)config,
                                                 result,
                                                 error);
    if (retval != NULL) {
        g_variant_get (retval, "(v)", &value);
        g_variant_unref (retval);
    }

    return value;
}

GVariant *
ibus_config_get_values (IBusConfig  *config,
                        const gchar *section)
{
    g_assert (IBUS_IS_CONFIG (config));
    g_assert (section != NULL);

    GError *error = NULL;
    GVariant *result;
    result = g_dbus_proxy_call_sync ((GDBusProxy *) config,
                                     "GetValues",
                                     g_variant_new ("(s)", section),
                                     G_DBUS_CALL_FLAGS_NONE,
                                     -1,
                                     NULL,
                                     &error);
    if (result == NULL) {
        g_warning ("%s.GetValues: %s", IBUS_INTERFACE_CONFIG, error->message);
        g_error_free (error);
        return NULL;
    }

    GVariant *value = NULL;
    g_variant_get (result, "(@a{sv})", &value);
    g_variant_unref (result);

    return value;
}

void
ibus_config_get_values_async (IBusConfig         *config,
                              const gchar        *section,
                              gint                timeout_ms,
                              GCancellable       *cancellable,
                              GAsyncReadyCallback callback,
                              gpointer            user_data)
{
    g_assert (IBUS_IS_CONFIG (config));
    g_assert (section != NULL);

    g_dbus_proxy_call ((GDBusProxy *)config,
                       "GetValues",
                       g_variant_new ("(s)", section),
                       G_DBUS_CALL_FLAGS_NONE,
                       timeout_ms,
                       cancellable,
                       callback,
                       user_data);
}

GVariant *
ibus_config_get_values_async_finish (IBusConfig    *config,
                                     GAsyncResult  *result,
                                     GError       **error)
{
    g_assert (IBUS_IS_CONFIG (config));
    g_assert (G_IS_ASYNC_RESULT (result));
    g_assert (error == NULL || *error == NULL);

    GVariant *value = NULL;
    GVariant *retval = g_dbus_proxy_call_finish ((GDBusProxy *)config,
                                                 result,
                                                 error);
    if (retval != NULL) {
        g_variant_get (retval, "(@a{sv})", &value);
        g_variant_unref (retval);
    }

    return value;
}
    
gboolean
ibus_config_set_value (IBusConfig   *config,
                       const gchar  *section,
                       const gchar  *name,
                       GVariant     *value)
{
    g_assert (IBUS_IS_CONFIG (config));
    g_assert (section != NULL);
    g_assert (name != NULL);
    g_assert (value != NULL);

    GError *error = NULL;
    GVariant *result;
    result = g_dbus_proxy_call_sync ((GDBusProxy *) config,
                                     "SetValue",                /* method_name */
                                     g_variant_new ("(ssv)",
                                        section, name, value),  /* parameters */
                                     G_DBUS_CALL_FLAGS_NONE,    /* flags */
                                     -1,                        /* timeout */
                                     NULL,                      /* cancellable */
                                     &error                     /* error */
                                     );
    if (result == NULL) {
        g_warning ("%s.SetValue: %s", IBUS_INTERFACE_CONFIG, error->message);
        g_error_free (error);
        return FALSE;
    }
    g_variant_unref (result);
    return TRUE;
}

void
ibus_config_set_value_async (IBusConfig         *config,
                             const gchar        *section,
                             const gchar        *name,
                             GVariant           *value,
                             gint                timeout_ms,
                             GCancellable       *cancellable,
                             GAsyncReadyCallback callback,
                             gpointer            user_data)
{
    g_assert (IBUS_IS_CONFIG (config));
    g_assert (section != NULL);
    g_assert (name != NULL);
    g_assert (value != NULL);

    g_dbus_proxy_call ((GDBusProxy *) config,
                       "SetValue",                /* method_name */
                       g_variant_new ("(ssv)",
                                      section, name, value),  /* parameters */
                       G_DBUS_CALL_FLAGS_NONE,    /* flags */
                       timeout_ms,
                       cancellable,
                       callback,
                       user_data);
}

gboolean
ibus_config_set_value_async_finish (IBusConfig         *config,
                                    GAsyncResult       *result,
                                    GError            **error)
{
    g_assert (IBUS_IS_CONFIG (config));
    g_assert (G_IS_ASYNC_RESULT (result));
    g_assert (error == NULL || *error == NULL);

    GVariant *retval = g_dbus_proxy_call_finish ((GDBusProxy *)config,
                                                 result,
                                                 error);
    if (retval != NULL) {
        g_variant_unref (retval);
        return TRUE;
    }

    return FALSE;
}

gboolean
ibus_config_unset (IBusConfig   *config,
                   const gchar  *section,
                   const gchar  *name)
{
    g_assert (IBUS_IS_CONFIG (config));
    g_assert (section != NULL);
    g_assert (name != NULL);

    GError *error = NULL;
    GVariant *result;
    result = g_dbus_proxy_call_sync ((GDBusProxy *) config,
                                     "UnsetValue",              /* method_name */
                                     g_variant_new ("(ss)",
                                        section, name),         /* parameters */
                                     G_DBUS_CALL_FLAGS_NONE,    /* flags */
                                     -1,                        /* timeout */
                                     NULL,                      /* cancellable */
                                     &error                     /* error */
                                     );
    if (result == NULL) {
        g_warning ("%s.UnsetValue: %s", IBUS_INTERFACE_CONFIG, error->message);
        g_error_free (error);
        return FALSE;
    }
    g_variant_unref (result);
    return TRUE;
}

static guint
_signal_subscribe (GDBusProxy *proxy)
{
    GDBusConnection *connection = g_dbus_proxy_get_connection (proxy);
    return g_dbus_connection_signal_subscribe (connection,
                                               NULL,
                                               IBUS_INTERFACE_CONFIG,
                                               NULL,
                                               NULL,
                                               NULL,
                                               G_DBUS_SIGNAL_FLAGS_NO_MATCH_RULE,
                                               (GDBusSignalCallback) _connection_signal_cb,
                                               g_object_ref (proxy),
                                               (GDestroyNotify) g_object_unref);
}

static void
_signal_unsubscribe (GDBusProxy *proxy, guint signal_id)
{
    GDBusConnection *connection = g_dbus_proxy_get_connection (proxy);
    g_dbus_connection_signal_unsubscribe (connection, signal_id);
}

static GInitableIface *initable_iface_parent = NULL;

static gboolean
initable_init (GInitable     *initable,
               GCancellable  *cancellable,
               GError       **error)
{
    if (!initable_iface_parent->init (initable, cancellable, error))
        return FALSE;

    IBusConfig *config = IBUS_CONFIG (initable);
    config->priv->watch_config_signal_id =
        _signal_subscribe (G_DBUS_PROXY (initable));

    gboolean retval = ibus_config_watch (config, NULL, NULL);
    if (!retval)
        g_set_error (error,
                     IBUS_ERROR,
                     IBUS_ERROR_FAILED,
                     "Cannot watch configuration change.");
    return retval;
}

static void
initable_iface_init (GInitableIface *initable_iface)
{
    initable_iface_parent = g_type_interface_peek_parent (initable_iface);
    initable_iface->init = initable_init;
}

static GAsyncInitableIface *async_initable_iface_parent = NULL;

static void
async_initable_init_async (GAsyncInitable      *initable,
                           gint                 io_priority,
                           GCancellable        *cancellable,
                           GAsyncReadyCallback  callback,
                           gpointer             user_data)
{
    async_initable_iface_parent->init_async (initable,
                                             io_priority,
                                             cancellable,
                                             callback,
                                             user_data);
}

static gboolean
async_initable_init_finish (GAsyncInitable  *initable,
                            GAsyncResult    *res,
                            GError         **error)
{
    if (!async_initable_iface_parent->init_finish (initable, res, error))
        return FALSE;

    IBusConfig *config = IBUS_CONFIG (initable);
    config->priv->watch_config_signal_id =
        _signal_subscribe (G_DBUS_PROXY (initable));
    return ibus_config_watch (config, NULL, NULL);
}

static void
async_initable_iface_init (GAsyncInitableIface *async_initable_iface)
{
    async_initable_iface_parent =
        g_type_interface_peek_parent (async_initable_iface);
    async_initable_iface->init_async = async_initable_init_async;
    async_initable_iface->init_finish = async_initable_init_finish;
}
