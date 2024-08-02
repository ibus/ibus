/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include <locale.h>
#include <string.h>
#include <unistd.h>
#include "ibus.h"

static IBusBus *bus;

#define BEFORE_ENGINE "xkb:us::eng"
#define AFTER_ENGINE "xkb:jp::jpn"

static const gchar *engine_names[] = {
    BEFORE_ENGINE,
    AFTER_ENGINE
};

static const gchar *engine_names2[] = {
    AFTER_ENGINE,
    BEFORE_ENGINE
};

static void
change_global_engine (gboolean reverse)
{
    gint i;

    for (i = 0; i < G_N_ELEMENTS (engine_names); i++) {
        const gchar *engine_name = engine_names[i];
        if (reverse)
            engine_name = engine_names2[i];
        ibus_bus_set_global_engine (bus, engine_name);
        IBusEngineDesc *engine_desc = ibus_bus_get_global_engine (bus);
        g_assert_cmpstr (ibus_engine_desc_get_name (engine_desc),
                         ==,
                         engine_name);
        g_object_unref (G_OBJECT (engine_desc));
    }
}

static void
change_context_engine (IBusInputContext *context)
{
    gint i;

    for (i = 0; i < G_N_ELEMENTS (engine_names); i++) {
        ibus_input_context_set_engine (context, engine_names[i]);
        IBusEngineDesc *engine_desc = ibus_input_context_get_engine (context);
        g_assert_cmpstr (ibus_engine_desc_get_name (engine_desc),
                         ==,
                         engine_names[i]);
        g_object_unref (engine_desc);
    }
}

typedef struct {
    gint count;
    guint timeout_id;
    guint idle_id;
    gboolean reverse;
} GlobalEngineChangedData;

static void
global_engine_changed_cb (IBusBus *bus, gchar *name, gpointer user_data)
{
    GlobalEngineChangedData *data = (GlobalEngineChangedData *) user_data;
    if (data->count++ == 1)
        ibus_quit ();
}

static gboolean
timeout_cb (gpointer user_data)
{
    GlobalEngineChangedData *data = (GlobalEngineChangedData *) user_data;
    if (data->count == 0)
        ibus_quit ();
    data->timeout_id = 0;
    return FALSE;
}

static gboolean
change_global_engine_cb (gpointer user_data)
{
    GlobalEngineChangedData *data = (GlobalEngineChangedData *) user_data;
    change_global_engine (data->reverse);
    data->idle_id = 0;
    return FALSE;
}

gboolean
_wait_for_key_release_cb (gpointer user_data)
{
    GMainLoop *loop = (GMainLoop *)user_data;
    /* If this program is invoked by manual with Enter key in GNOME
     * Wayland session, ibus_input_context_focus_in() can be called in
     * test_context_engine_set_by_global() before the key release of
     * the Enter key so ibus/bus/inputcontext.c:_ic_process_key_event()
     * could call another bus_input_context_focus_in() in that test case
     * and fail.
     */
    g_test_message ("Wait for 3 seconds for key release event");
    g_main_loop_quit (loop);
    return G_SOURCE_REMOVE;
}

static void
test_init (void)
{
    char *tty_name = ttyname (STDIN_FILENO);
    GMainLoop *loop = g_main_loop_new (NULL, TRUE);
    g_test_message ("Test on %s", tty_name ? tty_name : "(null)");
    if (tty_name && g_strstr_len (tty_name, -1, "pts")) {
        g_timeout_add_seconds (3, _wait_for_key_release_cb, loop);
        g_main_loop_run (loop);
    }
    g_main_loop_unref (loop);
}

static void
test_global_engine (void)
{
    GLogLevelFlags flags;
    IBusEngineDesc *desc;
    GlobalEngineChangedData data;
    guint handler_id;

    if (!ibus_bus_get_use_global_engine (bus))
        return;

    /* "No global engine." warning is not critical message. */
    flags = g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL);
    desc = ibus_bus_get_global_engine (bus);
    g_log_set_always_fatal (flags);
    if (desc &&
        !g_strcmp0 (BEFORE_ENGINE, ibus_engine_desc_get_name (desc))) {
        data.reverse = TRUE;
    } else {
        data.reverse = FALSE;
    }
    g_test_message ("Initial engine name: %s",
                    desc ? ibus_engine_desc_get_name (desc) : "(null)");
    if (desc)
        g_object_unref (desc);

    data.count = 0;

    handler_id = g_signal_connect (bus,
                                   "global-engine-changed",
                                   G_CALLBACK (global_engine_changed_cb),
                                   &data);
    data.timeout_id = g_timeout_add_seconds (3, timeout_cb, &data);
    data.idle_id = g_idle_add ((GSourceFunc) change_global_engine_cb, &data);

    ibus_main ();

    g_assert_cmpint (data.count, ==, G_N_ELEMENTS (engine_names));

    if (data.idle_id > 0)
        g_source_remove (data.idle_id);
    if (data.timeout_id > 0)
        g_source_remove (data.timeout_id);
    g_signal_handler_disconnect (bus, handler_id);
}

