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

#if !defined (__IBUS_H_INSIDE__) && !defined (IBUS_COMPILATION)
#error "Only <ibus.h> can be included directly"
#endif

#ifndef __IBUS_BUS_H_
#define __IBUS_BUS_H_

/**
 * SECTION: ibusbus
 * @short_description: Connect with IBus daemon.
 * @stability: Stable
 *
 * An IBusBus connects with IBus daemon.
 */
#include <gio/gio.h>
#include "ibusinputcontext.h"
#include "ibusconfig.h"
#include "ibuscomponent.h"

/*
 * Type macros.
 */

/* define GOBJECT macros */
#define IBUS_TYPE_BUS             \
    (ibus_bus_get_type ())
#define IBUS_BUS(obj)             \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), IBUS_TYPE_BUS, IBusBus))
#define IBUS_BUS_CLASS(klass)     \
    (G_TYPE_CHECK_CLASS_CAST ((klass), IBUS_TYPE_BUS, IBusBusClass))
#define IBUS_IS_BUS(obj)          \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IBUS_TYPE_BUS))
#define IBUS_IS_BUS_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), IBUS_TYPE_BUS))
#define IBUS_BUS_GET_CLASS(obj)   \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), IBUS_TYPE_BUS, IBusBusClass))

G_BEGIN_DECLS

typedef struct _IBusBus IBusBus;
typedef struct _IBusBusClass IBusBusClass;
typedef struct _IBusBusPrivate IBusBusPrivate;

/**
 * IBusBus:
 *
 * An opaque data type representing IBus bus (daemon communication) status.
 */
struct _IBusBus {
    IBusObject parent;
    /* instance members */

    IBusBusPrivate *priv;
};

struct _IBusBusClass {
    IBusObjectClass parent;
    /* class members */
};

GType        ibus_bus_get_type          (void);

/**
 * ibus_bus_new:
 * @returns: A newly allocated #IBusBus instance, and the instance is not floating.
 *
 * New an #IBusBus instance.
 */
IBusBus     *ibus_bus_new               (void);

/**
 * ibus_bus_new_async:
 * @returns: A newly allocated #IBusBus instance, and the instance is not floating.
 *
 * New an #IBusBus instance. The instance will asynchronously connect to the IBus
 * daemon.
 */
IBusBus     *ibus_bus_new_async         (void);


/**
 * ibus_bus_is_connected:
 * @bus: An #IBusBus.
 * @returns: %TRUE if @bus is connected, %FALSE otherwise.
 *
 * Return %TRUE if @bus is connected to IBus daemon.
 */
gboolean     ibus_bus_is_connected      (IBusBus        *bus);

/**
 * ibus_bus_get_connection:
 * @bus: An #IBusBus.
 * @returns: (transfer none): A #GDBusConnection of an #IBusBus instance.
 *
 * Return #GDBusConnection of an #IBusBus instance.
 */
GDBusConnection *
             ibus_bus_get_connection    (IBusBus        *bus);

/**
 * ibus_bus_hello:
 * @bus: An #IBusBus.
 * @returns: The unique name of IBus process in DBus.
 *
 * This function sends a "HELLO" message to DBus daemon,
 * which replies the unique name of current IBus process.
 */
const gchar *ibus_bus_hello             (IBusBus        *bus);

/**
 * ibus_bus_request_name:
 * @bus: the IBusBus instance to be processed.
 * @name: Name to be requested.
 * @flags: IBusBusNameFlag.
 * @returns: 0 if failed; IBusBusRequestNameReply otherwise.
 *
 * Request a name from IBus daemon synchronously.
 */
guint32      ibus_bus_request_name      (IBusBus        *bus,
                                         const gchar    *name,
                                         guint32         flags);

/**
 * ibus_bus_request_name_async:
 * @bus: An #IBusBus.
 * @name: Name to be requested.
 * @flags: Flags (FixMe).
 * @timeout_msec: The timeout in milliseconds or -1 to use the default timeout.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied or %NULL
 *      if you don't care about the result of the method invocation.
 * @user_data: The data to pass to callback.
 *
 * Request a name from IBus daemon asynchronously.
 */
void        ibus_bus_request_name_async (IBusBus        *bus,
                                         const gchar    *name,
                                         guint           flags,
                                         gint            timeout_msec,
                                         GCancellable   *cancellable,
                                         GAsyncReadyCallback
                                                         callback,
                                         gpointer        user_data);

/**
 * ibus_bus_request_name_async_finish:
 * @bus: An #IBusBus.
 * @res: A #GAsyncResult obtained from the #GAsyncReadyCallback passed to
 *   ibus_bus_request_name_async().
 * @error: Return location for error or %NULL.
 * @returns: 0 if failed; positive number otherwise.
 *
 * Finishes an operation started with ibus_bus_request_name_async().
 */
