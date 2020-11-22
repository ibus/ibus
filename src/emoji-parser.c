/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2016-2020 Takao Fujiwara <takao.fujiwara1@gmail.com>
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

/* Convert /usr/share/unicode/cldr/common/annotations/\*.xml and
 * /usr/share/unicode/emoji/emoji-test.txt
 * to the dictionary file which look up the Emoji from the annotation.
 * Get *.xml from https://github.com/fujiwarat/cldr-emoji-annotation
 * or http://www.unicode.org/repos/cldr/trunk/common/annotations .
 * Get emoji-test.txt from http://unicode.org/Public/emoji/4.0/ .
 * en.xml is used for the Unicode annotations and emoji-test.txt is used
 * for the category, e.g. "Smileys & People".
 * ASCII emoji annotations are saved in ../data/annotations/en_ascii.xml
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

#ifdef HAVE_JSON_GLIB1
#include <json-glib/json-glib.h>
#endif

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#include <string.h>

#include "ibusemoji.h"

/* This file has 21 lines about the license at the top of the file. */
#define LICENSE_LINES 21

typedef enum {
  EMOJI_STRICT,
  EMOJI_VARIANT,
  EMOJI_NOVARIANT
} EmojiDataSearchType;

typedef struct _EmojiData EmojiData;
struct _EmojiData {
    gchar              *emoji;
    gchar              *emoji_alternates;
    GSList             *annotations;
    gboolean            is_annotation;
    gchar              *description;
    gboolean            is_tts;
    gchar              *category;
    gchar              *subcategory;
    gboolean            is_derived;
    GSList             *list;
    EmojiDataSearchType search_type;
};

typedef struct _NoTransData NoTransData;
struct _NoTransData {
    const gchar *xml_file;
    const gchar *xml_derived_file;
    GSList      *emoji_list;
};

static gchar *unicode_emoji_version;


static void
init_annotations (IBusEmojiData *emoji,
                  gpointer       user_data)
{
    g_return_if_fail (IBUS_IS_EMOJI_DATA (emoji));
    ibus_emoji_data_set_annotations (emoji, NULL);
    ibus_emoji_data_set_description (emoji, "");
}

static void
check_no_trans (IBusEmojiData *emoji,
                NoTransData   *no_trans_data)
{
    const gchar *str = NULL;
    g_return_if_fail (IBUS_IS_EMOJI_DATA (emoji));
    if (ibus_emoji_data_get_annotations (emoji) != NULL)
        return;
    str = ibus_emoji_data_get_emoji (emoji);
    if (g_getenv ("IBUS_EMOJI_PARSER_DEBUG") != NULL) {
        gchar *basename = NULL;
        if (no_trans_data->xml_file)
            basename = g_path_get_basename (no_trans_data->xml_file);
        else if (no_trans_data->xml_derived_file)
            basename = g_path_get_basename (no_trans_data->xml_derived_file);
        else
            basename = g_strdup ("WRONG FILE");
        g_warning ("Not found emoji %s in the file %s", str, basename);
        g_free (basename);
    }
    no_trans_data->emoji_list =
            g_slist_append (no_trans_data->emoji_list, g_strdup (str));
}

int
strcmp_ibus_emoji_data_str (IBusEmojiData *emoji,
                            const gchar   *str)
{
    g_return_val_if_fail (IBUS_IS_EMOJI_DATA (emoji), -1);
    return g_strcmp0 (ibus_emoji_data_get_emoji (emoji), str);
}

static void
delete_emoji_from_list (const gchar  *str,
                        GSList      **list)
{
    IBusEmojiData *emoji;

    g_return_if_fail (list != NULL);
    GSList *p = g_slist_find_custom (*list,
                                     str,
                                     (GCompareFunc)strcmp_ibus_emoji_data_str);
    g_return_if_fail (p != NULL);
    emoji = p->data;
    *list = g_slist_remove (*list, emoji);
    g_object_unref (emoji);
}

static void
reset_emoji_element (EmojiData *data)
{
    g_assert (data != NULL);

    g_clear_pointer (&data->emoji, g_free);
    g_clear_pointer (&data->emoji_alternates, g_free);
    g_slist_free_full (data->annotations, g_free);
    data->annotations = NULL;
    g_clear_pointer (&data->description, g_free);
}

/**
 * strcmp_novariant:
 *
 * Return 0 between non-fully-qualified and fully-qualified emojis.
 * E.g. U+1F3CC-200D-2642 and U+1F3CC-FE0F-200D-2642-FE0F
 * in case @a_variant or @b_variant == U+FE0F
 */
