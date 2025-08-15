/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include <string.h>
#include "ibus.h"

static IBusBus *bus;

static void
print_engines (const GList *engines)
{
    for (; engines; engines = g_list_next (engines)) {
        IBusEngineDesc *engine_desc = IBUS_ENGINE_DESC (engines->data);
        g_assert (engine_desc);
#if 0
        g_debug ("%s (id:%s, icon:%s)",
                 ibus_engine_desc_get_longname (engine_desc),
                 ibus_engine_desc_get_name (engine_desc),
                 ibus_engine_desc_get_icon (engine_desc));
#endif
    }
}

#ifndef IBUS_DISABLE_DEPRECATED
static void
test_list_active_engines (void)
{
    GList *engines;
    IBUS_TYPE_ENGINE_DESC;

    engines = ibus_bus_list_active_engines (bus);
    print_engines (engines);

    g_list_foreach (engines, (GFunc) g_object_unref, NULL);
    g_list_free (engines);
}
#endif /* IBUS_DISABLE_DEPRECATED */

static void
test_list_engines (void)
{
    GList *engines;
    IBUS_TYPE_ENGINE_DESC;

    engines = ibus_bus_list_engines (bus);
    print_engines (engines);

    g_list_foreach (engines, (GFunc) g_object_unref, NULL);
    g_list_free (engines);
}

static void
name_owner_changed_cb (IBusBus  *bus,
                       gchar    *name,
                       gchar    *old,
                       gchar    *new,
                       gpointer  data)
{
    g_debug ("%s: bus=%s, old=%s, new=%s", G_STRFUNC, name, old, new);
}

static void call_next_async_function (void);

static void
finish_request_name_async (GObject *source_object,
                           GAsyncResult *res,
                           gpointer user_data)
{
    GError *error = NULL;
    guint id = ibus_bus_request_name_async_finish (bus, res, &error);

    g_assert (id != 0);

    g_debug ("request name returned %d: ", id);

    switch (id) {
    case IBUS_BUS_REQUEST_NAME_REPLY_PRIMARY_OWNER:
        g_debug ("got ownership");
        break;
    case IBUS_BUS_REQUEST_NAME_REPLY_IN_QUEUE:
        g_debug ("got queued");
        break;
    case IBUS_BUS_REQUEST_NAME_REPLY_EXISTS:
        g_debug ("request already in queue");
        break;
    case IBUS_BUS_REQUEST_NAME_REPLY_ALREADY_OWNER:
        g_debug ("already owner");
        break;
    default:
        g_assert_not_reached ();
    }

    if (error) {
        g_warning ("Error %s: %s", G_STRFUNC, error->message);
        g_error_free (error);
    }

    g_debug ("ibus_bus_request_name_async_finish: OK");
    call_next_async_function ();
}

static void
start_request_name_async (void)
{
    ibus_bus_set_watch_dbus_signal (bus, TRUE);
    ibus_bus_set_watch_ibus_signal (bus, TRUE);

    g_signal_connect (bus, "name-owner-changed",
                      (GCallback) name_owner_changed_cb, NULL);

    ibus_bus_request_name_async (bus,
                                 "org.freedesktop.IBus.IBusBusTest",
                                 IBUS_BUS_NAME_FLAG_REPLACE_EXISTING,
                                 -1, /* timeout */
                                 NULL, /* cancellable */
                                 finish_request_name_async,
                                 NULL); /* user_data */
}


static void
finish_name_has_owner_async (GObject *source_object,
                             GAsyncResult *res,
                             gpointer user_data)
{
    GError *error = NULL;
    gboolean has_owner = ibus_bus_name_has_owner_async_finish (bus,
                                                               res,
                                                               &error);
    g_assert (has_owner);
    g_debug ("ibus_bus_name_has_owner_async_finish: OK");
    call_next_async_function ();
}

static void
start_name_has_owner_async (void)
{
    ibus_bus_name_has_owner_async (bus,
                                   "org.freedesktop.IBus.IBusBusTest",
                                   -1, /* timeout */
                                   NULL, /* cancellable */
                                   finish_name_has_owner_async,
                                   NULL); /* user_data */
}

