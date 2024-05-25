/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* IBus - The Input Bus
 * Copyright (C) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2024 Takao Fujiwara <takao.fujiwara1@gmail.com>
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
#ifndef __BUS_MATCH_RULE_H_
#define __BUS_MATCH_RULE_H_

#include <ibus.h>

#include "connection.h"

/*
 * Type macros.
 */

/* define GOBJECT macros */
#define BUS_TYPE_MATCH_RULE             \
    (bus_match_rule_get_type ())
#define BUS_MATCH_RULE(obj)             \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BUS_TYPE_MATCH_RULE, BusMatchRule))
#define BUS_MATCH_RULE_CLASS(klass)     \
    (G_TYPE_CHECK_CLASS_CAST ((klass), BUS_TYPE_MATCH_RULE, BusMatchRuleClass))
#define BUS_IS_MATCH_RULE(obj)          \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BUS_TYPE_MATCH_RULE))
#define BUS_IS_MATCH_RULE_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), BUS_TYPE_MATCH_RULE))
#define BUS_MATCH_RULE_GET_CLASS(obj)   \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), BUS_TYPE_MATCH_RULE, BusMatchRuleClass))

G_BEGIN_DECLS

typedef struct _BusMatchRule BusMatchRule;
typedef struct _BusMatchRuleClass BusMatchRuleClass;

GType            bus_match_rule_get_type    (void);
BusMatchRule    *bus_match_rule_new         (const gchar        *text);
BusMatchRule    *bus_match_rule_ref         (BusMatchRule       *rule);
void             bus_match_rule_unref       (BusMatchRule       *rule);
void             bus_match_rule_free        (BusMatchRule       *rule);
gboolean         bus_match_rule_set_message_type
                                            (BusMatchRule       *rule,
                                             gint                type);
const gchar *    bus_match_rule_get_sender  (BusMatchRule       *rule);
gboolean         bus_match_rule_set_sender  (BusMatchRule       *rule,
                                             const gchar        *sender);
gboolean         bus_match_rule_set_interface
                                            (BusMatchRule       *rule,
                                             const gchar        *interface);
gboolean         bus_match_rule_set_member  (BusMatchRule       *rule,
                                             const gchar        *member);
gboolean         bus_match_rule_set_path    (BusMatchRule       *rule,
                                             const gchar        *path);
gboolean         bus_match_rule_set_destination
                                            (BusMatchRule       *rule,
                                             const gchar        *dest);
gboolean         bus_match_rule_set_arg     (BusMatchRule       *rule,
                                             guint               arg_i,
                                             const gchar        *arg);
gboolean         bus_match_rule_match       (BusMatchRule       *rule,
                                             GDBusMessage       *message);
gboolean         bus_match_rule_is_equal    (BusMatchRule       *a,
                                             BusMatchRule       *b);
void             bus_match_rule_add_recipient
                                            (BusMatchRule       *rule,
                                             BusConnection      *connection);
void             bus_match_rule_remove_recipient
                                            (BusMatchRule       *rule,
                                             BusConnection      *connection);
GList           *bus_match_rule_get_recipients
                                            (BusMatchRule   *rule,
                                             GDBusMessage   *message);

G_END_DECLS
#endif