gint
strcmp_novariant (const gchar *a,
                  const gchar *b,
                  gunichar     a_variant,
                  gunichar     b_variant)
{
    gint retval;
    GString *buff = NULL;;
    gchar *head = NULL;
    gchar *p;
    gchar *variant = NULL;
    gchar *substr = NULL;

    if (a_variant > 0) {
        if (g_utf8_strchr (a, -1, a_variant) != NULL) {
            buff = g_string_new (NULL);
            p = head = g_strdup (a);
            while (*p != '\0') {
                if ((variant = g_utf8_strchr (p, -1, a_variant)) == NULL) {
                    g_string_append (buff, p);
                    break;
                }
                if (p != variant) {
                    substr = g_strndup (p, variant - p);
                    g_string_append (buff, substr);
                    g_free (substr);
                }
                p = g_utf8_next_char (variant);
            }
            retval = g_strcmp0 (buff->str, b);
            g_string_free (buff, TRUE);
            g_free (head);
            return retval;
        } else {
            return -1;
        }
    } else if (b_variant > 0) {
        if (g_utf8_strchr (b, -1, b_variant) != NULL) {
            buff = g_string_new (NULL);
            p = head = g_strdup (b);
            while (*p != '\0') {
                if ((variant = g_utf8_strchr (p, -1, b_variant)) == NULL) {
                    g_string_append (buff, p);
                    break;
                }
                if (p != variant) {
                    substr = g_strndup (p, variant - p);
                    g_string_append (buff, substr);
                    g_free (substr);
                }
                p = g_utf8_next_char (variant);
            }
            retval = g_strcmp0 (a, buff->str);
            g_string_free (buff, TRUE);
            g_free (head);
            return retval;
        } else {
            return -1;
        }
    }
    return g_strcmp0 (a, b);
}

gint
find_emoji_data_list (IBusEmojiData *a,
                      EmojiData     *b)
{
    const gchar *a_str;

    g_return_val_if_fail (IBUS_IS_EMOJI_DATA (a), 0);

    a_str = ibus_emoji_data_get_emoji (a);
    switch (b->search_type) {
    case EMOJI_VARIANT:
        if (strcmp_novariant (a_str, b->emoji, 0xfe0e, 0) == 0)
            return 0;
        else if (strcmp_novariant (a_str, b->emoji, 0xfe0f, 0) == 0)
            return 0;
        else
            return -1;
        break;
    case EMOJI_NOVARIANT:
        if (strcmp_novariant (a_str, b->emoji, 0, 0xfe0e) == 0)
            return 0;
        else if (strcmp_novariant (a_str, b->emoji, 0, 0xfe0f) == 0)
            return 0;
        else
            return -1;
        break;
    default:;
    }
    return g_strcmp0 (a_str, b->emoji);
}

static void
emoji_data_update_object (EmojiData     *data,
                          IBusEmojiData *emoji)
{
    GSList *src_annotations = data->annotations;
    GSList *dest_annotations = ibus_emoji_data_get_annotations (emoji);
    GSList *l;
    gboolean updated_annotations = FALSE;
    for (l = src_annotations; l; l = l->next) {
        GSList *duplicated = g_slist_find_custom (dest_annotations,
                                                  l->data,
                                                  (GCompareFunc) g_strcmp0);
        if (duplicated == NULL) {
            dest_annotations = g_slist_append (dest_annotations,
                                               g_strdup (l->data));
            updated_annotations = TRUE;
        }
    }
    if (updated_annotations) {
        ibus_emoji_data_set_annotations (
                    emoji,
                    g_slist_copy_deep (dest_annotations,
                                       (GCopyFunc) g_strdup,
                                       NULL));
    }
    if (data->description)
        ibus_emoji_data_set_description (emoji, data->description);
}

static void
emoji_data_new_object (EmojiData *data)
{
    IBusEmojiData *emoji =
            ibus_emoji_data_new ("emoji",
                                 data->emoji,
                                 "annotations",
                                 data->annotations,
                                 "description",
                                 data->description ? data->description
                                         : g_strdup (""),
                                 "category",
                                 data->category ? data->category
                                         : g_strdup (""),
                                 NULL);
    data->list = g_slist_append (data->list, emoji);
}

static void
update_emoji_list (EmojiData *data,
                   gboolean   base_update)
{
    GSList *list;
    gboolean has_strict = FALSE;
    data->search_type = EMOJI_STRICT;
    list = g_slist_find_custom (
            data->list,
            data,
            (GCompareFunc) find_emoji_data_list);
    if (list) {
        emoji_data_update_object (data, list->data);
        has_strict = TRUE;
    } else if (base_update) {
        emoji_data_new_object (data);
        return;
    }
    if (g_utf8_strchr (data->emoji, -1, 0xfe0e) == NULL &&
        g_utf8_strchr (data->emoji, -1, 0xfe0f) == NULL) {
        data->search_type = EMOJI_VARIANT;
        list = g_slist_find_custom (
                data->list,
                data,
                (GCompareFunc) find_emoji_data_list);
        if (list) {
            emoji_data_update_object (data, list->data);
            return;
        }
    } else {
        data->search_type = EMOJI_NOVARIANT;
        list = g_slist_find_custom (
                data->list,
                data,
                (GCompareFunc) find_emoji_data_list);
        if (list) {
            emoji_data_update_object (data, list->data);
            return;
        }
    }
    if (!has_strict)
        emoji_data_new_object (data);
}