guint       ibus_bus_request_name_async_finish
                                        (IBusBus        *bus,
                                         GAsyncResult   *res,
                                         GError        **error);

/**
 * ibus_bus_release_name:
 * @bus: An #IBusBus.
 * @name: Name to be released.
 * @returns: 0 if failed; positive number otherwise.
 *
 * Release a name to IBus daemon synchronously.
 */
guint        ibus_bus_release_name      (IBusBus        *bus,
                                         const gchar    *name);

/**
 * ibus_bus_release_name_async:
 * @bus: An #IBusBus.
 * @name: Name to be released.
 * @timeout_msec: The timeout in milliseconds or -1 to use the default timeout.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied
 *      or %NULL if you don't care about the result of the method invocation.
 * @user_data: The data to pass to callback.
 *
 * Release a name to IBus daemon asynchronously.
 */
void         ibus_bus_release_name_async
                                        (IBusBus        *bus,
                                         const gchar    *name,
                                         gint            timeout_msec,
                                         GCancellable   *cancellable,
                                         GAsyncReadyCallback
                                                         callback,
                                         gpointer        user_data);

/**
 * ibus_bus_release_name_async_finish:
 * @bus: An #IBusBus.
 * @res: A #GAsyncResult obtained from the #GAsyncReadyCallback passed to
 *   ibus_bus_release_name_async().
 * @error: Return location for error or %NULL.
 * @returns: 0 if failed; positive number otherwise.
 *
 * Finishes an operation started with ibus_bus_release_name_async().
 */
guint        ibus_bus_release_name_async_finish
                                        (IBusBus        *bus,
                                         GAsyncResult   *res,
                                         GError        **error);

/**
 * ibus_bus_list_queued_owners:
 * @bus: An IBusBus.
 * @name: Name to be queried.
 * @returns: (transfer full) (element-type utf8):
 *           The unique bus names of connections currently queued for @name.
 *
 * Lists the unique bus names of connections currently queued for a bus name.
 *
 * FIXME add an asynchronous version.
 */
GList *      ibus_bus_list_queued_owners
                                        (IBusBus      *bus,
                                         const gchar  *name);

/**
 * ibus_bus_name_has_owner:
 * @bus: An #IBusBus.
 * @name: Name to be checked.
 * @returns: %TRUE if the name has owner, %FALSE otherwise.
 *
 * Checks whether the name has owner synchronously.
 */
gboolean     ibus_bus_name_has_owner    (IBusBus        *bus,
                                         const gchar    *name);

/**
 * ibus_bus_name_has_owner_async:
 * @bus: An #IBusBus.
 * @name: Name to be checked.
 * @timeout_msec: The timeout in milliseconds or -1 to use the default timeout.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied
 *      or %NULL if you don't care about the result of the method invocation.
 * @user_data: The data to pass to callback.
 *
 * Checks whether the name has owner asynchronously.
 */
void         ibus_bus_name_has_owner_async
                                        (IBusBus        *bus,
                                         const gchar    *name,
                                         gint            timeout_msec,
                                         GCancellable   *cancellable,
                                         GAsyncReadyCallback
                                                         callback,
                                         gpointer        user_data);

/**
 * ibus_bus_name_has_owner_async_finish:
 * @bus: An #IBusBus.
 * @res: A #GAsyncResult obtained from the #GAsyncReadyCallback passed to
 *   ibus_bus_name_has_owner_async().
 * @error: Return location for error or %NULL.
 * @returns: %TRUE if the name has owner, %FALSE otherwise.
 *
 * Finishes an operation started with ibus_bus_name_has_owner_async().
 */
gboolean     ibus_bus_name_has_owner_async_finish
                                        (IBusBus        *bus,
                                         GAsyncResult   *res,
                                         GError        **error);

/**
 * ibus_bus_list_names:
 * @bus: An #IBusBus.
 * @returns: (transfer full) (element-type utf8): Lists that attached to @bus.
 *
 * Return lists that attached to @bus.
 * <note><para>[FixMe] Not implemented yet, only return NULL.</para></note>
 * <note><para>[FixMe] Add async version.</para></note>
 */
GList       *ibus_bus_list_names        (IBusBus        *bus);

/**
 * ibus_bus_add_match:
 * @bus: An #IBusBus.
 * @rule: Match rule.
 * @returns: %TRUE if the rule is added. %FALSE otherwise.
 *
 * Add a match rule to an #IBusBus synchronously.
 */
gboolean     ibus_bus_add_match         (IBusBus        *bus,
                                         const gchar    *rule);

/**
 * ibus_bus_add_match_async:
 * @bus: An #IBusBus.
 * @rule: Match rule.
 * @timeout_msec: The timeout in milliseconds or -1 to use the default timeout.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied
 *      or %NULL if you don't care about the result of the method invocation.
 * @user_data: The data to pass to callback.
 *
 * Add a match rule to an #IBusBus asynchronously.
 */
