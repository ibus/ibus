/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2018-2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2018-2021 Red Hat, Inc.
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
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#include "ibusunicode.h"

#define NAMES_LIST_SUBJECT "The Unicode Standard"
#define BLOCKS_SUBJECT "Blocks-"

/* This file has 21 lines about the license at the top of the file. */
#define LICENSE_LINES 21

typedef enum
{
    UCD_NAMES_LIST,
    UCD_BLOCKS
} UCDType;

typedef struct _UnicodeData UnicodeData;
typedef struct _UnicodeDataIndex UnicodeDataIndex;

struct _UnicodeData{
    gunichar code;
    gchar   *name;
    gchar   *alias;
    gunichar start;
    gunichar end;
    GSList  *list;
};

struct _UnicodeDataIndex {
    gchar *index;
    UnicodeData *data_list;
};

static gchar *unicode_version;

static void
unicode_data_new_object (UnicodeData *data)
{
    g_return_if_fail (data != NULL);
    if (!data->name) {
        g_warning ("No name in U+%04X", data->code);
    }
    IBusUnicodeData *unicode =
            ibus_unicode_data_new ("code",
                                   data->code,
                                   "name",
                                   data->name ? data->name : "",
                                   "alias",
                                   data->alias ? data->alias : "",
                                   NULL);
    data->list = g_slist_append (data->list, unicode);
}

static void
unicode_block_new_object (UnicodeData *data)
{
    g_return_if_fail (data != NULL);
    if (!data->name) {
        g_warning ("No name in U+%04X", data->start);
    }
    IBusUnicodeBlock *block =
            ibus_unicode_block_new ("start",
                                    data->start,
                                    "end",
                                    data->end,
                                    "name",
                                    data->name ? data->name : "",
                                   NULL);
    data->list = g_slist_append (data->list, block);
}

static void
unicode_data_reset (UnicodeData *data)
{
    g_return_if_fail (data != NULL);
    data->code = 0;
    g_clear_pointer (&data->name, g_free);
    g_clear_pointer (&data->alias, g_free);
    data->start = 0;
    data->end = 0;
}

static gboolean
ucd_names_list_parse_comment (const gchar *line)
{
    static gboolean has_version = FALSE;

    if (has_version)
        return TRUE;
    if (strlen (line) > 4 && strncmp (line, "@@@", 3) == 0) {
        gchar **elements = g_strsplit (line, "\t", -1);
        if (strncmp (elements[1], NAMES_LIST_SUBJECT,
            strlen (NAMES_LIST_SUBJECT)) == 0) {
            unicode_version =
                    g_strdup (elements[1] + strlen (NAMES_LIST_SUBJECT) + 1);
            has_version = TRUE;
        }
        g_strfreev (elements);
    }
    return TRUE;
}

static gboolean
ucd_names_list_parse_alias (const gchar *line,
                            UnicodeData *data)
{
    g_return_val_if_fail (line != NULL, FALSE);
    g_return_val_if_fail (data != NULL, FALSE);

    if (*line == '\0')
        return FALSE;
    if (data->alias) {
        gchar *new_alias = g_strdup_printf ("%s;%s", data->alias, line);
        g_free (data->alias);
        data->alias = new_alias;
    } else {
        data->alias = g_strdup (line);
    }
    return TRUE;
}

static gboolean
ucd_names_list_parse_indent_line (const gchar *line,
                                  UnicodeData *data)
{
    g_return_val_if_fail (line != NULL, FALSE);

    switch (*line) {
    case '\0':
        return FALSE;
    case '=':
        line++;
        while (*line == ' ') line++;
        return ucd_names_list_parse_alias (line, data);
    default:;
    }
    return TRUE;
}

static gboolean
ucd_names_list_parse_line (const gchar *line,
                           UnicodeData *data)
{
    g_return_val_if_fail (line != NULL, FALSE);

    switch (*line) {
    case '\0':
        return TRUE;
    case ';':
        return TRUE;
    case '@':
        return ucd_names_list_parse_comment (line);
    case '\t':
        return ucd_names_list_parse_indent_line (line + 1, data);
    default:;
    }
    if (g_ascii_isxdigit (*line)) {
        gchar **elements = g_strsplit (line, "\t", -1);
        gunichar code;
        gchar *name;

        if (g_strv_length (elements) < 2) {
            g_strfreev (elements);
            return FALSE;
        }
        code = g_ascii_strtoull (elements[0], NULL, 16);
        name = g_strdup (elements[1]);
        if (data->name) {
            unicode_data_new_object (data);
            unicode_data_reset (data);
        }
        data->code = code;
        data->name = name;
        g_strfreev (elements);
    }
    return TRUE;
}