static void
unicode_annotations_start_element_cb (GMarkupParseContext *context,
                                      const gchar         *element_name,
                                      const gchar        **attribute_names,
                                      const gchar        **attribute_values,
                                      gpointer             user_data,
                                      GError             **error)
{
    EmojiData *data = (EmojiData *) user_data;
    int i;
    const gchar *attribute;
    const gchar *value;

    g_assert (data != NULL);

    if (g_strcmp0 (element_name, "annotation") != 0)
        return;

    reset_emoji_element (data);

    for (i = 0; (attribute = attribute_names[i]) != NULL; i++) {
        value = attribute_values[i];

        if (g_strcmp0 (attribute, "cp") == 0) {
            if (value == NULL || *value == '\0') {
                g_warning ("cp='' in unicode.org annotations file");
                return;
            } else if (value[0] == '[' && value[strlen(value) - 1] == ']') {
                g_warning ("cp!='[emoji]' is an old format in unicode.org"
                           " annotations file");
                data->emoji = g_strndup (value + 1, strlen(value) - 2);
            } else {
                data->emoji = g_strdup (value);
            }
        }
        /* tts seems 'text to speach' and it would be a description
         * instead of annotation.
         */
        else if (g_strcmp0 (attribute, "type") == 0) {
            if (g_strcmp0 (value, "tts") == 0) {
                data->is_tts = TRUE;
            }
        }
    }

    data->is_annotation = TRUE;
}

static void
unicode_annotations_end_element_cb (GMarkupParseContext *context,
                                    const gchar         *element_name,
                                    gpointer             user_data,
                                    GError             **error)
{
    EmojiData *data = (EmojiData *) user_data;

    g_assert (data != NULL);
    if (!data->is_annotation)
        return;

    update_emoji_list (data, FALSE);
    data->is_annotation = FALSE;
    data->is_tts = FALSE;
}

void
unicode_annotations_text_cb (GMarkupParseContext *context,
                             const gchar         *text,
                             gsize                text_len,
                             gpointer             user_data,
                             GError             **error)
{
    EmojiData *data = (EmojiData *) user_data;
    gchar **annotations = NULL;
    const gchar *annotation;
    int i;

    g_assert (data != NULL);
    if (!data->is_annotation)
        return;
    if (data->is_tts) {
        if (data->description) {
            g_warning ("Duplicated 'tts' is found: %s: %s",
                       data->description, text);
            g_clear_pointer (&data->description, g_free);
        }
        data->description = g_strdup (text);
        return;
    }
    annotations = g_strsplit (text, " | ", -1);
    for (i = 0; (annotation = annotations[i]) != NULL; i++) {
        GSList *duplicated = g_slist_find_custom (data->annotations,
                                                  annotation,
                                                  (GCompareFunc) g_strcmp0);
        if (duplicated == NULL) {
            data->annotations = g_slist_prepend (data->annotations,
                                                 g_strdup (annotation));
        }
    }
    g_strfreev (annotations);
}

static gboolean
unicode_annotations_parse_xml_file (const gchar  *filename,
                                    GSList      **list,
                                    gboolean      is_derived)
{
    gchar *content = NULL;
    gsize length = 0;
    GError *error = NULL;
    const static GMarkupParser parser = {
        unicode_annotations_start_element_cb,
        unicode_annotations_end_element_cb,
        unicode_annotations_text_cb,
        NULL,
        NULL
    };
    GMarkupParseContext *context = NULL;
    EmojiData data = { 0, };

    g_return_val_if_fail (filename != NULL, FALSE);
    g_return_val_if_fail (list != NULL, FALSE);

    if (!g_file_get_contents (filename, &content, &length, &error)) {
        g_warning ("Failed to load %s: %s", filename,
                   error ? error->message : "");
        goto failed_to_parse_unicode_annotations;
    }

    data.list = *list;
    data.is_derived = is_derived;

    context = g_markup_parse_context_new (&parser, 0, &data, NULL);
    if (!g_markup_parse_context_parse (context, content, length, &error)) {
        g_warning ("Failed to parse %s: %s", filename, error->message);
        goto failed_to_parse_unicode_annotations;
    }

    reset_emoji_element (&data);
    g_markup_parse_context_free (context);
    g_free (content);
    *list = data.list;
    return TRUE;

failed_to_parse_unicode_annotations:
    if (error)
        g_error_free (error);
    if (data.list)
        g_slist_free (data.list);
    if (context)
        g_markup_parse_context_free (context);
    g_free (content);
    return FALSE;
}