void         ibus_bus_add_match_async   (IBusBus        *bus,
                                         const gchar    *rule,
                                         gint            timeout_msec,
                                         GCancellable   *cancellable,
                                         GAsyncReadyCallback
                                                         callback,
                                         gpointer        user_data);

/**
 * ibus_bus_add_match_async_finish:
 * @bus: An #IBusBus.
 * @res: A #GAsyncResult obtained from the #GAsyncReadyCallback passed to
 *   ibus_bus_add_match_async().
 * @error: Return location for error or %NULL.
 * @returns: %TRUE if the rule is added. %FALSE otherwise.
 *
 * Finishes an operation started with ibus_bus_add_match_async().
 */
gboolean     ibus_bus_add_match_async_finish
                                        (IBusBus        *bus,
                                         GAsyncResult   *res,
                                         GError        **error);

/**
 * ibus_bus_remove_match:
 * @bus: An #IBusBus.
 * @rule: Match rule.
 * @returns: %TRUE if the rule is removed. %FALSE otherwise.
 *
 * Remove a match rule to an #IBusBus synchronously.
 */
gboolean     ibus_bus_remove_match      (IBusBus        *bus,
                                         const gchar    *rule);

/**
 * ibus_bus_remove_match_async:
 * @bus: An #IBusBus.
 * @rule: Match rule.
 * @timeout_msec: The timeout in milliseconds or -1 to use the default timeout.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied
 *      or %NULL if you don't care about the result of the method invocation.
 * @user_data: The data to pass to callback.
 *
 * Remove a match rule to an IBusBus asynchronously.
 */
void         ibus_bus_remove_match_async
                                        (IBusBus        *bus,
                                         const gchar    *rule,
                                         gint            timeout_msec,
                                         GCancellable   *cancellable,
                                         GAsyncReadyCallback
                                                         callback,
                                         gpointer        user_data);

/**
 * ibus_bus_remove_match_async_finish:
 * @bus: An #IBusBus.
 * @res: A #GAsyncResult obtained from the #GAsyncReadyCallback passed to
 *   ibus_bus_remove_match_async().
 * @error: Return location for error or %NULL.
 * @returns: %TRUE if the rule is removed. %FALSE otherwise.
 *
 * Finishes an operation started with ibus_bus_remove_match_async().
 */
gboolean     ibus_bus_remove_match_async_finish
                                        (IBusBus        *bus,
                                         GAsyncResult   *res,
                                         GError        **error);

/**
 * ibus_bus_get_name_owner:
 * @bus: An #IBusBus.
 * @name: Name.
 * @returns: Owner of the name. The returned value must be freed with g_free().
 *
 * Return the name owner synchronously.
 */
gchar       *ibus_bus_get_name_owner    (IBusBus        *bus,
                                         const gchar    *name);

/**
 * ibus_bus_get_name_owner_async:
 * @bus: An #IBusBus.
 * @name: Name.
 * @timeout_msec: The timeout in milliseconds or -1 to use the default timeout.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied
 *      or %NULL if you don't care about the result of the method invocation.
 * @user_data: The data to pass to callback.
 *
 * Return the name owner asynchronously.
 */
void         ibus_bus_get_name_owner_async
                                        (IBusBus        *bus,
                                         const gchar    *name,
                                         gint            timeout_msec,
                                         GCancellable   *cancellable,
                                         GAsyncReadyCallback
                                                         callback,
                                         gpointer        user_data);

/**
 * ibus_bus_get_name_owner_async_finish:
 * @bus: An #IBusBus.
 * @res: A #GAsyncResult obtained from the #GAsyncReadyCallback passed to
 *   ibus_bus_get_name_owner_async().
 * @error: Return location for error or %NULL.
 * @returns: Owner of the name. The returned value must be freed with g_free().
 *
 * Finishes an operation started with ibus_bus_get_name_owner_async().
 */
gchar       *ibus_bus_get_name_owner_async_finish
                                        (IBusBus        *bus,
                                         GAsyncResult   *res,
                                         GError        **error);
/* declare ibus methods */

/**
 * ibus_bus_exit:
 * @bus: An #IBusBus.
 * @restart: Whether restarting the ibus.
 * @returns: %TRUE if the "Exit" call is suceeded, %FALSE otherwise.
 *
 * Exit or restart ibus-daemon synchronously.
 */
gboolean     ibus_bus_exit              (IBusBus        *bus,
                                         gboolean        restart);

/**
 * ibus_bus_exit_async:
 * @bus: An #IBusBus.
 * @restart: Whether restarting the ibus.
 * @timeout_msec: The timeout in milliseconds or -1 to use the default timeout.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied
 *      or %NULL if you don't care about the result of the method invocation.
 * @user_data: The data to pass to callback.
 *
 * Exit or restart ibus-daemon asynchronously.
 */
