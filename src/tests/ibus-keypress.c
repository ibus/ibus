#include <gtk/gtk.h>
#include <glib/gstdio.h> /* g_access() */
#include "ibus.h"
#include <fcntl.h> /* creat() */
#include <locale.h>
#include <stdlib.h>
#include <sys/wait.h>

#include <linux/input-event-codes.h>
#include <linux/input.h>

#include "uinput-replay.h"

#define GREEN "\033[0;32m"
#define RED   "\033[0;31m"
#define NC    "\033[0m"

#define msleep(t) usleep((t) * 1000)

typedef enum {
    TEST_COMMIT_TEXT,
    TEST_CREATE_ENGINE,
    TEST_DELAYED_FOCUS_IN
} TestIDleCategory;

typedef struct _TestIdleData {
    TestIDleCategory category;
    guint            idle_id;
} TestIdleData;

typedef struct {
    guint source;
    GtkWidget *window;
} WindowDestroyData;

/* FIXME */
static int pipefds[2];

static const gunichar test_results[][60] = {
   { 'a', '<', 'b', '>', 'c', '?', 'd', ':', 'e', '"', 'f', '{', 'g', '|', 0 },
#if 0
   { '~', 'A', '!', 'B', '@', 'C', '#', 'D', '(', 'E', ')', 'F', '+', 'G', 0 },
#endif
   { 0 }
};