static gboolean
unicode_emoji_test_parse_unicode (const gchar *line,
                                  EmojiData   *data)
{
    GString *emoji = NULL;
    gchar *endptr = NULL;
    guint32 uch;
    static gchar outbuf[8] = { 0, };

    g_return_val_if_fail (line != NULL, FALSE);

    emoji = g_string_new (NULL);
    while (line && *line) {
        uch = g_ascii_strtoull (line, &endptr, 16);
        outbuf[g_unichar_to_utf8 (uch, outbuf)] = '\0';
        g_string_append (emoji, outbuf);
        if (*endptr == '\0') {
            break;
        }
        line = endptr + 1;
        while (*line == ' ')
            line++;
        endptr = NULL;
    }

    data->emoji = g_string_free (emoji, FALSE);
    return TRUE;
}

static gboolean
unicode_emoji_test_parse_description (const gchar *line,
                                      EmojiData   *data)
{
    g_return_val_if_fail (line != NULL, FALSE);

    /* skip spaces */
    while (*line == ' ')
        line++;
    /* skip emoji */
    while (*line != ' ')
        line++;
    /* skip spaces */
    while (*line == ' ')
        line++;
    if (*line == '\0')
        return FALSE;
    data->description = g_strdup (line);
    return TRUE;
}

#define EMOJI_VERSION_TAG "# Version: "
#define EMOJI_GROUP_TAG "# group: "
#define EMOJI_SUBGROUP_TAG "# subgroup: "
#define EMOJI_NON_FULLY_QUALIFIED_TAG "non-fully-qualified"

static gboolean
unicode_emoji_test_parse_line (const gchar *line,
                               EmojiData   *data)
{
    int tag_length;
    gchar **segments = NULL;

    g_return_val_if_fail (line != NULL, FALSE);

    tag_length = strlen (EMOJI_VERSION_TAG);
    if (strlen (line) > tag_length &&
        g_ascii_strncasecmp (line, EMOJI_VERSION_TAG, tag_length) == 0) {
        unicode_emoji_version = g_strdup (line + tag_length);
        return TRUE;
    }
    tag_length = strlen (EMOJI_GROUP_TAG);
    if (strlen (line) > tag_length &&
        g_ascii_strncasecmp (line, EMOJI_GROUP_TAG, tag_length) == 0) {
        g_free (data->category);
        g_clear_pointer (&data->subcategory, g_free);
        data->category = g_strdup (line + tag_length);
        return TRUE;
    }
    tag_length = strlen (EMOJI_SUBGROUP_TAG);
    if (strlen (line) > tag_length &&
        g_ascii_strncasecmp (line, EMOJI_SUBGROUP_TAG, tag_length) == 0) {
        g_free (data->subcategory);
        data->subcategory = g_strdup (line + tag_length);
        return TRUE;
    }
    if (*line == '#')
        return TRUE;
    segments = g_strsplit (line, "; ", 2);
    if (segments[1] == NULL) {
        g_warning ("No qualified line\n");
        goto failed_to_parse_unicode_emoji_test_line;
        return FALSE;
    }
    tag_length = strlen (EMOJI_NON_FULLY_QUALIFIED_TAG);
    /* Ignore the non-fully-qualified emoji */
    if (g_ascii_strncasecmp (segments[1], EMOJI_NON_FULLY_QUALIFIED_TAG,
                             tag_length) == 0) {
        g_strfreev (segments);
        return TRUE;
    }
    unicode_emoji_test_parse_unicode (segments[0], data);
    g_strfreev (segments);
    segments = g_strsplit (line, "# ", 2);
    if (segments[1] == NULL) {
        g_warning ("No description line\n");
        goto failed_to_parse_unicode_emoji_test_line;
        return FALSE;
    }
    unicode_emoji_test_parse_description (segments[1], data);
    g_strfreev (segments);
    if (data->annotations == NULL) {
        if (data->subcategory) {
            int i;
            gchar *amp;
            segments = g_strsplit(data->subcategory, "-", -1);
            for (i = 0; segments && segments[i]; i++) {
                if ((amp = strchr (segments[i], '&')) != NULL) {
                    if (amp - segments[i] <= 1) {
                        g_warning ("Wrong ampersand");
                        goto failed_to_parse_unicode_emoji_test_line;
                    }
                    data->annotations = g_slist_append (
                            data->annotations,
                            g_strndup (segments[i], amp - segments[i] - 1));
                    data->annotations = g_slist_append (
                            data->annotations,
                            g_strdup (amp + 1));
                    continue;
                }
                data->annotations = g_slist_append (data->annotations,
                                                    g_strdup (segments[i]));
            }
            g_strfreev (segments);
        } else {
            g_warning ("No subcategory line\n");
            goto failed_to_parse_unicode_emoji_test_line;
        }
    }
    update_emoji_list (data, TRUE);
    reset_emoji_element (data);
    return TRUE;

failed_to_parse_unicode_emoji_test_line:
    if (segments)
        g_strfreev (segments);
    reset_emoji_element (data);
    return FALSE;
}

