/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2023 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2023 Red Hat, Inc.
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

/* #2523 Should not include <ibus.h> but each IBus header file. */
#include <glib.h>
#include <glib/gstdio.h>
#include <errno.h>

#include "ibuscomposetable.h"
#include "ibusenginesimpleprivate.h"


static void
save_compose_table_endianness (IBusComposeTableEx *compose_table,
                               gboolean            reverse_endianness)
{
    GVariant *variant_table = NULL;
    const char *contents = NULL;
    gsize length = 0;
    const char *destname_be = "sequences-big-endian";
    const char *destname_le = "sequences-little-endian";
    const char *destname;
    GError *error = NULL;

    variant_table = ibus_compose_table_serialize (compose_table,
                                                  reverse_endianness);
    g_assert (variant_table);
    contents = g_variant_get_data (variant_table);
    length = g_variant_get_size (variant_table);
    g_assert (contents && (length > 0));
#if G_BYTE_ORDER == G_BIG_ENDIAN
    if (!reverse_endianness)
        destname = destname_be;
    else
        destname = destname_le;
#else
    if (!reverse_endianness)
        destname = destname_le;
    else
        destname = destname_be;
#endif
    if (g_file_test (destname, G_FILE_TEST_EXISTS))
        g_unlink (destname);
    if (!g_file_set_contents (destname, contents, length, &error)) {
        g_warning ("Failed to save compose table %s: %s",
                   destname, error->message);
        g_error_free (error);
    }
    g_variant_unref (variant_table);
}


int
main (int argc, char *argv[])
{
    char * const sys_langs[] = { "en_US.UTF-8", "en_US", "en", NULL };
    char * const *sys_lang = NULL;
    char *path = NULL;
    IBusComposeTableEx *compose_table;
    char *basename = NULL;

    path = g_strdup ("./Compose");
    if (!path || !g_file_test (path, G_FILE_TEST_EXISTS)) {
        g_clear_pointer (&path, g_free);
        for (sys_lang = sys_langs; *sys_lang; sys_lang++) {
            path = g_build_filename (X11_LOCALEDATADIR, *sys_lang,
                                     "Compose", NULL);
            if (!path)
                continue;
            if (g_file_test (path, G_FILE_TEST_EXISTS))
                break;
        }
    }
    if (!path) {
        g_warning ("en_US compose file is not found in %s.", X11_LOCALEDATADIR);
        return 1;
    } else {
        g_debug ("Create a cache of %s", path);
    }
    g_setenv ("IBUS_COMPOSE_CACHE_DIR", ".", TRUE);
    compose_table = ibus_compose_table_load_cache (path);
    if (!compose_table &&
        (compose_table = ibus_compose_table_new_with_file (path, NULL))
           == NULL) {
        g_warning ("Failed to generate the compose table.");
        return 1;
    }
    g_free (path);
    basename = g_strdup_printf ("%08x.cache", compose_table->id);
    g_debug ("Saving cache id %s", basename);
    g_free (basename);


    save_compose_table_endianness (compose_table, FALSE);
    save_compose_table_endianness (compose_table, TRUE);
    return 0;
}