#define INPUT_EVENT(type_, code_, value_) { .type = type_, .code = code_, .value = value_, }
static const struct input_event test_data[] = {
    INPUT_EVENT(EV_MSC, MSC_SCAN, 28),
    INPUT_EVENT(EV_KEY, KEY_ENTER, 0),
    INPUT_EVENT(EV_SYN, SYN_REPORT, 0),

    INPUT_EVENT(EV_MSC, MSC_SCAN, 30),
    INPUT_EVENT(EV_KEY, KEY_A, 1),
    INPUT_EVENT(EV_SYN, SYN_REPORT, 0),

    INPUT_EVENT(EV_MSC, MSC_SCAN, 30),
    INPUT_EVENT(EV_KEY, KEY_A, 0),
    INPUT_EVENT(EV_SYN, SYN_REPORT, 0),

    INPUT_EVENT(EV_MSC, MSC_SCAN, 42),
    INPUT_EVENT(EV_KEY, KEY_LEFTSHIFT, 1),
    INPUT_EVENT(EV_MSC, MSC_SCAN, 51),
    INPUT_EVENT(EV_KEY, KEY_COMMA, 1),
    INPUT_EVENT(EV_SYN, SYN_REPORT, 0),

    INPUT_EVENT(EV_MSC, MSC_SCAN, 51),
    INPUT_EVENT(EV_KEY, KEY_COMMA, 0),
    INPUT_EVENT(EV_MSC, MSC_SCAN, 42),
    INPUT_EVENT(EV_KEY, KEY_LEFTSHIFT, 0),
    INPUT_EVENT(EV_MSC, MSC_SCAN, 48),
    INPUT_EVENT(EV_KEY, KEY_B, 1),
    INPUT_EVENT(EV_SYN, SYN_REPORT, 0),

    INPUT_EVENT(EV_MSC, MSC_SCAN, 48),
    INPUT_EVENT(EV_KEY, KEY_B, 0),
    INPUT_EVENT(EV_SYN, SYN_REPORT, 0),


    INPUT_EVENT(EV_MSC, MSC_SCAN, 42),
    INPUT_EVENT(EV_KEY, KEY_LEFTSHIFT, 1),
    INPUT_EVENT(EV_MSC, MSC_SCAN, 52),
    INPUT_EVENT(EV_KEY, KEY_DOT, 1),
    INPUT_EVENT(EV_SYN, SYN_REPORT, 0),

    INPUT_EVENT(EV_MSC, MSC_SCAN, 42),
    INPUT_EVENT(EV_KEY, KEY_LEFTSHIFT, 0),
    INPUT_EVENT(EV_MSC, MSC_SCAN, 52),
    INPUT_EVENT(EV_KEY, KEY_DOT, 0),
    INPUT_EVENT(EV_MSC, MSC_SCAN, 46),
    INPUT_EVENT(EV_KEY, KEY_C, 1),
    INPUT_EVENT(EV_SYN, SYN_REPORT, 0),

    INPUT_EVENT(EV_MSC, MSC_SCAN, 46),
    INPUT_EVENT(EV_KEY, KEY_C, 0),
    INPUT_EVENT(EV_SYN, SYN_REPORT, 0),

    INPUT_EVENT(EV_MSC, MSC_SCAN, 42),
    INPUT_EVENT(EV_KEY, KEY_LEFTSHIFT, 1),
    INPUT_EVENT(EV_MSC, MSC_SCAN, 53),
    INPUT_EVENT(EV_KEY, KEY_SLASH, 1),
    INPUT_EVENT(EV_SYN, SYN_REPORT, 0),

    INPUT_EVENT(EV_MSC, MSC_SCAN, 42),
    INPUT_EVENT(EV_KEY, KEY_LEFTSHIFT, 0),
    INPUT_EVENT(EV_MSC, MSC_SCAN, 53),
    INPUT_EVENT(EV_KEY, KEY_SLASH, 0),
    INPUT_EVENT(EV_MSC, MSC_SCAN, 32),
    INPUT_EVENT(EV_KEY, KEY_D, 1),
    INPUT_EVENT(EV_SYN, SYN_REPORT, 0),

    INPUT_EVENT(EV_MSC, MSC_SCAN, 32),
    INPUT_EVENT(EV_KEY, KEY_D, 0),
    INPUT_EVENT(EV_SYN, SYN_REPORT, 0),

    INPUT_EVENT(EV_MSC, MSC_SCAN, 42),
    INPUT_EVENT(EV_KEY, KEY_LEFTSHIFT, 1),
    INPUT_EVENT(EV_MSC, MSC_SCAN, 39),
    INPUT_EVENT(EV_KEY, KEY_SEMICOLON, 1),
    INPUT_EVENT(EV_SYN, SYN_REPORT, 0),

    INPUT_EVENT(EV_MSC, MSC_SCAN, 39),
    INPUT_EVENT(EV_KEY, KEY_SEMICOLON, 0),
    INPUT_EVENT(EV_MSC, MSC_SCAN, 42),
    INPUT_EVENT(EV_KEY, KEY_LEFTSHIFT, 0),
    INPUT_EVENT(EV_MSC, MSC_SCAN, 18),
    INPUT_EVENT(EV_KEY, KEY_E, 1),
    INPUT_EVENT(EV_SYN, SYN_REPORT, 0),

    INPUT_EVENT(EV_MSC, MSC_SCAN, 18),
    INPUT_EVENT(EV_KEY, KEY_E, 0),
    INPUT_EVENT(EV_SYN, SYN_REPORT, 0),

    INPUT_EVENT(EV_MSC, MSC_SCAN, 42),
    INPUT_EVENT(EV_KEY, KEY_LEFTSHIFT, 1),
    INPUT_EVENT(EV_MSC, MSC_SCAN, 40),
    INPUT_EVENT(EV_KEY, KEY_APOSTROPHE, 1),
    INPUT_EVENT(EV_SYN, SYN_REPORT, 0),


    INPUT_EVENT(EV_MSC, MSC_SCAN, 42),
    INPUT_EVENT(EV_KEY, KEY_LEFTSHIFT, 0),
    INPUT_EVENT(EV_MSC, MSC_SCAN, 40),
    INPUT_EVENT(EV_KEY, KEY_APOSTROPHE, 0),
    INPUT_EVENT(EV_MSC, MSC_SCAN, 33),
    INPUT_EVENT(EV_KEY, KEY_F, 1),
    INPUT_EVENT(EV_SYN, SYN_REPORT, 0),

    INPUT_EVENT(EV_MSC, MSC_SCAN, 33),
    INPUT_EVENT(EV_KEY, KEY_F, 0),
    INPUT_EVENT(EV_SYN, SYN_REPORT, 0),

    INPUT_EVENT(EV_MSC, MSC_SCAN, 42),
    INPUT_EVENT(EV_KEY, KEY_LEFTSHIFT, 1),
    INPUT_EVENT(EV_MSC, MSC_SCAN, 26),
    INPUT_EVENT(EV_KEY, KEY_LEFTBRACE, 1),
    INPUT_EVENT(EV_SYN, SYN_REPORT, 0),

    INPUT_EVENT(EV_MSC, MSC_SCAN, 42),
    INPUT_EVENT(EV_KEY, KEY_LEFTSHIFT, 0),
    INPUT_EVENT(EV_MSC, MSC_SCAN, 26),
    INPUT_EVENT(EV_KEY, KEY_LEFTBRACE, 0),
    INPUT_EVENT(EV_MSC, MSC_SCAN, 34),
    INPUT_EVENT(EV_KEY, KEY_G, 1),
    INPUT_EVENT(EV_SYN, SYN_REPORT, 0),

    INPUT_EVENT(EV_MSC, MSC_SCAN, 34),
    INPUT_EVENT(EV_KEY, KEY_G, 0),
    INPUT_EVENT(EV_SYN, SYN_REPORT, 0),

    INPUT_EVENT(EV_MSC, MSC_SCAN, 42),
    INPUT_EVENT(EV_KEY, KEY_LEFTSHIFT, 1),
    INPUT_EVENT(EV_MSC, MSC_SCAN, 43),
    INPUT_EVENT(EV_KEY, KEY_BACKSLASH, 1),
    INPUT_EVENT(EV_SYN, SYN_REPORT, 0),

    INPUT_EVENT(EV_MSC, MSC_SCAN, 42),
    INPUT_EVENT(EV_KEY, KEY_LEFTSHIFT, 0),
    INPUT_EVENT(EV_MSC, MSC_SCAN, 43),
    INPUT_EVENT(EV_KEY, KEY_BACKSLASH, 0),
    INPUT_EVENT(EV_SYN, SYN_REPORT, 0),
};