#undef EMOJI_VERSION_TAG
#undef EMOJI_GROUP_TAG
#undef EMOJI_SUBGROUP_TAG
#undef EMOJI_NON_FULLY_QUALIFIED_TAG

static gboolean
unicode_emoji_test_parse_file (const gchar *filename,
                               GSList      **list)
{
    gchar *content = NULL;
    gsize length = 0;
    GError *error = NULL;
    gchar *head, *end, *line;
    int n = 1;
    EmojiData data = { 0, };

    g_return_val_if_fail (filename != NULL, FALSE);
    g_return_val_if_fail (list != NULL, FALSE);

    if (!g_file_get_contents (filename, &content, &length, &error)) {
        g_warning ("Failed to load %s: %s",
                   filename, error ? error->message : "");
        goto failed_to_parse_unicode_emoji_test;
    }
    head = end = content;
    while (*end == '\n' && end - content < length) {
        end++;
        n++;
    }
    head = end;
    data.list = *list;
    while (end - content < length) {
        while (*end != '\n' && end - content < length)
            end++;
        if (end - content >= length)
            break;
        line = g_strndup (head, end - head);
        if (!unicode_emoji_test_parse_line (line, &data))
            g_warning ("parse error #%d in %s version %s: %s",
                       n, filename,
                       unicode_emoji_version ? unicode_emoji_version : "(null)",
                       line);
        while (*end == '\n' && end - content < length) {
            end++;
            n++;
        }
        g_free (line);
        head = end;
    }
    g_free (content);
    g_free (unicode_emoji_version);
    *list = data.list;
    return TRUE;

failed_to_parse_unicode_emoji_test:
    if (error)
        g_error_free (error);
    g_clear_pointer (&content, g_free);
    return FALSE;
}

static gboolean 
unicode_emoji_parse_dir (const gchar *dirname,
                         GSList      **list)
{
    gchar *filename = NULL;
    g_return_val_if_fail (dirname != NULL, FALSE);
    g_return_val_if_fail (list != NULL, FALSE);

    filename = g_build_path ("/", dirname, "emoji-test.txt", NULL);
    if (!unicode_emoji_test_parse_file (filename, list)) {
        g_free (filename);
        return FALSE;
    }
    g_free (filename);
    return TRUE;
}

#ifdef HAVE_JSON_GLIB1
static gboolean
parse_emojione_unicode (JsonNode  *node,
                        EmojiData *data,
                        gboolean   is_alternates)
{
    const gchar *str, *unicode;
    gchar *endptr = NULL;
    guint32 uch;
    static gchar outbuf[8] = { 0, };
    GString *emoji;

    if (json_node_get_node_type (node) != JSON_NODE_VALUE) {
        g_warning ("'unicode' element is not string");
        return FALSE;
    }

    emoji = g_string_new (NULL);
    str = unicode = json_node_get_string (node);
    while (str && *str) {
        uch = g_ascii_strtoull (str, &endptr, 16);
        outbuf[g_unichar_to_utf8 (uch, outbuf)] = '\0';
        g_string_append (emoji, outbuf);
        if (*endptr == '\0') {
            break;
        } else {
            switch (*endptr) {
            case '-':
                endptr++;
                break;
            default:
                g_warning ("Failed to parse unicode %s", unicode);
            }
        }
        str = endptr;
        endptr = NULL;
    }

    if (is_alternates)
        data->emoji_alternates = g_string_free (emoji, FALSE);
    else
        data->emoji = g_string_free (emoji, FALSE);

    return TRUE;
}

static gboolean
parse_emojione_shortname (JsonNode  *node,
                          EmojiData *data)
{
#if 0
    const gchar *shortname;
    gchar *head, *s;
    int length;
    GSList *duplicated;

    if (json_node_get_node_type (node) != JSON_NODE_VALUE) {
        g_warning ("'shortname' element is not string");
        return FALSE;
    }

    /* The format is ':short_name:' */
    shortname = json_node_get_string (node);
    if (shortname == 0 || *shortname == '\0')
        return TRUE;
    if (*shortname != ':') {
        g_warning ("'shortname' format is different: %s", shortname);
        return FALSE;
    }

    length = strlen (shortname);
    head  = g_new0 (gchar, length);
    strcpy (head, shortname + 1);
    for (s = head; *s; s++) {
        if (*s == ':') {
            *s = '\0';
            break;
        } else if (*s == '_') {
            *s = ' ';
        }
    }

    if (head == NULL || *head == '\0') {
        g_warning ("'shortname' format is different: %s", shortname);
        g_free (head);
        return FALSE;
    }

    duplicated = g_slist_find_custom (data->annotations,
                                      head,
                                      (GCompareFunc) g_strcmp0);
    if (duplicated == NULL) {
        data->annotations = g_slist_prepend (data->annotations,
                                             head);
    } else {
       g_free (head);
    }

#endif
    return TRUE;
}

