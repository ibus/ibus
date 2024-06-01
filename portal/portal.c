/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2017-2024 Red Hat, Inc.
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
#include <config.h>
#include <fcntl.h>
#include <glib.h>
#include <gio/gio.h>
#include <ibus.h>
#include <locale.h>
#include <pwd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ibus-portal-dbus.h"

typedef struct _IBusPortal IBusPortal;
typedef struct _IBusPortalClass IBusPortalClass;
typedef struct _IBusPortalContext IBusPortalContext;
typedef struct _IBusPortalContextClass IBusPortalContextClass;

struct _IBusPortalContext
{
    IBusDbusInputContextSkeleton parent_instance;
    IBusInputContext *context;
    guint id;
    char *owner;
    char *object_path;
    IBusDbusService *service;
};

struct _IBusPortalContextClass
{
    IBusDbusInputContextSkeletonClass parent_class;
};

struct _IBusPortal
{
    IBusDbusPortalSkeleton parent_instance;

};

struct _IBusPortalClass
{
    IBusDbusPortalSkeletonClass parent_class;
};

enum
{
    PROP_POST_PROCESS_KEY_EVENT = 1,
    PROP_CONTENT_TYPE,
    PROP_CLIENT_COMMIT_PREEDIT,
    PROP_EFFECTIVE_POST_PROCESS_KEY_EVENT,
    N_PROPERTIES
};

static GMainLoop *loop = NULL;
static IBusBus *ibus_bus;
static IBusPortal *ibus_portal = NULL;
static gboolean opt_verbose;
static gboolean opt_replace;

static GList *all_contexts = NULL;

static guint next_context_id;

GType ibus_portal_context_get_type (void) G_GNUC_CONST;
static void ibus_portal_context_iface_init (IBusDbusInputContextIface *iface);

static void portal_context_g_signal (GDBusProxy        *proxy,
                                     const gchar       *sender_name,
                                     const gchar       *signal_name,
                                     GVariant          *parameters,
                                     IBusPortalContext *portal_context);

G_DEFINE_TYPE_WITH_CODE (IBusPortalContext,
                         ibus_portal_context,
                         IBUS_DBUS_TYPE_INPUT_CONTEXT_SKELETON,
                         G_IMPLEMENT_INTERFACE (IBUS_DBUS_TYPE_INPUT_CONTEXT,
                                 ibus_portal_context_iface_init));

static void
_forward_method_cb (GObject *source_object,
                    GAsyncResult *res,
                    gpointer user_data)
{
    GDBusMethodInvocation *invocation = user_data;
    GError *error = NULL;

    GVariant *variant = g_dbus_proxy_call_finish ((GDBusProxy *) source_object,
                                                  res, &error);
    if (variant == NULL) {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
        return;
    }

    g_dbus_method_invocation_return_value (invocation, variant);
}

static gboolean
_forward_method (IBusDbusInputContext  *object,
                 GDBusMethodInvocation *invocation)
{
    IBusPortalContext *portal_context = (IBusPortalContext *)object;
    GDBusMessage *message = g_dbus_method_invocation_get_message (invocation);

    g_dbus_proxy_call (G_DBUS_PROXY (portal_context->context),
                       g_dbus_method_invocation_get_method_name (invocation),
                       g_dbus_message_get_body (message),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL, /* cancellable */
                       _forward_method_cb, invocation);
    return TRUE;
}

static gboolean
ibus_dbus_context_cancel_hand_writing (IBusDbusInputContext  *object,
                                       GDBusMethodInvocation *invocation,
                                       guint                  arg_n_strokes)
{
    return _forward_method (object, invocation);
}

static gboolean
ibus_dbus_context_focus_in (IBusDbusInputContext  *object,
                            GDBusMethodInvocation *invocation)
{
    return _forward_method (object, invocation);
}

static gboolean
ibus_dbus_context_focus_out (IBusDbusInputContext  *object,
                             GDBusMethodInvocation *invocation)
{
    return _forward_method (object, invocation);
}

static gboolean
ibus_dbus_context_get_engine (IBusDbusInputContext  *object,
                              GDBusMethodInvocation *invocation)
{
    return _forward_method (object, invocation);
}

static gboolean
ibus_dbus_context_process_hand_writing_event (IBusDbusInputContext  *object,
                                              GDBusMethodInvocation *invocation,
                                              GVariant
                                                               *arg_coordinates)
{
    return _forward_method (object, invocation);
}

