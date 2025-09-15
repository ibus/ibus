/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* IBus - The Input Bus
 * Copyright (C) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2024-2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2008-2024 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#include "matchrule.h"

#include <string.h>

#include "dbusimpl.h"

typedef enum {
    MATCH_TYPE          = 1 << 0,
    MATCH_INTERFACE     = 1 << 1,
    MATCH_MEMBER        = 1 << 2,
    MATCH_SENDER        = 1 << 3,
    MATCH_DESTINATION   = 1 << 4,
    MATCH_PATH          = 1 << 5,
    MATCH_ARGS          = 1 << 6,
} BusMatchFlags;

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

struct _BusMatchRuleClass {
    IBusObjectClass parent;
    /* class members */
};

typedef struct _BusRecipient BusRecipient;
struct _BusRecipient {
    BusConnection *connection;
    gint refcount;
};

static BusRecipient *bus_recipient_new          (BusConnection      *connection);
static void          bus_recipient_free         (BusRecipient       *recipient);
static BusRecipient *bus_recipient_ref          (BusRecipient       *recipient);
static gboolean      bus_recipient_unref        (BusRecipient       *recipient);
static void          bus_match_rule_destroy     (BusMatchRule       *rule);
static void          bus_match_rule_connection_destroy_cb
                                                (BusConnection      *connection,
                                                 BusMatchRule       *rule);

static BusRecipient *
bus_recipient_new (BusConnection *connection)
{
    BusRecipient *recipient = g_slice_new (BusRecipient);
    g_object_ref (connection);
    recipient->connection = connection;
    recipient->refcount = 1;
    return recipient;
}

static void
bus_recipient_free (BusRecipient *recipient)
{
    g_object_unref (recipient->connection);
    g_slice_free (BusRecipient, recipient);
}

static BusRecipient *
bus_recipient_ref (BusRecipient *recipient)
{
    recipient->refcount ++;
    return recipient;
}

static gboolean
bus_recipient_unref (BusRecipient *recipient)
{
    recipient->refcount --;
    if (recipient->refcount == 0) {
        bus_recipient_free (recipient);
        return TRUE;
    }
    return FALSE;
}

G_DEFINE_TYPE (BusMatchRule, bus_match_rule, IBUS_TYPE_OBJECT)

static void
bus_match_rule_class_init (BusMatchRuleClass *class)
{
    IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (class);

    ibus_object_class->destroy = (IBusObjectDestroyFunc) bus_match_rule_destroy;
}

static void
bus_match_rule_init (BusMatchRule *rule)
{
    rule->flags = 0;
    rule->message_type = G_DBUS_MESSAGE_TYPE_INVALID;
    rule->interface = NULL;
    rule->member = NULL;
    rule->sender = NULL;
    rule->destination = NULL;
    rule->path = NULL;
    rule->args = g_array_new (TRUE, TRUE, sizeof (gchar *));
}

static void
bus_match_rule_destroy (BusMatchRule *rule)
{
    g_free (rule->interface);
    g_free (rule->member);
    g_free (rule->sender);
    g_free (rule->destination);
    g_free (rule->path);

    gint i;
    for (i = 0; i < rule->args->len; i++) {
        g_free (g_array_index (rule->args, gchar *, i));
    }
    g_array_free (rule->args, TRUE);

    GList *p;
    for (p = rule->recipients; p != NULL; p = p->next) {
        BusRecipient *recipient = (BusRecipient *) p->data;
        g_signal_handlers_disconnect_by_func (recipient->connection,
                                              G_CALLBACK (bus_match_rule_connection_destroy_cb), rule);
        bus_recipient_free (recipient);
    }
    g_list_free (rule->recipients);

    IBUS_OBJECT_CLASS(bus_match_rule_parent_class)->destroy (IBUS_OBJECT (rule));
}


typedef struct _Token {
    gchar *key;
    gchar *value;
} Token;

#define SKIP_WHITE(a)   \
    while (*(a) == ' ' || *(a) == '\t' || *(a) == '\n') { (a)++; }