static void
finish_get_name_owner_async (GObject *source_object,
                             GAsyncResult *res,
                             gpointer user_data)
{
    GError *error = NULL;
    gchar *owner = ibus_bus_get_name_owner_async_finish (bus,
                                                         res,
                                                         &error);
    g_assert (owner);
    g_free (owner);
    g_debug ("ibus_bus_name_get_name_owner_async_finish: OK");
    call_next_async_function ();
}

static void
start_get_name_owner_async (void)
{
    ibus_bus_get_name_owner_async (bus,
                                   "org.freedesktop.IBus.IBusBusTest",
                                   -1, /* timeout */
                                   NULL, /* cancellable */
                                   finish_get_name_owner_async,
                                   NULL); /* user_data */
}

static void
finish_release_name_async (GObject *source_object,
                           GAsyncResult *res,
                           gpointer user_data)
{
    GError *error = NULL;
    guint id = ibus_bus_release_name_async_finish (bus,
                                                   res,
                                                   &error);
    g_assert (id != 0);
    g_debug ("ibus_bus_release_name_async_finish: OK");
    call_next_async_function ();
}

static void
start_release_name_async (void)
{
    ibus_bus_release_name_async (bus,
                                 "org.freedesktop.IBus.IBusBusTest",
                                 -1, /* timeout */
                                 NULL, /* cancellable */
                                 finish_release_name_async,
                                 NULL); /* user_data */
}

static void
finish_add_match_async (GObject *source_object,
                        GAsyncResult *res,
                        gpointer user_data)
{
    GError *error = NULL;
    gboolean result = ibus_bus_add_match_async_finish (bus,
                                                       res,
                                                       &error);
    g_assert (result);
    g_debug ("ibus_bus_add_match_finish: OK");
    call_next_async_function ();
}

static void
start_add_match_async (void)
{
    ibus_bus_add_match_async (bus,
                              "type='signal'",
                              -1, /* timeout */
                              NULL, /* cancellable */
                              finish_add_match_async,
                              NULL); /* user_data */
}

static void
finish_remove_match_async (GObject *source_object,
                           GAsyncResult *res,
                           gpointer user_data)
{
    GError *error = NULL;
    gboolean result = ibus_bus_remove_match_async_finish (bus,
                                                          res,
                                                          &error);
    g_assert (result);
    g_debug ("ibus_bus_remove_match_finish: OK");
    call_next_async_function ();
}

static void
start_remove_match_async (void)
{
    ibus_bus_remove_match_async (bus,
                                 "type='signal'",
                                 -1, /* timeout */
                                 NULL, /* cancellable */
                                 finish_remove_match_async,
                                 NULL); /* user_data */
}

static int create_input_context_count = 0;
static void
finish_create_input_context_async_success (GObject      *source_object,
                                          GAsyncResult *res,
                                          gpointer      user_data)
{
    GMainLoop *loop = (GMainLoop *)user_data;
    GError *error = NULL;
    IBusInputContext *context =
          ibus_bus_create_input_context_async_finish (bus, res, &error);

    g_assert (IBUS_IS_INPUT_CONTEXT (context));
    g_object_unref (context);
    if (--create_input_context_count == 0)
        g_main_loop_quit (loop);
}

static void
finish_create_input_context_async_failed (GObject      *source_object,
                                          GAsyncResult *res,
                                          gpointer      user_data)
{
    GMainLoop *loop = (GMainLoop *)user_data;
    GError *error = NULL;
    IBusInputContext *context =
            ibus_bus_create_input_context_async_finish (bus, res, &error);

    g_assert (context == NULL);
    g_assert (error != NULL);
    g_error_free (error);
    if (--create_input_context_count <= 0)
        g_main_loop_quit (loop);
}