static gboolean
ibus_dbus_context_process_key_event (IBusDbusInputContext  *object,
                                     GDBusMethodInvocation *invocation,
                                     guint                  arg_keyval,
                                     guint                  arg_keycode,
                                     guint                  arg_state)
{
    return _forward_method (object, invocation);
}

static gboolean
ibus_dbus_context_property_activate (IBusDbusInputContext  *object,
                                     GDBusMethodInvocation *invocation,
                                     const gchar           *arg_name,
                                     guint                  arg_state)
{
    return _forward_method (object, invocation);
}

static gboolean
ibus_dbus_context_reset (IBusDbusInputContext  *object,
                         GDBusMethodInvocation *invocation)
{
    return _forward_method (object, invocation);
}

static gboolean
ibus_dbus_context_set_capabilities (IBusDbusInputContext  *object,
                                    GDBusMethodInvocation *invocation,
                                    guint                  arg_caps)
{
    return _forward_method (object, invocation);
}

static gboolean
ibus_dbus_context_set_cursor_location (IBusDbusInputContext  *object,
                                       GDBusMethodInvocation *invocation,
                                       gint                   arg_x,
                                       gint                   arg_y,
                                       gint                   arg_w,
                                       gint                   arg_h)
{
    return _forward_method (object, invocation);
}

static gboolean
ibus_dbus_context_set_cursor_location_relative (IBusDbusInputContext  *object,
                                                GDBusMethodInvocation
                                                                    *invocation,
                                                gint                   arg_x,
                                                gint                   arg_y,
                                                gint                   arg_w,
                                                gint                   arg_h)
{
    return _forward_method (object, invocation);
}

static gboolean
ibus_dbus_context_set_engine (IBusDbusInputContext  *object,
                              GDBusMethodInvocation *invocation,
                              const gchar           *arg_name)
{
    return _forward_method (object, invocation);
}

static gboolean
ibus_dbus_context_set_surrounding_text (IBusDbusInputContext  *object,
                                        GDBusMethodInvocation *invocation,
                                        GVariant              *arg_text,
                                        guint                  arg_cursor_pos,
                                        guint                  arg_anchor_pos)
{
    return _forward_method (object, invocation);
}

static void
ibus_portal_context_iface_init (IBusDbusInputContextIface *iface)
{
    iface->handle_cancel_hand_writing = ibus_dbus_context_cancel_hand_writing;
    iface->handle_focus_in = ibus_dbus_context_focus_in;
    iface->handle_focus_out = ibus_dbus_context_focus_out;
    iface->handle_get_engine = ibus_dbus_context_get_engine;
    iface->handle_process_hand_writing_event =
            ibus_dbus_context_process_hand_writing_event;
    iface->handle_process_key_event = ibus_dbus_context_process_key_event;
    iface->handle_property_activate = ibus_dbus_context_property_activate;
    iface->handle_reset = ibus_dbus_context_reset;
    iface->handle_set_capabilities = ibus_dbus_context_set_capabilities;
    iface->handle_set_cursor_location = ibus_dbus_context_set_cursor_location;
    iface->handle_set_cursor_location_relative =
            ibus_dbus_context_set_cursor_location_relative;
    iface->handle_set_engine = ibus_dbus_context_set_engine;
    iface->handle_set_surrounding_text = ibus_dbus_context_set_surrounding_text;
}

static void
ibus_portal_context_init (IBusPortalContext *portal_context)
{
}

static void
ibus_portal_context_finalize (GObject *object)
{
    IBusPortalContext *portal_context = (IBusPortalContext *)object;

    all_contexts = g_list_remove (all_contexts, portal_context);

    g_dbus_interface_skeleton_unexport (
            G_DBUS_INTERFACE_SKELETON (portal_context->service));
    g_dbus_interface_skeleton_unexport (
            G_DBUS_INTERFACE_SKELETON (portal_context));

    g_free (portal_context->owner);
    g_free (portal_context->object_path);
    g_object_unref (portal_context->service);

    g_signal_handlers_disconnect_by_func (
            portal_context->context,
            G_CALLBACK(portal_context_g_signal),
            portal_context);
    g_object_unref (portal_context->context);

    G_OBJECT_CLASS (ibus_portal_context_parent_class)->finalize (object);
}

