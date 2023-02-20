/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2015 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2015-2023 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2008-2019 Red Hat, Inc.
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
#include "ibusservice.h"
#include "ibusinternal.h"

#define IBUS_SERVICE_GET_PRIVATE(o)  \
   ((IBusServicePrivate *)ibus_service_get_instance_private (o))

enum {
    LAST_SIGNAL
};

enum {
    PROP_0,
    PROP_OBJECT_PATH,
    PROP_CONNECTION,
};

/* IBusServicePrivate */
struct _IBusServicePrivate {
    gchar *object_path;
    GDBusConnection *connection;
    GHashTable *table;
    gboolean constructed;
};

/*
static guint    service_signals[LAST_SIGNAL] = { 0 };
*/

/* functions prototype */
static void      ibus_service_base_init      (IBusServiceClass   *class);
static void      ibus_service_base_fini      (IBusServiceClass   *class);
static void      ibus_service_class_init     (IBusServiceClass   *class);
static void      ibus_service_init           (IBusService        *service);
static void      ibus_service_constructed    (GObject            *object);
static void      ibus_service_set_property   (IBusService        *service,
                                              guint               prop_id,
                                              const GValue       *value,
                                              GParamSpec         *pspec);
static void      ibus_service_get_property   (IBusService        *service,
                                              guint               prop_id,
                                              GValue             *value,
                                              GParamSpec         *pspec);
static void      ibus_service_destroy        (IBusService        *service);
static void      ibus_service_service_method_call
                                             (IBusService        *service,
                                              GDBusConnection    *connection,
                                              const gchar        *sender,
                                              const gchar        *object_path,
                                              const gchar        *interface_name,
                                              const gchar        *method_name,
                                              GVariant           *parameters,
                                              GDBusMethodInvocation
                                                                 *invocation);
static GVariant *ibus_service_service_get_property
                                             (IBusService        *service,
                                              GDBusConnection    *connection,
                                              const gchar        *sender,
                                              const gchar        *object_path,
                                              const gchar        *interface_name,
                                              const gchar        *property_name,
                                              GError            **error);
static gboolean  ibus_service_service_set_property
                                             (IBusService        *service,
                                              GDBusConnection    *connection,
                                              const gchar        *sender,
                                              const gchar        *object_path,
                                              const gchar        *interface_name,
                                              const gchar        *property_name,
                                              GVariant           *value,
                                              GError            **error);
static void      ibus_service_service_method_call_cb
                                             (GDBusConnection    *connection,
                                              const gchar        *sender,
                                              const gchar        *object_path,
                                              const gchar        *interface_name,
                                              const gchar        *method_name,
                                              GVariant           *parameters,
                                              GDBusMethodInvocation
                                                                 *invocation,
                                              IBusService        *service);
static GVariant *ibus_service_service_get_property_cb
                                             (GDBusConnection    *connection,
                                              const gchar        *sender,
                                              const gchar        *object_path,
                                              const gchar        *interface_name,
                                              const gchar        *property_name,
                                              GError            **error,
                                              IBusService        *service);
static gboolean  ibus_service_service_set_property_cb
                                             (GDBusConnection    *connection,
                                              const gchar        *sender,
                                              const gchar        *object_path,
                                              const gchar        *interface_name,
                                              const gchar        *property_name,
                                              GVariant           *value,
                                              GError            **error,
                                              IBusService        *service);
static void      ibus_service_connection_closed_cb
                                             (GDBusConnection    *connection,
                                              gboolean            remote_peer_vanished,
                                              GError             *error,
                                              IBusService        *service);
static void      ibus_service_unregister_cb  (GDBusConnection    *connection,
                                              guint              *ids,
                                              IBusService        *service);

static const GDBusInterfaceVTable ibus_service_interface_vtable = {
    (GDBusInterfaceMethodCallFunc) ibus_service_service_method_call_cb,
    (GDBusInterfaceGetPropertyFunc) ibus_service_service_get_property_cb,
    (GDBusInterfaceSetPropertyFunc) ibus_service_service_set_property_cb
};

static IBusObjectClass *ibus_service_parent_class = NULL;
static gint ibus_service_private_offset;

static inline gpointer
ibus_service_get_instance_private (IBusService *self)
{
    return (G_STRUCT_MEMBER_P (self, ibus_service_private_offset));
}

GType
ibus_service_get_type (void)
{
    static GType type = 0;

    static const GTypeInfo type_info = {
        sizeof (IBusServiceClass),
        (GBaseInitFunc)     ibus_service_base_init,
        (GBaseFinalizeFunc) ibus_service_base_fini,
        (GClassInitFunc)    ibus_service_class_init,
        NULL,               /* class finalize */
        NULL,               /* class data */
        sizeof (IBusService),
        0,
        (GInstanceInitFunc) ibus_service_init,
    };

    if (type == 0) {
        type = g_type_register_static (IBUS_TYPE_OBJECT,
                                       "IBusService",
                                       &type_info,
                                       0);
        ibus_service_private_offset =
                g_type_add_instance_private (type,
                                             sizeof (IBusServicePrivate));
    }

    return type;
}

