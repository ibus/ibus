/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2008-2010 Red Hat, Inc.
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
#include "ibusshare.h"
#include "ibusconfigservice.h"

enum {
    LAST_SIGNAL,
};

enum {
    PROP_0,
};

/* functions prototype */
static void      ibus_config_service_class_init      (IBusConfigServiceClass *class);
static void      ibus_config_service_init            (IBusConfigService      *config);
static void      ibus_config_service_destroy         (IBusConfigService      *config);
static void      ibus_config_service_service_method_call
                                                     (IBusService            *service,
                                                      GDBusConnection        *connection,
                                                      const gchar            *sender,
                                                      const gchar            *object_path,
                                                      const gchar            *interface_name,
                                                      const gchar            *method_name,
                                                      GVariant               *parameters,
                                                      GDBusMethodInvocation  *invocation);
static GVariant *ibus_config_service_service_get_property
                                                     (IBusService            *service,
                                                      GDBusConnection        *connection,
                                                      const gchar            *sender,
                                                      const gchar            *object_path,
                                                      const gchar            *interface_name,
                                                      const gchar            *property_name,
                                                      GError                **error);
static gboolean  ibus_config_service_service_set_property
                                                     (IBusService            *service,
                                                      GDBusConnection        *connection,
                                                      const gchar            *sender,
                                                      const gchar            *object_path,
                                                      const gchar            *interface_name,
                                                      const gchar            *property_name,
                                                      GVariant               *value,
                                                      GError                **error);
static gboolean  ibus_config_service_set_value       (IBusConfigService      *config,
                                                      const gchar            *section,
                                                      const gchar            *name,
                                                      GVariant               *value,
                                                      GError                **error);
static GVariant *ibus_config_service_get_value       (IBusConfigService      *config,
                                                      const gchar            *section,
                                                      const gchar            *name,
                                                      GError                **error);
static GVariant *ibus_config_service_get_values      (IBusConfigService      *config,
                                                      const gchar            *section,
                                                      GError                **error);
static gboolean  ibus_config_service_unset_value     (IBusConfigService      *config,
                                                      const gchar            *section,
                                                      const gchar            *name,
                                                      GError                **error);

G_DEFINE_TYPE (IBusConfigService, ibus_config_service, IBUS_TYPE_SERVICE)

static const gchar introspection_xml[] =
    "<node>"
    "  <interface name='org.freedesktop.IBus.Config'>"
    "    <method name='SetValue'>"
    "      <arg direction='in'  type='s' name='section' />"
    "      <arg direction='in'  type='s' name='name' />"
    "      <arg direction='in'  type='v' name='value' />"
    "    </method>"
    "    <method name='GetValue'>"
    "      <arg direction='in'  type='s' name='section' />"
    "      <arg direction='in'  type='s' name='name' />"
    "      <arg direction='out' type='v' name='value' />"
    "    </method>"
    "    <method name='GetValues'>"
    "      <arg direction='in'  type='s' name='section' />"
    "      <arg direction='out' type='a{sv}' name='values' />"
    "    </method>"
    "    <method name='UnsetValue'>"
    "      <arg direction='in'  type='s' name='section' />"
    "      <arg direction='in'  type='s' name='name' />"
    "    </method>"
    "    <signal name='ValueChanged'>"
    "      <arg type='s' name='section' />"
    "      <arg type='s' name='name' />"
    "      <arg type='v' name='value' />"
    "    </signal>"
    "  </interface>"
    "</node>";

static void
ibus_config_service_class_init (IBusConfigServiceClass *class)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (class);

    IBUS_OBJECT_CLASS (gobject_class)->destroy = (IBusObjectDestroyFunc) ibus_config_service_destroy;

    IBUS_SERVICE_CLASS (class)->service_method_call  = ibus_config_service_service_method_call;
    IBUS_SERVICE_CLASS (class)->service_get_property = ibus_config_service_service_get_property;
    IBUS_SERVICE_CLASS (class)->service_set_property = ibus_config_service_service_set_property;

    ibus_service_class_add_interfaces (IBUS_SERVICE_CLASS (class), introspection_xml);

    class->set_value   = ibus_config_service_set_value;
    class->get_value   = ibus_config_service_get_value;
    class->get_values  = ibus_config_service_get_values;
    class->unset_value = ibus_config_service_unset_value;
}

static void
ibus_config_service_init (IBusConfigService *config)
{
}

static void
ibus_config_service_destroy (IBusConfigService *config)
{
    IBUS_OBJECT_CLASS(ibus_config_service_parent_class)->destroy ((IBusObject *) config);
}

