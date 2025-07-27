#include <gtk/gtk.h>
#include <glib/gstdio.h> /* g_access() */
#include "ibus.h"
#include <fcntl.h> /* creat() */
#include <locale.h>
#include <stdlib.h>
#include <sys/wait.h>

#define GREEN "\033[0;32m"
#define RED   "\033[0;31m"
#define NC    "\033[0m"

typedef enum {
    TEST_START_KEYPRESS,
    TEST_WAIT_START_KEYPRESS,
    TEST_END_KEYPRESS,
    TEST_PROCESS_KEY_EVENT,
    TEST_COMMIT_TEXT,
    TEST_CREATE_ENGINE,
    TEST_DELAYED_FOCUS_IN
} TestIDleCategory;

typedef struct _TestIdleData {
    TestIDleCategory category;
    guint            idle_id;
} TestIdleData;

static const gunichar test_results[][60] = {
   { 'a', '<', 'b', '>', 'c', '?', 'd', ':', 'e', '"', 'f', '{', 'g', '|', 0 },
#if 0
   { '~', 'A', '!', 'B', '@', 'C', '#', 'D', '(', 'E', ')', 'F', '+', 'G', 0 },
#endif
   { 0 }
};


static gchar *m_tmpfile_start;
static gchar *m_tmpfile_end;
static const gchar *m_arg0;
static pid_t m_pid;
static gchar *m_session_name;
static IBusBus *m_bus;
static IBusEngine *m_engine;
static GMainLoop *m_loop;
static char *m_engine_is_focused;
static guint m_process_key_id;
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
    /* See ibus-compose:_wait_for_key_release_cb() */
    g_test_message ("Wait for 3 seconds for key release event");
    g_main_loop_quit (loop);
    return G_SOURCE_REMOVE;
}