static void
ibus_portal_context_set_property (IBusPortalContext *portal_context,
                                  guint              prop_id,
                                  const GValue      *value,
                                  GParamSpec        *pspec)
{
    switch (prop_id) {
    case PROP_CONTENT_TYPE:
        g_dbus_proxy_call (G_DBUS_PROXY (portal_context->context),
                           "org.freedesktop.DBus.Properties.Set",
                           g_variant_new ("(ssv)",
                                          IBUS_INTERFACE_INPUT_CONTEXT,
                                          "ContentType",
                                          g_value_get_variant (value)),
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL, /* cancellable */
                           NULL, /* callback */
                           NULL  /* user_data */
                           );
        break;
    case PROP_CLIENT_COMMIT_PREEDIT:
        g_dbus_proxy_call (G_DBUS_PROXY (portal_context->context),
                           "org.freedesktop.DBus.Properties.Set",
                           g_variant_new ("(ssv)",
                                          IBUS_INTERFACE_INPUT_CONTEXT,
                                          "ClientCommitPreedit",
                                          g_value_get_variant (value)),
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL, /* cancellable */
                           NULL, /* callback */
                           NULL  /* user_data */
                           );
        break;
    case PROP_EFFECTIVE_POST_PROCESS_KEY_EVENT:
        g_dbus_proxy_call (G_DBUS_PROXY (portal_context->context),
                           "org.freedesktop.DBus.Properties.Set",
                           g_variant_new ("(ssv)",
                                          IBUS_INTERFACE_INPUT_CONTEXT,
                                          "EffectivePostProcessKeyEvent",
                                          g_value_get_variant (value)),
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL, /* cancellable */
                           NULL, /* callback */
                           NULL  /* user_data */
                           );
        break;
    case PROP_POST_PROCESS_KEY_EVENT:
        g_warning ("No support for setting content type");
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (portal_context, prop_id, pspec);
    }
}

static void
ibus_portal_context_get_property (IBusPortalContext *portal_context,
                                  guint              prop_id,
                                  GValue            *value,
                                  GParamSpec        *pspec)
{
    GVariant *result;
    GVariant *variant = NULL;
    GError *error = NULL;
    switch (prop_id) {
    case PROP_POST_PROCESS_KEY_EVENT:
        result = g_dbus_proxy_call_sync (G_DBUS_PROXY (portal_context->context),
                                         "org.freedesktop.DBus.Properties.Get",
                                         g_variant_new (
                                                 "(ss)",
                                                 IBUS_INTERFACE_INPUT_CONTEXT,
                                                 "PostProcessKeyEvent"),
                                         G_DBUS_CALL_FLAGS_NONE,
                                         -1,
                                         NULL, /* cancellable */
                                         &error);
        if (error) {
            g_warning ("Error PostProcessKeyEvent: %s", error->message);
            g_error_free (error);
            break;
        }
        g_variant_get (result, "(v)", &variant);
        if (!variant) {
            g_warning ("No child in PostProcessKeyEvent");
            break;
        }
        g_value_set_variant (value, variant);
        break;
    case PROP_CONTENT_TYPE:
    case PROP_CLIENT_COMMIT_PREEDIT:
    case PROP_EFFECTIVE_POST_PROCESS_KEY_EVENT:
        g_warning ("No support for getting content type");
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (portal_context, prop_id, pspec);
    }
}

static gboolean
ibus_portal_context_g_authorize_method (GDBusInterfaceSkeleton *interface,
                                        GDBusMethodInvocation  *invocation)
{
    IBusPortalContext *portal_context = (IBusPortalContext *)interface;

    if (g_strcmp0 (g_dbus_method_invocation_get_sender (invocation),
                   portal_context->owner) == 0) {
        return TRUE;
    }

    g_dbus_method_invocation_return_error (invocation,
                                           G_DBUS_ERROR,
                                           G_DBUS_ERROR_FAILED,
                                           "Access denied");
    return FALSE;
}


static void
ibus_portal_context_class_init (IBusPortalContextClass *klass)
{
    GObjectClass *gobject_class;
    GDBusInterfaceSkeletonClass *skeleton_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize  = ibus_portal_context_finalize;
    gobject_class->set_property  =
            (GObjectSetPropertyFunc) ibus_portal_context_set_property;
    gobject_class->get_property  =
            (GObjectGetPropertyFunc) ibus_portal_context_get_property;

    skeleton_class = G_DBUS_INTERFACE_SKELETON_CLASS(klass);
    skeleton_class->g_authorize_method = ibus_portal_context_g_authorize_method;

    ibus_dbus_input_context_override_properties (gobject_class,
                                                 PROP_POST_PROCESS_KEY_EVENT);
}