static gchar *m_session_name;
static IBusBus *m_bus;
static IBusEngine *m_engine;
static GMainLoop *m_loop;
static char *m_engine_is_focused;
static struct uinput_replay_device *m_replay;

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


static int
quit (gpointer userdata)
{
    if (userdata) {
        g_info (".... exiting");
        g_main_loop_quit (userdata);
    }
    return G_SOURCE_REMOVE;
}


static gboolean
quit_on_poke (GIOChannel  *source,
              GIOCondition condition,
              gpointer     data)
{
    return quit (data);
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

static int
register_ibus_engine (gpointer unused)
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


static gboolean
destroy_window (gpointer user_data)
{
    WindowDestroyData *data = user_data;

    data->source = 0;

    g_info ("Destroying window after timeout");
    gtk_window_destroy (GTK_WINDOW (data->window));

    data->window = NULL;

    return G_SOURCE_REMOVE;
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

    if (!ibus_bus_set_global_engine_async_finish (bus, res, &error)) {
        g_critical ("set engine failed: %s", error->message);
        g_error_free (error);
        return;
    }

    /* FIXME: this bit is now effectively useless, the timeout isn't called
     * for at least 1 second but by then we've probably finished replaying
     * our uinput device and moved to TEST_PROCESS_KEY_EVENT */

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

    /* Because uinput doesn't take that long (for our recordings anyway) we can
     * simply create and run this here this here. */
    g_info("running uinput now");
    for (size_t i = 0; i < G_N_ELEMENTS(test_data); i++) {
        uinput_replay_device_event(m_replay, &test_data[i]);
        if (test_data[i].type == EV_SYN)
            msleep(2);
    }
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
        id = g_timeout_add_seconds (1,
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
    } else {
        g_print (GREEN "PASS" NC " ");
        for (k = 0; k < j; k++)
            g_print ("%lc(%X) ", test_results[i][k], test_results[i][k]);
        g_print ("\n");
    }

    ++i;
    j = 0;
#if GTK_CHECK_VERSION (4, 0, 0)
    gtk_editable_set_text (GTK_EDITABLE (entry), "");
#else
    gtk_entry_set_text (GTK_ENTRY (entry), "");
#endif
    if (!test_results[i][j]) {
       g_assert (!j);

       const char buf[] = "BYE\n";
       write(pipefds[1], buf, sizeof(buf));
       fsync(pipefds[1]);
       close(pipefds[1]);


       ibus_quit ();
    }
}


static GtkWidget *
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

    return window;
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
    WindowDestroyData destroy_data;

    int rc = pipe (pipefds);
    g_asser t(rc == 0);

    pid_t pid = fork();
    if (pid == 0) {
        GMainContext *context;

        close (pipefds[1]);

        context = g_main_context_default ();
        m_loop = g_main_loop_new (context, FALSE);

        GIOChannel *channel =  g_io_channel_unix_new (pipefds[0]);
        g_io_add_watch (channel, G_IO_IN|G_IO_ERR, quit_on_poke, m_loop);
        g_idle_add (register_ibus_engine, NULL);
        g_timeout_add_seconds (10, quit, m_loop);

        g_main_loop_run (m_loop);
        g_object_unref (c);

        exit (rc ? EXIT_FAILURE : EXIT_SUCCESS);
    }

    /* FIXME: Calling this before the fork means we never get an ibus_bus_new().
     * let's move this here for now.
     */
#if GTK_CHECK_VERSION (4, 0, 0)
    gtk_init ();
#else
    gtk_init (0, NULL);;
#endif

    close(pipefds[0]);

    m_bus = ibus_bus_new ();
    m_replay = uinput_replay_create_keyboard (NULL);

    if (!m_replay) {
        g_warning ("Failed to create uinput device");
        return;
    }

    destroy_data.window = create_window ();
    destroy_data.source = g_timeout_add_seconds (5, destroy_window, &destroy_data);

    ibus_main ();

    uinput_replay_device_destroy (g_steal_pointer (&m_replay));
    g_clear_pointer (&m_session_name, g_free);
    if (destroy_data.source)
	g_source_remove (destroy_data.source);
    if (destroy_data.window)
	gtk_window_destroy (GTK_WINDOW (destroy_data.window));
}


