#include <gtk/gtk.h>
#include "ibus.h"
#include "ibuscomposetable.h"
#include "ibusenginesimpleprivate.h"

#define GREEN "\033[0;32m"
#define RED   "\033[0;31m"
#define NC    "\033[0m"

static gchar *m_test_name;
static gchar *m_session_name;
static IBusBus *m_bus;
static IBusComponent *m_component;
static gchar *m_compose_file;
static IBusComposeTableEx *m_compose_table;
static IBusEngine *m_engine;
static gchar *m_srcdir;
static gboolean m_is_gtk_32bit_compose_error;
static GMainLoop *m_loop;
static char *m_engine_is_focused;
#if GTK_CHECK_VERSION (4, 0, 0)
static gboolean m_list_toplevel;
#endif

typedef enum {
    TEST_COMMIT_TEXT,
    TEST_CREATE_ENGINE,
    TEST_DELAYED_FOCUS_IN
} TestIDleCategory;

typedef struct _TestIdleData {
    TestIDleCategory category;
    guint            idle_id;
} TestIdleData;

extern guint ibus_compose_key_flag (guint key);

#if GTK_CHECK_VERSION (4, 0, 0)
static void     event_controller_enter_cb (GtkEventController *controller,
                                           gpointer            user_data);
#else
static gboolean window_focus_in_event_cb (GtkWidget     *entry,
                                          GdkEventFocus *event,
                                          gpointer       data);
#endif