static gboolean
ucd_blocks_parse_comment (const gchar *line)
{
    static gboolean has_version = FALSE;

    g_return_val_if_fail (line != NULL, FALSE);

    if (has_version)
        return TRUE;
    while (*line == ' ') line++;
    if (strlen (line) > strlen (BLOCKS_SUBJECT) &&
        strncmp (line, BLOCKS_SUBJECT, strlen (BLOCKS_SUBJECT)) == 0) {
            unicode_version = g_strdup (line + strlen (BLOCKS_SUBJECT) + 1);
            has_version = TRUE;
    }
    return TRUE;
}

static gboolean
ucd_blocks_parse_line (const gchar *line,
                       UnicodeData *data)
{
    g_return_val_if_fail (line != NULL, FALSE);

    switch (*line) {
    case '\0':
        return TRUE;
    case '#':
        return ucd_blocks_parse_comment (line + 1);
    default:;
    }
    if (g_ascii_isxdigit (*line)) {
        gchar *endptr = NULL;
        gunichar start = g_ascii_strtoull (line, &endptr, 16);
        gunichar end;
        gchar *name = NULL;

        if (endptr == NULL || *endptr == '\0')
            return FALSE;
        while (*endptr == '.') endptr++;
        line = endptr;
        endptr = NULL;
        end = g_ascii_strtoull (line, &endptr, 16);
        if (endptr == NULL || *endptr == '\0')
            return FALSE;
        while (*endptr == ';') endptr++;
        while (*endptr == ' ') endptr++;
        if (*endptr == '\0')
            return FALSE;
        name = g_strdup (endptr);
        if (data->name) {
            unicode_block_new_object (data);
            unicode_data_reset (data);
        }
        data->start = start;
        data->end = end;
        data->name = name;
    }
    return TRUE;
}

static gboolean
ucd_parse_file (const gchar *filename,
                GSList     **list,
                UCDType      type)
{
    UnicodeData data = { 0, };
    gchar *content = NULL;
    gsize length = 0;
    GError *error = NULL;
    gchar *head, *end, *line;
    int n = 1;

    g_return_val_if_fail (filename != NULL, FALSE);
    g_return_val_if_fail (list != NULL, FALSE);

    if (!g_file_get_contents (filename, &content, &length, &error)) {
        g_warning ("Failed to load %s: %s",
                   filename, error ? error->message : "");
        goto failed_to_parse_ucd_names_list;
    }
    end = content;
    while (*end == '\n' && end - content < length) {
        end++;
        n++;
    }
    head = end;
    while (end - content < length) {
        while (*end != '\n' && end - content < length)
            end++;
        if (end - content >= length)
            break;
        line = g_strndup (head, end - head);
        switch (type) {
        case UCD_NAMES_LIST:
            if (!ucd_names_list_parse_line (line, &data)) {
                g_warning ("parse error #%d in %s version %s: %s",
                           n, filename,
                           unicode_version ? unicode_version : "(null)",
                           line);
            }
            break;
        case UCD_BLOCKS:
            if (!ucd_blocks_parse_line (line, &data)) {
                g_warning ("parse error #%d in %s version %s: %s",
                           n, filename,
                           unicode_version ? unicode_version : "(null)",
                           line);
            }
            break;
        default:
            abort ();
        }
        while (*end == '\n' && end - content < length) {
            end++;
            n++;
        }
        g_free (line);
        head = end;
    }
    if (data.name != NULL) {
        switch (type) {
        case UCD_NAMES_LIST:
            unicode_data_new_object (&data);
            break;
        case UCD_BLOCKS:
            unicode_block_new_object (&data);
            break;
        default:;
        }
        unicode_data_reset (&data);
    }
    g_free (content);
    *list = data.list;
    return TRUE;

failed_to_parse_ucd_names_list:
    if (error)
        g_error_free (error);
    g_clear_pointer (&content, g_free);
    *list = data.list;
    return FALSE;
}

static void
update_license_years (gchar *content)
{
    time_t now = time (NULL);
    GDate *date;
    guint year;
    gchar year_buff[5] = { '\0' };

    g_return_if_fail (now != (time_t)-1);
    date = g_date_new ();
    g_assert (date != NULL);
    g_date_set_time_t (date, now);
    year = date->year;
    g_date_free (date);
    g_return_if_fail (year != 0);
    g_return_if_fail (g_snprintf (year_buff, 5, "%u", year) > 0);

    do {
        gchar *copyright = g_strstr_len (content, -1, "Copyright (C) ");
        if (copyright != NULL && *(copyright + 18) == '-') {
            copyright += 19;
            memcpy (copyright, year_buff, 4);
        } else {
            copyright = NULL;
        }
        content = copyright;
    } while (content != NULL);
}

static void
block_list_dump (IBusUnicodeBlock *block,
                 GString          *buff)
{
    gchar *line;
    g_return_if_fail (buff != NULL);

    g_string_append (buff, "    /* TRANSLATORS: You might refer the "         \
                           "translations from gucharmap with\n"               \
                           "                    the following command:\n"     \
                           "       msgmerge -C gucharmap.po ibus.po "         \
                           "ibus.pot */\n");
    line = g_strdup_printf ("    N_(\"%s\"),\n",
                            ibus_unicode_block_get_name (block));
    g_string_append (buff, line);
    g_free (line);
}

