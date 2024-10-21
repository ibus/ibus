#include <ibus.h>
#include <dbus/dbus.h>

static void
test_machine_id (void)
{
    DBusError error = DBUS_ERROR_INIT;
    const gchar *s1 = ibus_get_local_machine_id ();
    gchar *s2 = dbus_try_get_local_machine_id (&error);

    g_assert (s1);
    if (!s2) {
        g_warning ("Failed to get locale machine id: %s: %s",
                   error.name, error.message);
        dbus_error_free (&error);
        g_assert_not_reached ();
    } else {
        g_assert_cmpstr (s1, ==, s2);
    }
    dbus_free (s2);
}

gint
main (gint    argc,
      gchar **argv)
{
    ibus_init ();

    g_test_init (&argc, &argv, NULL);
    g_test_add_func ("/ibus/marchine-id", test_machine_id);

    return g_test_run ();
}