static void
test_create_input_context_async (void)
{
    GMainLoop *loop = NULL;
    GCancellable *cancellable = NULL;

    /* create an IC */
    create_input_context_count = 1;
    loop = g_main_loop_new (NULL, TRUE);
    ibus_bus_create_input_context_async (bus,
            "test-async",
            -1, /* timeout */
            NULL, /* cancellable */
            finish_create_input_context_async_success,
            loop); /* user_data */
    g_main_loop_run (loop);
    g_main_loop_unref (loop);

    /* call create, and then cancel */
    create_input_context_count = 1;
    loop = g_main_loop_new (NULL, TRUE);
    cancellable = g_cancellable_new ();
    ibus_bus_create_input_context_async (bus,
            "test-async",
            -1, /* timeout */
            cancellable, /* cancellable */
            finish_create_input_context_async_failed,
            loop); /* user_data */
    g_cancellable_cancel (cancellable);
    g_object_unref (cancellable);
    g_main_loop_run (loop);
    g_main_loop_unref (loop);

    /* create four IC, and cancel two */
    create_input_context_count = 4;
    loop = g_main_loop_new (NULL, TRUE);
    cancellable = g_cancellable_new ();
    ibus_bus_create_input_context_async (bus,
            "test-async",
            -1, /* timeout */
            cancellable, /* cancellable */
            finish_create_input_context_async_failed,
            loop); /* user_data */
    ibus_bus_create_input_context_async (bus,
            "test-async",
            -1, /* timeout */
            NULL, /* cancellable */
            finish_create_input_context_async_success,
            loop); /* user_data */
    ibus_bus_create_input_context_async (bus,
            "test-async",
            -1, /* timeout */
            NULL, /* cancellable */
            finish_create_input_context_async_success,
            loop); /* user_data */
    ibus_bus_create_input_context_async (bus,
            "test-async",
            -1, /* timeout */
            cancellable, /* cancellable */
            finish_create_input_context_async_failed,
            loop); /* user_data */
    g_cancellable_cancel (cancellable);
    g_object_unref (cancellable);

    g_main_loop_run (loop);
    g_main_loop_unref (loop);
}

static void
finish_current_input_context_async (GObject *source_object,
                                    GAsyncResult *res,
                                    gpointer user_data)
{
    GError *error = NULL;
    g_free (ibus_bus_current_input_context_async_finish (bus,
                                                         res,
                                                         &error));
    // no null check.
    g_debug ("ibus_bus_current_input_context_finish: OK");
    call_next_async_function ();
}

static void
start_current_input_context_async (void)
{
    ibus_bus_current_input_context_async (bus,
                                          -1, /* timeout */
                                          NULL, /* cancellable */
                                          finish_current_input_context_async,
                                          NULL); /* user_data */
}

static void
finish_list_engines_async (GObject *source_object,
                           GAsyncResult *res,
                           gpointer user_data)
{
    GError *error = NULL;
    GList *engines = ibus_bus_list_engines_async_finish (bus,
                                                         res,
                                                         &error);
    // no null check.
    g_list_foreach (engines, (GFunc) g_object_unref, NULL);
    g_list_free (engines);
    g_debug ("ibus_bus_list_engines_finish: OK");
    call_next_async_function ();
}

static void
start_list_engines_async (void)
{
    ibus_bus_list_engines_async (bus,
                                 -1, /* timeout */
                                 NULL, /* cancellable */
                                 finish_list_engines_async,
                                 NULL); /* user_data */
}

#ifndef IBUS_DISABLE_DEPRECATED
static void
finish_list_active_engines_async (GObject *source_object,
                                  GAsyncResult *res,
                                  gpointer user_data)
{
    GError *error = NULL;
    GList *engines = ibus_bus_list_active_engines_async_finish (bus,
                                                                res,
                                                                &error);
    // no null check.
    g_list_foreach (engines, (GFunc) g_object_unref, NULL);
    g_list_free (engines);
    g_debug ("ibus_bus_list_active_engines_finish: OK");
    call_next_async_function ();
}

static void
start_list_active_engines_async (void)
{
    ibus_bus_list_active_engines_async (bus,
                                        -1, /* timeout */
                                        NULL, /* cancellable */
                                        finish_list_active_engines_async,
                                        NULL); /* user_data */
}

static void
finish_get_use_sys_layout_async (GObject *source_object,
                                 GAsyncResult *res,
                                 gpointer user_data)
{
    GError *error = NULL;
    ibus_bus_get_use_sys_layout_async_finish (bus,
                                              res,
                                              &error);
    g_debug ("ibus_bus_get_use_sys_layout_finish: OK");
    call_next_async_function ();
}

