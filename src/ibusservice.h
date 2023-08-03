/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2015 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2011-2023 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2008-2015 Red Hat, Inc.
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

#if !defined (__IBUS_H_INSIDE__) && !defined (IBUS_COMPILATION)
#error "Only <ibus.h> can be included directly"
#endif

#ifndef __IBUS_SERVICE_H_
#define __IBUS_SERVICE_H_

/**
 * SECTION: ibusservice
 * @short_description: IBus service back-end.
 * @stability: Stable
 *
 * An IBusService is a base class for services.
 */

#include <gio/gio.h>
#include "ibusobject.h"

/*
 * Type macros.
 */

/* define GOBJECT macros */
#define IBUS_TYPE_SERVICE             \
    (ibus_service_get_type ())
#define IBUS_SERVICE(obj)             \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), IBUS_TYPE_SERVICE, IBusService))
#define IBUS_SERVICE_CLASS(klass)     \
    (G_TYPE_CHECK_CLASS_CAST ((klass), IBUS_TYPE_SERVICE, IBusServiceClass))
#define IBUS_IS_SERVICE(obj)          \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IBUS_TYPE_SERVICE))
#define IBUS_IS_SERVICE_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), IBUS_TYPE_SERVICE))
#define IBUS_SERVICE_GET_CLASS(obj)   \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), IBUS_TYPE_SERVICE, IBusServiceClass))

G_BEGIN_DECLS

typedef struct _IBusService IBusService;
typedef struct _IBusServiceClass IBusServiceClass;
typedef struct _IBusServicePrivate IBusServicePrivate;

/**
 * IBusService:
 *
 * An opaque data type representing an IBusService.
 */
struct _IBusService {
    /*< private >*/
    IBusObject parent;
    IBusServicePrivate *priv;
};

struct _IBusServiceClass {
    /*< private >*/
    IBusObjectClass parent;

    /*< public >*/
    /* virtual functions */
    /**
     * IBusServiceClass::service_method_call:
     * @service: An #IBusService.
     * @connection: A dbus connection.
     * @sender: A sender.
     * @object_path: An object path.
     * @interface_name: An interface name.
     * @method_name: A method name.
     * @parameters: A parameters.
     * @invocation: A dbus method invocation.
     *
     * The ::service_method_call class method is to connect
     * GDBusInterfaceMethodCallFunc().
     */
    void        (* service_method_call)
                                    (IBusService        *service,
                                     GDBusConnection    *connection,
                                     const gchar        *sender,
                                     const gchar        *object_path,
                                     const gchar        *interface_name,
                                     const gchar        *method_name,
                                     GVariant           *parameters,
                                     GDBusMethodInvocation
                                                        *invocation);
    /**
     * IBusServiceClass::service_get_property:
     * @service: An #IBusService.
     * @connection: A dbus connection.
     * @sender: A sender.
     * @object_path: An object path.
     * @interface_name: An interface name.
     * @property_name: A property name.
     * @error: Return location for error or %NULL.
     *
     * The ::service_get_property class method is to connect
     * GDBusInterfaceGetPropertyFunc().
     *
     * Returns: (nullable) (transfer full): A variant.
     */
    GVariant *  (* service_get_property)
                                    (IBusService        *service,
                                     GDBusConnection    *connection,
                                     const gchar        *sender,
                                     const gchar        *object_path,
                                     const gchar        *interface_name,
                                     const gchar        *property_name,
                                     GError            **error);
    /**
     * IBusServiceClass::service_set_property:
     * @service: An #IBusService.
     * @connection: A dbus connection.
     * @sender: A sender.
     * @object_path: An object path.
     * @interface_name: An interface name.
     * @property_name: An property name.
     * @value: An property value.
     * @error: Return location for error or %NULL.
     *
     * The ::service_set_property class method is to connect
     * GDBusInterfaceSetPropertyFunc().
     *
     * Returns: %TRUE if set the value else %FALSE.
     */
    gboolean    (* service_set_property)
                                    (IBusService        *service,
                                     GDBusConnection    *connection,
                                     const gchar        *sender,
                                     const gchar        *object_path,
                                     const gchar        *interface_name,
                                     const gchar        *property_name,
                                     GVariant           *value,
                                     GError            **error);
    /*< private >*/
    GArray *interfaces;

    /* padding */
    gpointer pdummy[4];
};


GType            ibus_service_get_type          (void);

/**
 * ibus_service_new:
 * @connection: A GDBusConnection.
 * @path: Object path.
 *
 * Creantes a new #IBusService.
 *
 * Returns: A newly allocated #IBusService
 */
IBusService     *ibus_service_new               (GDBusConnection    *connection,
                                                 const gchar        *path);
/**
 * ibus_service_get_object_path:
 * @service: An IBusService.
 *
 * Gets the object path of an IBusService.
 *
 * Returns: The object path of @service
 */
const gchar     *ibus_service_get_object_path   (IBusService        *service);

/**
 * ibus_service_get_connection:
 * @service: An IBusService.
 *
 * Gets a connections.
 *
 * Returns: (transfer none): A #GDBusConnection of an #IBusService instance.
 */
GDBusConnection *ibus_service_get_connection    (IBusService        *service);

/**
 * ibus_service_register:
 * @service: An IBusService.
 * @connection: A GDBusConnection the service will be registered to.
 * @error: Return location for error or NULL.
 *
 * Registers service to a connection.
 *
 * Returns: %TRUE if the service was registered, %FALSE otherwise.
 */
gboolean         ibus_service_register          (IBusService        *service,
                                                 GDBusConnection    *connection,
                                                 GError            **error);
/**
 * ibus_service_unregister:
 * @service: An IBusService.
 * @connection: A GDBusConnection the service was registered with.
 *
 * Unregisters service from a connection.
 */
void             ibus_service_unregister        (IBusService        *service,
                                                 GDBusConnection    *connection);



/**
 * ibus_service_send_signal:
 * @service: An IBusService.
 * @interface: The interface the signal is emitted from.
 * @name: Name of the signal.
 * @first_arg_type: Type of first argument.
 * @...: Rest of arguments, NULL to mark the end.
 *
 * Sends signal to all the #IBusConnections of an #IBusService.
 *
 * Returns: %TRUE if succeed; %FALSE otherwise.
 *
 * see_also: g_dbus_connection_emit_signal()
 */
gboolean         ibus_service_emit_signal       (IBusService        *service,
                                                 const gchar        *dest_bus_name,
                                                 const gchar        *interface_name,
                                                 const gchar        *signal_name,
                                                 GVariant           *parameters,
                                                 GError            **error);
/**
 * ibus_service_class_add_interfaces:
 * @klass: An IBusServiceClass.
 * @xml_data: The introspection xml data.
 *
 * Set the interface introspection information with the service class.
 *
 * Returns: %TRUE if @xml_data is valid and succeeded to be added;
 *          %FALSE otherwise.
 */
gboolean         ibus_service_class_add_interfaces
                                                (IBusServiceClass   *klass,
                                                 const gchar        *xml_data);

/**
 * ibus_service_class_free_interfaces:
 * @klass: An IBusServiceClass.
 * @depth: The number of D-Bus introspection interfaces.
 *
 * Free the first @depth interfaces if @depth is positive. Free the last
 * -@depth interfaces if @depth is negative.
 *
 * Returns: The actual freed number of the introspection interfaces
 * if @depth is not 0.
 * If @depth is 0, the total number of the introspection interfaces is
 * returned but any interfaces are not freed.
 */
int              ibus_service_class_free_interfaces
                                                (IBusServiceClass   *klass,
                                                 int                 depth);

G_END_DECLS
#endif

