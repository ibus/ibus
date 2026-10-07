/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2015 Peng Huang <shawn.p.huang@gmail.com>
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
#include "ibusfactory.h"
#include "ibusengine.h"
#include "ibusmarshalers.h"
#include "ibusshare.h"
#include "ibusinternal.h"

#define IBUS_FACTORY_GET_PRIVATE(o)  \
   ((IBusFactoryPrivate *)ibus_factory_get_instance_private (o))

enum {
    CREATE_ENGINE,
    LAST_SIGNAL,
};

enum {
    PROP_0,
};

/* IBusFactoryPriv */
struct _IBusFactoryPrivate {
    guint id;
    GList          *engine_list;
    GHashTable     *engine_table;
};

static guint            factory_signals[LAST_SIGNAL] = { 0 };

/* functions prototype */
static void      ibus_factory_destroy        (IBusFactory        *factory);
static void      ibus_factory_set_property   (IBusFactory        *engine,
                                              guint               prop_id,
                                              const GValue       *value,
                                              GParamSpec         *pspec);
static void      ibus_factory_get_property   (IBusFactory        *factory,
                                              guint               prop_id,
                                              GValue             *value,
                                              GParamSpec         *pspec);
static void      ibus_factory_service_method_call
                                              (IBusService        *service,
                                               GDBusConnection    *connection,
                                               const gchar        *sender,
                                               const gchar        *object_path,
                                               const gchar        *interface_name,
                                               const gchar        *method_name,
                                               GVariant           *parameters,
                                               GDBusMethodInvocation
                                                                  *invocation);
static GVariant *ibus_factory_service_get_property
                                             (IBusService        *service,
                                              GDBusConnection    *connection,
                                              const gchar        *sender,
                                              const gchar        *object_path,
                                              const gchar        *interface_name,
                                              const gchar        *property_name,
                                              GError            **error);
static gboolean  ibus_factory_service_set_property
                                             (IBusService        *service,
                                              GDBusConnection    *connection,
                                              const gchar        *sender,
                                              const gchar        *object_path,
                                              const gchar        *interface_name,
                                              const gchar        *property_name,
                                              GVariant           *value,
                                              GError            **error);
static void      ibus_factory_engine_destroy_cb
                                             (IBusEngine         *engine,
                                              IBusFactory        *factory);

G_DEFINE_TYPE_WITH_PRIVATE (IBusFactory, ibus_factory, IBUS_TYPE_SERVICE)

static const gchar introspection_xml[] =
    "<node>"
    "  <interface name='org.freedesktop.IBus.Factory'>"
    "    <method name='CreateEngine'>"
    "      <arg direction='in'  type='s' name='name' />"
    "      <arg direction='out' type='o' />"
    "    </method>"
    "  </interface>"
    "</node>";

static IBusEngine *
ibus_factory_real_create_engine (IBusFactory    *factory,
                                 const gchar    *engine_name)
{
    GType engine_type;
    gchar *object_path = NULL;
    IBusEngine *engine = NULL;

    engine_type = (GType) g_hash_table_lookup (factory->priv->engine_table,
                                               engine_name);

    g_return_val_if_fail (engine_type != G_TYPE_INVALID, NULL);

    object_path = g_strdup_printf ("/org/freedesktop/IBus/Engine/%d",
                                   ++factory->priv->id);
    engine = ibus_engine_new_with_type (engine_type,
                                        engine_name,
                                        object_path,
                                        ibus_service_get_connection ((IBusService *)factory));
    g_free (object_path);

    return engine;
}

static gboolean
_ibus_factory_create_engine_accumulator (GSignalInvocationHint *ihint,
                                         GValue                *return_accu,
                                         const GValue          *handler_return,
                                         gpointer               dummy)
{
    gboolean retval = TRUE;
    GObject *object = g_value_get_object (handler_return);

    if (object != NULL) {
        g_value_copy (handler_return, return_accu);
        retval = FALSE;
    }

    return retval;
}

static void
ibus_factory_class_init (IBusFactoryClass *class)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (class);
    IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (class);

    gobject_class->set_property = (GObjectSetPropertyFunc) ibus_factory_set_property;
    gobject_class->get_property = (GObjectGetPropertyFunc) ibus_factory_get_property;

    ibus_object_class->destroy = (IBusObjectDestroyFunc) ibus_factory_destroy;

    IBUS_SERVICE_CLASS (class)->service_method_call  = ibus_factory_service_method_call;
    IBUS_SERVICE_CLASS (class)->service_get_property = ibus_factory_service_get_property;
    IBUS_SERVICE_CLASS (class)->service_set_property = ibus_factory_service_set_property;
    class->create_engine = ibus_factory_real_create_engine;

    ibus_service_class_add_interfaces (IBUS_SERVICE_CLASS (class), introspection_xml);

    /**
     * IBusFactory::create-engine:
     * @factory: the factory which received the signal
     * @engine_name: the engine_name which received the signal
     * @returns: (nullable) (transfer full): An IBusEngine
     *
     * The ::create-engine signal is a signal to create IBusEngine
     * with @engine_name, which gets emitted when IBusFactory
     * received CreateEngine dbus method. The callback functions
     * will be called until a callback returns a non-null object
     * of IBusEngine.
     */
    factory_signals[CREATE_ENGINE] =
        g_signal_new (I_("create-engine"),
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusFactoryClass, create_engine),
            _ibus_factory_create_engine_accumulator,
            NULL,
            _ibus_marshal_OBJECT__STRING,
            IBUS_TYPE_ENGINE,
            1,
            G_TYPE_STRING);
}