gboolean
idle_cb (gpointer user_data)
{
    TestIdleData *data = (TestIdleData *)user_data;
    static int n = 0;
    gboolean terminate_program = FALSE;

    g_assert (data);
    switch (data->category) {
    case TEST_START_KEYPRESS:
        if (g_access (m_tmpfile_start, 0) != -1) {
            data->idle_id = 0;
            n = 0;
            g_unlink (m_tmpfile_start);
            g_main_loop_quit (m_loop);
            return G_SOURCE_REMOVE;
        }
        if (n++ < 600) {
            if (!(n % 30))
                g_test_message ("Waiting for set_engine %dth times", n);
            return G_SOURCE_CONTINUE;
        }
        g_test_fail_printf ("set_engine is timeout.");
        g_main_loop_quit (m_loop);
        break;
    case TEST_WAIT_START_KEYPRESS:
        if (g_access (m_tmpfile_start, 0) == -1) {
            data->idle_id = 0;
            n = 0;
            g_main_loop_quit (m_loop);
            fprintf (stderr, "Started keypress\n");
            return G_SOURCE_REMOVE;
        }
        if (n++ < 30) {
            if (!(n % 5))
                g_test_message ("Waiting for starting keypress %dth times", n);
            return G_SOURCE_CONTINUE;
        }
        g_test_fail_printf ("Starting keypress is timeout.");
        g_main_loop_quit (m_loop);
        break;
    case TEST_END_KEYPRESS:
        if (g_access (m_tmpfile_end, 0) != -1) {
            data->idle_id = 0;
            n = 0;
            g_unlink (m_tmpfile_end);
            g_main_loop_quit (m_loop);
            fprintf (stderr, "Finished keypress\n");
            return G_SOURCE_REMOVE;
        }
        if (n++ < 600) {
            if (!(n % 30))
                g_test_message ("Waiting for finishing keypress %dth times", n);
            return G_SOURCE_CONTINUE;
        }
        g_test_fail_printf ("Finishing keypress is timeout.");
        g_main_loop_quit (m_loop);
        break;
    case TEST_PROCESS_KEY_EVENT:
        if (n++ < 30000) {
            if (!(n % 10))
                fprintf (stderr, "Waiting for process_key_event %dth times\n", n);
            return G_SOURCE_CONTINUE;
        }
        fprintf (stderr, "process_key_event is timeout.\n");
        g_main_loop_quit (m_loop);
        break;
    case TEST_CREATE_ENGINE:
        g_test_fail_printf ("\"create-engine\" signal is timeout.");
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
    if (terminate_program)
        ibus_quit ();
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


static gboolean
engine_process_key_event_cb (IBusEngine *engine,
                             guint       keyval,
                             guint       keycode,
                             guint       state,
                             gpointer    user_data)
{
    g_test_message ("engine_process_key_event_cb keyval:%X keycode:%u state:%x",
                    keyval, keycode, state);
    return FALSE;
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
    g_signal_connect (m_engine, "process-key-event",
                      G_CALLBACK (engine_process_key_event_cb), NULL);

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
        g_critical ("ibus-daemon is not running.");
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
    ibus_quit ();
}


static void
exec_keypress (void)
{
    static TestIdleData data = { .category = TEST_START_KEYPRESS,
                                 .idle_id = 0 };
    gchar *build_dir;
    gchar *keypress_path = NULL;
    gchar *standard_output = NULL;
    gchar *standard_error = NULL;
    gint wait_status = 0;
    GError *error = NULL;
    int fd;

    m_loop = g_main_loop_new (NULL, TRUE);
    data.idle_id = g_timeout_add_seconds (1, idle_cb, &data);
    g_main_loop_run (m_loop);
    g_main_loop_unref (m_loop);
    if (data.idle_id != 0)
        return;
    g_assert (m_arg0);
    build_dir = g_path_get_dirname (m_arg0);
    if (g_str_has_suffix (build_dir, "/.libs"))
        *(build_dir + strlen (build_dir) - 6) = '\0';
    keypress_path = g_build_filename (build_dir, "uinput-replay-test.sh", NULL);
    g_spawn_command_line_sync (keypress_path,
                               &standard_output,
                               &standard_error,
                               &wait_status,
                               &error);
    if (standard_output) {
        g_test_message ("keypress output: %s", standard_output);
        g_free (standard_output);
    }
    if (standard_error) {
        if (*standard_error)
            g_test_message ("keypress error: %s", standard_error);
        g_free (standard_error);
    }
    if (error) {
        g_test_message ("keypress error2: %s", error->message);
        g_error_free (error);
    }
    g_free (keypress_path);
    g_free (build_dir);

    fd = g_creat (m_tmpfile_end, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    g_close (fd, &error);
    exit (EXIT_SUCCESS);
}


static void
set_engine_cb (GObject      *object,
               GAsyncResult *res,
               gpointer      user_data)
{
    IBusBus *bus = IBUS_BUS (object);
    GError *error = NULL;
    static TestIdleData data = { .category = TEST_COMMIT_TEXT, .idle_id = 0 };
    int i;
    int fd;
    int wstatus = 0;
    pid_t wpid;

    if (!ibus_bus_set_global_engine_async_finish (bus, res, &error)) {
        g_critical ("set engine failed: %s", error->message);
        g_error_free (error);
        return;
    }

    /* See ibus-compose:set_engine_cb() */
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

    fd = g_creat (m_tmpfile_start, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    g_close (fd, &error);

    data.category = TEST_WAIT_START_KEYPRESS;
    data.idle_id = g_timeout_add_seconds (1, idle_cb, &data);
    g_main_loop_run (m_loop);
    if (data.idle_id != 0)
        return;
    errno = 0;
    if ((wpid = waitpid (m_pid, &wstatus, 0)) == -1)
        g_test_fail_printf ("keypress is failed: %s", g_strerror (errno));
    data.category = TEST_END_KEYPRESS;
    data.idle_id = g_timeout_add_seconds (1, idle_cb, &data);
    g_main_loop_run (m_loop);
    if (data.idle_id != 0)
        return;
    data.category = TEST_PROCESS_KEY_EVENT;
    m_process_key_id = data.idle_id = g_timeout_add_seconds (1, idle_cb, &data);
    g_main_loop_run (m_loop);
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

    if (gtk_widget_get_realized (text)) {
        set_engine (user_data);
        return G_SOURCE_REMOVE;
    }
    if (i++ == 10) {
        g_test_fail_printf ("Window is not realized with %d times", i);
        ibus_quit ();
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
    /* See ibus-compose:event_controller_enter_cb() */
    if (is_integrated_desktop ()) {
        id = g_timeout_add_seconds (3,
                                    event_controller_enter_delay,
                                    controller);
    } else {
        id = g_idle_add (event_controller_enter_delay, controller);
    }
}

#else

static gboolean
window_focus_in_event_cb (GtkWidget     *entry,
                          GdkEventFocus *event,
                          gpointer       user_data)
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
    GtkWidget *entry = user_data;
    static int i = 0;
    static int j = 0;
    int k;
    gunichar code;
    const gchar *test;

    g_test_message ("Window emits \"inserted-text\" %s %u", chars, nchars);
    g_assert (nchars == 1);
    /* NULL is inserted in GTK3 */
    if (!(code = g_utf8_get_char (chars)))
        return;
    if (code != test_results[i][j++]) {
        test = RED "FAIL" NC;
        g_test_fail_printf ("%05d:%05d %s expected: %04X typed: %04X",
                            i, j - 1,
                            test,
                            test_results[i][j - 1],
                            code);
    } else if (test_results[i][j]) {
        return;
    }

    g_print (GREEN "PASS" NC " ");
    for (k = 0; k < j; k++)
        g_print ("%lc(%X) ", test_results[i][k], test_results[i][k]);
    g_print ("\n");

    ++i;
    j = 0;
#if GTK_CHECK_VERSION (4, 0, 0)
    gtk_editable_set_text (GTK_EDITABLE (entry), "");
#else
    gtk_entry_set_text (GTK_ENTRY (entry), "");
#endif
    if (!test_results[i][j]) {
       g_assert (!j);
       if (m_process_key_id) {
           g_source_remove (m_process_key_id);
           m_process_key_id = 0;
           g_main_loop_quit (m_loop);
       }
       ibus_quit ();
    }
}

static void
create_window ()
{
    GtkWidget *window;
    GtkWidget *entry = gtk_entry_new ();
    GtkEntryBuffer *buffer;
#if GTK_CHECK_VERSION (4, 0, 0)
    GtkEventController *controller;

    window = gtk_window_new ();
#else
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
#endif

    g_signal_connect (window, "destroy",
                      G_CALLBACK (window_destroy_cb), NULL);
#if GTK_CHECK_VERSION (4, 0, 0)
    controller = gtk_event_controller_focus_new ();
    g_signal_connect (controller, "enter",
                      G_CALLBACK (event_controller_enter_cb), NULL);
    gtk_widget_add_controller (window, controller);
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
test_keypress (void)
{
    if (!register_ibus_engine ())
        return;

    create_window ();
    ibus_main ();
    g_clear_pointer (&m_session_name, g_free);
}


int
main (int argc, char *argv[])
{
    int fd;
    GError *error = NULL;

    setlocale (LC_ALL, "");

    m_tmpfile_start = g_strdup ("/tmp/ibus-keypress_start_XXXXXX.log");
    errno = 0;
    if ((fd = g_mkstemp (m_tmpfile_start)) == -1) {
        /* g_warning() before g_test_init() */
        g_warning ("mkstemp is failed: %s", g_strerror (errno));
        g_unlink (m_tmpfile_start);
        exit (EXIT_FAILURE);
    }
    g_close (fd, &error);
    g_unlink (m_tmpfile_start);

    m_tmpfile_end = g_strdup ("/tmp/ibus-keypress_end_XXXXXX.log");
    errno = 0;
    if ((fd = g_mkstemp (m_tmpfile_end)) == -1) {
        /* g_warning() before g_test_init() */
        g_warning ("mkstemp is failed: %s", g_strerror (errno));
        g_unlink (m_tmpfile_end);
        exit (EXIT_FAILURE);
    }
    g_close (fd, &error);
    g_unlink (m_tmpfile_end);

    m_arg0 = argv[0];
    if (!(m_pid = fork ()))
        exec_keypress ();

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
    ibus_init ();

    g_test_add_func ("/ibus-keypress/test-init", test_init);
    m_loop = g_main_loop_new (NULL, TRUE);
    g_test_add_func ("/ibus-keypress/keyrepss", test_keypress);

    return g_test_run ();
}