static void
test_context_engine (void)
{
    IBusEngineDesc *engine_desc;
    IBusInputContext *context;

    if (ibus_bus_get_use_global_engine (bus))
        return;

    context = ibus_bus_create_input_context (bus, "test");
    ibus_input_context_set_capabilities (context, IBUS_CAP_FOCUS);

    /* ibus_bus_set_global_engine() changes focused context engine. */
    ibus_input_context_focus_in (context);

    change_context_engine (context);
    engine_desc = ibus_input_context_get_engine (context);
    g_assert_cmpstr (ibus_engine_desc_get_name (engine_desc), ==, AFTER_ENGINE);
    g_object_unref (engine_desc);

    g_object_unref (context);
}

static void
test_context_engine_set_by_global (void)
{
    IBusEngineDesc *engine_desc;
    IBusInputContext *context;

    if (!ibus_bus_get_use_global_engine (bus))
        return;

    context = ibus_bus_create_input_context (bus, "test");
    ibus_input_context_set_capabilities (context, IBUS_CAP_FOCUS);

    /* ibus_bus_set_global_engine() changes focused context engine. */
    ibus_input_context_focus_in (context);

    change_global_engine (FALSE);

    /* ibus_input_context_set_engine() does not take effect when
       global engine is used. */
    ibus_input_context_set_engine (context, BEFORE_ENGINE);

    engine_desc = ibus_input_context_get_engine (context);
    g_assert_cmpstr (ibus_engine_desc_get_name (engine_desc), ==, AFTER_ENGINE);
    g_object_unref (engine_desc);

    g_object_unref (context);
}

static void
test_context_engine_set_by_focus (void)
{
    IBusEngineDesc *engine_desc;
    IBusInputContext *context, *another_context;

    if (!ibus_bus_get_use_global_engine (bus))
        return;

    context = ibus_bus_create_input_context (bus, "test");
    ibus_input_context_set_capabilities (context, IBUS_CAP_FOCUS);

    another_context = ibus_bus_create_input_context (bus, "another");
    ibus_input_context_set_capabilities (another_context, IBUS_CAP_FOCUS);

    ibus_input_context_focus_in (context);

    change_global_engine (FALSE);

    /* When focus is lost, context engine is set to "dummy". */
    ibus_input_context_focus_in (another_context);

    engine_desc = ibus_input_context_get_engine (context);
    g_assert_cmpstr (ibus_engine_desc_get_name (engine_desc), ==, "dummy");
    g_object_unref (engine_desc);

    engine_desc = ibus_input_context_get_engine (another_context);
    g_assert_cmpstr (ibus_engine_desc_get_name (engine_desc), ==, AFTER_ENGINE);
    g_object_unref (engine_desc);

    g_object_unref (context);
    g_object_unref (another_context);
}

gint
main (gint    argc,
      gchar **argv)
{
    gint result;
    /* To get UTF-8 error messages with glib2 */
    setlocale (LC_ALL, "");
    ibus_init ();
    g_test_init (&argc, &argv, NULL);
    bus = ibus_bus_new ();
    g_object_unref (bus);
    bus = ibus_bus_new (); // crosbug.com/17293

    ibus_bus_set_watch_ibus_signal (bus, TRUE);

    g_test_add_func ("/ibus/engine-switch/test-init",
                     test_init);
    g_test_add_func ("/ibus/engine-switch/global-engine",
                     test_global_engine);
    g_test_add_func ("/ibus/engine-switch/context-engine",
                     test_context_engine);
    g_test_add_func ("/ibus/engine-switch/context-engine-set-by-global",
                     test_context_engine_set_by_global);
    g_test_add_func ("/ibus/engine-switch/context-engine-set-by-focus",
                     test_context_engine_set_by_focus);

    result = g_test_run ();
    g_object_unref (bus);

    return result;
}