static void
start_get_use_sys_layout_async (void)
{
    ibus_bus_get_use_sys_layout_async (bus,
                                       -1, /* timeout */
                                       NULL, /* cancellable */
                                       finish_get_use_sys_layout_async,
                                       NULL); /* user_data */
}

static void
finish_get_use_global_engine_async (GObject *source_object,
                                    GAsyncResult *res,
                                    gpointer user_data)
{
    GError *error = NULL;
    ibus_bus_get_use_global_engine_async_finish (bus,
                                                 res,
                                                 &error);
    g_debug ("ibus_bus_get_use_global_engine_finish: OK");
    call_next_async_function ();
}

static void
start_get_use_global_engine_async (void)
{
    ibus_bus_get_use_global_engine_async (bus,
                                          -1, /* timeout */
                                          NULL, /* cancellable */
                                          finish_get_use_global_engine_async,
                                          NULL); /* user_data */
}

static void
finish_is_global_engine_enabled_async (GObject *source_object,
                                       GAsyncResult *res,
                                       gpointer user_data)
{
    GError *error = NULL;
    ibus_bus_is_global_engine_enabled_async_finish (bus,
                                                    res,
                                                    &error);
    g_debug ("ibus_bus_is_global_engine_enabled_finish: OK");
    call_next_async_function ();
}

static void
start_is_global_engine_enabled_async (void)
{
    ibus_bus_is_global_engine_enabled_async (bus,
                                             -1, /* timeout */
                                             NULL, /* cancellable */
                                             finish_is_global_engine_enabled_async,
                                             NULL); /* user_data */
}
#endif /* IBUS_DISABLE_DEPRECATED */

static void
finish_get_global_engine_async (GObject *source_object,
                                GAsyncResult *res,
                                gpointer user_data)
{
    GError *error = NULL;
    IBusEngineDesc *desc = ibus_bus_get_global_engine_async_finish (bus,
                                                                    res,
                                                                    &error);
    if (desc)
        g_object_unref (desc);
    g_debug ("ibus_bus_get_global_engine_finish: OK");
    call_next_async_function ();
}

static void
start_get_global_engine_async (void)
{
    ibus_bus_get_global_engine_async (bus,
                                      -1, /* timeout */
                                      NULL, /* cancellable */
                                      finish_get_global_engine_async,
                                      NULL); /* user_data */
}

static void
finish_set_global_engine_async (GObject *source_object,
                                GAsyncResult *res,
                                gpointer user_data)
{
    GError *error = NULL;
    ibus_bus_set_global_engine_async_finish (bus,
                                             res,
                                             &error);
    g_debug ("ibus_bus_set_global_engine_finish: OK");
    /* anthy not found */
    if (error)
        g_error_free (error);
    call_next_async_function ();
}

static void
start_set_global_engine_async (void)
{
    ibus_bus_set_global_engine_async (bus,
                                      "anthy",
                                      -1, /* timeout */
                                      NULL, /* cancellable */
                                      finish_set_global_engine_async,
                                      NULL); /* user_data */
}

static void
finish_preload_engines_async (GObject      *source_object,
                              GAsyncResult *res,
                              gpointer      user_data)
{
    GError *error = NULL;
    ibus_bus_preload_engines_async_finish (bus, res, &error);
    g_debug ("ibus_bus_preload_engines_async_finish: OK");
    call_next_async_function ();
}

static void
start_preload_engines_async (void)
{
    const gchar *preload_engines[] = { "xkb:us::eng", NULL };

    ibus_bus_preload_engines_async (
            bus,
            preload_engines,
            -1, /* timeout */
            NULL, /* cancellable */
            finish_preload_engines_async,
            NULL); /* user_data */
}

static void
test_get_address (void)
{
    GVariant *result;

    result = ibus_bus_get_ibus_property (bus, "Address");
    g_variant_get_string (result, NULL);
    g_variant_unref (result);
}

static void
test_get_current_input_context (void)
{
    GVariant *result;

    result = ibus_bus_get_ibus_property (bus, "CurrentInputContext");
    g_variant_get_string (result, NULL);
    g_variant_unref (result);
}