#define IS_ALPHA(a) \
    ((*(a) >= 'a' && *(a) <= 'z') || (*(a) >= 'A' && *(a) <= 'Z'))
#define IS_NUMBER(a) \
    (*(a) >= '0' && *(a) <= '9')

static gchar *
find_key (const gchar **p)
{
    GString *text;

    text = g_string_new ("");

    SKIP_WHITE(*p)
    if (!IS_ALPHA (*p))
        goto failed;

    g_string_append_c (text, **p);
    (*p) ++;

    while (IS_ALPHA (*p) || IS_NUMBER (*p)) {
        g_string_append_c (text, **p);
        (*p) ++;
    }

    return g_string_free (text, FALSE);

failed:
    g_string_free (text, TRUE);
    return NULL;

}

static gchar *
find_value (const gchar **p)
{
    SKIP_WHITE (*p);

    if (**p == '\'') {
        GString *text = g_string_new ("");
        (*p) ++;
        while (**p != '\'') {
            if (**p == '\0') {
                g_string_free (text, TRUE);
                return NULL;
            }
            if (**p == '\\')
                (*p) ++;
            g_string_append_c (text, **p);
            (*p) ++;
        }
        (*p) ++;
        return g_string_free (text, FALSE);
    } else if (strncmp (*p, "true", 4) == 0) {
        *p += 4;
        return g_strdup ("true");
    } else if (strncmp (*p, "false", 5) == 0) {
        *p += 5;
        return g_strdup ("false");
    }

    return NULL;
}

static Token *
tokenize_rule (const gchar *text)
{
    GArray *tokens;
    Token token;
    const gchar *p;
    gint i;

    tokens = g_array_new (TRUE, TRUE, sizeof (Token));

    p = text;

    while (*p != '\0') {
        gchar *key;
        gchar *value;

        SKIP_WHITE (p);
        key = find_key (&p);
        if (key == NULL)
            goto failed;
        SKIP_WHITE (p);
        if (*p != '=')
            goto failed;
        p ++;
        SKIP_WHITE (p);
        value = find_value (&p);
        if (value == NULL) {
            g_free (key);
            goto failed;
        }
        SKIP_WHITE (p);
        if (*p != ',' && *p != '\0') {
            g_free (key);
            g_free (value);
            goto failed;
        }

        if (*p == ',')
         p ++;
        token.key = key;
        token.value = value;
        g_array_append_val (tokens, token);
    }

    return (Token *)g_array_free (tokens, FALSE);

failed:
    for (i = 0; i < tokens->len; i++) {
        Token *p = &g_array_index (tokens, Token, i);
        g_free (p->key);
        g_free (p->value);
    }

    g_array_free (tokens, TRUE);
    return NULL;
}

static void
tokens_free (Token *tokens)
{
    Token *p;
    p = tokens;

    while (p != NULL && p->key != NULL) {
        g_free (p->key);
        g_free (p->value);
        p ++;
    }
    g_free (tokens);
}

static gboolean
_atoi (const gchar *text, gint *i)
{
    const gchar *p = text;
    *i = 0;
    while (*p != '\0') {
        if (!IS_NUMBER(p))
            return FALSE;
        *i = (*i) * 10 - '0'  + *p;
        p ++;
    }
    return TRUE;
}