void        ibus_bus_exit_async         (IBusBus        *bus,
                                         gboolean        restart,
                                         gint            timeout_msec,
                                         GCancellable   *cancellable,
                                         GAsyncReadyCallback
                                                         callback,
                                         gpointer        user_data);

/**
 * ibus_bus_exit_async_finish:
 * @bus: An #IBusBus.
 * @res: A #GAsyncResult obtained from the #GAsyncReadyCallback passed to
 *   ibus_bus_exit_async().
 * @error: Return location for error or %NULL.
 * @returns: %TRUE if the "Exit" call is suceeded, %FALSE otherwise.
 *
 * Finishes an operation started with ibus_bus_exit_async().
 */
gboolean    ibus_bus_exit_async_finish  (IBusBus        *bus,
                                         GAsyncResult   *res,
                                         GError        **error);

/**
 * ibus_bus_create_input_context:
 * @bus: An #IBusBus.
 * @client_name: Name of client.
 * @returns: (transfer full): An newly allocated #IBusInputContext if the
 *      "CreateInputContext" call is suceeded, %NULL otherwise.
 *
 * Create an input context for client synchronously.
 */
IBusInputContext *
            ibus_bus_create_input_context
                                        (IBusBus        *bus,
                                         const gchar    *client_name);

/**
 * ibus_bus_create_input_context_async:
 * @bus: An #IBusBus.
 * @client_name: Name of client.
 * @timeout_msec: The timeout in milliseconds or -1 to use the default timeout.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied.
 *      It should not be %NULL.
 * @user_data: The data to pass to callback.
 *
 * Create an input context for client asynchronously.
 */
void        ibus_bus_create_input_context_async
                                        (IBusBus        *bus,
                                         const gchar    *client_name,
                                         gint            timeout_msec,
                                         GCancellable   *cancellable,
                                         GAsyncReadyCallback
                                                         callback,
                                         gpointer        user_data);

/**
 * ibus_bus_create_input_context_async_finish:
 * @bus: An #IBusBus.
 * @res: A #GAsyncResult obtained from the #GAsyncReadyCallback passed to
 *   ibus_bus_create_input_context_async().
 * @error: Return location for error or %NULL.
 * @returns: (transfer full): An newly allocated #IBusInputContext if the
 *      "CreateInputContext" call is suceeded, %NULL otherwise.
 *
 * Finishes an operation started with ibus_bus_create_input_context_async().
 */
IBusInputContext *
             ibus_bus_create_input_context_async_finish
                                        (IBusBus        *bus,
                                         GAsyncResult   *res,
                                         GError        **error);

/**
 * ibus_bus_current_input_context:
 * @bus: An #IBusBus.
 * @returns: The named of currently focued #IBusInputContext if the
 *            "CurrentInputContext" call suceeded, %NULL otherwise. The return
 *            value must be freed with g_free().
 *
 * Get the current focused input context synchronously.
 */
gchar       *ibus_bus_current_input_context
                                        (IBusBus        *bus);

/**
 * ibus_bus_current_input_context_async:
 * @bus: An #IBusBus.
 * @timeout_msec: The timeout in milliseconds or -1 to use the default timeout.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied
 *      or %NULL if you don't care about the result of the method invocation.
 * @user_data: The data to pass to callback.
 *
 * Get the current focused input context asynchronously.
 */
void         ibus_bus_current_input_context_async
                                        (IBusBus        *bus,
                                         gint            timeout_msec,
                                         GCancellable   *cancellable,
                                         GAsyncReadyCallback
                                                         callback,
                                         gpointer        user_data);

/**
 * ibus_bus_current_input_context_async_finish:
 * @bus: An #IBusBus.
 * @res: A #GAsyncResult obtained from the #GAsyncReadyCallback passed to
 *   ibus_bus_current_input_context_async().
 * @error: Return location for error or %NULL.
 * @returns: The named of currently focued IBusInputContext if the
 *            "CurrentInputContext" call suceeded, %NULL otherwise. The return
 *            value must be freed with g_free().
 *
 * Finishes an operation started with ibus_bus_current_input_context_async().
 */
gchar       *ibus_bus_current_input_context_async_finish
                                        (IBusBus        *bus,
                                         GAsyncResult   *res,
                                         GError        **error);

/**
 * ibus_bus_register_component:
 * @bus: An #IBusBus.
 * @component: A input engine component.
 * @returns: %TRUE if the "RegisterComponent" call is suceeded, %FALSE otherwise.
 *
 * Register a componet to an #IBusBus synchronously.
 */
gboolean     ibus_bus_register_component
                                        (IBusBus        *bus,
                                         IBusComponent  *component);

/**
 * ibus_bus_register_component_async:
 * @bus: An #IBusBus.
 * @component: A input engine component.
 * @timeout_msec: The timeout in milliseconds or -1 to use the default timeout.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied
 *      or %NULL if you don't care about the result of the method invocation.
 * @user_data: The data to pass to callback.
 *
 * Register a componet to an #IBusBus asynchronously.
 */