static void
test_get_engines (void)
{
    GVariant *result, *var;
    GVariantIter *iter;
    GList *engines = NULL;

    result = ibus_bus_get_ibus_property (bus, "Engines");
    iter = g_variant_iter_new (result);
    while (g_variant_iter_loop (iter, "v", &var)) {
        IBusSerializable *serializable = ibus_serializable_deserialize (var);
        g_object_ref_sink (serializable);
        engines = g_list_append (engines, serializable);
    }
    g_variant_iter_free (iter);
    g_variant_unref (result);

    print_engines (engines);

    g_list_foreach (engines, (GFunc) g_object_unref, NULL);
    g_list_free (engines);
}

static void
test_get_global_engine (void)
{
    GVariant *result, *obj;
    IBusEngineDesc *desc = NULL;

    if (!ibus_bus_set_global_engine (bus, "xkb:us::eng"))
        return;

    result = ibus_bus_get_ibus_property (bus, "GlobalEngine");
    g_assert (result);

    obj = g_variant_get_variant (result);
    g_assert (obj);

    desc = IBUS_ENGINE_DESC (ibus_serializable_deserialize (obj));
    g_assert (desc);
    g_assert_cmpstr (ibus_engine_desc_get_name (desc), ==, "xkb:us::eng");

    g_variant_unref (obj);
    g_variant_unref (result);

    g_object_unref (desc);
}

static void
test_set_preload_engines (void)
{
    const gchar *preload_engines[] = { "xkb:us::eng", "xkb:jp::jpn", NULL };
    GVariant *variant;

    variant = g_variant_new_strv (preload_engines, -1);
    ibus_bus_set_ibus_property (bus, "PreloadEngines", variant);
}

static void
finish_get_address_async (GObject      *source_object,
                          GAsyncResult *res,
                          gpointer      user_data)
{
    GError *error = NULL;
    GVariant *result;

    result = ibus_bus_get_ibus_property_async_finish (bus, res, &error);
    g_variant_get_string (result, NULL);
    g_variant_unref (result);
    g_debug ("finish_get_address_async: OK");
    call_next_async_function ();
}

static void
start_get_address_async (void)
{
    ibus_bus_get_ibus_property_async (
            bus,
            "Address",
            -1, /* timeout */
            NULL, /* cancellable */
            finish_get_address_async,
            NULL); /* user_data */
}

static void
finish_get_current_input_context_async (GObject      *source_object,
                                        GAsyncResult *res,
                                        gpointer      user_data)
{
    GError *error = NULL;
    GVariant *result;

    result = ibus_bus_get_ibus_property_async_finish (bus, res, &error);
    g_variant_get_string (result, NULL);
    g_variant_unref (result);
    g_debug ("finish_get_current_input_context_async: OK");
    call_next_async_function ();
}

static void
start_get_current_input_context_async (void)
{
    ibus_bus_get_ibus_property_async (
            bus,
            "CurrentInputContext",
            -1, /* timeout */
            NULL, /* cancellable */
            finish_get_current_input_context_async,
            NULL); /* user_data */
}

static void
finish_get_engines_async (GObject      *source_object,
                          GAsyncResult *res,
                          gpointer      user_data)
{
    GError *error = NULL;
    GVariant *result, *var;
    GVariantIter *iter;
    GList *engines = NULL;

    result = ibus_bus_get_ibus_property_async_finish (bus, res, &error);
    iter = g_variant_iter_new (result);
    while (g_variant_iter_loop (iter, "v", &var)) {
        IBusSerializable *serializable = ibus_serializable_deserialize (var);
        g_object_ref_sink (serializable);
        engines = g_list_append (engines, serializable);
    }
    g_variant_iter_free (iter);
    g_variant_unref (result);

    print_engines (engines);

    g_list_foreach (engines, (GFunc) g_object_unref, NULL);
    g_list_free (engines);

    g_debug ("finish_get_engines_async: OK");
    call_next_async_function ();
}

static void
start_get_engines_async (void)
{
    ibus_bus_get_ibus_property_async (
            bus,
            "Engines",
            -1, /* timeout */
            NULL, /* cancellable */
            finish_get_engines_async,
            NULL); /* user_data */
}