static void
portal_context_g_signal (GDBusProxy        *proxy,
                         const gchar       *sender_name,
                         const gchar       *signal_name,
                         GVariant          *parameters,
                         IBusPortalContext *portal_context)
{
    GError *error = NULL;
    GDBusConnection *connection;

    connection = g_dbus_interface_skeleton_get_connection (
            G_DBUS_INTERFACE_SKELETON (portal_context));
    if (!g_dbus_connection_emit_signal (connection,
                                        portal_context->owner,
                                        portal_context->object_path,
                                        IBUS_INTERFACE_INPUT_CONTEXT,
                                        signal_name,
                                        parameters,
                                        &error)) {
        g_warning ("Unable to emit signal %s: %s", signal_name, error->message);
        g_error_free (error);
    }

    g_signal_stop_emission_by_name (proxy, "g-signal");
}

static gboolean
ibus_portal_context_handle_destroy (IBusDbusService       *object,
                                    GDBusMethodInvocation *invocation,
                                    IBusPortalContext     *portal_context)
{
    g_object_unref (portal_context);
    return FALSE;
}

static IBusPortalContext *
ibus_portal_context_new (IBusInputContext *context,
                         GDBusConnection  *connection,
                         const char       *owner,
                         GError          **error)
{
    IBusPortalContext *portal_context =
            g_object_new (ibus_portal_context_get_type (), NULL);

    g_signal_connect (context,
                      "g-signal",
                      G_CALLBACK(portal_context_g_signal),
                      portal_context);

    portal_context->id = ++next_context_id;
    portal_context->context = g_object_ref (context);
    portal_context->owner = g_strdup (owner);
    portal_context->object_path =
            g_strdup_printf (IBUS_PATH_INPUT_CONTEXT, portal_context->id);
    portal_context->service = ibus_dbus_service_skeleton_new ();

    g_signal_connect (portal_context->service,
                      "handle-destroy",
                      G_CALLBACK (ibus_portal_context_handle_destroy),
                      portal_context);

    if (!g_dbus_interface_skeleton_export (
                G_DBUS_INTERFACE_SKELETON (portal_context->service),
                connection, portal_context->object_path,
                error) ||
        !g_dbus_interface_skeleton_export (
                G_DBUS_INTERFACE_SKELETON (portal_context),
                connection, portal_context->object_path,
                error)) {
        g_object_unref (portal_context);
        return NULL;
    }

    all_contexts = g_list_prepend (all_contexts, portal_context);

    return portal_context;
}

GType ibus_portal_get_type (void) G_GNUC_CONST;
static void ibus_portal_iface_init (IBusDbusPortalIface *iface);

G_DEFINE_TYPE_WITH_CODE (IBusPortal, ibus_portal,
                         IBUS_DBUS_TYPE_PORTAL_SKELETON,
                         G_IMPLEMENT_INTERFACE (IBUS_DBUS_TYPE_PORTAL,
                                                ibus_portal_iface_init));


static void
create_input_context_done (IBusBus               *bus,
                           GAsyncResult          *res,
                           GDBusMethodInvocation *invocation)
{
    GError *error = NULL;
    IBusInputContext *context;
    IBusPortalContext *portal_context;

    context = ibus_bus_create_input_context_async_finish (ibus_bus,
                                                          res,
                                                          &error);
    if (context == NULL) {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
        return;
    }

    portal_context = ibus_portal_context_new (
            context,
            g_dbus_method_invocation_get_connection (invocation),
            g_dbus_method_invocation_get_sender (invocation),
            &error);
    g_object_unref (context);

    if (portal_context == NULL) {
        g_dbus_method_invocation_return_gerror (invocation, error);
        g_error_free (error);
        return;
    }

    ibus_dbus_portal_complete_create_input_context (
            IBUS_DBUS_PORTAL(ibus_portal),
            invocation, portal_context->object_path);
}

static gboolean
ibus_portal_handle_create_input_context (IBusDbusPortal        *object,
                                         GDBusMethodInvocation *invocation,
                                         const gchar           *arg_client_name)
{
    ibus_bus_create_input_context_async (
            ibus_bus,
            arg_client_name, -1,
            NULL,
            (GAsyncReadyCallback)create_input_context_done,
            invocation);
    return TRUE;
}

static void
ibus_portal_iface_init (IBusDbusPortalIface *iface)
{
    iface->handle_create_input_context =
            ibus_portal_handle_create_input_context;
}