static gboolean
parse_emojione_name (JsonNode  *node,
                     EmojiData *data)
{
    const gchar *name;

    if (json_node_get_node_type (node) != JSON_NODE_VALUE) {
        g_warning ("'name' element is not string");
        return FALSE;
    }

    name = json_node_get_string (node);

    if (name == NULL || *name == '\0')
        return TRUE;

    data->description = g_strdup (name);

    return TRUE;
}

static gboolean
parse_emojione_category (JsonNode  *node,
                         EmojiData *data)
{
    const gchar *category;
    GSList *duplicated;

    if (json_node_get_node_type (node) != JSON_NODE_VALUE) {
        g_warning ("'category' element is not string");
        return FALSE;
    }

    category = json_node_get_string (node);

    if (category == NULL || *category == '\0')
        return TRUE;

    data->category = g_strdup (category);
    duplicated = g_slist_find_custom (data->annotations,
                                      category,
                                      (GCompareFunc) g_strcmp0);
    if (duplicated == NULL) {
        data->annotations = g_slist_prepend (data->annotations,
                                             g_strdup (category));
    }

    return TRUE;
}

#ifdef EMOJIONE_ALIASES_ASCII_PRINT
static gchar *
text_to_entity (const gchar *text)
{
    gchar *p;
    GString *buff = g_string_new (NULL);
    for (p = text; *p; p++) {
        switch (*p) {
        case '<':
            g_string_append (buff, "&lt;");
            break;
        case '>':
            g_string_append (buff, "&gt;");
            break;
        case '&':
            g_string_append (buff, "&amp;");
            break;
        default:
            g_string_append_c (buff, *p);
        }
    }
    g_string_free (buff, FALSE);
}
#endif

static gboolean
parse_emojione_aliases_ascii (JsonNode  *node,
                              EmojiData *data)
{
    JsonArray *aliases_ascii;
    guint i, length;

    if (json_node_get_node_type (node) != JSON_NODE_ARRAY) {
        g_warning ("'aliases_ascii' element is not array");
        return FALSE;
    }

    aliases_ascii = json_node_get_array (node);
    length = json_array_get_length (aliases_ascii);
    for (i = 0; i < length; i++) {
#ifdef EMOJIONE_ALIASES_ASCII_PRINT
        if (i == 0)
            printf ("        <annotation cp=\"%s\">", data->emoji);
#endif
        const gchar *alias = json_array_get_string_element (aliases_ascii, i);
        GSList *duplicated = g_slist_find_custom (data->annotations,
                                                  alias,
                                                  (GCompareFunc) g_strcmp0);
        if (duplicated == NULL) {
#ifdef EMOJIONE_ALIASES_ASCII_PRINT
            gchar *entity = text_to_entity (alias);
            if (i != length - 1)
                printf ("%s | ", entity);
            else
                printf ("%s</annotation>\n", entity);
            g_free (entity);
#endif
            data->annotations = g_slist_prepend (data->annotations,
                                                 g_strdup (alias));
        }
    }

    return TRUE;
}

static gboolean
parse_emojione_keywords (JsonNode  *node,
                         EmojiData *data)
{
#if 0
    JsonArray *keywords;
    guint i, length;

    if (json_node_get_node_type (node) != JSON_NODE_ARRAY) {
        g_warning ("'keywords' element is not array");
        return FALSE;
    }

    keywords = json_node_get_array (node);
    length = json_array_get_length (keywords);
    for (i = 0; i < length; i++) {
        const gchar *keyword = json_array_get_string_element (keywords, i);
        GSList *duplicated = g_slist_find_custom (data->annotations,
                                                  keyword,
                                                  (GCompareFunc) g_strcmp0);
        if (duplicated == NULL) {
            data->annotations = g_slist_prepend (data->annotations,
                                                 g_strdup (keyword));
        }
    }

#endif
    return TRUE;
}