gboolean
is_integrated_desktop ()
{
    if (!m_session_name)
        m_session_name = g_strdup (g_getenv ("XDG_SESSION_DESKTOP"));
    if (!m_session_name)
        return FALSE;
    if (!g_ascii_strncasecmp (m_session_name, "gnome", strlen ("gnome")))
       return TRUE;
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


static gchar *
get_compose_path ()
{
    const gchar * const *langs;
    const gchar * const *l;
    gchar *compose_path = NULL;

    if (m_is_gtk_32bit_compose_error)
        g_assert (g_setenv ("LANG", m_test_name, TRUE));
#if GLIB_CHECK_VERSION (2, 58, 0)
    langs = g_get_language_names_with_category ("LC_CTYPE");
#else
    langs = g_get_language_names ();
#endif
    if (m_is_gtk_32bit_compose_error)
        g_assert (g_setenv ("LANG", "en_US.UTF-8", TRUE));
    for (l = langs; *l; l++) {
        if (g_str_has_prefix (*l, "en_US"))
            break;
        if (g_strcmp0 (*l, "C") == 0)
            break;
        compose_path = g_build_filename (X11_LOCALEDATADIR,
                                         *l,
                                         "Compose",
                                         NULL);
        if (g_file_test (compose_path, G_FILE_TEST_EXISTS))
            break;
        g_free (compose_path);
        compose_path = NULL;
    }

    return compose_path;
}


gboolean
idle_cb (gpointer user_data)
{
    TestIdleData *data = (TestIdleData *)user_data;
    static int n = 0;
    gboolean terminate_program = FALSE;

    g_assert (data);
    switch (data->category) {
    case TEST_CREATE_ENGINE:
        g_test_fail_printf ("\"create-engine\" signal is timeout.");
        break;
    case TEST_COMMIT_TEXT:
        if (data->idle_id)
            g_test_fail_printf ("Commiting composed chars is timeout.");
        if (m_loop) {
            if (g_main_loop_is_running (m_loop))
                g_main_loop_quit (m_loop);
            g_clear_pointer (&m_loop, g_main_loop_unref);
            terminate_program = TRUE;
        }
        data->idle_id = 0;
        break;
    case TEST_DELAYED_FOCUS_IN:
        if (m_engine_is_focused) {
            data->idle_id = 0;
            n = 0;
            g_main_loop_quit (m_loop);
            return G_SOURCE_REMOVE;
        }
        if (n++ < 10) {
            g_test_message ("Waiting for \"focus-in\" signal %dth times", n);
            return G_SOURCE_CONTINUE;
        }
        g_test_fail_printf ("\"focus-in\" signal is timeout.");
        g_main_loop_quit (m_loop);
        terminate_program = TRUE;
        n = 0;
        break;
    default:
        g_test_fail_printf ("Idle func is called by wrong category:%d.",
                            data->category);
        break;
    }
    if (terminate_program) {
#if GTK_CHECK_VERSION (4, 0, 0)
        m_list_toplevel = FALSE;
#else
        gtk_main_quit ();
#endif
    }
    return G_SOURCE_REMOVE;
}


static void
engine_focus_in_cb (IBusEngine *engine,
#ifdef IBUS_FOCUS_IN_ID
                    gchar      *object_path,
                    gchar      *client,
#endif
                    gpointer    user_data)
{
#ifdef IBUS_FOCUS_IN_ID
    g_test_message ("engine_focus_in_cb %s %s", object_path, client);
    m_engine_is_focused = g_strdup (client);
#else
    g_test_message ("engine_focus_in_cb");
    m_engine_is_focused = g_strdup ("No named");
#endif
}

static void
engine_focus_out_cb (IBusEngine *engine,
#ifdef IBUS_FOCUS_IN_ID
                     gchar      *object_path,
#endif
                     gpointer    user_data)
{
#ifdef IBUS_FOCUS_IN_ID
    g_test_message ("engine_focus_out_cb %s", object_path);
#else
    g_test_message ("engine_focus_out_cb");
#endif
    g_clear_pointer (&m_engine_is_focused, g_free);
}


static IBusEngine *
create_engine_cb (IBusFactory *factory,
                  const gchar *name,
                  gpointer     user_data)
{
    static int i = 1;
    gchar *engine_path =
            g_strdup_printf ("/org/freedesktop/IBus/engine/simpletest/%d",
                             i++);
    gchar *compose_path;
    TestIdleData *data = (TestIdleData *)user_data;

    g_assert (data);
    /* Don't reset idle_id to avoid duplicated register_ibus_engine(). */
    g_source_remove (data->idle_id);
    m_engine = (IBusEngine *)g_object_new (
            IBUS_TYPE_ENGINE_SIMPLE,
            "engine-name",      name,
            "object-path",      engine_path,
            "connection",       ibus_bus_get_connection (m_bus),
#ifdef IBUS_FOCUS_IN_ID
            "has-focus-id",     TRUE,
#endif
            NULL);
    g_free (engine_path);

    m_engine_is_focused = NULL;
#ifdef IBUS_FOCUS_IN_ID
    g_signal_connect (m_engine, "focus-in-id",
#else
    g_signal_connect (m_engine, "focus-in",
#endif
                      G_CALLBACK (engine_focus_in_cb), NULL);
#ifdef IBUS_FOCUS_IN_ID
    g_signal_connect (m_engine, "focus-out-id",
#else
    g_signal_connect (m_engine, "focus-out",
#endif
                      G_CALLBACK (engine_focus_out_cb), NULL);
    if (m_compose_file)
        compose_path = g_build_filename (m_srcdir, m_compose_file, NULL);
    else
        compose_path = get_compose_path ();
    if (compose_path != NULL) {
        guint16 saved_version = 0;
        ibus_engine_simple_add_compose_file (IBUS_ENGINE_SIMPLE (m_engine),
                                             compose_path);
        m_compose_table = ibus_compose_table_load_cache (compose_path,
                                                         &saved_version);
        if (m_compose_table)
            g_assert (saved_version);
    }
    g_free (compose_path);
    return m_engine;
}


static gboolean
register_ibus_engine ()
{
    static TestIdleData data = { .category = TEST_CREATE_ENGINE, .idle_id = 0 };
    IBusFactory *factory;
    IBusEngineDesc *desc;

    if (data.idle_id) {
        g_test_incomplete ("Test is called twice due to a timeout.");
        return TRUE;
    }
    m_bus = ibus_bus_new ();
    if (!ibus_bus_is_connected (m_bus)) {
        g_test_fail_printf ("ibus-daemon is not running.");
        return FALSE;
    }
    factory = ibus_factory_new (ibus_bus_get_connection (m_bus));
    data.idle_id = g_timeout_add_seconds (20, idle_cb, &data);
    g_signal_connect (factory, "create-engine",
                      G_CALLBACK (create_engine_cb), &data);

    m_component = ibus_component_new (
            "org.freedesktop.IBus.SimpleTest",
            "Simple Engine Test",
            "0.0.1",
            "GPL",
            "Takao Fujiwara <takao.fujiwara1@gmail.com>",
            "https://github.com/ibus/ibus/wiki",
            "",
            "ibus");
    desc = ibus_engine_desc_new (
            "xkbtest:us::eng",
            "XKB Test",
            "XKB Test",
            "en",
            "GPL",
            "Takao Fujiwara <takao.fujiwara1@gmail.com>",
            "ibus-engine",
            "us");
    ibus_component_add_engine (m_component, desc);
    ibus_bus_register_component (m_bus, m_component);

    return TRUE;
}


static void
window_destroy_cb (void)
{
#if GTK_CHECK_VERSION (4, 0, 0)
    m_list_toplevel = FALSE;
#else
    gtk_main_quit ();
#endif
}


static void
set_engine_cb (GObject      *object,
               GAsyncResult *res,
               gpointer      user_data)
{
    IBusBus *bus = IBUS_BUS (object);
#if GTK_CHECK_VERSION (4, 0, 0)
    GtkEventController *controller = GTK_EVENT_CONTROLLER (user_data);
#else
    GtkWidget *entry = GTK_WIDGET (user_data);
#endif
    GError *error = NULL;
    static TestIdleData data = { .category = TEST_COMMIT_TEXT, .idle_id = 0 };
    int i, j;
    int index_stride;
    IBusComposeTablePrivate *priv;

    if (!ibus_bus_set_global_engine_async_finish (bus, res, &error)) {
        g_test_fail_printf ("set engine failed: %s", error->message);
        g_error_free (error);
        return;
    }

    /* ibus_im_context_focus_in() is called after GlboalEngine is set.
     * The focus-in/out events happen more slowly in a busy system
     * likes with a TMT tool.
     */
    if (is_integrated_desktop () && g_getenv ("IBUS_DAEMON_WITH_SYSTEMD")) {
        g_test_message ("Start tiny \"focus-in\" signal test");
        for (i = 0; i < 3; i++) {
            data.category = TEST_DELAYED_FOCUS_IN;
            data.idle_id = g_timeout_add_seconds (1, idle_cb, &data);
            g_main_loop_run (m_loop);
            if (data.idle_id != 0)
                return;
        }
        g_test_message ("End tiny \"focus-in\" signal test");
        data.category = TEST_COMMIT_TEXT;
    }
    if (m_compose_table == NULL) {
        g_test_skip ("Your locale uses en_US compose table.");
        idle_cb (&data);
        return;
    }

    index_stride = m_compose_table->max_seq_len + 2;
    for (i = 0;
         i < (m_compose_table->n_seqs * index_stride);
         i += index_stride) {
        data.idle_id = g_timeout_add_seconds (20, idle_cb, &data);
        for (j = i; j < i + (index_stride - 2); j++) {
            guint keyval = m_compose_table->data[j];
            guint keycode = 0;
            guint modifiers = 0;
            gboolean retval;

            if (keyval == 0)
                break;
            keyval += ibus_compose_key_flag (keyval);
            g_signal_emit_by_name (m_engine, "process-key-event",
                                   keyval, keycode, modifiers, &retval);
            modifiers |= IBUS_RELEASE_MASK;
            g_signal_emit_by_name (m_engine, "process-key-event",
                                   keyval, keycode, modifiers, &retval);
        }
        /* Need to wait for calling window_inserted_text_cb() with
         * g_main_loop_run() because the commit-text event could be cancelled
         * by the next commit-text event with gnome-shell in Wayland.
         */
        g_main_loop_run (m_loop);
        if (data.idle_id) {
            g_source_remove (data.idle_id);
            data.idle_id = 0;
        }
    }
    priv = m_compose_table->priv;
    if (priv) {
        for (i = 0;
             i < (priv->first_n_seqs * index_stride);
             i += index_stride) {
            data.idle_id = g_timeout_add_seconds (20, idle_cb, &data);
            for (j = i; j < i + (index_stride - 2); j++) {
                guint keyval = priv->data_first[j];
                guint keycode = 0;
                guint modifiers = 0;
                gboolean retval;

                if (keyval == 0)
                    break;
                keyval += ibus_compose_key_flag (keyval);
                g_signal_emit_by_name (m_engine, "process-key-event",
                                       keyval, keycode, modifiers, &retval);
                modifiers |= IBUS_RELEASE_MASK;
                g_signal_emit_by_name (m_engine, "process-key-event",
                                       keyval, keycode, modifiers, &retval);
            }
            g_main_loop_run (m_loop);
            if (data.idle_id) {
                g_source_remove (data.idle_id);
                data.idle_id = 0;
            }
        }
    }

#if GTK_CHECK_VERSION (4, 0, 0)
    g_signal_handlers_disconnect_by_func (
            controller,
            G_CALLBACK (event_controller_enter_cb),
            NULL);
#else
    g_signal_handlers_disconnect_by_func (entry, 
                                          G_CALLBACK (window_focus_in_event_cb),
                                          NULL);
#endif
    data.idle_id = g_timeout_add_seconds (10, idle_cb, &data);
}


static void
set_engine (gpointer user_data)
{
    g_test_message ("set_engine() is calling");
    g_assert (m_bus != NULL);

    ibus_bus_set_global_engine_async (m_bus,
                                      "xkbtest:us::eng",
                                      -1,
                                      NULL,
                                      set_engine_cb,
                                      user_data);
}


#if GTK_CHECK_VERSION (4, 0, 0)
static gboolean
event_controller_enter_delay (gpointer user_data)
{
    GtkEventController *controller = (GtkEventController *)user_data;
    GtkWidget *text = gtk_event_controller_get_widget (controller);
    static int i = 0;

    /* Wait for gtk_text_realize() which calls gtk_text_im_set_focus_in()
     * while gtk_text_focus_changed() also calls gtk_text_im_set_focus_in()
     * in GNOME Xorg.
     */
    if (gtk_widget_get_realized (text)) {
        set_engine (user_data);
        return G_SOURCE_REMOVE;
    }
    if (i++ == 10) {
        g_test_fail_printf ("Window is not realized with %d times", i);
#if GTK_CHECK_VERSION (4, 0, 0)
        m_list_toplevel = FALSE;
#else
        gtk_main_quit ();
#endif
        return G_SOURCE_REMOVE;
    }
    g_test_message ("event_controller_enter_delay %d", i);
    return G_SOURCE_CONTINUE;
}


static void
event_controller_enter_cb (GtkEventController *controller,
                           gpointer            user_data)
{
    static guint id = 0;

    g_test_message ("EventController emits \"enter\" signal");
    if (id)
        return;
    if (is_integrated_desktop ()) {
        /* Wait for 3 seconds in GNOME Wayland because there is a long time lag
         * between the "enter" signal on the event controller in GtkText
         * and the "FocusIn" D-Bus signal in BusInputContext of ibus-daemon
         * because mutter/core/window.c:meta_window_show() calls
         * mutter/core/window.c:meta_window_focus() ->
         * mutter/wayland/meta-wayland-text-input.c:
         *   meta_wayland_text_input_set_focus() ->
         * mutter/clutter/clutter/clutter-input-method.c:
         *   clutter_input_method_focus_out()
         * I.e. "FocusOut" and "FocusIn" D-Bus methods are always delayed
         * against the window present in GNOME Wayland.
         * If "FocusOut" and "FocusIn" D-Bus signals would be called after
         * "SetGlobalEngine" D-BUs signal was called in ibus-daemon,
         * the following functions could be called:
         *   engine_focus_out_cb() for the "gnome-shell" context
         *   engine_focus_in_cb() for the "fake" context
         *   engine_focus_out_cb() for the "fake" context
         *   engine_focus_in_cb() for the "gnome-shell" context
         * and ibus_engine_commit_text() would not work.
         * This assume the focus-in/out signals are called within the timeout
         * seconds.
         */
        id = g_timeout_add_seconds (3,
                                    event_controller_enter_delay,
                                    controller);
    } else {
        /* Call an idle function in Xorg because gtk_widget_add_controller()
         * calls g_list_prepend() for event_controllers and this controller is
         * always called before "gtk-text-focus-controller"
         * is caleld and the IM context does not receive the focus-in yet.
         */
        id = g_idle_add (event_controller_enter_delay, controller);
    }
}
#else

static gboolean
window_focus_in_event_cb (GtkWidget     *entry,
                          GdkEventFocus *event,
                          gpointer       data)
{
    g_test_message ("Entry emits \"focus-in-event\" signal");
    set_engine (entry);
    return FALSE;
}
#endif


static void
window_inserted_text_cb (GtkEntryBuffer *buffer,
                         guint           position,
                         const gchar    *chars,
                         guint           nchars,
                         gpointer        user_data)
{
/* https://gitlab.gnome.org/GNOME/gtk/commit/9981f46e0b
 * The latest GTK does not emit "inserted-text" when the text is "".
 */
#if !GTK_CHECK_VERSION (3, 22, 16)
    static int n_loop = 0;
#endif
    static guint stride = 0;
    static gboolean enable_32bit = FALSE;
    guint i;
    int seq;
    gunichar code = g_utf8_get_char (chars);
    const gchar *test;
#if ! GTK_CHECK_VERSION (4, 0, 0)
    GtkEntry *entry = GTK_ENTRY (user_data);
#endif
    IBusComposeTablePrivate *priv;
    static TestIdleData data = { .category = TEST_COMMIT_TEXT, .idle_id = 0 };

    g_assert (m_compose_table != NULL);

    priv = m_compose_table->priv;

#if !GTK_CHECK_VERSION (3, 22, 16)
    if (n_loop % 2 == 1) {
        n_loop = 0;
        return;
    }
#endif
#if GTK_CHECK_VERSION (4, 0, 0)
    if (code == 0)
        return;
#endif
    i = stride + (m_compose_table->max_seq_len + 2) - 2;
    seq = (i + 2) / (m_compose_table->max_seq_len + 2);
    if (!enable_32bit && !m_compose_table->n_seqs && priv)
        enable_32bit = TRUE;
    if (!enable_32bit) {
        if (m_compose_table->data[i] == code) {
            test = GREEN "PASS" NC;
        } else {
            test = RED "FAIL" NC;
            g_test_fail ();
        }
        g_test_message ("%05d/%05d %s expected: %04X typed: %04X",
                        seq,
                        m_compose_table->n_seqs,
                        test,
                        m_compose_table->data[i],
                        code);
    } else {
        const gchar *p = chars;
        guint num = priv->data_first[i];
        guint index = priv->data_first[i + 1];
        guint j = 0;
        gboolean valid_output = TRUE;
        for (j = 0; j < num; j++) {
            if (priv->data_second[index + j] != code) {
                valid_output = FALSE;
                break;
            }
            p = g_utf8_next_char (p);
            code = g_utf8_get_char (p);
        }
        if (valid_output) {
            test = GREEN "PASS" NC;
        } else {
            test = RED "FAIL" NC;
            g_test_fail ();
        }
        g_test_message ("%05d/%05ld %s expected: %04X[%d] typed: %04X",
                        seq,
                        priv->first_n_seqs,
                        test,
                        valid_output ? priv->data_second[index]
                                : priv->data_second[index + j],
                        valid_output ? index + num : index + j,
                        valid_output ? g_utf8_get_char (chars) : code);
    }

    stride += m_compose_table->max_seq_len + 2;

    if (!enable_32bit && seq == m_compose_table->n_seqs) {
        if (priv) {
            enable_32bit = TRUE;
            stride = 0;
            seq = 0;
        } else {
            /* Finish tests */
            idle_cb (&data);
            return;
        }
    }
    if (enable_32bit && seq == priv->first_n_seqs) {
        /* Finish tests */
        idle_cb (&data);
        return;
    }

#if !GTK_CHECK_VERSION (3, 22, 16)
    n_loop++;
#endif

#if GTK_CHECK_VERSION (4, 0, 0)
    gtk_entry_buffer_set_text (buffer, "", 0);
#else
    gtk_entry_set_text (entry, "");
#endif
    g_main_loop_quit (m_loop);
}


static void
create_window ()
{
    GtkWidget *window;
    GtkWidget *entry;
    GtkEntryBuffer *buffer;
#if GTK_CHECK_VERSION (4, 0, 0)
    GtkEventController *controller;
    GtkEditable *text;

    window = gtk_window_new ();
#else
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
#endif
    entry = gtk_entry_new ();

    g_signal_connect (window, "destroy",
                      G_CALLBACK (window_destroy_cb), NULL);
#if GTK_CHECK_VERSION (4, 0, 0)
    controller = gtk_event_controller_focus_new ();
    text = gtk_editable_get_delegate (GTK_EDITABLE (entry));
    gtk_event_controller_set_propagation_phase (controller, GTK_PHASE_BUBBLE);
    g_signal_connect_after (controller, "enter",
                            G_CALLBACK (event_controller_enter_cb), NULL);
    gtk_widget_add_controller (GTK_WIDGET (text), controller);
#else
    g_signal_connect (entry, "focus-in-event",
                      G_CALLBACK (window_focus_in_event_cb), NULL);
#endif
    buffer = gtk_entry_get_buffer (GTK_ENTRY (entry));
    g_signal_connect (buffer, "inserted-text",
                      G_CALLBACK (window_inserted_text_cb), entry);

#if GTK_CHECK_VERSION (4, 0, 0)
    gtk_window_set_child (GTK_WINDOW (window), entry);
    gtk_window_set_focus (GTK_WINDOW (window), entry);
    gtk_window_present (GTK_WINDOW (window));
#else
    gtk_container_add (GTK_CONTAINER (window), entry);
    gtk_widget_show_all (window);
#endif
}


static void
test_init (void)
{
    char *tty_name = ttyname (STDIN_FILENO);
    GMainLoop *loop;
    static guint idle_id = 0;

    if (idle_id) {
        g_test_incomplete ("Test is called twice due to a timeout.");
        return;
    }

    loop = g_main_loop_new (NULL, TRUE);
    g_test_message ("Test on %s", tty_name ? tty_name : "(null)");
    if (tty_name && g_strstr_len (tty_name, -1, "pts")) {
        idle_id = g_timeout_add_seconds (3, _wait_for_key_release_cb, loop);
        g_main_loop_run (loop);
    }
    g_main_loop_unref (loop);
}

static void
test_compose (void)
{
    GLogLevelFlags flags;
    if (!register_ibus_engine ())
        return;

#if GTK_CHECK_VERSION (4, 0, 0)
    m_list_toplevel = TRUE;
#endif
    create_window ();
    /* FIXME:
     * IBusIMContext opens GtkIMContextSimple as the slave and
     * GtkIMContextSimple opens the compose table on el_GR.UTF-8, and the
     * multiple outputs in el_GR's compose causes a warning in gtkcomposetable
     * and the warning always causes a fatal in GTest:
     " "GTK+ supports to output one char only: "
     */
    flags = g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL);
#if GTK_CHECK_VERSION (4, 0, 0)
    {
        GList *toplevels = gtk_window_list_toplevels ();
        while (m_list_toplevel)
            g_main_context_iteration (NULL, TRUE);
        g_list_free (toplevels);
    }
#else
    gtk_main ();
#endif
    g_log_set_always_fatal (flags);
    g_clear_pointer (&m_engine_is_focused, g_free);
    g_clear_pointer (&m_session_name, g_free);
    if (m_component)
        g_clear_object (&m_component);
    if (m_bus)
        g_clear_object (&m_bus);
}


int
main (int argc, char *argv[])
{
    gchar *test_path;
    int retval;

    ibus_init ();
    /* Avoid a warning of "AT-SPI: Could not obtain desktop path or name"
     * with gtk_main().
     */
    if (!g_setenv ("NO_AT_BRIDGE", "1", TRUE))
        g_message ("Failed setenv NO_AT_BRIDGE\n");
    g_test_init (&argc, &argv, NULL);
#if GTK_CHECK_VERSION (4, 0, 0)
    gtk_init ();
#else
    gtk_init (&argc, &argv);
#endif

    m_srcdir = (argc > 1 && strlen (argv[1]) < FILENAME_MAX)
            ? g_strdup (argv[1]) : g_strdup (".");
    m_compose_file = g_strdup (g_getenv ("COMPOSE_FILE"));
#if GLIB_CHECK_VERSION (2, 58, 0)
    m_test_name = g_strdup (g_get_language_names_with_category ("LC_CTYPE")[0]);
#else
    m_test_name = g_strdup (g_getenv ("LANG"));
#endif
    if (m_compose_file &&
        (!m_test_name || !g_ascii_strncasecmp (m_test_name, "en_US", 5))) {
        g_free (m_test_name);
        m_test_name = g_path_get_basename (m_compose_file);
    }
    /* The parent of GtkIMContextWayland is GtkIMContextSimple and
     * it outputs a warning of "Can't handle >16bit keyvals" in
     * gtk/gtkcomposetable.c:parse_compose_sequence() in pt-BR locales
     * and any warnings are treated as errors with g_test_run()
     * So export LANG=en_US.UTF-8 for GNOME Wayland as a workaround.
     */
    if (m_test_name && (!g_ascii_strncasecmp (m_test_name, "pt_BR", 5) ||
                        !g_ascii_strncasecmp (m_test_name, "fi_FI", 5)
                       )) {
        m_is_gtk_32bit_compose_error = TRUE;
    }
    if (m_is_gtk_32bit_compose_error) {
#if 1
        g_assert (g_setenv ("LANG", "en_US.UTF-8", TRUE));
#else
        /* FIXME: Use expected_messages in g_log_structured() */
        g_test_expect_message ("Gtk", G_LOG_LEVEL_WARNING,
                               "Can't handle >16bit keyvals");
#endif
    }
    g_test_add_func ("/ibus-compose/test-init", test_init);
    m_loop = g_main_loop_new (NULL, TRUE);
    test_path = g_build_filename ("/ibus-compose", m_test_name, NULL);
    g_test_add_func (test_path, test_compose);
    g_free (test_path);

    retval = g_test_run ();
    g_free (m_test_name);
    return retval;
}