static void
finish_get_prop_global_engine_async (GObject *source_object,
                                     GAsyncResult *res,
                                     gpointer user_data)
{
    GError *error = NULL;
    GVariant *result, *obj;
    IBusEngineDesc *desc = NULL;

    result = ibus_bus_get_ibus_property_async_finish (bus, res, &error);
    obj = g_variant_get_variant (result);
    desc = IBUS_ENGINE_DESC (ibus_serializable_deserialize (obj));
    g_variant_unref (obj);
    g_variant_unref (result);

    if (desc)
        g_object_unref (desc);

    g_debug ("finish_get_prop_global_engine_async: OK");
    call_next_async_function ();
}

static void
start_get_prop_global_engine_async (void)
{
    ibus_bus_get_ibus_property_async (
            bus,
            "GlobalEngine",
            -1, /* timeout */
            NULL, /* cancellable */
            finish_get_prop_global_engine_async,
            NULL); /* user_data */
}

static void
finish_set_preload_engines_async (GObject      *source_object,
                                  GAsyncResult *res,
                                  gpointer      user_data)
{
    GError *error = NULL;

    ibus_bus_set_ibus_property_async_finish (bus, res, &error);
    g_debug ("finish_set_preload_engines_async: OK");
    call_next_async_function ();
}

static void
start_set_preload_engines_async (void)
{
    const gchar *preload_engines[] = { "xkb:us::eng", "xkb:jp::jpn", NULL };
    GVariant *variant;

    variant = g_variant_new_strv (preload_engines, -1);
    ibus_bus_set_ibus_property_async (
            bus,
            "PreloadEngines",
            variant,
            -1, /* timeout */
            NULL, /* cancellable */
            finish_set_preload_engines_async,
            NULL); /* user_data */
}

typedef struct _ExitAsyncData {
    gboolean has_socket_path;
    gboolean exited;
    guint    timeout_id;
} ExitAsyncData;

static void
_socket_changed_cb (GFileMonitor       *monitor,
                    GFile              *file,
                    GFile              *other_file,
                    GFileMonitorEvent   event_type,
                    ExitAsyncData      *data)
{
    switch (event_type) {
    case G_FILE_MONITOR_EVENT_CHANGED:
        g_debug ("IBus socket file is changed");
        call_next_async_function ();
        data->exited = TRUE;
        g_signal_handlers_disconnect_by_func (monitor,
                                              G_CALLBACK (_socket_changed_cb),
                                              data);
        if (data->timeout_id)
            g_source_remove (data->timeout_id);
        g_object_unref (monitor);
        break;
    case G_FILE_MONITOR_EVENT_CREATED:
        g_debug ("IBus socket file is created");
        break;
    case G_FILE_MONITOR_EVENT_DELETED:
        g_debug ("IBus socket file is deleted");
        break;
    default:
        g_debug ("IBus socket file's status is %d\n", event_type);
    }
}

static gboolean
_exit_timeout (gpointer user_data)
{
    g_error ("start_exit_async() is timeout. You might run ibus-daemon " \
             "with systemd under GNOME and the exit API does not work. " \
             "You need to export IBUS_DAEMON_WITH_SYSTEMD=1 .\n");
    return G_SOURCE_REMOVE;
}

static void
finish_ibus_restart_async (GPid      pid,
                           gint      status,
                           gpointer *user_data)
{
    ExitAsyncData *data = (ExitAsyncData *)user_data;
    g_spawn_close_pid (pid);
    if (data->has_socket_path == FALSE) {
        g_debug ("ibus_bus_exit_finish: OK socket file: none");
        g_usleep (G_USEC_PER_SEC);
        call_next_async_function ();
    } else {
        g_debug ("ibus_bus_exit_finish: OK socket file: monitored");
        if (!data->exited)
            data->timeout_id = g_timeout_add_seconds (10, _exit_timeout, NULL);
    }
}