static void
ibus_service_base_init (IBusServiceClass *class)
{
    GArray *old = class->interfaces;
    class->interfaces = g_array_new (TRUE, TRUE, sizeof (GDBusInterfaceInfo *));
    if (old != NULL) {
        GDBusInterfaceInfo **p = (GDBusInterfaceInfo **)old->data;
        while (*p != NULL) {
            g_array_append_val (class->interfaces, *p++);
        }
    }
}

static void
ibus_service_base_fini (IBusServiceClass *class)
{
    GDBusInterfaceInfo **interfaces = (GDBusInterfaceInfo **) g_array_free (class->interfaces, FALSE);
    GDBusInterfaceInfo **p = interfaces;
    while (*p != NULL) {
        g_dbus_interface_info_unref (*p++);
    }
    g_free (interfaces);
}

static const gchar introspection_xml[] =
    "<node>"
    "  <interface name='org.freedesktop.IBus.Service'>"
    "    <method name='Destroy' />"
    "  </interface>"
    "</node>";

static void
ibus_service_class_init (IBusServiceClass *class)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (class);
    IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (class);

    ibus_service_parent_class =
            IBUS_OBJECT_CLASS (g_type_class_peek_parent (class));
    if (ibus_service_private_offset) {
        g_type_class_adjust_private_offset (class,
                                            &ibus_service_private_offset);
    }

    gobject_class->constructed  = ibus_service_constructed;
    gobject_class->set_property =
            (GObjectSetPropertyFunc) ibus_service_set_property;
    gobject_class->get_property =
            (GObjectGetPropertyFunc) ibus_service_get_property;
    ibus_object_class->destroy  =
            (IBusObjectDestroyFunc) ibus_service_destroy;

    /* virtual functions */
    class->service_method_call = ibus_service_service_method_call;
    class->service_get_property = ibus_service_service_get_property;
    class->service_set_property = ibus_service_service_set_property;

    /* class members */
    ibus_service_class_add_interfaces (class, introspection_xml);

    /* install properties */
    /**
     * IBusService:object-path:
     *
     * The path of service object.
     */
    g_object_class_install_property (
                    gobject_class,
                    PROP_OBJECT_PATH,
                    g_param_spec_string (
                        "object-path",
                        "object path",
                        "The path of service object",
                        NULL,
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY |
                        G_PARAM_STATIC_NAME |
                        G_PARAM_STATIC_NICK |
                        G_PARAM_STATIC_BLURB)
                    );
    /**
     * IBusService:connection:
     *
     * The connection of service object.
     */
    g_object_class_install_property (
                    gobject_class,
                    PROP_CONNECTION,
                    g_param_spec_object (
                        "connection",
                        "connection",
                        "The connection of service object",
                        G_TYPE_DBUS_CONNECTION,
                        G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY |
                        G_PARAM_STATIC_NAME |
                        G_PARAM_STATIC_NICK |
                        G_PARAM_STATIC_BLURB)
                    );
}

static void
ibus_service_init (IBusService *service)
{
    service->priv = IBUS_SERVICE_GET_PRIVATE (service);
    service->priv->table = g_hash_table_new (NULL, NULL);
}

static void
ibus_service_constructed (GObject *object)
{
    IBusService *service = (IBusService *)object;
    if (service->priv->connection) {
        GError *error = NULL;
        if (!ibus_service_register (service, service->priv->connection, &error)) {
            g_warning ("%s", error->message);
            g_error_free (error);
        }
    }
    service->priv->constructed = TRUE;
}