void         ibus_bus_register_component_async
                                        (IBusBus        *bus,
                                         IBusComponent  *component,
                                         gint            timeout_msec,
                                         GCancellable   *cancellable,
                                         GAsyncReadyCallback
                                                         callback,
                                         gpointer            user_data);

/**
 * ibus_bus_register_component_async_finish:
 * @bus: An #IBusBus.
 * @res: A #GAsyncResult obtained from the #GAsyncReadyCallback passed to
 *   ibus_bus_register_component_async().
 * @error: Return location for error or %NULL.
 * @returns: %TRUE if the "RegisterComponent" call is suceeded, %FALSE otherwise.
 *
 * Finishes an operation started with ibus_bus_register_component_async().
 */
gboolean     ibus_bus_register_component_async_finish
                                        (IBusBus        *bus,
                                         GAsyncResult   *res,
                                         GError        **error);

/**
 * ibus_bus_list_engines:
 * @bus: An #IBusBus.
 * @returns: (transfer container) (element-type IBusEngineDesc): A List of engines.
 *
 * List engines synchronously.
 */
GList       *ibus_bus_list_engines      (IBusBus        *bus);

/**
 * ibus_bus_list_engines_async:
 * @bus: An #IBusBus.
 * @timeout_msec: The timeout in milliseconds or -1 to use the default timeout.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied or %NULL
 *      if you don't care about the result of the method invocation.
 * @user_data: The data to pass to callback.
 *
 * List engines asynchronously.
 */
void         ibus_bus_list_engines_async
                                        (IBusBus        *bus,
                                         gint            timeout_msec,
                                         GCancellable   *cancellable,
                                         GAsyncReadyCallback
                                                         callback,
                                         gpointer        user_data);

/**
 * ibus_bus_list_engines_async_finish:
 * @bus: An #IBusBus.
 * @res: A #GAsyncResult obtained from the #GAsyncReadyCallback passed to
 *   ibus_bus_list_engines_async().
 * @error: Return location for error or %NULL.
 * @returns: (transfer container) (element-type IBusEngineDesc): A List of engines.
 *
 * Finishes an operation started with ibus_bus_list_engines_async().
 */
GList       *ibus_bus_list_engines_async_finish
                                        (IBusBus        *bus,
                                         GAsyncResult   *res,
                                         GError        **error);

/**
 * ibus_bus_list_active_engines:
 * @bus: An #IBusBus.
 * @returns: (transfer container) (element-type IBusEngineDesc): A List of active engines.
 *
 * List active engines synchronously.
 */
GList       *ibus_bus_list_active_engines
                                        (IBusBus        *bus);

/**
 * ibus_bus_list_active_engines_async:
 * @bus: An #IBusBus.
 * @timeout_msec: The timeout in milliseconds or -1 to use the default timeout.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied or %NULL
 *      if you don't care about the result of the method invocation.
 * @user_data: The data to pass to callback.
 *
 * List active engines asynchronously.
 */
void         ibus_bus_list_active_engines_async
                                        (IBusBus        *bus,
                                         gint            timeout_msec,
                                         GCancellable   *cancellable,
                                         GAsyncReadyCallback
                                                         callback,
                                         gpointer        user_data);

/**
 * ibus_bus_list_active_engines_async_finish:
 * @bus: An #IBusBus.
 * @res: A #GAsyncResult obtained from the #GAsyncReadyCallback passed to
 *   ibus_bus_list_active_engines_async().
 * @error: Return location for error or %NULL.
 * @returns: (transfer container) (element-type IBusEngineDesc): A List of active engines.
 *
 * Finishes an operation started with ibus_bus_list_active_engines_async().
 */
GList       *ibus_bus_list_active_engines_async_finish
                                        (IBusBus        *bus,
                                         GAsyncResult   *res,
                                         GError         **error);

/**
 * ibus_bus_get_engines_by_names:
 * @bus: An #IBusBus.
 * @names: (array zero-terminated=1): A %NULL-terminated array of names.
 * @returns: (array zero-terminated=1) (transfer full): A %NULL-terminated array of engines.
 *
 * Get engines by given names synchronously. If some engine names do not exist, this function
 * will simplly ignore them, and return rest of engines.
 * TODO(penghuang): add asynchronous version
 */
IBusEngineDesc **
             ibus_bus_get_engines_by_names
                                        (IBusBus             *bus,
                                         const gchar * const *names);
/**
 * ibus_bus_get_use_sys_layout:
 * @bus: An #IBusBus.
 * @returns: %TRUE if "use_sys_layout" option is enabled.
 *
 * Check if the bus's "use_sys_layout" option is enabled or not synchronously.
 */
gboolean     ibus_bus_get_use_sys_layout
                                        (IBusBus        *bus);

