/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2016-2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2016 Red Hat, Inc.
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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __IBUS_ENGINE_SIMPLE_PRIVATE_H__
#define __IBUS_ENGINE_SIMPLE_PRIVATE_H__

#include <glib.h>


G_BEGIN_DECLS

/**
 * IBUS_COMPOSE_ERROR:
 *
 * The `GQuark` used for `IBusComposeTableEx` errors.
 *
 * Since: 1.5.33
 */
#define IBUS_COMPOSE_ERROR (ibus_compose_error_quark ())


struct _IBusComposeTablePrivate
{
    const guint16 *data_first;
    const guint32 *data_second;
    gsize first_n_seqs;
    gsize second_size;
};


/**
 * ibus_compose_error_quark:
 *
 * Since: 1.5.33
 * Stability: Unstable
 */
G_GNUC_INTERNAL
GQuark   ibus_compose_error_quark   (void);
guint    ibus_compose_key_flag      (guint                       key);
G_GNUC_INTERNAL
gboolean ibus_check_algorithmically (const guint                *compose_buffer,
                                     int                         n_compose,
                                     gunichar                   *output);
GVariant *
         ibus_compose_table_serialize
                                    (IBusComposeTableEx
                                                                *compose_table,
                                     gboolean                   reverse_endian);
G_GNUC_INTERNAL
IBusComposeTableEx *
         ibus_compose_table_deserialize
                                    (const char                 *contents,
                                     gsize                       length,
                                     guint16                    *saved_version);
G_GNUC_INTERNAL
gboolean ibus_compose_table_check   (const IBusComposeTableEx   *table,
                                     guint                      *compose_buffer,
                                     int                         n_compose,
                                     gboolean                   *compose_finish,
                                     gboolean                   *compose_match,
                                     GString                    *output,
                                     gboolean                    is_32bit);
G_GNUC_INTERNAL
gunichar ibus_keysym_to_unicode     (guint                       keysym,
                                     gboolean                    combining,
                                     gboolean                   *need_space);
/**
 * ibus_keysym_to_unicode_with_layout:
 *
 * Since: 1.5.33
 * Stability: Unstable
 */
G_GNUC_INTERNAL
gunichar ibus_keysym_to_unicode_with_layout
                                    (guint                       keysym,
                                     gboolean                    combining,
                                     gboolean                   *need_space,
                                     const gchar                *layout,
                                     G_GNUC_UNUSED const gchar  *variant);

G_END_DECLS


#endif /* __IBUS_IM_CONTEXT_SIMPLE_PRIVATE_H__ */