static void
ibus_service_set_property (IBusService  *service,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
    switch (prop_id) {
    case PROP_OBJECT_PATH:
        service->priv->object_path = g_value_dup_string (value);
        break;
    case PROP_CONNECTION:
        service->priv->connection = g_value_dup_object (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (service, prop_id, pspec);
    }
}

static void
ibus_service_get_property (IBusService *service,
                           guint        prop_id,
                           GValue      *value,
                           GParamSpec  *pspec)
{
    switch (prop_id) {
    case PROP_OBJECT_PATH:
        g_value_set_string (value, service->priv->object_path);
        break;
    case PROP_CONNECTION:
        g_value_set_object (value, service->priv->connection);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (service, prop_id, pspec);
    }
}

static void
ibus_service_destroy (IBusService *service)
{
    g_free (service->priv->object_path);
    service->priv->object_path = NULL;

    if (service->priv->connection) {
        g_object_unref (service->priv->connection);
        service->priv->connection = NULL;
    }

    if (service->priv->table) {
        g_hash_table_foreach_remove (service->priv->table,
                        (GHRFunc)ibus_service_unregister_cb, service);
        g_hash_table_destroy (service->priv->table);
        service->priv->table = NULL;
    }

    IBUS_OBJECT_CLASS(ibus_service_parent_class)->destroy (IBUS_OBJECT (service));
}


static void
ibus_service_service_method_call (IBusService           *service,
                                  GDBusConnection       *connection,
                                  const gchar           *sender,
                                  const gchar           *object_path,
                                  const gchar           *interface_name,
                                  const gchar           *method_name,
                                  GVariant              *parameters,
                                  GDBusMethodInvocation *invocation)
{
    if (g_strcmp0 (method_name, "Destroy") == 0) {
        g_dbus_method_invocation_return_value (invocation, NULL);
        ibus_object_destroy ((IBusObject *)service);
        return;
    }

    g_dbus_method_invocation_return_error (invocation,
                                           G_DBUS_ERROR,
                                           G_DBUS_ERROR_UNKNOWN_METHOD,
                                           "%s::%s",
                                           interface_name, method_name);
    g_return_if_reached ();
}

static GVariant *
ibus_service_service_get_property (IBusService     *service,
                                  GDBusConnection  *connection,
                                  const gchar      *sender,
                                  const gchar      *object_path,
                                  const gchar      *interface_name,
                                  const gchar      *property_name,
                                  GError          **error)
{
    if (error) {
        *error = NULL;
        g_set_error (error,
                     G_DBUS_ERROR,
                     G_DBUS_ERROR_FAILED,
                     "service_get_property received an unknown property: %s",
                     property_name ? property_name : "(null)");
    }
    g_return_val_if_reached (NULL);
}

static gboolean
ibus_service_service_set_property (IBusService     *service,
                                  GDBusConnection  *connection,
                                  const gchar      *sender,
                                  const gchar      *object_path,
                                  const gchar      *interface_name,
                                  const gchar      *property_name,
                                  GVariant         *value,
                                  GError          **error)
{
    if (error) {
        *error = NULL;
        g_set_error (error,
                     G_DBUS_ERROR,
                     G_DBUS_ERROR_FAILED,
                     "service_set_property received an unknown property: %s",
                     property_name ? property_name : "(null)");
    }
    g_return_val_if_reached (FALSE);
}

static void
ibus_service_service_method_call_cb (GDBusConnection       *connection,
                                     const gchar           *sender,
                                     const gchar           *object_path,
                                     const gchar           *interface_name,
                                     const gchar           *method_name,
                                     GVariant              *parameters,
                                     GDBusMethodInvocation *invocation,
                                     IBusService           *service)
{
    IBUS_SERVICE_GET_CLASS (service)->service_method_call (service,
                                                           connection,
                                                           sender,
                                                           object_path,
                                                           interface_name,
                                                           method_name,
                                                           parameters,
                                                           invocation);
}

static GVariant *
ibus_service_service_get_property_cb (GDBusConnection   *connection,
                                      const gchar       *sender,
                                      const gchar       *object_path,
                                      const gchar       *interface_name,
                                      const gchar       *property_name,
                                      GError           **error,
                                      IBusService       *service)
{
    return IBUS_SERVICE_GET_CLASS (service)->service_get_property (service,
                                                                   connection,
                                                                   sender,
                                                                   object_path,
                                                                   interface_name,
                                                                   property_name,
                                                                   error);
}

static gboolean
ibus_service_service_set_property_cb (GDBusConnection    *connection,
                                      const gchar        *sender,
                                      const gchar        *object_path,
                                      const gchar        *interface_name,
                                      const gchar        *property_name,
                                      GVariant           *value,
                                      GError            **error,
                                      IBusService        *service)
{
    return IBUS_SERVICE_GET_CLASS (service)->service_set_property (service,
                                                                   connection,
                                                                   sender,
                                                                   object_path,
                                                                   interface_name,
                                                                   property_name,
                                                                   value,
                                                                   error);
}

static void
ibus_service_connection_closed_cb (GDBusConnection    *connection,
                                   gboolean            remote_peer_vanished,
                                   GError             *error,
                                   IBusService        *service)
{
    ibus_service_unregister (service, connection);
    if (connection == service->priv->connection) {
        ibus_object_destroy ((IBusObject *) service);
    }
}

static void
ibus_service_unregister_cb (GDBusConnection    *connection,
                            guint              *ids,
                            IBusService        *service)
{
    guint *p = ids;
    while (*p != 0) {
        g_dbus_connection_unregister_object (connection, *p++);
    }
    g_signal_handlers_disconnect_by_func (connection,
                    G_CALLBACK (ibus_service_connection_closed_cb), service);
    g_object_unref (connection);
    g_free (ids);
}

IBusService *
ibus_service_new (GDBusConnection *connection,
                  const gchar     *object_path)
{
    g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), NULL);
    g_return_val_if_fail (object_path != NULL, NULL);

    GObject *obj = g_object_new (IBUS_TYPE_SERVICE,
                                 "object-path", object_path,
                                 "connection", connection,
                                 NULL);
    return IBUS_SERVICE (obj);
}