/**
 * ibus_bus_get_use_sys_layout_async:
 * @bus: An #IBusBus.
 * @timeout_msec: The timeout in milliseconds or -1 to use the default timeout.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied
 *      or %NULL if you don't care about the result of the method invocation.
 * @user_data: The data to pass to callback.
 *
 * Check if the bus's "use_sys_layout" option is enabled or not asynchronously.
 */
void         ibus_bus_get_use_sys_layout_async
                                        (IBusBus        *bus,
                                         gint            timeout_msec,
                                         GCancellable   *cancellable,
                                         GAsyncReadyCallback
                                                         callback,
                                         gpointer        user_data);

/**
 * ibus_bus_get_use_sys_layout_async_finish:
 * @bus: An #IBusBus.
 * @res: A #GAsyncResult obtained from the #GAsyncReadyCallback passed to
 *   ibus_bus_get_use_sys_layout_async().
 * @error: Return location for error or %NULL.
 * @returns: TRUE if "use_sys_layout" option is enabled.
 *
 * Finishes an operation started with ibus_bus_get_use_sys_layout_async().
 */
gboolean     ibus_bus_get_use_sys_layout_async_finish
                                        (IBusBus        *bus,
                                         GAsyncResult   *res,
                                         GError        **error);

/**
 * ibus_bus_get_use_global_engine:
 * @bus: An #IBusBus.
 * @returns: TRUE if "use_global_engine" option is enabled.
 *
 * Check if the bus's "use_global_engine" option is enabled or not synchronously.
 */
gboolean     ibus_bus_get_use_global_engine
                                        (IBusBus        *bus);

/**
 * ibus_bus_get_use_global_engine_async:
 * @bus: An #IBusBus.
 * @timeout_msec: The timeout in milliseconds or -1 to use the default timeout.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied
 *      or %NULL if you don't care about the result of the method invocation.
 * @user_data: The data to pass to callback.
 *
 * Check if the bus's "use_global_engine" option is enabled or not asynchronously.
 */
void         ibus_bus_get_use_global_engine_async
                                        (IBusBus        *bus,
                                         gint            timeout_msec,
                                         GCancellable   *cancellable,
                                         GAsyncReadyCallback
                                                         callback,
                                         gpointer        user_data);

/**
 * ibus_bus_get_use_global_engine_async_finish:
 * @bus: An #IBusBus.
 * @res: A #GAsyncResult obtained from the #GAsyncReadyCallback passed to
 *   ibus_bus_get_use_global_engine_async().
 * @error: Return location for error or %NULL.
 * @returns: %TRUE if "use_global_engine" option is enabled.
 *
 * Finishes an operation started with ibus_bus_get_use_global_engine_async().
 */
gboolean     ibus_bus_get_use_global_engine_async_finish
                                        (IBusBus        *bus,
                                         GAsyncResult   *res,
                                         GError         **error);

/**
 * ibus_bus_is_global_engine_enabled:
 * @bus: An #IBusBus.
 * @returns: %TRUE if the current global engine is enabled.
 *
 * Check if the current global engine is enabled or not synchronously.
 */
gboolean     ibus_bus_is_global_engine_enabled
                                        (IBusBus        *bus);

/**
 * ibus_bus_is_global_engine_enabled_async:
 * @bus: An #IBusBus.
 * @timeout_msec: The timeout in milliseconds or -1 to use the default timeout.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied
 *      or %NULL if you don't care about the result of the method invocation.
 * @user_data: The data to pass to callback.
 *
 * Check if the current global engine is enabled or not asynchronously.
 */
void         ibus_bus_is_global_engine_enabled_async
                                        (IBusBus        *bus,
                                         gint            timeout_msec,
                                         GCancellable   *cancellable,
                                         GAsyncReadyCallback
                                                         callback,
                                        gpointer         user_data);

/**
 * ibus_bus_is_global_engine_enabled_async_finish:
 * @bus: An #IBusBus.
 * @res: A #GAsyncResult obtained from the #GAsyncReadyCallback passed to
 *   ibus_bus_is_global_engine_enabled_async().
 * @error: Return location for error or %NULL.
 * @returns: %TRUE if the current global engine is enabled.
 *
 * Finishes an operation started with ibus_bus_is_global_engine_enabled_async().
 */
gboolean     ibus_bus_is_global_engine_enabled_async_finish
                                        (IBusBus        *bus,
                                         GAsyncResult   *res,
                                         GError        **error);

/**
 * ibus_bus_get_current_engine:
 * @bus: An #IBusBus.
 * @returns: (transfer none): The description of current current engine,
 * or %NULL if there is no current engine.
 *
 * Get the description of current current engine synchronously.
 */
IBusEngineDesc *
             ibus_bus_get_current_engine (IBusBus        *bus);

/**
 * ibus_bus_get_current_engine_async:
 * @bus: An #IBusBus.
 * @timeout_msec: The timeout in milliseconds or -1 to use the default timeout.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied or %NULL
 *      if you don't care about the result of the method invocation.
 * @user_data: The data to pass to callback.
 *
 * Get the description of current engine asynchronously.
 */
