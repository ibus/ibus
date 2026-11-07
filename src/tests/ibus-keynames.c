#include "ibus.h"

static void
test_keyname (void)
{
    g_assert_cmpstr (ibus_keyval_name (IBUS_KEY_Home), ==, "Home");
    g_assert (ibus_keyval_from_name ("Home") == IBUS_KEY_Home);
}

static void
test_key_event_from_string (void)
{
    guint keyval, modifiers;

    g_assert_true (ibus_key_event_from_string ("Control+a",
                                               &keyval, &modifiers));
    g_assert_cmpuint (keyval, ==, IBUS_KEY_a);
    g_assert_cmpuint (modifiers, ==, IBUS_CONTROL_MASK);

    g_assert_true (ibus_key_event_from_string ("a", &keyval, &modifiers));
    g_assert_cmpuint (keyval, ==, IBUS_KEY_a);
    g_assert_cmpuint (modifiers, ==, 0);
}

static void
test_key_event_from_string_empty (void)
{
    guint keyval, modifiers;

    g_assert_false (ibus_key_event_from_string ("",
                                                &keyval, &modifiers));
}

gint
main (gint    argc,
      gchar **argv)
{
    ibus_init ();

    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/ibus/keyname", test_keyname);
    g_test_add_func ("/ibus/key-event-from-string",
                     test_key_event_from_string);
    g_test_add_func ("/ibus/key-event-from-string-empty",
                     test_key_event_from_string_empty);

    return g_test_run ();
}
