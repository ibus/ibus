/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2013-2014 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2013-2023 Takao Fujiwara <takao.fujiwara1@gmail.com>
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

#ifndef __IBUS_COMPOSETABLE_H_
#define __IBUS_COMPOSETABLE_H_

#include <glib.h>


G_BEGIN_DECLS

typedef struct _IBusComposeTable IBusComposeTable;
typedef struct _IBusComposeTableEx IBusComposeTableEx;
typedef struct _IBusComposeTablePrivate IBusComposeTablePrivate;
typedef struct _IBusComposeTableCompact IBusComposeTableCompact;
typedef struct _IBusComposeTableCompactEx IBusComposeTableCompactEx;
typedef struct _IBusComposeTableCompactPrivate IBusComposeTableCompactPrivate;

struct _IBusComposeTable
{
    guint16 *data;
    gint max_seq_len;
    gint n_seqs;
    guint32 id;
};

struct _IBusComposeTableEx
{
    IBusComposeTablePrivate *priv;
    /* @data is const value to accept mmap data and the releasable allocation
     * is assigned to @rawdata. */
    const guint16 *data;
    gint max_seq_len;
    gint n_seqs;
    guint32 id;
    char *rawdata;
};


/**
 * ibus_compose_table_new_with_file:
 * @compose_file: The path of the compose file
 * @compose_tables: (nullable): The list of other @IBusComposeTableEx
 * and the generating @IBusComposeTableEx excludes the compose keys
 * which are included in the other @IBusComposeTableEx.
 *
 * Generate @IBusComposeTableEx from the compose file.
 *
 * Returns: @IBusComposeTableEx
 */
IBusComposeTableEx *
                  ibus_compose_table_new_with_file (const gchar *compose_file,
                                                    GSList
                                                               *compose_tables);
IBusComposeTableEx *
                  ibus_compose_table_load_cache    (const gchar *compose_file);
void              ibus_compose_table_save_cache    (IBusComposeTableEx
                                                                *compose_table);
GSList *          ibus_compose_table_list_add_array
                                                   (GSList
                                                                *compose_tables,
                                                    const guint16
                                                                *data,
                                                    gint         max_seq_len,
                                                    gint         n_seqs);
GSList *          ibus_compose_table_list_add_file (GSList
                                                                *compose_tables,
                                                    const gchar *compose_file);
GSList *          ibus_compose_table_list_add_table (GSList
                                                                *compose_tables,
                                                     IBusComposeTableEx
                                                                *compose_table);

G_BEGIN_DECLS
#endif
