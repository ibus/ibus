#include <gtk/gtk.h>
#include "ibus.h"
#include "ibuscomposetable.h"
#include "ibusenginesimpleprivate.h"

#define GREEN "\033[0;32m"
#define RED   "\033[0;31m"
#define NC    "\033[0m"

static IBusBus *m_bus;
static gchar *m_compose_file;
static IBusComposeTableEx *m_compose_table;
static IBusEngine *m_engine;
static gchar *m_srcdir;
static GMainLoop *m_loop;
#if GTK_CHECK_VERSION (4, 0, 0)
static gboolean m_list_toplevel;
#endif

typedef enum {
    TEST_CREATE_ENGINE,
    TEST_COMMIT_TEXT
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

#if GLIB_CHECK_VERSION (2, 58, 0)
    langs = g_get_language_names_with_category ("LC_CTYPE");
#else
    langs = g_get_language_names ();
#endif
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
#if GTK_CHECK_VERSION (4, 0, 0)
            m_list_toplevel = FALSE;
#else
            gtk_main_quit ();
#endif
        }
        data->idle_id = 0;
        break;
    default:
        g_test_fail_printf ("Idle func is called by wrong category:%d.",
                            data->category);
        break;
    }
    return G_SOURCE_REMOVE;
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
    m_engine = ibus_engine_new_with_type (IBUS_TYPE_ENGINE_SIMPLE,
                                          name,
                                          engine_path,
                                          ibus_bus_get_connection (m_bus));
    g_free (engine_path);
    if (m_compose_file)
        compose_path = g_build_filename (m_srcdir, m_compose_file, NULL);
    else
        compose_path = get_compose_path ();
    if (compose_path != NULL) {
        ibus_engine_simple_add_compose_file (IBUS_ENGINE_SIMPLE (m_engine),
                                             compose_path);
        m_compose_table = ibus_compose_table_load_cache (compose_path);
    }
    g_free (compose_path);
    return m_engine;
}


static gboolean
register_ibus_engine ()
{
    static TestIdleData data = { .category = TEST_CREATE_ENGINE, .idle_id = 0 };
    IBusFactory *factory;
    IBusComponent *component;
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

    component = ibus_component_new (
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
    ibus_component_add_engine (component, desc);
    ibus_bus_register_component (m_bus, component);

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
    /* Call an idle function because gtk_widget_add_controller()
     * calls g_list_prepend() for event_controllers and this controller is
     * always called before "gtk-text-focus-controller"
     * is caleld and the IM context does not receive the focus-in yet.
     */
    if (id)
        return;
    id = g_idle_add (event_controller_enter_delay, controller);
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
    GMainLoop *loop = g_main_loop_new (NULL, TRUE);
    g_test_message ("Test on %s", tty_name ? tty_name : "(null)");
    if (tty_name && g_strstr_len (tty_name, -1, "pts")) {
        g_timeout_add_seconds (3, _wait_for_key_release_cb, loop);
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
    gtk_window_list_toplevels ();
    while (m_list_toplevel)
        g_main_context_iteration (NULL, TRUE);
#else
    gtk_main ();
#endif
    g_log_set_always_fatal (flags);
}


int
main (int argc, char *argv[])
{
    gchar *test_name;
    gchar *test_path;

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
    test_name = g_strdup (g_get_language_names_with_category ("LC_CTYPE")[0]);
#else
    test_name = g_strdup (g_getenv ("LANG"));
#endif
    if (m_compose_file &&
        (!test_name || !g_ascii_strncasecmp (test_name, "en_US", 5))) {
        g_free (test_name);
        test_name = g_path_get_basename (m_compose_file);
    }
    g_test_add_func ("/ibus-compose/test-init", test_init);
    m_loop = g_main_loop_new (NULL, TRUE);
    test_path = g_build_filename ("/ibus-compose", test_name, NULL);
    g_test_add_func (test_path, test_compose);
    g_free (test_path);
    g_free (test_name);

    return g_test_run ();
}
