/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* IBus - The Input Bus
 * Copyright (C) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2018 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2008-2018 Red Hat, Inc.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
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
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 */
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib/gstdio.h>
#include "ibuskeys.h"
#include "ibuskeysyms.h"
#include "ibuskeymap.h"

typedef guint KEYMAP[256][7];
/* functions prototype */
static void         ibus_keymap_destroy         (IBusKeymap             *keymap);
static gboolean     ibus_keymap_load            (const gchar            *name,
                                                 KEYMAP                  keymap);
static GHashTable   *keymaps = NULL;

G_DEFINE_TYPE (IBusKeymap, ibus_keymap, IBUS_TYPE_OBJECT)

static void
ibus_keymap_class_init (IBusKeymapClass *class)
{
    IBusObjectClass *object_class = IBUS_OBJECT_CLASS (class);

    object_class->destroy = (IBusObjectDestroyFunc) ibus_keymap_destroy;
}

static void
ibus_keymap_init (IBusKeymap *keymap)
{
    gint i;
    keymap->name = NULL;
    for (i = 0; i < sizeof (keymap->keymap) / sizeof (guint); i++) {
        ((guint *)keymap->keymap)[i] = IBUS_KEY_VoidSymbol;
    }
}

static void
ibus_keymap_destroy (IBusKeymap *keymap)
{
    if (keymap->name != NULL) {
        g_free (keymap->name);
        keymap->name = NULL;
    }
    IBUS_OBJECT_CLASS (ibus_keymap_parent_class)->destroy ((IBusObject *)keymap);
}

#define SKIP_SPACE(p)   \
    while (*p == ' ') p++;

static gboolean
ibus_keymap_parse_line (gchar  *str,
                        KEYMAP  keymap)
{
    gchar *p1, *p2, ch;
    gint i;
    guint keycode;
    guint keysym;

    const struct {
        const gchar *prefix;
        const gint len;
    } prefix [] = {
        { "keycode ", sizeof ("keycode ") - 1 },
        { "shift keycode ", sizeof ("shift keycode ") - 1 },
        { "capslock keycode ", sizeof ("capslock keycode ") - 1 },
        { "shift capslock keycode ", sizeof ("shift capslock keycode ") - 1 },
        { "altgr keycode ", sizeof ("altgr keycode ") - 1},
        { "shift altgr keycode ", sizeof ("shift altgr keycode ") - 1},
        { "numlock keycode ", sizeof ("numlock keycode ") - 1},
    };

    p1 = str;

    SKIP_SPACE(p1);

    if (*p1 == '#')
        return TRUE;

    if (strncmp (p1, "include ", sizeof ("include ") - 1) == 0) {
        p1 += sizeof ("include ") - 1;
        for (p2 = p1; *p2 != '\n'; p2++);
        *p2 = '\0';
        return ibus_keymap_load (p1, keymap);
    }

    for (i = 0; i < sizeof (prefix) / sizeof (prefix[0]); i++) {
        if (strncmp (p1, prefix[i].prefix, prefix[i].len) == 0) {
            p1 += prefix[i].len;
            break;
        }
    }

    if (i >= sizeof (prefix) / sizeof (prefix[0]))
        return FALSE;

    keycode = (guint) strtoul (p1, &p2, 10);

    if (keycode == 0 && p1 == p2)
        return FALSE;

    if ((int) keycode < 0 || keycode > 255)
        return FALSE;

    p1 = p2;

    if (*p1++ != ' ')
        return FALSE;
    if (*p1++ != '=')
        return FALSE;
    if (*p1++ != ' ')
        return FALSE;

    for (p2 = p1; *p2 != '\n' && *p2 != ' '; p2++);
    *p2 = '\0'; p2++;

    keysym = ibus_keyval_from_name (p1);

    if (keysym == IBUS_KEY_VoidSymbol)
        return FALSE;

    /* Do not assign *p1 to g_ascii_isalpha() directly for the syntax check */
    if (i == 0 &&
        strncmp (p2, "addupper", sizeof ("addupper") - 1) == 0 &&
        (ch = *p1) && (ch >= 0) && g_ascii_isalpha (ch)) {
        gchar buf[] = "a";
        buf[0] = g_ascii_toupper(ch);
        keymap[keycode][0] = keymap[keycode][3] = keysym;
        keymap[keycode][1] = keymap[keycode][2] = ibus_keyval_from_name (buf);

    }
    else {
        keymap[keycode][i] = keysym;
    }

    return TRUE;
}