void         ibus_bus_get_current_engine_async
                                        (IBusBus        *bus,
                                         gint            timeout_msec,
                                         GCancellable   *cancellable,
                                         GAsyncReadyCallback
                                                         callback,
                                         gpointer        user_data);

/**
 * ibus_bus_get_current_engine_async_finish:
 * @bus: An #IBusBus.
 * @res: A #GAsyncResult obtained from the #GAsyncReadyCallback passed to
 *   ibus_bus_get_current_engine_async_finish().
 * @error: Return location for error or %NULL.
 * @returns: (transfer none): The description of current current engine,
 * or %NULL if there is no current engine.
 *
 * Finishes an operation started with ibus_bus_get_current_engine_async_finish().
 */
IBusEngineDesc *
             ibus_bus_get_current_engine_async_finish
                                        (IBusBus        *bus,
                                         GAsyncResult   *res,
                                         GError        **error);

/**
 * ibus_bus_set_current_engine:
 * @bus: An #IBusBus.
 * @current_engine: A new engine name.
 * @returns: %TRUE if the current engine was set successfully.
 *
 * Set current engine synchronously.
 */
gboolean     ibus_bus_set_current_engine (IBusBus        *bus,
                                         const gchar    *current_engine);

/**
 * ibus_bus_set_current_engine_async:
 * @bus: An #IBusBus.
 * @current_engine: A new engine name.
 * @timeout_msec: The timeout in milliseconds or -1 to use the default timeout.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied
 *      or %NULL if you don't care about the result of the method invocation.
 * @user_data: The data to pass to callback.
 *
 * Set current engine asynchronously.
 */
void         ibus_bus_set_current_engine_async
                                        (IBusBus        *bus,
                                         const gchar    *current_engine,
                                         gint            timeout_msec,
                                         GCancellable   *cancellable,
                                         GAsyncReadyCallback
                                                         callback,
                                         gpointer        user_data);

/**
 * ibus_bus_set_current_engine_async_finish:
 * @bus: An #IBusBus.
 * @res: A #GAsyncResult obtained from the #GAsyncReadyCallback passed to
 *   ibus_bus_set_current_engine_async().
 * @error: Return location for error or %NULL.
 * @returns: %TRUE if no IPC errros. %FALSE otherwise.
 *
 * Finishes an operation started with ibus_bus_set_current_engine_async().
 */
gboolean     ibus_bus_set_current_engine_async_finish
                                        (IBusBus        *bus,
                                         GAsyncResult   *res,
                                         GError        **error);

/**
 * ibus_bus_get_global_engine:
 * @bus: An #IBusBus.
 * @returns: (transfer none): The description of current global engine,
 * or %NULL if there is no global engine.
 *
 * Get the description of current global engine synchronously.
 */
IBusEngineDesc *
             ibus_bus_get_global_engine (IBusBus        *bus);

/**
 * ibus_bus_get_global_engine_async:
 * @bus: An #IBusBus.
 * @timeout_msec: The timeout in milliseconds or -1 to use the default timeout.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied or %NULL
 *      if you don't care about the result of the method invocation.
 * @user_data: The data to pass to callback.
 *
 * Get the description of current global engine asynchronously.
 */
void         ibus_bus_get_global_engine_async
                                        (IBusBus        *bus,
                                         gint            timeout_msec,
                                         GCancellable   *cancellable,
                                         GAsyncReadyCallback
                                                         callback,
                                         gpointer        user_data);

/**
 * ibus_bus_get_global_engine_async_finish:
 * @bus: An #IBusBus.
 * @res: A #GAsyncResult obtained from the #GAsyncReadyCallback passed to
 *   ibus_bus_get_global_engine_async_finish().
 * @error: Return location for error or %NULL.
 * @returns: (transfer none): The description of current global engine,
 * or %NULL if there is no global engine.
 *
 * Finishes an operation started with ibus_bus_get_global_engine_async_finish().
 */
IBusEngineDesc *
             ibus_bus_get_global_engine_async_finish
                                        (IBusBus        *bus,
                                         GAsyncResult   *res,
                                         GError        **error);

/**
 * ibus_bus_set_global_engine:
 * @bus: An #IBusBus.
 * @global_engine: A new engine name.
 * @returns: %TRUE if the global engine was set successfully.
 *
 * Set current global engine synchronously.
 */
gboolean     ibus_bus_set_global_engine (IBusBus        *bus,
                                         const gchar    *global_engine);

/**
 * ibus_bus_set_global_engine_async:
 * @bus: An #IBusBus.
 * @global_engine: A new engine name.
 * @timeout_msec: The timeout in milliseconds or -1 to use the default timeout.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied
 *      or %NULL if you don't care about the result of the method invocation.
 * @user_data: The data to pass to callback.
 *
 * Set current global engine asynchronously.
 */