static void
finish_exit_async (GObject *source_object,
                   GAsyncResult *res,
                   gpointer user_data)
{
    GError *error = NULL;
    gboolean result = ibus_bus_exit_async_finish (bus,
                                                  res,
                                                  &error);
    ExitAsyncData *data = (ExitAsyncData *)user_data;
    if (error) {
        g_warning ("Failed to ibus_bus_exit(): %s", error->message);
        g_error_free (error);
    }
    g_assert (result);
    g_assert (data);
    if (data->has_socket_path == FALSE) {
        g_debug ("ibus_bus_exit_finish: OK socket file: none");
        g_usleep (G_USEC_PER_SEC);
        call_next_async_function ();
    } else {
        g_debug ("ibus_bus_exit_finish: OK socket file: monitored");
        if (!data->exited)
            data->timeout_id = g_timeout_add_seconds (10, _exit_timeout, NULL);
    }
}

static void
start_exit_async (void)
{
    static ExitAsyncData data = {
        .has_socket_path = FALSE,
        .exited          = FALSE,
        .timeout_id      = 0
    };
    /* When `./runtest ibus-bus` runs, ibus-daemon sometimes failed to
     * restart because closing a file descriptor was failed in
     * bus/server.c:_restart_server() with a following error:
     *     "inotify read(): Bad file descriptor"
     * Now g_usleep() is added here to write down the buffer and not to
     * fail to restart ibus-daemon.
     */
    g_usleep (G_USEC_PER_SEC);
    /* IBus socket file can be deleted after finish_exit_async() is called
     * so the next ibus_bus_new_async() in test_bus_new_async() could be failed
     * if the socket file is deleted after ibus_bus_new_async() is called
     * in case that the socket file is not monitored.
     */
    if (!g_getenv ("IBUS_ADDRESS")) {
        const gchar *address_path = ibus_get_socket_path ();
        GFile *file;
        GError *error = NULL;
        GFileMonitor *monitor;

        g_assert (address_path);
        file = g_file_new_for_path (address_path);
        g_assert (file);
        data.has_socket_path = TRUE;
        monitor = g_file_monitor (file, G_FILE_MONITOR_NONE, NULL, &error);
        if (error) {
            g_warning ("Failed to monitor socket file: %s", error->message);
            g_error_free (error);
        }
        g_assert (monitor);
        g_signal_connect (monitor, "changed",
                          G_CALLBACK (_socket_changed_cb),
                          &data);
        g_object_unref (file);
    }
    /* When ibus-daemon runs with systemd, restarting the daemon with
     * ibus_bus_exit_async() does not work so runs `ibus restart` command
     * with IBUS_DAEMON_WITH_SYSTEMD variable instead.
     */
    if (g_getenv ("IBUS_DAEMON_WITH_SYSTEMD")) {
        gchar *argv[] = { "ibus", "restart", NULL };
        GSpawnFlags flags = G_SPAWN_DO_NOT_REAP_CHILD \
                            | G_SPAWN_SEARCH_PATH \
                            | G_SPAWN_STDOUT_TO_DEV_NULL \
                            | G_SPAWN_STDERR_TO_DEV_NULL;
        GPid pid = 0;
        GError *error = NULL;
        g_spawn_async (NULL, argv, NULL, flags, NULL, NULL, &pid, &error);
        if (error) {
            g_warning ("Failed to call ibus restart: %s", error->message);
            g_error_free (error);
        }
        g_child_watch_add (pid,
                           (GChildWatchFunc)finish_ibus_restart_async,
                           &data);
    } else {
        ibus_bus_exit_async (bus,
                             TRUE, /* restart */
                             -1, /* timeout */
                             NULL, /* cancellable */
                             finish_exit_async,
                             &data); /* user_data */
    }
}

static gboolean
test_async_apis_finish (gpointer user_data)
{
    /* INFO: g_warning() causes SEGV with runtest script */
    if (ibus_get_address () == NULL)
        g_warning ("ibus-daemon does not restart yet from start_exit_async().");
    ibus_quit ();
    return FALSE;
}