BusMatchRule *
bus_match_rule_new (const gchar *text)
{
    g_assert (text != NULL);

    Token *tokens, *p;
    BusMatchRule *rule;

    rule = BUS_MATCH_RULE (g_object_new (BUS_TYPE_MATCH_RULE, NULL));

    /* parse rule */
    tokens = tokenize_rule (text);

    if (tokens == NULL)
        goto failed;

    for (p = tokens; p != NULL && p->key != 0; p++) {
        if (g_strcmp0 (p->key, "type") == 0) {
            if (g_strcmp0 (p->value, "signal") == 0) {
                bus_match_rule_set_message_type (rule, G_DBUS_MESSAGE_TYPE_SIGNAL);
            }
            else if (g_strcmp0 (p->value, "method_call") == 0) {
                bus_match_rule_set_message_type (rule, G_DBUS_MESSAGE_TYPE_METHOD_CALL);
            }
            else if (g_strcmp0 (p->value, "method_return") == 0) {
                bus_match_rule_set_message_type (rule, G_DBUS_MESSAGE_TYPE_METHOD_RETURN);
            }
            else if (g_strcmp0 (p->value, "error") == 0) {
                bus_match_rule_set_message_type (rule, G_DBUS_MESSAGE_TYPE_ERROR);
            }
            else
                goto failed;
        }
        else if (g_strcmp0 (p->key, "sender") == 0) {
            if (!g_dbus_is_name (p->value))
                goto failed;
            bus_match_rule_set_sender (rule, p->value);
        }
        else if (g_strcmp0 (p->key, "interface") == 0) {
            if (!g_dbus_is_interface_name (p->value))
                goto failed;
            bus_match_rule_set_interface (rule, p->value);
        }
        else if (g_strcmp0 (p->key, "member") == 0) {
            if (!g_dbus_is_member_name (p->value))
                goto failed;
            bus_match_rule_set_member (rule, p->value);
        }
        else if (g_strcmp0 (p->key, "path") == 0) {
            bus_match_rule_set_path (rule, p->value);
        }
        else if (g_strcmp0 (p->key, "destination") == 0) {
            if (!g_dbus_is_name (p->value))
                goto failed;
            bus_match_rule_set_destination (rule, p->value);
        }
        else if (strncmp (p->key, "arg", 3) == 0) {
            gint i;
            if (! _atoi (p->key + 3, &i))
                goto failed;
            bus_match_rule_set_arg (rule, i, p->value);
        }
        else if (g_strcmp0 (p->key, "eavesdrop") == 0) {
            if (g_strcmp0 (p->value, "true") != 0 &&
                g_strcmp0 (p->value, "false") != 0)
                goto failed;
        }
        else
            goto failed;
    }

    tokens_free (tokens);
    return rule;

failed:
    tokens_free (tokens);
    g_object_unref (rule);
    return NULL;
}

gboolean
bus_match_rule_set_message_type (BusMatchRule   *rule,
                                 gint            type)
{
    g_assert (rule != NULL);
    g_assert (type == G_DBUS_MESSAGE_TYPE_SIGNAL ||
              type == G_DBUS_MESSAGE_TYPE_METHOD_CALL ||
              type == G_DBUS_MESSAGE_TYPE_METHOD_RETURN ||
              type == G_DBUS_MESSAGE_TYPE_ERROR);

    rule->flags |= MATCH_TYPE;
    rule->message_type = type;

    return TRUE;
}

const gchar *
bus_match_rule_get_sender  (BusMatchRule    *rule)
{
    g_return_val_if_fail (rule != NULL, NULL);
    return rule->sender;
}

gboolean
bus_match_rule_set_sender  (BusMatchRule    *rule,
                            const gchar     *sender)
{
    g_assert (rule != NULL);
    g_assert (sender != NULL);

    rule->flags |= MATCH_SENDER;

    g_free (rule->sender);
    rule->sender = g_strdup (sender);

    return TRUE;
}

gboolean
bus_match_rule_set_interface (BusMatchRule   *rule,
                              const gchar    *interface)
{
    g_assert (rule != NULL);
    g_assert (interface != NULL);

    rule->flags |= MATCH_INTERFACE;

    g_free (rule->interface);
    rule->interface = g_strdup (interface);
    return TRUE;
}

gboolean
bus_match_rule_set_member (BusMatchRule   *rule,
                           const gchar    *member)
{
    g_assert (rule != NULL);
    g_assert (member != NULL);

    rule->flags |= MATCH_MEMBER;

    g_free (rule->member);
    rule->member = g_strdup (member);

    return TRUE;
}

gboolean
bus_match_rule_set_path (BusMatchRule   *rule,
                         const gchar    *path)
{
    g_assert (rule != NULL);
    g_assert (path != NULL);

    rule->flags |= MATCH_PATH;

    g_free (rule->path);
    rule->path = g_strdup (path);

    return TRUE;
}