void         ibus_bus_set_global_engine_async
                                        (IBusBus        *bus,
                                         const gchar    *global_engine,
                                         gint            timeout_msec,
                                         GCancellable   *cancellable,
                                         GAsyncReadyCallback
                                                         callback,
                                         gpointer        user_data);

/**
 * ibus_bus_set_global_engine_async_finish:
 * @bus: An #IBusBus.
 * @res: A #GAsyncResult obtained from the #GAsyncReadyCallback passed to
 *   ibus_bus_set_global_engine_async().
 * @error: Return location for error or %NULL.
 * @returns: %TRUE if no IPC errros. %FALSE otherwise.
 *
 * Finishes an operation started with ibus_bus_set_global_engine_async().
 */
gboolean     ibus_bus_set_global_engine_async_finish
                                        (IBusBus        *bus,
                                         GAsyncResult   *res,
                                         GError        **error);

/**
 * ibus_bus_set_watch_dbus_signal:
 * @bus: An #IBusBus.
 * @watch: %TRUE if you want ibusbus to emit "name-owner-changed" signal when
 * ibus-daemon emits the NameOwnerChanged DBus signal.
 *
 * Start or stop watching the NameOwnerChanged DBus signal.
 */
void         ibus_bus_set_watch_dbus_signal
                                        (IBusBus        *bus,
                                         gboolean        watch);

/**
 * ibus_bus_set_watch_ibus_signal:
 * @bus: An #IBusBus.
 * @watch: %TRUE if you want ibusbus to emit "global-engine-changed" signal when
 * ibus-daemon emits the GlobalEngineChanged IBus signal.
 *
 * Start or stop watching the GlobalEngineChanged IBus signal.
 */
void         ibus_bus_set_watch_ibus_signal
                                        (IBusBus        *bus,
                                         gboolean        watch);

/* declare config apis */
/**
 * ibus_bus_get_config:
 * @bus: An #IBusBus.
 * @returns: (transfer none): An #IBusConfig object which is configurable with
 * @bus.
 *
 * Get the config instance from #IBusBus.
 */
IBusConfig  *ibus_bus_get_config        (IBusBus        *bus);

/**
 * ibus_bus_preload_engines:
 * @bus: An #IBusBus.
 * @names: (array zero-terminated=1): A %NULL-terminated array of engine names.
 * @returns: %TRUE if components start. %FALSE otherwise.
 *
 * Start bus components by engine names synchronously.
 */
gboolean     ibus_bus_preload_engines   (IBusBus        *bus,
                                         const gchar * const *names);

/**
 * ibus_bus_preload_engines_async:
 * @bus: An #IBusBus.
 * @names: (array zero-terminated=1): A %NULL-terminated array of engine names.
 * @timeout_msec: The timeout in milliseconds or -1 to use the default timeout.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied
 *      or %NULL if you don't care about the result of the method invocation.
 * @user_data: The data to pass to callback.
 *
 * Start bus components by engine names asynchronously.
 */
void         ibus_bus_preload_engines_async
                                        (IBusBus        *bus,
                                         const gchar * const
                                                        *names,
                                         gint            timeout_msec,
                                         GCancellable   *cancellable,
                                         GAsyncReadyCallback
                                                         callback,
                                         gpointer        user_data);

/**
 * ibus_bus_preload_engines_async_finish:
 * @bus: An #IBusBus.
 * @res: A #GAsyncResult obtained from the #GAsyncReadyCallback passed to
 *   ibus_bus_preload_engines_async().
 * @error: Return location for error or %NULL.
 * @returns: %TRUE if component starts. %FALSE otherwise.
 *
 * Finishes an operation started with ibus_bus_preload_engines_async().
 */
gboolean     ibus_bus_preload_engines_async_finish
                                        (IBusBus        *bus,
                                         GAsyncResult   *res,
                                         GError        **error);

/**
 * ibus_bus_get_ibus_property:
 * @bus: An #IBusBus.
 * @property_name: property name in org.freedesktop.DBus.Properties.Get
 * @returns: (transfer full): The value in org.freedesktop.DBus.Properties.Get
 *           The returned value must be freed with g_variant_unref().
 *
 * Get org.freedesktop.DBus.Properties.
 */
GVariant *   ibus_bus_get_ibus_property (IBusBus        *bus,
                                         const gchar    *property_name);

/**
 * ibus_bus_set_ibus_property:
 * @bus: An #IBusBus.
 * @property_name: property name in org.freedesktop.DBus.Properties.Set
 * @value: value in org.freedesktop.DBus.Properties.Set
 *
 * Set org.freedesktop.DBus.Properties.
 */
void         ibus_bus_set_ibus_property (IBusBus        *bus,
                                         const gchar    *property_name,
                                         GVariant       *value);

G_END_DECLS
#endif