static void
test_get_engines_by_names (void)
{
    IBusEngineDesc **engines = NULL;
    const gchar *names[] = {
        "xkb:us::eng",
        "xkb:ca:eng:eng",
        "xkb:fr::fra",
        "xkb:jp::jpn",
        "invalid_engine_name",
        NULL,
    };

    engines = ibus_bus_get_engines_by_names (bus, names);

    g_assert(engines != NULL);
    IBusEngineDesc **p;

    gint i = 0;
    for (p = engines; *p != NULL; p++) {
        g_assert (IBUS_IS_ENGINE_DESC (*p));
        g_assert_cmpstr (names[i], ==, ibus_engine_desc_get_name (*p));
        i++;
        g_object_unref (*p);
    }

    // The last engine does not exist.
    g_assert_cmpint (i, ==, G_N_ELEMENTS(names) - 2);

    g_free (engines);

    engines = NULL;
}

static void
test_async_apis (void)
{
    g_debug ("start");
    call_next_async_function ();
    ibus_main ();
}

static void
call_next_async_function (void)
{
    static void (*async_functions[])(void) = {
        start_request_name_async,
        start_name_has_owner_async,
        start_get_name_owner_async,
        start_release_name_async,
        start_add_match_async,
        start_remove_match_async,
        start_current_input_context_async,
        // FIXME test ibus_bus_register_component_async.
        start_list_engines_async,
        start_list_active_engines_async,
        start_get_use_sys_layout_async,
        start_get_use_global_engine_async,
        start_is_global_engine_enabled_async,
        start_set_global_engine_async,
        start_get_global_engine_async,
        start_preload_engines_async,
        start_get_address_async,
        start_get_current_input_context_async,
        start_get_engines_async,
        start_get_prop_global_engine_async,
        start_set_preload_engines_async,
        start_exit_async,
    };
    static guint index = 0;

    /* Use g_timeout_add to make sure test_async_apis finishes even if
     * async_functions is empty.
     */
    if (index >= G_N_ELEMENTS (async_functions))
        g_timeout_add (1, test_async_apis_finish, NULL);
    else
        (*async_functions[index++])();
}

static void
_bus_connected_cb (IBusBus *bus,
                   gpointer user_data)
{
    g_assert (ibus_bus_is_connected (bus));
    ibus_quit ();
}

static void
test_bus_new_async (void)
{
    g_object_unref (bus);
    bus = ibus_bus_new_async ();
    g_signal_connect (bus, "connected", G_CALLBACK (_bus_connected_cb), NULL);
    ibus_main ();
}

gint
main (gint    argc,
      gchar **argv)
{
    gint result;
    ibus_init ();
    g_test_init (&argc, &argv, NULL);
    bus = ibus_bus_new ();
    g_object_unref (bus);
    bus = ibus_bus_new (); // crosbug.com/17293

    if (!ibus_bus_is_connected (bus)) {
        g_warning ("Not connected to ibus-daemon");
        g_object_unref (bus);
        return -1;
    }

    g_test_add_func ("/ibus/list-engines", test_list_engines);
    g_test_add_func ("/ibus/list-active-engines", test_list_active_engines);
    g_test_add_func ("/ibus/create-input-context-async",
                     test_create_input_context_async);
    g_test_add_func ("/ibus/get-engines-by-names", test_get_engines_by_names);
    g_test_add_func ("/ibus/get-address", test_get_address);
    g_test_add_func ("/ibus/get-current-input-context",
                     test_get_current_input_context);
    g_test_add_func ("/ibus/get-engines", test_get_engines);
    g_test_add_func ("/ibus/get-global-engine", test_get_global_engine);
    g_test_add_func ("/ibus/set-preload-engines", test_set_preload_engines);
    g_test_add_func ("/ibus/async-apis", test_async_apis);
    g_test_add_func ("/ibus/bus-new-async", test_bus_new_async);
    g_test_add_func ("/ibus/bus-new-async/list-engines", test_list_engines);
    g_test_add_func ("/ibus/bus-new-async/list-active-engines", test_list_active_engines);
    g_test_add_func ("/ibus/bus-new-async/create-input-context-async",
                     test_create_input_context_async);
    g_test_add_func ("/ibus/bus-new-async/get-engines-by-names", test_get_engines_by_names);
    g_test_add_func ("/ibus/bus-new-async/async-apis", test_async_apis);

    result = g_test_run ();
    g_object_unref (bus);

    return result;
}