gboolean
bus_match_rule_set_destination (BusMatchRule   *rule,
                                const gchar    *dest)
{
    g_assert (rule != NULL);
    g_assert (dest != NULL);

    rule->flags |= MATCH_DESTINATION;

    g_free (rule->destination);
    rule->destination = g_strdup (dest);

    return TRUE;
}

gboolean
bus_match_rule_set_arg (BusMatchRule   *rule,
                        guint           arg_i,
                        const gchar    *arg)
{
    g_assert (rule != NULL);
    g_assert (arg != NULL);

    rule->flags |= MATCH_ARGS;

    if (arg_i >= rule->args->len) {
        g_array_set_size (rule->args, arg_i + 1);
    }

    g_free (g_array_index (rule->args, gchar *, arg_i));
    g_array_index (rule->args, gchar *, arg_i) = g_strdup (arg);
    return TRUE;
}

static gboolean
bus_match_rule_match_name (const gchar *name,
                           const gchar *match_name)
{
    if (g_dbus_is_unique_name (name) && !g_dbus_is_unique_name (match_name)) {
        BusConnection *connection =
                bus_dbus_impl_get_connection_by_name (BUS_DEFAULT_DBUS, match_name);
        if (connection == NULL)
            return FALSE;
        return g_strcmp0 (name, bus_connection_get_unique_name (connection)) == 0;
    }
    return g_strcmp0 (name, match_name) == 0;
}

gboolean
bus_match_rule_match (BusMatchRule   *rule,
                      GDBusMessage   *message)
{
    g_assert (rule != NULL);
    g_assert (message != NULL);

    if (rule->flags & MATCH_TYPE) {
        if (g_dbus_message_get_message_type (message) != rule->message_type)
            return FALSE;
    }

    if (rule->flags & MATCH_INTERFACE) {
        if (g_strcmp0 (g_dbus_message_get_interface (message), rule->interface) != 0)
            return FALSE;
    }

    if (rule->flags & MATCH_MEMBER) {
        if (g_strcmp0 (g_dbus_message_get_member (message), rule->member) != 0)
            return FALSE;
    }

    if (rule->flags & MATCH_SENDER) {
        if (!bus_match_rule_match_name (g_dbus_message_get_sender (message), rule->sender))
            return FALSE;
    }

    if (rule->flags & MATCH_DESTINATION) {
        if (!bus_match_rule_match_name (g_dbus_message_get_destination (message), rule->destination))
            return FALSE;
    }

    if (rule->flags & MATCH_PATH) {
        if (g_strcmp0 (g_dbus_message_get_path (message), rule->path) != 0)
            return FALSE;
    }

    if (rule->flags & MATCH_ARGS) {
        guint i;
        GVariant *arguments = g_dbus_message_get_body (message);
        if (arguments == NULL)
            return FALSE;

        for (i = 0; i < rule->args->len; i++) {
            const gchar *arg = g_array_index (rule->args, const gchar *, i);
            if (arg == NULL)
                continue;
            GVariant * variant = g_variant_get_child_value (arguments, i);
            if (variant == NULL)
                return FALSE;
            switch (g_variant_classify (variant)) {
            case G_VARIANT_CLASS_STRING:
            case G_VARIANT_CLASS_OBJECT_PATH:
                if (g_strcmp0 (arg, g_variant_get_string (variant, NULL)) == 0) {
                    g_variant_unref (variant);
                    continue;
                }
            default:
                break;
            }
            g_variant_unref (variant);
            return FALSE;
        }
    }
    return TRUE;
}