static void
ibus_portal_init (IBusPortal *portal)
{
}

static void
ibus_portal_class_init (IBusPortalClass *klass)
{
}


static void
show_version_and_quit (void)
{
    g_print ("%s - Version %s\n", g_get_application_name (), VERSION);
    exit (EXIT_SUCCESS);
}

static const GOptionEntry entries[] =
{
    { "version",   'V', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
      show_version_and_quit, "Show the application's version.", NULL },
    { "verbose",   'v', 0, G_OPTION_ARG_NONE, 
      &opt_verbose,   "verbose.", NULL },
    { "replace",   'r', 0, G_OPTION_ARG_NONE,
      &opt_replace,   "Replace.", NULL },
    { NULL },
};

static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar     *name,
                 gpointer         user_data)
{
    GError *error = NULL;

    ibus_portal = g_object_new (ibus_portal_get_type (), NULL);

    if (!g_dbus_interface_skeleton_export (
                G_DBUS_INTERFACE_SKELETON (ibus_portal),
                connection,
                IBUS_PATH_IBUS,
                &error)) {
        g_warning ("Error exporting portal: %s", error->message);
        g_error_free (error);
        return;
    }
}

static void
on_name_acquired (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
}

static void
on_name_lost (GDBusConnection *connection,
              const gchar     *name,
              gpointer         user_data)
{
    g_main_loop_quit (loop);
}

static void
name_owner_changed (GDBusConnection *connection,
                    const gchar     *sender_name,
                    const gchar     *object_path,
                    const gchar     *interface_name,
                    const gchar     *signal_name,
                    GVariant        *parameters,
                    gpointer         user_data)
{
  const char *name, *from, *to;

  g_variant_get (parameters, "(sss)", &name, &from, &to);

  if (name[0] == ':' &&
      g_strcmp0 (name, from) == 0 &&
      g_strcmp0 (to, "") == 0)
    {
        GList *l, *next;
        /* Client disconnected, free any input contexts it may have */
        for (l = all_contexts; l != NULL; l = next) {
            IBusPortalContext *portal_context = l->data;
            next = l->next;

            if (g_strcmp0 (portal_context->owner, name) == 0) {
                g_object_unref (portal_context);
            }
        }
    }
}

static void
_bus_disconnected_cb (IBusBus            *ibusbus)
{
    g_main_loop_quit (loop);
}

gint
main (gint argc, gchar **argv)
{
    GDBusConnection *session_bus = NULL;
    guint owner_id;

    setlocale (LC_ALL, "");

    GOptionContext *context = g_option_context_new ("- ibus daemon");
    g_option_context_add_main_entries (context, entries, "ibus-daemon");

    GError *error = NULL;
    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_printerr ("Option parsing failed: %s\n", error->message);
        g_error_free (error);
        exit (-1);
    }

     errno = 0;
    /* Avoid even loading gvfs to avoid accidental confusion */
    if (!g_setenv ("GIO_USE_VFS", "local", TRUE))
        g_warning ("Failed setenv %s", strerror (errno));

    ibus_init ();

    ibus_set_log_handler (opt_verbose);

    ibus_bus = ibus_bus_new ();
    if (!ibus_bus_is_connected (ibus_bus)) {
        g_printerr ("Not connected to the ibus bus\n");
        exit (1);
    }

    g_signal_connect (ibus_bus, "disconnected",
                      G_CALLBACK (_bus_disconnected_cb), NULL);

    loop = g_main_loop_new (NULL, FALSE);

    session_bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
    if (session_bus == NULL) {
        g_printerr ("No session bus: %s", error->message);
        exit (-1);
    }

    g_dbus_connection_signal_subscribe (session_bus,
                                        "org.freedesktop.DBus",
                                        "org.freedesktop.DBus",
                                        "NameOwnerChanged",
                                        "/org/freedesktop/DBus",
                                        NULL,
                                        G_DBUS_SIGNAL_FLAGS_NONE,
                                        name_owner_changed,
                                        NULL, NULL);

    owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                               IBUS_SERVICE_PORTAL,
                               G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
                               (opt_replace ? G_BUS_NAME_OWNER_FLAGS_REPLACE
                                            : 0),
                               on_bus_acquired,
                               on_name_acquired,
                               on_name_lost,
                               NULL,
                               NULL);

    g_main_loop_run (loop);

    g_bus_unown_name (owner_id);
    g_main_loop_unref (loop);

    return 0;
}
