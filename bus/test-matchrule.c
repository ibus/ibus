/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include "matchrule.h"
#include "connection.h"

struct _BusMatchRule {
    IBusObject parent;
    /* instance members */
    gint   flags;
    gint   message_type;
    gchar *interface;
    gchar *member;
    gchar *sender;
    gchar *destination;
    gchar *path;
    GArray *args;
    GList *recipients;
};

static void
test (void)
{
    BusMatchRule *rule, *rule1;

    rule = bus_match_rule_new (" type='signal' ,"
                               " interface = 'org.freedesktop.IBus' ");
    g_assert (rule->message_type == G_DBUS_MESSAGE_TYPE_SIGNAL);
    g_assert (g_strcmp0 (rule->interface, "org.freedesktop.IBus") == 0 );
    g_object_unref (rule);

    rule = bus_match_rule_new ("type='method_call'    ,\n"
        "    interface='org.freedesktop.IBus'   ");
    g_assert (rule->message_type == G_DBUS_MESSAGE_TYPE_METHOD_CALL);
    g_assert (g_strcmp0 (rule->interface, "org.freedesktop.IBus") == 0 );
    g_object_unref (rule);

    rule = bus_match_rule_new ("type='signal',"
                               "interface='org.freedesktop.DBus',"
                               "member='NameOwnerChanged',"
                               "arg0='ibus.freedesktop.IBus.config',"
                               "arg0='ibus.freedesktop.IBus.config',"
                               "arg2='ibus.freedesktop.IBus.config'");
    g_assert (rule->message_type == G_DBUS_MESSAGE_TYPE_SIGNAL);
    g_assert (g_strcmp0 (rule->interface, "org.freedesktop.DBus") == 0 );
    rule1 = bus_match_rule_new ("type='signal',"
                               "interface='org.freedesktop.DBus',"
                               "member='NameOwnerChanged',"
                               "arg0='ibus.freedesktop.IBus.config',"
                               "arg0='ibus.freedesktop.IBus.config',"
                               "arg2='ibus.freedesktop.IBus.config'");

    g_assert (bus_match_rule_is_equal (rule, rule1));

    g_object_unref (rule);
    g_object_unref (rule1);

    rule = bus_match_rule_new ("type='method_call',"
                               "interface='org.freedesktop.IBus ");
    g_assert (rule == NULL);

    rule = bus_match_rule_new ("eavesdrop=true");
    g_assert (rule != NULL);
    g_object_unref (rule);
}

static void
test_connection_destroy (void)
{
    BusMatchRule *rule;
    BusConnection *connection1;
    BusConnection *connection2;

    rule = bus_match_rule_new ("type='signal'");
    connection1 = BUS_CONNECTION (g_object_new (BUS_TYPE_CONNECTION, NULL));
    connection2 = BUS_CONNECTION (g_object_new (BUS_TYPE_CONNECTION, NULL));

    bus_match_rule_add_recipient (rule, connection1);
    bus_match_rule_add_recipient (rule, connection2);
    g_assert_cmpuint (g_list_length (rule->recipients), ==, 2);

    ibus_object_destroy (IBUS_OBJECT (connection1));
    g_assert_false (IBUS_OBJECT_DESTROYED (rule));
    g_assert_cmpuint (g_list_length (rule->recipients), ==, 1);

    ibus_object_destroy (IBUS_OBJECT (connection2));
    g_assert_true (IBUS_OBJECT_DESTROYED (rule));
    g_assert_null (rule->recipients);

    g_object_unref (connection1);
    g_object_unref (connection2);
    g_object_unref (rule);
}

int
main (int argc, char *argv[])
{
    g_test_init (&argc, &argv, NULL);
#if !GLIB_CHECK_VERSION(2,35,0)
    g_type_init ();
#endif
    g_test_add_func ("/test-matchrule", test);
    g_test_add_func ("/test-matchrule/connection-destroy", test_connection_destroy);
    return g_test_run ();
}