gboolean
bus_match_rule_is_equal (BusMatchRule   *a,
                         BusMatchRule   *b)
{
    g_assert (a != NULL);
    g_assert (b != NULL);

    if (a->flags != b->flags)
        return FALSE;

    if (a->flags & MATCH_TYPE) {
        if (a->message_type != b->message_type)
            return FALSE;
    }

    if (a->flags & MATCH_INTERFACE) {
        if (g_strcmp0 (a->interface, b->interface) != 0)
            return FALSE;
    }

    if (a->flags & MATCH_MEMBER) {
        if (g_strcmp0 (a->member, b->member) != 0)
            return FALSE;
    }

    if (a->flags & MATCH_SENDER) {
        if (g_strcmp0 (a->sender, b->sender) != 0)
            return FALSE;
    }

    if (a->flags & MATCH_DESTINATION) {
        if (g_strcmp0 (a->destination, b->destination) != 0)
            return FALSE;
    }

    if (a->flags & MATCH_PATH) {
        if (g_strcmp0 (a->path, b->path) != 0)
            return FALSE;
    }

    if (a->flags & MATCH_ARGS) {
        if (a->args->len != b->args->len)
            return FALSE;

        gint i;

        for (i = 0; i < a->args->len; i++) {
            if (g_strcmp0 (g_array_index (a->args, gchar *, i),
                           g_array_index (b->args, gchar *, i)) != 0)
                return FALSE;
        }
    }

    return TRUE;
}

static void
bus_match_rule_connection_destroy_cb (BusConnection *connection,
                                      BusMatchRule  *rule)
{
    GList *p;
    for (p = rule->recipients; p != NULL; p = p->next) {
        BusRecipient *recipient = (BusRecipient *)p->data;

        if (recipient->connection == connection) {
            rule->recipients = g_list_delete_link (rule->recipients, p);
            bus_recipient_free (recipient);
            return;
        }

        if (rule->recipients == NULL) {
            ibus_object_destroy (IBUS_OBJECT (rule));
        }
    }
    g_assert_not_reached ();
}

void
bus_match_rule_add_recipient (BusMatchRule    *rule,
                              BusConnection   *connection)
{
    g_assert (BUS_IS_MATCH_RULE (rule));
    g_assert (BUS_IS_CONNECTION (connection));

    GList *p;
    for (p = rule->recipients; p != NULL; p = p->next) {
        BusRecipient *recipient = (BusRecipient *) p->data;
        if (connection == recipient->connection) {
            bus_recipient_ref (recipient);
            return;
        }
    }

    /* alloc a new recipient */
    BusRecipient *recipient = bus_recipient_new (connection);
    rule->recipients = g_list_append (rule->recipients, recipient);
    g_signal_connect (connection,
                      "destroy",
                      G_CALLBACK (bus_match_rule_connection_destroy_cb),
                      rule);
}

void
bus_match_rule_remove_recipient (BusMatchRule  *rule,
                                 BusConnection *connection)
{
    g_assert (BUS_IS_MATCH_RULE (rule));
    g_assert (BUS_IS_CONNECTION (connection));

    GList *p;
    for (p = rule->recipients; p != NULL; p = p->next) {
        BusRecipient *recipient = (BusRecipient *) p->data;
        if (connection == recipient->connection) {
            if (bus_recipient_unref (recipient)) {
                /* Use g_list_delete_link() instead of g_list_remove_link()
                 * to delete the node slice in g_list_append() in
                 * bus_match_rule_add_recipient().
                 */
                rule->recipients = g_list_delete_link (rule->recipients, p);
                g_signal_handlers_disconnect_by_func (
                        connection,
                        G_CALLBACK (bus_match_rule_connection_destroy_cb),
                        rule);
            }

            if (rule->recipients == NULL ) {
                ibus_object_destroy (IBUS_OBJECT(rule));
            }
            return;
        }
    }
    g_return_if_reached ();
}

GList *
bus_match_rule_get_recipients (BusMatchRule   *rule,
                               GDBusMessage   *message)
{
    g_assert (BUS_IS_MATCH_RULE (rule));
    g_assert (message != NULL);

    GList *link;
    GList *recipients = NULL;

    if (!bus_match_rule_match (rule, message))
        return FALSE;

    for (link = rule->recipients; link != NULL; link = link->next) {
        BusRecipient *recipient = (BusRecipient *) link->data;

        recipients = g_list_append (recipients, recipient->connection);
    }

    return recipients;
}