static void
ibus_config_service_service_method_call (IBusService           *service,
                                         GDBusConnection       *connection,
                                         const gchar           *sender,
                                         const gchar           *object_path,
                                         const gchar           *interface_name,
                                         const gchar           *method_name,
                                         GVariant              *parameters,
                                         GDBusMethodInvocation *invocation)
{
    IBusConfigService *config = IBUS_CONFIG_SERVICE (service);

    if (g_strcmp0 (interface_name, IBUS_INTERFACE_CONFIG) != 0) {
        IBUS_SERVICE_CLASS (ibus_config_service_parent_class)->
                service_method_call (service,
                                     connection,
                                     sender,
                                     object_path,
                                     interface_name,
                                     method_name,
                                     parameters,
                                     invocation);
        return;
    }

    if (g_strcmp0 (method_name, "SetValue") == 0) {
        gchar *section;
        gchar *name;
        GVariant *value;
        gboolean retval;
        GError *error = NULL;

        g_variant_get (parameters, "(&s&sv)", &section, &name, &value);

        retval = IBUS_CONFIG_SERVICE_GET_CLASS (config)->set_value (config, section, name, value, &error);
        if (retval) {
            g_dbus_method_invocation_return_value (invocation, NULL);
        }
        else {
            g_dbus_method_invocation_return_gerror (invocation, error);
            g_error_free (error);
        }
        g_variant_unref (value);
        return;
    }

    if (g_strcmp0 (method_name, "GetValue") == 0) {
        gchar *section;
        gchar *name;
        GVariant *value;
        GError *error = NULL;

        g_variant_get (parameters, "(&s&s)", &section, &name);

        value = IBUS_CONFIG_SERVICE_GET_CLASS (config)->get_value (config, section, name, &error);
        if (value != NULL) {
            g_dbus_method_invocation_return_value (invocation, g_variant_new ("(v)", value));
            g_variant_unref (value);
        }
        else {
            g_dbus_method_invocation_return_gerror (invocation, error);
            g_error_free (error);
        }
        return;
    }

    if (g_strcmp0 (method_name, "GetValues") == 0) {
        gchar *section;
        GVariant *value;
        GError *error = NULL;

        g_variant_get (parameters, "(&s)", &section);

        value = IBUS_CONFIG_SERVICE_GET_CLASS (config)->get_values (config,
                                                                     section,
                                                                     &error);
        if (value) {
            g_dbus_method_invocation_return_value (invocation,
                    g_variant_new ("(@a{sv})", value));
        }
        else {
            g_dbus_method_invocation_return_gerror (invocation, error);
            g_error_free (error);
        }
        return;
    }

    if (g_strcmp0 (method_name, "UnsetValue") == 0) {
        gchar *section;
        gchar *name;
        gboolean retval;
        GError *error = NULL;

        g_variant_get (parameters, "(&s&s)", &section, &name);

        retval = IBUS_CONFIG_SERVICE_GET_CLASS (config)->unset_value (config, section, name, &error);
        if (retval) {
            g_dbus_method_invocation_return_value (invocation, NULL);
        }
        else {
            g_dbus_method_invocation_return_gerror (invocation, error);
            g_error_free (error);
        }
        return;
    }

    /* should not be reached */
    g_return_if_reached ();
}

static GVariant *
ibus_config_service_service_get_property (IBusService        *service,
                                          GDBusConnection    *connection,
                                          const gchar        *sender,
                                          const gchar        *object_path,
                                          const gchar        *interface_name,
                                          const gchar        *property_name,
                                          GError            **error)
{
    return IBUS_SERVICE_CLASS (ibus_config_service_parent_class)->
                service_get_property (service,
                                      connection,
                                      sender,
                                      object_path,
                                      interface_name,
                                      property_name,
                                      error);
}

static gboolean
ibus_config_service_service_set_property (IBusService        *service,
                                          GDBusConnection    *connection,
                                          const gchar        *sender,
                                          const gchar        *object_path,
                                          const gchar        *interface_name,
                                          const gchar        *property_name,
                                          GVariant           *value,
                                          GError            **error)
{
    return IBUS_SERVICE_CLASS (ibus_config_service_parent_class)->
                service_set_property (service,
                                      connection,
                                      sender,
                                      object_path,
                                      interface_name,
                                      property_name,
                                      value,
                                      error);
}

static gboolean
ibus_config_service_set_value (IBusConfigService *config,
                               const gchar       *section,
                               const gchar       *name,
                               GVariant          *value,
                               GError           **error)
{
    if (error) {
        gchar *str = g_variant_print (value, TRUE);
        *error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                              "Cannot set value %s::%s to %s", section, name, str);
        g_free (str);
    }
    return FALSE;
}

static GVariant *
ibus_config_service_get_value (IBusConfigService *config,
                               const gchar       *section,
                               const gchar       *name,
                               GError           **error)
{
    if (error) {
        *error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                              "Cannot get value %s::%s", section, name);
    }
    return NULL;
}

static GVariant *
ibus_config_service_get_values (IBusConfigService *config,
                                const gchar       *section,
                                GError           **error)
{
    if (error) {
        *error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                              "Cannot get values %s", section);
    }
    return NULL;
}

static gboolean
ibus_config_service_unset_value (IBusConfigService *config,
                                 const gchar       *section,
                                 const gchar       *name,
                                 GError           **error)
{
    if (error) {
        *error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                              "Cannot unset value %s::%s", section, name);
    }
    return FALSE;
}

IBusConfigService *
ibus_config_service_new (GDBusConnection *connection)
{
    g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), NULL);

    GObject *object = g_object_new (IBUS_TYPE_CONFIG_SERVICE,
                                    "object-path", IBUS_PATH_CONFIG,
                                    "connection", connection,
                                    NULL);
    return IBUS_CONFIG_SERVICE (object);
}

void
ibus_config_service_value_changed (IBusConfigService  *config,
                                   const gchar        *section,
                                   const gchar        *name,
                                   GVariant           *value)
{
    g_return_if_fail (IBUS_IS_CONFIG_SERVICE (config));
    g_return_if_fail (section != NULL);
    g_return_if_fail (name != NULL);
    g_return_if_fail (value != NULL);

    ibus_service_emit_signal ((IBusService *) config,
                              NULL,
                              IBUS_INTERFACE_CONFIG,
                              "ValueChanged",
                              g_variant_new ("(ssv)", section, name, value),
                              NULL);
}