static gboolean
ibus_keymap_load (const gchar *name,
                  KEYMAP       keymap)
{
    const gchar *envstr;
    gchar *fname;
    FILE *pf;
    gchar buf[256];
    gint lineno;


    if ((envstr = g_getenv ("IBUS_KEYMAP_PATH")) != NULL)
        fname = g_build_filename (envstr, name, NULL);
    else
        fname = g_build_filename (IBUS_DATA_DIR, "keymaps", name, NULL);

    if (fname == NULL) {
        return FALSE;
    }
    pf = g_fopen (fname, "r");
    g_free (fname);

    if (pf == NULL) {
        return FALSE;
    }

    lineno = 0;
    while (fgets (buf, sizeof (buf), pf) != NULL) {
        lineno ++;
        if (!ibus_keymap_parse_line (buf, keymap)) {
            g_warning ("parse %s failed on %d line", name, lineno);
            lineno = -1;
            break;
        }
    }

    fclose (pf);

    if (lineno == -1) {
        return FALSE;
    }

    return TRUE;
}

void
ibus_keymap_fill (KEYMAP keymap)
{
    gint i;
    for (i = 0; i < 256; i++) {
        /* fill shift */
        if (keymap[i][1] == IBUS_KEY_VoidSymbol)
            keymap[i][1] = keymap[i][0];

        /* fill capslock */
        if (keymap[i][2] == IBUS_KEY_VoidSymbol)
            keymap[i][2] = keymap[i][0];

        /* fill shift capslock */
        if (keymap[i][3] == IBUS_KEY_VoidSymbol)
            keymap[i][3] = keymap[i][1];

        /* fill altgr */
        if (keymap[i][4] == IBUS_KEY_VoidSymbol)
            keymap[i][4] = keymap[i][0];

        /* fill shift altgr */
        if (keymap[i][5] == IBUS_KEY_VoidSymbol)
            keymap[i][5] = keymap[i][1];
    }
}

static void
_keymap_destroy_cb (IBusKeymap *keymap,
                    gpointer    user_data)
{
    g_hash_table_remove (keymaps, keymap->name);
    g_object_unref (keymap);
}

IBusKeymap *
ibus_keymap_new (const gchar *name)
{
    return ibus_keymap_get (name);
}

IBusKeymap *
ibus_keymap_get (const gchar *name)
{
    g_assert (name != NULL);

    IBusKeymap *keymap;

    if (keymaps == NULL) {
        keymaps = g_hash_table_new_full (g_str_hash,
                                         g_str_equal,
                                         (GDestroyNotify) g_free,
                                         (GDestroyNotify)  g_object_unref);
    }

    keymap = (IBusKeymap *) g_hash_table_lookup (keymaps, name);

    if (keymap == NULL) {
        keymap = g_object_new (IBUS_TYPE_KEYMAP, NULL);
        g_object_ref_sink (keymap);

        if (ibus_keymap_load (name, keymap->keymap)) {
            ibus_keymap_fill (keymap->keymap);
            keymap->name = g_strdup (name);
            g_hash_table_insert (keymaps, g_strdup (keymap->name), keymap);

            g_signal_connect (keymap, "destroy", G_CALLBACK (_keymap_destroy_cb), NULL);
        }
        else {
            g_object_unref (keymap);
            keymap = NULL;
        }
    }
    if (keymap != NULL)
        g_object_ref_sink (keymap);
    return keymap;
}

guint32
ibus_keymap_lookup_keysym (IBusKeymap *keymap,
                           guint16     keycode,
                           guint32     state)
{
    g_assert (IBUS_IS_KEYMAP (keymap));

    if (keycode < 256) {
        /* numlock */
        if ((state & IBUS_MOD2_MASK) &&
            (keymap->keymap[keycode][6] != IBUS_KEY_VoidSymbol)) {
            return keymap->keymap[keycode][6];
        }

        state &= IBUS_SHIFT_MASK | IBUS_LOCK_MASK | IBUS_MOD5_MASK;

        switch (state) {
        case 0:
            return keymap->keymap[keycode][0];
        case IBUS_SHIFT_MASK:
            return keymap->keymap[keycode][1];
        case IBUS_LOCK_MASK:
            return keymap->keymap[keycode][2];
        case IBUS_SHIFT_MASK | IBUS_LOCK_MASK:
            return keymap->keymap[keycode][3];
        case IBUS_MOD5_MASK:
        case IBUS_MOD5_MASK | IBUS_LOCK_MASK:
            return keymap->keymap[keycode][4];
        case IBUS_MOD5_MASK | IBUS_SHIFT_MASK:
        case IBUS_MOD5_MASK | IBUS_LOCK_MASK | IBUS_SHIFT_MASK:
            return keymap->keymap[keycode][5];
        default:
            break;
        }
    }

    return IBUS_KEY_VoidSymbol;
}