static gboolean
parse_emojione_emoji_data (JsonNode    *node,
                           const gchar *member,
                           EmojiData   *data)
{
    if (g_strcmp0 (member, "unicode") == 0)
        return parse_emojione_unicode (node, data, FALSE);
    else if (g_strcmp0 (member, "unicode_alt") == 0)
        return parse_emojione_unicode (node, data, TRUE);
    else if (g_strcmp0 (member, "unicode_alternates") == 0)
        return parse_emojione_unicode (node, data, TRUE);
    else if (g_strcmp0 (member, "shortname") == 0)
        return parse_emojione_shortname (node, data);
    else if (g_strcmp0 (member, "name") == 0)
        return parse_emojione_name (node, data);
    else if (g_strcmp0 (member, "category") == 0)
        return parse_emojione_category (node, data);
    else if (g_strcmp0 (member, "aliases_ascii") == 0)
        return parse_emojione_aliases_ascii (node, data);
    else if (g_strcmp0 (member, "keywords") == 0)
        return parse_emojione_keywords (node, data);
    return TRUE;
}

static gboolean
parse_emojione_element (JsonNode  *node,
                        EmojiData *data)
{
    JsonObject *object;
    GList *members, *m;

    if (json_node_get_node_type (node) != JSON_NODE_OBJECT) {
            return FALSE;
    }

    reset_emoji_element (data);

    object = json_node_get_object (node);
    m = members = json_object_get_members (object);
    while (m) {
       const gchar *member = (const gchar *) m->data;
       if (!parse_emojione_emoji_data (json_object_get_member (object, member),
                                       member,
                                       data)) {
           g_list_free (members);
           return FALSE;
       }
       m = m->next;
    }
    g_list_free (members);

    update_emoji_list (data, TRUE);

    return TRUE;
}

static gboolean
emojione_parse_json_file (const gchar  *filename,
                          GSList      **list)
{
    JsonParser *parser = json_parser_new ();
    JsonNode *node;
    JsonObject *object;
    GList *members, *m;
    GError *error = NULL;
    EmojiData data = { 0, };

    g_return_val_if_fail (filename != NULL, FALSE);
    g_return_val_if_fail (list != NULL, FALSE);

    if (!json_parser_load_from_file (parser, filename, &error)) {
        g_error ("%s", error->message);
        g_error_free (error);
        goto fail_to_json_file;
    }

    node = json_parser_get_root (parser);
    if (json_node_get_node_type (node) != JSON_NODE_OBJECT) {
        g_warning ("Json file does not have Json object %s", filename);
        goto fail_to_json_file;
    }

    object = json_node_get_object (node);
    members = json_object_get_members (object);
    data.list = *list;

    m = members;
    while (m) {
       const gchar *member = (const gchar *) m->data;
       if (!parse_emojione_element (json_object_get_member (object, member),
                                    &data)) {
           g_warning ("Failed to parse member '%s' in %s", member, filename);
       }
       m = m->next;
    }

    g_list_free (members);
    reset_emoji_element (&data);
    g_object_unref (parser);
    *list = data.list;

    return TRUE;

fail_to_json_file:
    g_object_unref (parser);
    return FALSE;
}
#endif /* HAVE_JSON_GLIB1 */

static void
emoji_data_list_unify_categories (IBusEmojiData  *data,
                                  GSList        **list)
{
    g_return_if_fail (IBUS_IS_EMOJI_DATA (data));
    g_return_if_fail (list != NULL);

    const gchar *category = ibus_emoji_data_get_category (data);
    if (*category == '\0')
        return;
    if (g_slist_find_custom (*list, category, (GCompareFunc)g_strcmp0) == NULL)
        *list = g_slist_append (*list, g_strdup (category));
}

static void
category_list_dump (const gchar *category,
                    GString     *buff)
{
    g_return_if_fail (buff != NULL);

    const gchar *line = g_strdup_printf ("    N_(\"%s\"),\n", category);
    g_string_append (buff, line);
}

static void
category_file_save (const gchar *filename,
                    GSList      *list)
{
    gchar *content = NULL;
    gsize length = 0;
    GError *error = NULL;
    gchar *p;
    GString *buff = NULL;
    int i;
    GSList *list_categories = NULL;

    g_return_if_fail (filename != NULL);
    g_return_if_fail (list != NULL);

    g_slist_foreach (list, (GFunc)emoji_data_list_unify_categories, &list_categories);
    if (list_categories == NULL) {
        g_warning ("Not found categories in IBusEmojiData list");
        return;
    }

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
        g_string_append (buff, g_strndup (content, p - content));
        g_string_append_c (buff, '\n');
    }
    g_clear_pointer (&content, g_free);

    g_string_append (buff, g_strdup ("\n"));
    g_string_append (buff, g_strdup_printf ("/* This file is generated by %s. */", __FILE__));
    g_string_append (buff, g_strdup ("\n"));
    g_string_append (buff, g_strdup ("include <glib/gi18n.h>\n"));
    g_string_append (buff, g_strdup ("\n"));
    g_string_append (buff, g_strdup ("#ifndef __IBUS_EMOJI_GEN_H_\n"));
    g_string_append (buff, g_strdup ("#define __IBUS_EMOJI_GEN_H_\n"));
    g_string_append (buff, g_strdup ("const static char *unicode_emoji_categories[] = {\n"));
    list_categories = g_slist_sort (list_categories, (GCompareFunc)g_strcmp0);
    g_slist_foreach (list_categories, (GFunc)category_list_dump, buff);
    g_slist_free (list_categories);
    g_string_append (buff, g_strdup ("};\n"));
    g_string_append (buff, g_strdup ("#endif\n"));

    if (!g_file_set_contents (filename, buff->str, -1, &error)) {
        g_warning ("Failed to save emoji category file %s: %s", filename, error->message);
        g_error_free (error);
    }

    g_string_free (buff, TRUE);
}