static void
ibus_factory_init (IBusFactory *factory)
{
    factory->priv = IBUS_FACTORY_GET_PRIVATE (factory);
    factory->priv->engine_table =
        g_hash_table_new_full (g_str_hash,
                               g_str_equal,
                               g_free,
                               NULL);
}

static void
ibus_factory_destroy (IBusFactory *factory)
{
    GList *list;

    list = g_list_copy (factory->priv->engine_list);
    g_list_free_full (list, (GDestroyNotify)ibus_object_destroy);
    g_list_free(factory->priv->engine_list);
    factory->priv->engine_list = NULL;

    if (factory->priv->engine_table) {
        g_hash_table_destroy (factory->priv->engine_table);
    }

    IBUS_OBJECT_CLASS(ibus_factory_parent_class)->destroy (IBUS_OBJECT (factory));
}

static void
ibus_factory_set_property (IBusFactory  *factory,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
    switch (prop_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (factory, prop_id, pspec);
    }
}

static void
ibus_factory_get_property (IBusFactory *factory,
                           guint        prop_id,
                           GValue      *value,
                           GParamSpec  *pspec)
{
    switch (prop_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (factory, prop_id, pspec);
    }
}

static void
ibus_factory_engine_destroy_cb (IBusEngine  *engine,
                                IBusFactory *factory)
{
    factory->priv->engine_list = g_list_remove (factory->priv->engine_list, engine);
    g_object_unref (engine);
}

static void
ibus_factory_service_method_call (IBusService           *service,
                                  GDBusConnection       *connection,
                                  const gchar           *sender,
                                  const gchar           *object_path,
                                  const gchar           *interface_name,
                                  const gchar           *method_name,
                                  GVariant              *parameters,
                                  GDBusMethodInvocation *invocation)
{
    IBusFactory *factory = IBUS_FACTORY (service);

    if (g_strcmp0 (method_name, "CreateEngine") == 0) {
        gchar *engine_name = NULL;
        IBusEngine *engine = NULL;

        g_variant_get (parameters, "(&s)", &engine_name);
        g_signal_emit (factory, factory_signals[CREATE_ENGINE],
                       0, engine_name, &engine);

        if (engine != NULL) {
            gchar *object_path = NULL;
            GValue value = { 0, };

            g_value_init (&value, G_TYPE_STRING);
            g_object_get_property (G_OBJECT (engine), "object-path", &value);
            object_path = g_value_dup_string (&value);
            g_value_unset (&value);

            g_assert (engine != NULL);
            g_assert (object_path != NULL);
            g_object_ref_sink (engine);
            factory->priv->engine_list = g_list_append (factory->priv->engine_list, engine);
            g_signal_connect (engine,
                              "destroy",
                              G_CALLBACK (ibus_factory_engine_destroy_cb),
                              factory);
            g_dbus_method_invocation_return_value (invocation,
                                                   g_variant_new ("(o)", object_path));
            g_free (object_path);
        }
        else {
            g_dbus_method_invocation_return_error (invocation,
                                                   G_DBUS_ERROR,
                                                   G_DBUS_ERROR_FAILED,
                                                   "Cannot find engine %s",
                                                   engine_name);
        }
        return;
    }

    IBUS_SERVICE_CLASS (ibus_factory_parent_class)->
            service_method_call (service,
                                 connection,
                                 sender,
                                 object_path,
                                 interface_name,
                                 method_name,
                                 parameters,
                                 invocation);
}

static GVariant *
ibus_factory_service_get_property (IBusService        *service,
                                   GDBusConnection    *connection,
                                   const gchar        *sender,
                                   const gchar        *object_path,
                                   const gchar        *interface_name,
                                   const gchar        *property_name,
                                   GError            **error)
{
    return IBUS_SERVICE_CLASS (ibus_factory_parent_class)->
                service_get_property (service,
                                      connection,
                                      sender,
                                      object_path,
                                      interface_name,
                                      property_name,
                                      error);
}

static gboolean
ibus_factory_service_set_property (IBusService        *service,
                                   GDBusConnection    *connection,
                                   const gchar        *sender,
                                   const gchar        *object_path,
                                   const gchar        *interface_name,
                                   const gchar        *property_name,
                                   GVariant           *value,
                                   GError            **error)
{
    return IBUS_SERVICE_CLASS (ibus_factory_parent_class)->
                service_set_property (service,
                                      connection,
                                      sender,
                                      object_path,
                                      interface_name,
                                      property_name,
                                      value,
                                      error);
}

IBusFactory *
ibus_factory_new (GDBusConnection *connection)
{
    g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), NULL);

    IBusFactory *object = g_object_new (IBUS_TYPE_FACTORY,
                                        "object-path", IBUS_PATH_FACTORY,
                                        "connection", connection,
                                        NULL);

    return IBUS_FACTORY (object);
}

void
ibus_factory_add_engine (IBusFactory *factory,
                         const gchar *engine_name,
                         GType        engine_type)
{
    g_return_if_fail (IBUS_IS_FACTORY (factory));
    g_return_if_fail (engine_name != NULL);
    g_return_if_fail (g_type_is_a (engine_type, IBUS_TYPE_ENGINE));

    g_hash_table_insert (factory->priv->engine_table, g_strdup (engine_name), (gpointer) engine_type);
}

IBusEngine *
ibus_factory_create_engine (IBusFactory    *factory,
                            const gchar    *engine_name)
{
    IBusEngine *engine = NULL;

    g_assert (engine_name != NULL);

    g_signal_emit (factory, factory_signals[CREATE_ENGINE],
                   0, engine_name, &engine);

    return engine;
}