static void
test_keypress_from_recording (void)
{
#if 0
    GtkWidget *window;
    const char *recording = getenv("IBUS_KEY_RECORDING");

    if (!register_ibus_engine ())
        return;

    g_return_if_fail(recording);

    m_replay = uinput_replay_create_device(recording, NULL);
    if (!m_replay) {
        g_warning ("Failed to create uinput device");
        return;
    }

    window = create_window ();
    ibus_main ();

    uinput_replay_device_destroy(g_steal_pointer (&m_replay));
    g_clear_pointer (&m_session_name, g_free);
    gtk_window_destroy (GTK_WINDOW (window));
#endif
}


int
main (int argc, char *argv[])
{
    setlocale (LC_ALL, "");

    /* Avoid a warning of "AT-SPI: Could not obtain desktop path or name"
     * with gtk_main().
     */
    if (!g_setenv ("NO_AT_BRIDGE", "1", TRUE))
        g_message ("Failed setenv NO_AT_BRIDGE\n");
    g_test_init (&argc, &argv, NULL);

    ibus_init ();
    //g_test_add_func ("/ibus-keypress/test-init", test_init);
    //m_loop = g_main_loop_new (NULL, TRUE);
    if (getenv("IBUS_KEY_RECORDING"))
        g_test_add_func ("/ibus-keypress/keypress_from_recording", test_keypress_from_recording);
    else
        g_test_add_func ("/ibus-keypress/keypress", test_keypress);

    return g_test_run ();
}