int
main (int argc, char *argv[])
{
    gchar *prgname;
#ifdef HAVE_JSON_GLIB1
    gchar *json_file = NULL;
#endif
    gchar *emoji_dir = NULL;
    gchar *xml_file = NULL;
    gchar *xml_derived_file = NULL;
    gchar *xml_ascii_file = NULL;
    gchar *output = NULL;
    gchar *output_category = NULL;
    GOptionEntry     entries[] = {
#ifdef HAVE_JSON_GLIB1
        { "json", 'j', 0, G_OPTION_ARG_STRING, &json_file,
          "Parse Emoji One JSON file",
          "JSON"
        },
#endif
        { "unicode-emoji-dir", 'd', 0, G_OPTION_ARG_STRING, &emoji_dir,
          "Parse Emoji files in DIRECTORY which includes emoji-test.txt " \
          "emoji-sequences.txt emoji-zwj-sequences.txt in unicode.org",
          "DIRECTORY"
        },
        { "out", 'o', 0, G_OPTION_ARG_STRING, &output,
          "Save the emoji data as FILE",
          "FILE"
        },
        { "out-category", 'C', 0, G_OPTION_ARG_STRING, &output_category,
          "Save the translatable categories as FILE",
          "FILE"
        },
        { "xml", 'x', 0, G_OPTION_ARG_STRING, &xml_file,
          "Parse Unocode.org ANNOTATIONS file",
          "ANNOTATIONS"
        },
        { "xml-derived", 'X', 0, G_OPTION_ARG_STRING, &xml_derived_file,
          "Parse Unocode.org derived ANNOTATIONS file",
          "ANNOTATIONS"
        },
        { "xml-ascii", 'A', 0, G_OPTION_ARG_STRING, &xml_ascii_file,
          "Parse ASCII ANNOTATIONS file",
          "ANNOTATIONS"
        },
        { NULL }
    };
    GOptionContext *context;
    GError *error = NULL;
    GSList *list = NULL;
    gboolean is_en = TRUE;

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

#ifdef HAVE_JSON_GLIB1
    if (json_file)
        emojione_parse_json_file (json_file, &list);
#endif
    if (emoji_dir)
        unicode_emoji_parse_dir (emoji_dir, &list);
    if (list) {
#define CHECK_IS_EN(file) if ((file)) {                                     \
    gchar *basename = g_path_get_basename ((file));                         \
    is_en = (g_ascii_strncasecmp (basename, "en.", 3) == 0) ?               \
            TRUE : FALSE;                                                   \
    g_free (basename);                                                      \
}

        CHECK_IS_EN(xml_derived_file);
        CHECK_IS_EN(xml_file);
#undef CHECK_IS_EN

        /* Use English emoji-test.txt to get fully-qualified. */
        if (!is_en)
            g_slist_foreach (list, (GFunc)init_annotations, NULL);
    }
    if (xml_file)
        unicode_annotations_parse_xml_file (xml_file, &list, FALSE);
    if (xml_derived_file)
        unicode_annotations_parse_xml_file (xml_derived_file, &list, TRUE);
    if (xml_ascii_file)
        unicode_annotations_parse_xml_file (xml_ascii_file, &list, FALSE);
    if (list != NULL && !is_en) {
        /* If emoji-test.txt has an emoji but $lang.xml does not, clear it
         * since the language dicts do not want English annotations.
         */
        NoTransData no_trans_data = {
            xml_file,
            xml_derived_file,
            NULL
        };
        g_slist_foreach (list, (GFunc)check_no_trans, &no_trans_data);
        if (no_trans_data.emoji_list) {
            g_slist_foreach (no_trans_data.emoji_list,
                             (GFunc)delete_emoji_from_list,
                             &list);
            g_slist_free_full (no_trans_data.emoji_list, g_free);
        }
    }
    if (list != NULL && output)
        ibus_emoji_data_save (output, list);
    if (list != NULL && output_category)
        category_file_save (output_category, list);
    if (list)
        g_slist_free (list);
    else
        return 99;

    return 0;
}