static void
ucd_block_translatable_save (const gchar *filename,
                             GSList      *blocks_list)
{
    gchar *content = NULL;
    gsize length = 0;
    GError *error = NULL;
    gchar *p, *substr;
    GString *buff = NULL;
    int i;
    GSList *list = blocks_list;

    g_return_if_fail (filename != NULL);
    g_return_if_fail (list != NULL);

    if (!g_file_get_contents (__FILE__, &content, &length, &error)) {
        g_warning ("Failed to load %s: %s", __FILE__, error->message);
        g_clear_pointer (&error, g_error_free);
        return;
    }

    buff = g_string_new (NULL);
    p = content;
    for (i = 0; i < LICENSE_LINES; i++, p++) {
        if ((p = strchr (p, '\n')) == NULL)
            break;
    }
    if (p != NULL) {
        substr = g_strndup (content, p - content);
        update_license_years (substr);
        g_string_append (buff, substr);
        g_free (substr);
        g_string_append_c (buff, '\n');
    }
    g_clear_pointer (&content, g_free);

    g_string_append (buff, "\n");
    substr = g_strdup_printf ("/* This file is generated by %s. */", __FILE__);
    g_string_append (buff, substr);
    g_free (substr);
    g_string_append (buff, "\n");
    g_string_append (buff, "include <glib/gi18n.h>\n");
    g_string_append (buff, "\n");
    g_string_append (buff, "#ifndef __IBUS_UNICODE_GEN_H_\n");
    g_string_append (buff, "#define __IBUS_UNICODE_GEN_H_\n");
    g_string_append (buff, "const static char *unicode_blocks[] = {\n");
    g_slist_foreach (list, (GFunc)block_list_dump, buff);
    g_string_append (buff, "};\n");
    g_string_append (buff, "#endif\n");

    if (!g_file_set_contents (filename, buff->str, -1, &error)) {
        g_warning ("Failed to save emoji category file %s: %s",
                   filename, error->message);
        g_error_free (error);
    }

    g_string_free (buff, TRUE);
}

int
main (int argc, char *argv[])
{
    gchar *prgname;
    gchar *input_names_list = NULL;
    gchar *input_blocks = NULL;
    gchar *output_names_list = NULL;
    gchar *output_blocks = NULL;
    gchar *output_blocks_trans = NULL;
    GOptionEntry     entries[] = {
        { "input-names-list", 'n', 0, G_OPTION_ARG_STRING, &input_names_list,
          "Parse NamesList.txt FILE in unicode.org ",
          "FILE"
        },
        { "input-blocks", 'b', 0, G_OPTION_ARG_STRING, &input_blocks,
          "Parse Blocks.txt FILE in unicode.org ",
          "FILE"
        },
        { "output-names-list", 'o', 0, G_OPTION_ARG_STRING, &output_names_list,
          "Save the Unicode data as FILE",
          "FILE"
        },
        { "output-blocks", 'B', 0, G_OPTION_ARG_STRING, &output_blocks,
          "Save the Unicode block list as FILE",
          "FILE"
        },
        { "output-blocks-trans", 'C', 0, G_OPTION_ARG_STRING,
          &output_blocks_trans,
          "Save the translatable Unicode blocks as FILE",
          "FILE"
        },
        { NULL }
    };
    GOptionContext *context;
    GError *error = NULL;
    GSList *names_list = NULL;
    GSList *blocks_list = NULL;

#ifdef HAVE_LOCALE_H
    /* To output emoji warnings. */
    setlocale (LC_ALL, "");
#endif

    prgname = g_path_get_basename (argv[0]);
    g_set_prgname (prgname);
    g_free (prgname);

    context = g_option_context_new (NULL);
    g_option_context_add_main_entries (context, entries, NULL);

    if (argc < 3) {
        g_print ("%s", g_option_context_get_help (context, TRUE, NULL));
        g_option_context_free (context);
        return -1;
    }

    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_warning ("Failed options: %s", error->message);
        g_error_free (error);
        return -1;
    }
    g_option_context_free (context);

    if (input_names_list) {
        ucd_parse_file (input_names_list, &names_list, UCD_NAMES_LIST);
        g_free (input_names_list);
    }
    if (output_names_list && names_list)
        ibus_unicode_data_save (output_names_list, names_list);
    g_free (output_names_list);
    g_slist_free_full (names_list, g_object_unref);

    if (input_blocks) {
        ucd_parse_file (input_blocks, &blocks_list, UCD_BLOCKS);
        g_free (input_blocks);
    }
    if (output_blocks && blocks_list)
        ibus_unicode_block_save (output_blocks, blocks_list);
    if (output_blocks_trans && blocks_list)
        ucd_block_translatable_save (output_blocks_trans, blocks_list);
    g_free (output_blocks);
    g_free (output_blocks_trans);
    g_slist_free_full (blocks_list, g_object_unref);

    g_free (unicode_version);
    return 0;
}
