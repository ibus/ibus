/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* bus - The Input Bus
 * Copyright (C) 2008-2015 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2010-2019 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2008-2016 Red Hat, Inc.
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <string.h>
#include "ibusxml.h"

#ifdef ENABLE_NLS
#include <libintl.h>
#endif

/* gettext macro */
#define N_(t) t

static GHashTable *__languages_dict;

static gboolean
_iso_codes_parse_xml_node (XMLNode          *node)
{
    GList *p;
    g_assert (node);

    if (G_UNLIKELY (g_strcmp0 (node->name, "iso_639_3_entries") != 0)) {
        return FALSE;
    }

    for (p = node->sub_nodes; p != NULL; p = p->next) {
        XMLNode *sub_node = (XMLNode *)p->data;
        gchar **attributes = NULL;
        int i, j;
        struct {
            const gchar *key;
            gchar *value;
        } entries[] = {
            { "id", NULL },
            { "part1_code", NULL },
            { "part2_code", NULL },
        };

        if (sub_node->attributes == NULL) {
            continue;
        }

        attributes = sub_node->attributes;
        for (i = 0; attributes[i]; i += 2) {
            if (g_strcmp0 (attributes[i], "name") == 0) {
                for (j = 0; j < G_N_ELEMENTS (entries); j++) {
                    if (entries[j].value == NULL)
                        continue;
                    g_hash_table_insert (__languages_dict,
                                         (gpointer) g_strdup (entries[j].value),
                                         (gpointer) g_strdup (attributes[i + 1]));
                    entries[j].value = NULL;
                }
            } else {
                for (j = 0; j < G_N_ELEMENTS (entries); j++) {
                    if (g_strcmp0 (attributes[i], entries[j].key) == 0 &&
                        attributes[i + 1] != NULL) {
                        entries[j].value = attributes[i + 1];
                    }
                }
            }
        }
    }

    return TRUE;
}

void
_load_lang()
{
    gchar *filename;
    XMLNode *node;
    struct stat buf;

#ifdef ENABLE_NLS
    bindtextdomain ("iso_639_3", LOCALEDIR);
    bind_textdomain_codeset ("iso_639_3", "UTF-8");
#endif

    __languages_dict = g_hash_table_new_full (g_str_hash,
            g_str_equal, g_free, g_free);
    filename = g_build_filename (ISOCODES_PREFIX,
                                 "share/xml/iso-codes/iso_639_3.xml",
                                 NULL);
    if (g_stat (filename, &buf) != 0) {
        g_warning ("Can not get stat of file %s", filename);
        g_free (filename);
        return;
    }

    node = ibus_xml_parse_file (filename);
    g_free (filename);

    if (!node) {
        return;
    }

    _iso_codes_parse_xml_node (node);
    ibus_xml_free (node);
}

const static gchar *
ibus_get_untranslated_raw_language_name (const gchar *_locale)
{
    const gchar *retval;
    gchar *p = NULL;
    gchar *lang = NULL;

    if (__languages_dict == NULL )
        _load_lang();
    if ((p = strchr (_locale, '_')) !=  NULL)
        p = g_strndup (_locale, p - _locale);
    else
        p = g_strdup (_locale);
    lang = g_ascii_strdown (p, -1);
    g_free (p);
    retval = (const gchar *) g_hash_table_lookup (__languages_dict, lang);
    g_free (lang);
    if (retval != NULL)
        if (g_strcmp0 (retval, "Bengali") == 0)
            /* use common_name "Bangla" if lang is "bn" */
            return "Bangla";
        else
            return retval;
    else
        return "Other";
}

static char *
get_first_item_in_semicolon_list (const char *list)
{
        char **items;
        char  *item;

        items = g_strsplit (list, "; ", 2);

        item = g_strdup (items[0]);
        g_strfreev (items);

        return item;
}

static char *
capitalize_utf8_string (const char *str)
{
    char first[8] = { 0 };

    if (!str)
        return NULL;

    g_unichar_to_utf8 (g_unichar_totitle (g_utf8_get_char (str)), first);

    return g_strconcat (first, g_utf8_offset_to_pointer (str, 1), NULL);
}

gchar *
ibus_get_untranslated_language_name (const gchar *_locale)
{
    const gchar *raw = ibus_get_untranslated_raw_language_name (_locale);
    gchar *tmp = get_first_item_in_semicolon_list (raw);
    gchar *retval = capitalize_utf8_string (tmp);
    g_free (tmp);
    return retval;
}

gchar *
ibus_get_language_name (const gchar *_locale)
{
    const gchar *raw = ibus_get_untranslated_raw_language_name (_locale);
    const gchar *translation = NULL;
    gchar *tmp;
    gchar *retval;

#ifdef ENABLE_NLS
    if (g_strcmp0 (raw, "Other") == 0)
        return g_strdup (dgettext (GETTEXT_PACKAGE, N_("Other")));
    else
        translation = dgettext ("iso_639_3", raw);
#else
    translation = raw;
#endif

    tmp = get_first_item_in_semicolon_list (translation);
    retval = capitalize_utf8_string (tmp);
    g_free (tmp);
    return retval;
}

void
ibus_g_variant_get_child_string (GVariant *variant, gsize index, char **str)
{
    g_return_if_fail (str != NULL);

    g_free (*str);
    g_variant_get_child (variant, index, "s", str);
}