const gchar *
ibus_service_get_object_path (IBusService *service)
{
    g_return_val_if_fail (IBUS_IS_SERVICE (service), NULL);
    return service->priv->object_path;
}

GDBusConnection *
ibus_service_get_connection (IBusService *service)
{
    g_return_val_if_fail (IBUS_IS_SERVICE (service), NULL);
    return service->priv->connection;
}

gboolean
ibus_service_register (IBusService     *service,
                       GDBusConnection *connection,
                       GError         **error)
{
    GArray *array = NULL;
    GArray *interfaces;
    GDBusInterfaceInfo **p;

    g_return_val_if_fail (IBUS_IS_SERVICE (service), FALSE);
    g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    if (g_hash_table_lookup (service->priv->table, connection)) {
        if (error) {
            *error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_OBJECT_PATH_IN_USE,
                            "Service %p has been registered with connection %p.",
                            service, connection);
        }
        goto error_out;
    }

    interfaces = IBUS_SERVICE_GET_CLASS (service)->interfaces;
    g_assert (interfaces);
    p = (GDBusInterfaceInfo **)interfaces->data;
    if (*p == NULL) {
        if (error) {
            *error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                            "Service %p does not have any interface.",
                            service);
        }
        goto error_out;
    }

    array = g_array_new (TRUE, TRUE, sizeof (guint));
    while (*p != NULL) {
        guint id = g_dbus_connection_register_object (connection,
                                                      service->priv->object_path,
                                                      *p,
                                                      &ibus_service_interface_vtable,
                                                      g_object_ref (service),
                                                      (GDestroyNotify)g_object_unref,
                                                      error);
        if (id != 0) {
            g_array_append_val (array, id);
        }
        else {
            g_object_unref (service);
            goto error_out;
        }
        p++;
    }

    g_signal_connect (connection, "closed",
                    G_CALLBACK (ibus_service_connection_closed_cb), service);
    g_hash_table_insert (service->priv->table,
                    g_object_ref (connection), g_array_free (array, FALSE));
    return TRUE;

error_out:
    if (array != NULL) {
        guint *ids = (guint*) array->data;
        while (*ids != 0) {
            g_dbus_connection_unregister_object (connection, *ids++);
        }
        g_array_free (array, TRUE);
    }
    return FALSE;
}

void
ibus_service_unregister (IBusService     *service,
                         GDBusConnection *connection)
{
    g_return_if_fail (IBUS_IS_SERVICE (service));
    g_return_if_fail (G_IS_DBUS_CONNECTION (connection));

    guint *ids = (guint *) g_hash_table_lookup (service->priv->table, connection);
    g_return_if_fail (ids != NULL);

    g_hash_table_remove (service->priv->table, connection);
    ibus_service_unregister_cb (connection, ids, service);
}

gboolean
ibus_service_emit_signal (IBusService *service,
                          const gchar *dest_bus_name,
                          const gchar *interface_name,
                          const gchar *signal_name,
                          GVariant    *parameters,
                          GError    **error)
{
    g_return_val_if_fail (IBUS_IS_SERVICE (service), FALSE);
    g_return_val_if_fail (interface_name != NULL, FALSE);
    g_return_val_if_fail (signal_name != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
    g_return_val_if_fail (service->priv->connection != NULL, FALSE);
    
    return g_dbus_connection_emit_signal (service->priv->connection,
                                          dest_bus_name,
                                          service->priv->object_path,
                                          interface_name,
                                          signal_name,
                                          parameters,
                                          error);
}

gboolean
ibus_service_class_add_interfaces (IBusServiceClass   *class,
                                   const gchar        *xml_data)
{
    g_return_val_if_fail (IBUS_IS_SERVICE_CLASS (class), FALSE);
    g_return_val_if_fail (xml_data != NULL, FALSE);

    GError *error = NULL;
    GDBusNodeInfo *introspection_data = g_dbus_node_info_new_for_xml (xml_data, &error);
    if (introspection_data == NULL) {
        g_warning ("%s", error->message);
        g_error_free (error);
        return FALSE;
    }
    else {
        GDBusInterfaceInfo **p = introspection_data->interfaces;
        while (*p != NULL) {
            g_dbus_interface_info_ref (*p);
            g_array_append_val (class->interfaces, *p);
            p++;
        }
        g_dbus_node_info_unref (introspection_data);
        return TRUE;
    }
}
