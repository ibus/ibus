#include <gtk/gtk.h>
#include "ibus.h"
#include "ibuscomposetable.h"
#include "ibusenginesimpleprivate.h"

#define GREEN "\033[0;32m"
#define RED   "\033[0;31m"
#define NC    "\033[0m"

IBusBus *m_bus;
gchar *m_compose_file;
IBusComposeTableEx *m_compose_table;
IBusEngine *m_engine;
gchar *m_srcdir;

static gboolean window_focus_in_event_cb (GtkWidget     *entry,
                                          GdkEventFocus *event,
                                          gpointer       data);


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


static IBusEngine *
create_engine_cb (IBusFactory *factory,
                  const gchar *name,
                  gpointer     data)
{
    static int i = 1;
    gchar *engine_path =
            g_strdup_printf ("/org/freedesktop/IBus/engine/simpletest/%d",
                             i++);
    gchar *compose_path;

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
        if (m_compose_table == NULL)
            g_warning ("Your locale uses en_US compose table.");
    }
    g_free (compose_path);
    return m_engine;
}

static gboolean
register_ibus_engine ()
{
    IBusFactory *factory;
    IBusComponent *component;
    IBusEngineDesc *desc;

    m_bus = ibus_bus_new ();
    if (!ibus_bus_is_connected (m_bus)) {
        g_critical ("ibus-daemon is not running.");
        return FALSE;
    }
    factory = ibus_factory_new (ibus_bus_get_connection (m_bus));
    g_signal_connect (factory, "create-engine",
                      G_CALLBACK (create_engine_cb), NULL);

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

static gboolean
finit (gpointer data)
{
    g_test_incomplete ("time out");
    gtk_main_quit ();
    return FALSE;
}

static void
set_engine_cb (GObject *object, GAsyncResult *res, gpointer data)
{
    IBusBus *bus = IBUS_BUS (object);
    GtkWidget *entry = GTK_WIDGET (data);
    GError *error = NULL;
    int i, j;
    int index_stride;
    IBusComposeTablePrivate *priv;

    if (!ibus_bus_set_global_engine_async_finish (bus, res, &error)) {
        gchar *msg = g_strdup_printf ("set engine failed: %s", error->message);
        g_test_incomplete (msg);
        g_free (msg);
        g_error_free (error);
        return;
    }

    if (m_compose_table == NULL) {
        gtk_main_quit ();
        return;
    }

    index_stride = m_compose_table->max_seq_len + 2;
    for (i = 0;
         i < (m_compose_table->n_seqs * index_stride);
         i += index_stride) {
        for (j = i; j < i + (index_stride - 2); j++) {
            guint keyval = m_compose_table->data[j];
            guint keycode = 0;
            guint modifiers = 0;
            gboolean retval;

            if (keyval == 0)
                break;
            g_signal_emit_by_name (m_engine, "process-key-event",
                                   keyval, keycode, modifiers, &retval);
            modifiers |= IBUS_RELEASE_MASK;
            g_signal_emit_by_name (m_engine, "process-key-event",
                                   keyval, keycode, modifiers, &retval);
        }
    }
    priv = m_compose_table->priv;
    if (priv) {
        for (i = 0;
             i < (priv->first_n_seqs * index_stride);
             i += index_stride) {
            for (j = i; j < i + (index_stride - 2); j++) {
                guint keyval = priv->data_first[j];
                guint keycode = 0;
                guint modifiers = 0;
                gboolean retval;

                if (keyval == 0)
                    break;
                g_signal_emit_by_name (m_engine, "process-key-event",
                                       keyval, keycode, modifiers, &retval);
                modifiers |= IBUS_RELEASE_MASK;
                g_signal_emit_by_name (m_engine, "process-key-event",
                                       keyval, keycode, modifiers, &retval);
            }
        }
    }

    g_signal_handlers_disconnect_by_func (entry, 
                                          G_CALLBACK (window_focus_in_event_cb),
                                          NULL);
    g_timeout_add_seconds (10, finit, NULL);
}

static gboolean
window_focus_in_event_cb (GtkWidget *entry, GdkEventFocus *event, gpointer data)
{
    g_assert (m_bus != NULL);
    ibus_bus_set_global_engine_async (m_bus,
                                      "xkbtest:us::eng",
                                      -1,
                                      NULL,
                                      set_engine_cb,
                                      entry);
    return FALSE;
}

static void
window_inserted_text_cb (GtkEntryBuffer *buffer,
                         guint           position,
                         const gchar    *chars,
                         guint           nchars,
                         gpointer        data)
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
    GtkEntry *entry = GTK_ENTRY (data);
    IBusComposeTablePrivate *priv;

    g_assert (m_compose_table != NULL);

    priv = m_compose_table->priv;

#if !GTK_CHECK_VERSION (3, 22, 16)
    if (n_loop % 2 == 1) {
        n_loop = 0;
        return;
    }
#endif
    i = stride + (m_compose_table->max_seq_len + 2) - 2;
    seq = (i + 2) / (m_compose_table->max_seq_len + 2);
    if (!enable_32bit) {
        if (m_compose_table->data[i] == code) {
            test = GREEN "PASS" NC;
        } else {
            test = RED "FAIL" NC;
            g_test_fail ();
        }
        g_print ("%05d/%05d %s expected: %04X typed: %04X\n",
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
        g_print ("%05d/%05ld %s expected: %04X[%d] typed: %04X\n",
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
        } else {
            gtk_main_quit ();
            return;
        }
    }
    if (enable_32bit && seq == priv->first_n_seqs) {
        gtk_main_quit ();
        return;
    }

#if !GTK_CHECK_VERSION (3, 22, 16)
    n_loop++;
#endif
    gtk_entry_set_text (entry, "");
}

static void
create_window ()
{
    GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    GtkWidget *entry = gtk_entry_new ();
    GtkEntryBuffer *buffer;

    g_signal_connect (window, "destroy",
                      G_CALLBACK (gtk_main_quit), NULL);
    g_signal_connect (entry, "focus-in-event",
                      G_CALLBACK (window_focus_in_event_cb), NULL);
    buffer = gtk_entry_get_buffer (GTK_ENTRY (entry));
    g_signal_connect (buffer, "inserted-text",
                      G_CALLBACK (window_inserted_text_cb), entry);
    gtk_container_add (GTK_CONTAINER (window), entry);
    gtk_widget_show_all (window);
}

static void
test_compose (void)
{
    GLogLevelFlags flags;
    if (!register_ibus_engine ()) {
        g_test_fail ();
        return;
    }

    create_window ();
    /* FIXME:
     * IBusIMContext opens GtkIMContextSimple as the slave and
     * GtkIMContextSimple opens the compose table on el_GR.UTF-8, and the
     * multiple outputs in el_GR's compose causes a warning in gtkcomposetable
     * and the warning always causes a fatal in GTest:
     " "GTK+ supports to output one char only: "
     */
    flags = g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL);
    gtk_main ();
    g_log_set_always_fatal (flags);
}

int
main (int argc, char *argv[])
{
    const gchar *test_name;
    gchar *test_path;

    ibus_init ();
    /* Avoid a warning of "AT-SPI: Could not obtain desktop path or name"
     * with gtk_main().
     */
    if (!g_setenv ("NO_AT_BRIDGE", "1", TRUE))
        g_message ("Failed setenv NO_AT_BRIDGE\n");
    g_test_init (&argc, &argv, NULL);
    gtk_init (&argc, &argv);

    m_srcdir = (argc > 1 && strlen (argv[1]) < FILENAME_MAX)
            ? g_strdup (argv[1]) : g_strdup (".");
    m_compose_file = g_strdup (g_getenv ("COMPOSE_FILE"));
#if GLIB_CHECK_VERSION (2, 58, 0)
    test_name = g_get_language_names_with_category ("LC_CTYPE")[0];
#else
    test_name = g_getenv ("LANG");
#endif
    test_path = g_build_filename ("/ibus-compose", test_name, NULL);
    g_test_add_func (test_path, test_compose);
    g_free (test_path);

    return g_test_run ();
}
