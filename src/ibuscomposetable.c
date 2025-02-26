/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* ibus - The Input Bus
 * Copyright (C) 2013-2014 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2013-2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
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
#include <stdlib.h>
#include <string.h>

#include "ibuscomposetable.h"
#include "ibuserror.h"
#include "ibusenginesimple.h"
#include "ibuskeys.h"
#include "ibuskeysyms.h"
#include "ibustypes.h"

#include "ibusenginesimpleprivate.h"


#define IBUS_COMPOSE_TABLE_MAGIC "IBusComposeTable"
#define IBUS_COMPOSE_TABLE_VERSION (4)
#define IBUS_MAX_COMPOSE_ALGORITHM_LEN 9

typedef struct {
  gunichar     *sequence;
  gunichar     *values;
  char         *comment;
} IBusComposeData;


static void
ibus_compose_data_free (IBusComposeData *compose_data)
{
    g_free (compose_data->sequence);
    g_free (compose_data->values);
    g_free (compose_data->comment);
    g_slice_free (IBusComposeData, compose_data);
}


static void
ibus_compose_list_element_free (IBusComposeData *compose_data,
                                gpointer         data)
{
    ibus_compose_data_free (compose_data);
}


static guint
unichar_length (gunichar *uni_array)
{
    guint i = 0;
    g_return_val_if_fail (uni_array, 0);
    while (uni_array[i])
        ++i;
    return i;
}


static gboolean
is_codepoint (const char *str)
{
    int i;

    /* 'U' is not code point but 'U00C0' is code point */
    if (str[0] == '\0' || str[0] != 'U' || str[1] == '\0')
        return FALSE;

    for (i = 1; str[i] != '\0'; i++) {
        if (!g_ascii_isxdigit (str[i]))
            return FALSE;
    }

    return TRUE;
}


static gboolean
parse_compose_value (IBusComposeData  *compose_data,
                     const char       *val,
                     const char       *line)
{
    char *head, *end, *p;
    char *ustr = NULL;
    gunichar *uchars = NULL, *up;
    GError *error = NULL;
    int n_uchars = 0;

    if (!(head = strchr (val, '\"'))) {
        g_warning ("Need to double-quote the value: %s: %s", val, line);
        goto fail;
    }
    ++head;
    p = head;
    end = NULL;
    while ((*p != '\0') && (end = strchr (p, '\"'))) {
        if (*(end - 1) == '\\' && *(end - 2) == '\\')
            break;
        if (*(end - 1) != '\\')
            break;
        p = end + 1;
    }
    if (end == NULL || *p == '\0') {
        g_warning ("Need to double-quote the value: %s: %s", val, line);
        goto fail;
    }
    ustr = g_strndup (head, end - head);
    p = ustr + 1;
    /* The escaped octal */
    if (*ustr == '\\' && *p >= '0' && *p <= '8') {
        compose_data->values = g_new (gunichar, 2);
        compose_data->values[0] = g_ascii_strtoll(p, NULL, 8);
        compose_data->values[1] = 0;
    } else {
        if (!(uchars = g_utf8_to_ucs4 (ustr, -1, NULL, NULL, &error))) {
            g_warning ("Invalid Unicode: %s: %s in %s:",
                       error->message, ustr, line);
            g_error_free (error);
            goto fail;
        } else if (!uchars[0]) {
            g_warning ("Invalid Unicode: \"\" in %s:", line);
            goto fail;
        }

        for (up = uchars; *up; up++) {
            if (*up == '\\') {
                ++up;
                if (*up != '"' && *up != '\\') {
                    g_warning ("Invalid backslash: %s: %s", val, line);
                    goto fail;
                }
            }
            if (!compose_data->values) {
                compose_data->values = g_new (gunichar, 2);
            } else {
                compose_data->values = g_renew (gunichar,
                                                compose_data->values,
                                                n_uchars + 2);
            }
            compose_data->values[n_uchars++] = *up;
        }
        compose_data->values[n_uchars] = 0;
    }

    g_free (ustr);
    g_free (uchars);
    compose_data->comment = g_strdup (g_strstrip (end + 1));

    return TRUE;

fail:
    g_free (ustr);
    g_free (uchars);
    return FALSE;
}


static int
parse_compose_sequence (IBusComposeData *compose_data,
                        const char      *seq,
                        const char      *line)
{
    char **words = g_strsplit (seq, "<", -1);
    int i;
    int n = 0;

    if (g_strv_length (words) < 2) {
        g_warning ("too few words; key sequence format is <a> <b>...: %s",
                   line);
        goto fail;
    }

    for (i = 1; words[i] != NULL; i++) {
        char *start = words[i];
        char *end = strchr (start, '>');
        char *match;
        gunichar codepoint;

        if (words[i][0] == '\0')
             continue;

        if (start == NULL || end == NULL || end <= start) {
            g_warning ("key sequence format is <a> <b>...: %s", line);
            goto fail;
        }

        match = g_strndup (start, end - start);

        if (compose_data->sequence == NULL) {
            compose_data->sequence = g_new (gunichar, 2);
        } else {
            compose_data->sequence = g_renew (gunichar,
                                              compose_data->sequence,
                                              n + 2);
        }

        if (is_codepoint (match)) {
            codepoint = (gunichar) g_ascii_strtoll (match + 1, NULL, 16);
            compose_data->sequence[n] = codepoint;
        } else {
            codepoint = (gunichar) ibus_keyval_from_name (match);
            compose_data->sequence[n] = codepoint;
        }

        if (codepoint >= 0x10000) {
            if (!ibus_compose_key_flag (0xffff & codepoint)) {
                g_warning ("The keysym %s > 0xffff is not supported: %s",
                           match, line);
            }
        }
        if (codepoint == IBUS_KEY_VoidSymbol) {
            g_warning ("Could not get code point of keysym %s: %s",
                       match, line);
            g_free (match);
            goto fail;
        }
        g_free (match);
        n++;
    }
    if (compose_data->sequence)
        compose_data->sequence[n] = 0;

    g_strfreev (words);
    if (0 == n || n >= IBUS_MAX_COMPOSE_LEN) {
        g_warning ("The max number of sequences is %d: %s",
                   IBUS_MAX_COMPOSE_LEN, line);
        return -1;
    }

    return n;

fail:
    g_strfreev (words);
    return -1;
}


static char *
expand_include_path (const char *include_path) {
    char *out = strdup ("");
    const char *head, *i;
    char *former, *o;

    for (head = i = include_path; *i; ++i) {
        /* expand sequence */
        if (*i == '%') {
            switch (*(i + 1)) {
            case 'H': { /* $HOME */
                const char *home = getenv ("HOME");
                if (!home) {
                    g_warning ("while parsing XCompose include target %s, "
                               "%%H replacement failed because HOME is not "
                               "defined; the include has been ignored",
                               include_path);
                    goto fail;
                }
                o = out;
                former = g_strndup (head, i - head);
                out = g_strdup_printf ("%s%s%s", o, former, home);
                head = i + 2;
                g_free (o);
                g_free (former);
                break;
            }
            case 'L': /* locale compose file */
                g_warning ("while handling XCompose include target %s, found "
                          "redundant %%L include; the include has been "
                          "ignored", include_path);
                goto fail;
            case 'S': /* system compose dir */
                o = out;
                former = g_strndup (head, i - head);
                out = g_strdup_printf ("%s%s%s", o, former, X11_LOCALEDATADIR);
                head = i + 2;
                g_free (o);
                g_free (former);
                break;
            case '%': /* escaped % */
                o = out;
                former = g_strndup (head, i - head);
                out = g_strdup_printf ("%s%s%s", o, former, "%");
                head = i + 2;
                g_free (o);
                g_free (former);
                break;
            default:
                g_warning ("while parsing XCompose include target %s, found "
                           "unknown substitution character '%c'; the include "
                           "has been ignored", include_path, *(i+1));
                goto fail;
            }
            ++i;
        }
    }
    o = out;
    out = g_strdup_printf ("%s%s", o, head);
    g_free (o);
    return out;
fail:
    g_free (out);
    return NULL;
}


static void
parse_compose_line (GList       **compose_list,
                    const char   *line,
                    int          *compose_len,
                    char        **include)
{
    char **components = NULL;
    IBusComposeData *compose_data = NULL;
    int l;

    g_assert (compose_len);
    *compose_len = 0;

    /* eat spaces at the start of the line */
    while (*line && (*line == ' ' || *line == '\t')) line++;

    if (line[0] == '\0' || line[0] == '#')
        return;

    if (g_str_has_prefix (line, "include ")) {
        const char *rest = line + sizeof ("include ") - 1;
        while (*rest && *rest == ' ') rest++;

        /* grabbed the path part of the line */
        char *rest2;
        if (*rest == '"') {
            rest++;
            rest2 = g_strdup (rest);
            /* eat the closing quote */
            char *i = rest2;
            while (*i && *i != '"') i++;
            *i = '\0';
        } else {
            rest2 = g_strdup (rest);
        }
        *include = expand_include_path (rest2);
        g_free (rest2);
        return;
    }

    components = g_strsplit (line, ":", 2);

    if (components[1] == NULL) {
        g_warning ("No delimiter ':': %s", line);
        goto fail;
    }

    compose_data = g_slice_new0 (IBusComposeData);

    if ((l = parse_compose_sequence (compose_data,
                                     g_strstrip (components[0]),
                                     line)) < 1) {
        goto fail;
    }
    *compose_len = l;

    if (!parse_compose_value (compose_data, g_strstrip (components[1]), line))
        goto fail;

    g_strfreev (components);

    *compose_list = g_list_append (*compose_list, compose_data);

    return;

fail:
    g_strfreev (components);
    if (compose_data)
        ibus_compose_data_free (compose_data);
}


static char *
get_en_compose_file (void)
{
    char * const sys_langs[] = { "en_US.UTF-8", "en_US", "en.UTF-8",
                                 "en", NULL };
    char * const *sys_lang = NULL;
    char *path = NULL;
    for (sys_lang = sys_langs; *sys_lang; sys_lang++) {
        path = g_build_filename (X11_LOCALEDATADIR, *sys_lang, "Compose", NULL);
        if (g_file_test (path, G_FILE_TEST_EXISTS))
            break;
        g_clear_pointer (&path, g_free);
    }
    return path;
}


static GList *
ibus_compose_list_parse_file (const char *compose_file,
                              int        *max_compose_len)
{
    char *contents = NULL;
    char **lines = NULL;
    gsize length = 0;
    GError *error = NULL;
    GList *compose_list = NULL;
    int i;

    g_assert (max_compose_len);

    if (!g_file_get_contents (compose_file, &contents, &length, &error)) {
        g_error ("%s", error->message);
        g_error_free (error);
        return NULL;
    }

    lines = g_strsplit (contents, "\n", -1);
    g_free (contents);
    for (i = 0; lines[i] != NULL; i++) {
        int compose_len = 0;
        char *include = NULL;
        parse_compose_line (&compose_list, lines[i], &compose_len, &include);
        if (*max_compose_len < compose_len)
            *max_compose_len = compose_len;
        if (include && *include) {
            GStatBuf buf_include = { 0, };
            GStatBuf buf_parent = { 0, };
            char *en_compose;
            errno = 0;
            if (g_stat (include,  &buf_include)) {
                g_warning ("Cannot access %s: %s",
                           include,
                           g_strerror (errno));
                g_clear_pointer (&include, g_free);
                continue;
            }
            errno = 0;
            if (g_stat (compose_file,  &buf_parent)) {
                g_warning ("Cannot access %s: %s",
                           compose_file,
                           g_strerror (errno));
                g_clear_pointer (&include, g_free);
                continue;
            }
            if (buf_include.st_ino == buf_parent.st_ino) {
                g_warning ("Found recursive nest same file %s", include);
                g_clear_pointer (&include, g_free);
                continue;
            }
            en_compose = get_en_compose_file ();
            if (en_compose) {
                errno = 0;
                if (g_stat (en_compose,  &buf_parent)) {
                    g_warning ("Cannot access %s: %s",
                               compose_file,
                               g_strerror (errno));
                    g_clear_pointer (&include, g_free);
                    g_free (en_compose);
                    continue;
                }
            }
            g_free (en_compose);
            if (buf_include.st_ino == buf_parent.st_ino) {
                g_message ("System en_US Compose is already loaded %s",
                           include);
                g_clear_pointer (&include, g_free);
                continue;
            }
        }
        if (include && *include) {
            GList *rest = ibus_compose_list_parse_file (
                    include,
                    max_compose_len);
            if (rest) {
                compose_list = g_list_concat (compose_list, rest);
            }
        }
        g_clear_pointer (&include, g_free);
    }
    g_strfreev (lines);

    return compose_list;
}


static GList *
ibus_compose_list_check_duplicated_with_en (GList  *compose_list,
                                            int     max_compose_len,
                                            GSList *compose_tables)
{
    GList *list;
    static guint *keysyms;
    GList *removed_list = NULL;
    IBusComposeData *compose_data;
    GString *output = g_string_new ("");

    keysyms = g_new (guint, max_compose_len + 1);

    for (list = compose_list; list != NULL; list = list->next) {
        int i;
        int n_compose = 0;
        gboolean is_32bit;
        guint n_outputs;
        GSList *tmp_list;
        gboolean compose_finish = FALSE;
        gboolean compose_match = FALSE;
        gboolean success = FALSE;
        gunichar output_char = 0;

        compose_data = list->data;

        for (i = 0; i < max_compose_len + 1; i++)
            keysyms[i] = 0;

        for (i = 0; i < max_compose_len + 1; i++) {
            gunichar codepoint = compose_data->sequence[i];
            keysyms[i] = (guint)codepoint;

            if (codepoint == 0)
                break;

            n_compose++;
        }

        n_outputs = unichar_length (compose_data->values);
        is_32bit = (n_outputs > 1) ? TRUE :
                (compose_data->values[0] >= 0xFFFF) ? TRUE : FALSE;
        g_string_erase (output, 0, -1);
        tmp_list = compose_tables;
        while (tmp_list) {
            is_32bit = FALSE;
            if (ibus_compose_table_check (
                (IBusComposeTableEx *)tmp_list->data,
                keysyms,
                n_compose,
                &compose_finish,
                &compose_match,
                output,
                is_32bit) && compose_finish && compose_match) {
                if (compose_data->values[0] == g_utf8_get_char (output->str)) {
                    success = TRUE;
                    break;
                }
            }
            is_32bit = TRUE;
            if (ibus_compose_table_check (
                (IBusComposeTableEx *)tmp_list->data,
                keysyms,
                n_compose,
                &compose_finish,
                &compose_match,
                output,
                is_32bit) && compose_finish && compose_match) {
                int j = 0;
                gchar *str = output->str;
                while (j < n_outputs && compose_data->values[j]) {
                    gunichar ch = g_utf8_get_char (str);
                    if (compose_data->values[j] != ch)
                        break;
                    str = g_utf8_next_char (str);
                    ++j;
                }
                if (j == n_outputs && *str == '\0') {
                    success = TRUE;
                    break;
                }
            }
            tmp_list = tmp_list->next;
        }
        if (success) {
            removed_list = g_list_append (removed_list, compose_data);
        } else if (ibus_check_algorithmically (keysyms,
                                               n_compose,
                                               &output_char)) {
            if (compose_data->values[0] == output_char)
                removed_list = g_list_append (removed_list, compose_data);
        }
    }
    g_string_free (output, TRUE);

    for (list = removed_list; list != NULL; list = list->next) {
        compose_data = list->data;
        compose_list = g_list_remove (compose_list, compose_data);
        ibus_compose_data_free (compose_data);
    }

    g_list_free (removed_list);
    g_free (keysyms);

    return compose_list;
}


/*
 * Actual typed keysyms have a flag against the definition.
 * https://gitlab.freedesktop.org/xorg/proto/xorgproto/-/blob/master/include/X11/keysymdef.h?ref_type=heads#L82
 * https://gitlab.freedesktop.org/xorg/lib/libx11/-/blob/master/nls/en_US.UTF-8/Compose.pre#L4559
 */
guint
ibus_compose_key_flag (guint key)
{
    const char *name;
    if (key <= 0xff)
        return 0;
    /* en-US has MUSICAL SYMBOL compose table */
    if (key >= 0xd143 && key <= 0xd1e8)
        return 0x10000;
    switch (key) {
    /* <Aogonek> is not used in UTF-8 compose sequences but <ohorn> in EN
     * compose file and vn keymap is assumed instead.
     */
    case 0x1a1:
    /* <Zabovedot> is not used in UTF-8 compose sequences but <Uhorn> in EN
     * compose file and vn keymap s assumed instead.
     */
    case 0x1af:
    /* <caron> is not used in UTF-8 compose sequences but <EZH> in EN compose
     * file and fr(nodeadkeys) keymap is assumed instead.
     */
    case 0x1b7:
        return 0x1000000;
    default:;
    }
    name = ibus_keyval_name (key);
    /* If name is null, the key sequence is expressed as "<Uxxxx>" format in
     * the Compose file and the typed keysym has the flag.
     */
    if (!name || g_str_has_prefix (name, "0x"))
        return 0x1000000;
    /* "<Pointer_EnableKeys>" is not described in the Compose file but <UFEF9>
     * in the file.
     */
    if (*name == 'P' && *(name + 1) == 'o' && *(name + 2) == 'i')
        return 0x1000000;
    /* Other names, "<Multi_key>", "<dead_*>", "<Cyrillic_*>" are described in
     * the Compose file.
     */
    return 0;
}


static int
ibus_compose_data_compare (gpointer a,
                           gpointer b,
                           gpointer data)
{
    IBusComposeData *compose_data_a = a;
    IBusComposeData *compose_data_b = b;
    int max_compose_len = GPOINTER_TO_INT (data);
    int i;
    /* The allocation length of compose_data_a->sequence[] is different from
     * one of compose_data_b->sequence[] and max_compose_len indicates
     * the sequence length only but not include the compose value length.
     * So max_compose_len is greater than any allocation lengths of sequence[]
     * and this API should return if code_a or code_b is 0.
     */
    for (i = 0; i < max_compose_len; i++) {
        gunichar code_a = compose_data_a->sequence[i];
        gunichar code_b = compose_data_b->sequence[i];

        code_a &= 0xffff;
        code_b &= 0xffff;
        if (code_a != code_b)
            return code_a - code_b;
        if (code_a == 0 && code_b == 0)
            return 0;
    }
    return 0;
}


static GList *
ibus_compose_list_check_duplicated_with_own (GList  *compose_list,
                                             int     max_compose_len)
{
    GList *list;
    IBusComposeData *compose_data_a, *compose_data_b;
    int i;
    GList *removed_list = NULL;

    for (list = compose_list; list != NULL; list = list->next) {
        gboolean is_different_value = FALSE;
        if (!list->next)
            break;
        if (!ibus_compose_data_compare (list->data,
                                        list->next->data,
                                        GINT_TO_POINTER (max_compose_len))) {
            compose_data_a = list->data;
            compose_data_b = list->next->data;
            for (i = 0; compose_data_a->values[i]; i++) {
                if (compose_data_a->values[i] != compose_data_b->values[i]) {
                    is_different_value = TRUE;
                    break;
                }
            }
            if (is_different_value)
                g_warning ("Deleting different outputs for same sequence.");
            else
                g_debug ("Deleting same compose output for same sequence.");
            g_print ("{");
            for (i = 0; compose_data_a->values[i]; i++)
                g_print ("U+%X, ", compose_data_a->values[i]);
            g_print ("}");
            g_print (" {");
            for (i = 0; compose_data_b->values[i]; i++)
                g_print ("U+%X, ", compose_data_b->values[i]);
            g_print ("}\n");
            removed_list = g_list_append (removed_list, compose_data_a);
        }
    }
    for (list = removed_list; list != NULL; list = list->next) {
        compose_data_a = list->data;
        compose_list = g_list_remove (compose_list, compose_data_a);
        ibus_compose_data_free (compose_data_a);
    }

    g_list_free (removed_list);
    return compose_list;
}


static void
ibus_compose_list_print (GList *compose_list,
                         int    max_compose_len,
                         int    n_index_stride)
{
    GList *list;
    int i, j;
    IBusComposeData *compose_data;
    int total_size = 0;
    const char *keyval;

    for (list = compose_list; list != NULL; list = list->next) {
        compose_data = list->data;
        g_printf ("  ");

        for (i = 0; i < max_compose_len; i++) {

            if (compose_data->sequence[i] == 0) {
                for (j = i; j < max_compose_len; j++) {
                    if (i == max_compose_len -1)
                        g_printf ("0,\n");
                    else
                        g_printf ("0, ");
                }
                break;
            }

            keyval = ibus_keyval_name (compose_data->sequence[i]);
            if (i == max_compose_len -1)
                g_printf ("%s,\n", keyval ? keyval : "(null)");
            else
                g_printf ("%s, ", keyval ? keyval : "(null)");
        }
        g_printf ("    ");
        for (i = 0; compose_data->values[i]; ++i)
            g_printf ("%#06X, ", compose_data->values[i]);
        g_printf (" /* %s */,\n", compose_data->comment);

        total_size += n_index_stride;
    }

    g_printerr ("TOTAL_SIZE: %d\nMAX_COMPOSE_LEN: %d\nN_INDEX_STRIDE: %d\n",
                total_size, max_compose_len, n_index_stride);
}


/* Implemented from g_str_hash() */
static guint32
ibus_compose_table_data_hash (gconstpointer v,
                              int           length)
{
    const guint16 *p, *head;
    unsigned char c;
    guint32 h = 5381;

    for (p = v, head = v; (p - head) < length; p++) {
        c = 0x00ff & (*p >> 8);
        h = (h << 5) + h + c;
        c = 0x00ff & *p;
        h = (h << 5) + h + c;
    }

    return h;
}


static char *
ibus_compose_hash_get_cache_path (guint32 hash)
{
    char *basename = NULL;
    const char *cache_dir;
    char *dir = NULL;
    char *path = NULL;

    basename = g_strdup_printf ("%08x.cache", hash);

    if ((cache_dir = g_getenv ("IBUS_COMPOSE_CACHE_DIR"))) {
        dir = g_strdup (cache_dir);
    } else {
        dir = g_build_filename (g_get_user_cache_dir (),
                                "ibus", "compose", NULL);
    }
    path = g_build_filename (dir, basename, NULL);
    errno = 0;
    if (g_mkdir_with_parents (dir, 0755)) {
        g_warning ("Failed to mkdir %s: %s", dir, g_strerror (errno));
        g_clear_pointer (&path, g_free);
    }

    g_free (dir);
    g_free (basename);

    return path;
}


static GVariant *
compose_data_to_variant (gconstpointer compose_data,
                         gboolean is_32bit,
                         guint16  index_stride,
                         gsize    n_seqs,
                         gboolean reverse_endianness,
                         GError **error)
{
    guint16 data16;
    guint32 data32;
    guint16 *target_data16 = NULL;
    guint32 *target_data32 = NULL;
    gsize i, length;
    GVariant *variant_data = NULL;

    g_assert (compose_data);
    if (error)
        *error = NULL;
    if (n_seqs == 0 || index_stride > (G_MAXSIZE / n_seqs)) {
        if (error) {
            g_set_error (error, IBUS_ERROR, IBUS_ERROR_FAILED,
                         "Length %u x %lu is too long",
                         index_stride, n_seqs);
        }
        return NULL;
    }
    length = index_stride * n_seqs;

    if (reverse_endianness) {
        if (is_32bit) {
            if (!(target_data32 = g_new0 (guint32, length))) {
                g_set_error (error, IBUS_ERROR, IBUS_ERROR_FAILED,
                             "Failed to malloc");
                return NULL;
            }
            for (i = 0; i < length; i++) {
                memcpy(&data32, (char *)compose_data + i * sizeof (data32), sizeof (data32));
                target_data32[i] = GUINT32_SWAP_LE_BE (data32);
            }
        } else {
            if (!(target_data16 = g_new0 (guint16, length))) {
                g_set_error (error, IBUS_ERROR, IBUS_ERROR_FAILED,
                             "Failed to malloc");
                return NULL;
            }
            for (i = 0; i < length; i++) {
                memcpy(&data16, (char *)compose_data + i * sizeof (data16), sizeof (data16));
                target_data16[i] = GUINT16_SWAP_LE_BE (data16);
            }
        }
    } else {
        if (is_32bit)
            target_data32 = (guint32*)compose_data;
        else
            target_data16 = (guint16*)compose_data;
    }

    if (is_32bit) {
        variant_data = g_variant_new_fixed_array (G_VARIANT_TYPE_UINT32,
                                                  target_data32,
                                                  length,
                                                  sizeof (guint32));
    } else {
        variant_data = g_variant_new_fixed_array (G_VARIANT_TYPE_UINT16,
                                                  target_data16,
                                                  length,
                                                  sizeof (guint16));
    }
    if (reverse_endianness) {
        g_free (target_data16);
        g_free (target_data32);
    }
    if (!variant_data) {
        g_set_error (error, IBUS_ERROR, IBUS_ERROR_FAILED,
                     "Could not change compose data to GVariant.");
        return NULL;
    }
    return variant_data;
}


GVariant *
ibus_compose_table_serialize (IBusComposeTableEx *compose_table,
                              gboolean            reverse_endianness)
{
    const char *header = IBUS_COMPOSE_TABLE_MAGIC;
    const guint16 version = IBUS_COMPOSE_TABLE_VERSION;
    guint16 max_seq_len;
    guint16 index_stride;
    guint16 n_seqs;
    gsize n_seqs_32bit = 0;
    gsize second_size = 0;
    GVariant *variant_data = NULL;
    GVariant *variant_data_32bit_first = NULL;
    GVariant *variant_data_32bit_second = NULL;
    GVariant *variant_table;
    GError *error = NULL;

    g_return_val_if_fail (compose_table != NULL, NULL);

    max_seq_len = compose_table->max_seq_len;
    index_stride = max_seq_len + 2;
    n_seqs = compose_table->n_seqs;

    g_return_val_if_fail (max_seq_len, NULL);

    if (n_seqs) {
        g_return_val_if_fail (compose_table->data, NULL);
        variant_data = compose_data_to_variant (compose_table->data,
                                                FALSE,
                                                index_stride,
                                                n_seqs,
                                                reverse_endianness,
                                                &error);
        if (!variant_data) {
            g_warning (
                    "Failed to generate variant from 16bit compose ID %u: %s",
                    compose_table->id,
                    error->message);
            g_error_free (error);
            return NULL;
        }
    } else {
        variant_data = g_variant_new_fixed_array (
                G_VARIANT_TYPE_UINT16,
                NULL,
                0,
                sizeof (guint16));
        g_assert (variant_data);
    }
    if (compose_table->priv) {
        n_seqs_32bit = compose_table->priv->first_n_seqs;
        second_size = compose_table->priv->second_size;
    }
    if (!n_seqs && !n_seqs_32bit) {
        g_warning ("ComposeTable has not key sequences.");
        goto out_serialize;
    } else if (n_seqs_32bit && !second_size) {
        g_warning ("Compose key sequences are loaded but the values could " \
                   "not be loaded.");
        goto out_serialize;
    } else if (!n_seqs_32bit && second_size) {
        g_warning ("Compose values are loaded but the key sequences could " \
                   "not be loaded.");
        goto out_serialize;
    } else if (n_seqs_32bit && second_size) {
        if (!compose_table->priv->data_first) {
            g_warning ("data_first is NULL");
            goto out_serialize;
        }
        if (!compose_table->priv->data_second) {
            g_warning ("data_second is NULL");
            goto out_serialize;
        }
        variant_data_32bit_first =
                compose_data_to_variant (compose_table->priv->data_first,
                                         FALSE,
                                         index_stride,
                                         n_seqs_32bit,
                                         reverse_endianness,
                                         &error);
        if (!variant_data_32bit_first) {
            g_warning (
                    "Failed to generate variant from compose first ID %u: %s",
                    compose_table->id,
                    error->message);
            g_error_free (error);
            goto out_serialize;
        }
        variant_data_32bit_second =
                compose_data_to_variant (compose_table->priv->data_second,
                                         TRUE,
                                         1,
                                         compose_table->priv->second_size,
                                         reverse_endianness,
                                         &error);
        if (!variant_data_32bit_second) {
            g_warning (
                    "Failed to generate variant from compose second ID %u: %s",
                    compose_table->id,
                    error->message);
            g_error_free (error);
            goto out_serialize;
        }
    } else {
        variant_data_32bit_first = g_variant_new_fixed_array (
                G_VARIANT_TYPE_UINT16,
                NULL,
                0,
                sizeof (guint16));
        variant_data_32bit_second = g_variant_new_fixed_array (
                G_VARIANT_TYPE_UINT32,
                NULL,
                0,
                sizeof (guint32));
        g_assert (variant_data_32bit_first && variant_data_32bit_second);
    }
    variant_table = g_variant_new ("(sqqqqqvvv)",
                                   header,
                                   version,
                                   max_seq_len,
                                   n_seqs,
                                   n_seqs_32bit,
                                   second_size,
                                   variant_data,
                                   variant_data_32bit_first,
                                   variant_data_32bit_second);
    return g_variant_ref_sink (variant_table);

out_serialize:
    g_clear_pointer (&variant_data, g_variant_unref);
    g_clear_pointer (&variant_data_32bit_first, g_variant_unref);
    g_clear_pointer (&variant_data_32bit_second, g_variant_unref);
    return NULL;
}


static int
ibus_compose_table_find (gconstpointer data1,
                         gconstpointer data2)
{
    const IBusComposeTableEx *compose_table =
            (const IBusComposeTableEx *) data1;
    guint32 hash = (guint32) GPOINTER_TO_INT (data2);
    return compose_table->id != hash;
}


IBusComposeTableEx *
ibus_compose_table_deserialize (const char *contents,
                                gsize       length)
{
    IBusComposeTableEx *retval = NULL;
    GVariantType *type;
    GVariant *variant_data = NULL;
    GVariant *variant_data_32bit_first = NULL;
    GVariant *variant_data_32bit_second = NULL;
    GVariant *variant_table = NULL;
    const char *header = NULL;
    guint16 version = 0;
    guint16 max_seq_len = 0;
    guint16 n_seqs = 0;
    guint16 n_seqs_32bit = 0;
    guint16 second_size = 0;
    guint16 index_stride;
    const guint16 *data = NULL;
    const guint16 *data_32bit_first = NULL;
    const guint32 *data_32bit_second = NULL;
    gsize data_length = 0;

    g_return_val_if_fail (contents != NULL, NULL);
    g_return_val_if_fail (length > 0, NULL);

    /* Check the cache version at first before load whole the file content. */
    type = g_variant_type_new ("(sq)");
    variant_table = g_variant_new_from_data (type,
                                             contents,
                                             length,
                                             FALSE,
                                             NULL,
                                             NULL);
    g_variant_type_free (type);

    if (!variant_table) {
        g_warning ("cache is broken.");
        goto out_load_cache;
    }

    g_variant_ref_sink (variant_table);
    g_variant_get (variant_table, "(&sq)", &header, &version);

    if (g_strcmp0 (header, IBUS_COMPOSE_TABLE_MAGIC) != 0) {
        g_warning ("cache is not IBusComposeTable.");
        goto out_load_cache;
    }

    if (version != IBUS_COMPOSE_TABLE_VERSION) {
        g_warning ("cache version is different: %u != %u",
                   version, IBUS_COMPOSE_TABLE_VERSION);
        goto out_load_cache;
    }

    version = 0;
    header = NULL;
    g_variant_unref (variant_table);
    variant_table = NULL;

    type = g_variant_type_new ("(sqqqqqvvv)");
    variant_table = g_variant_new_from_data (type,
                                             contents,
                                             length,
                                             FALSE,
                                             NULL,
                                             NULL);
    g_variant_type_free (type);

    if (!variant_table) {
        g_warning ("cache is broken.");
        goto out_load_cache;
    }

    g_variant_ref_sink (variant_table);
    g_variant_get (variant_table, "(&sqqqqqvvv)",
                   NULL,
                   NULL,
                   &max_seq_len,
                   &n_seqs,
                   &n_seqs_32bit,
                   &second_size,
                   &variant_data,
                   &variant_data_32bit_first,
                   &variant_data_32bit_second);

    if (max_seq_len == 0 || (n_seqs == 0 && n_seqs_32bit == 0)) {
        g_warning ("cache size is not correct %d %d %d",
                   max_seq_len, n_seqs, n_seqs_32bit);
        goto out_load_cache;
    }

    if (n_seqs && variant_data) {
        data = (const guint16*)g_variant_get_fixed_array (variant_data,
                                                          &data_length,
                                                          sizeof (guint16));
    }
    index_stride = max_seq_len + 2;

    if (data_length != (gsize)index_stride * n_seqs) {
        g_warning ("cache size is not correct %d %d %lu",
                   max_seq_len, n_seqs, data_length);
        goto out_load_cache;
    }

    retval = g_new0 (IBusComposeTableEx, 1);
    if (data_length)
        retval->data = data;
    retval->max_seq_len = max_seq_len;
    retval->n_seqs = n_seqs;

    if (n_seqs_32bit && !second_size) {
        g_warning ("32bit key sequences are loaded but the values " \
                   "could not be loaded.");
        goto out_load_cache;
    }
    if (!n_seqs_32bit && second_size) {
        g_warning ("32bit key sequences could not loaded but the values " \
                   "are loaded.");
        goto out_load_cache;
    }

    data_length = 0;
    if (n_seqs_32bit && variant_data_32bit_first) {
        data_32bit_first = (const guint16*)g_variant_get_fixed_array (
                variant_data_32bit_first,
                &data_length,
                sizeof (guint16));
        if (data_length != (gsize) index_stride * n_seqs_32bit) {
            g_warning ("32bit cache size is not correct %d %d %lu",
                       max_seq_len, n_seqs_32bit, data_length);
            goto out_load_cache;
        }
    }
    if (!data && !data_32bit_first) {
        g_warning ("cache data is null.");
        goto out_load_cache;
    }
    if (data_length) {
        if (!(retval->priv = g_new0 (IBusComposeTablePrivate, 1))) {
            g_warning ("Failed g_new");
            goto out_load_cache;
        }
        /* Do not memcpy for the possible mmap data */
        retval->priv->data_first = data_32bit_first;
        retval->priv->first_n_seqs = n_seqs_32bit;
    }

    data_length = 0;
    if (second_size && variant_data_32bit_second) {
        data_32bit_second = (const guint32*)g_variant_get_fixed_array (
                variant_data_32bit_second,
                &data_length,
                sizeof (guint32));
        if (data_length != (gsize) second_size) {
            g_warning ("32bit cache size is not correct %d %d",
                       max_seq_len, second_size);
            goto out_load_cache;
        }
    }
    if (data_length) {
        /* Do not memcpy for the possible mmap data */
        retval->priv->data_second = data_32bit_second;
        retval->priv->second_size = second_size;
    }


out_load_cache:
    g_clear_pointer (&variant_data, g_variant_unref);
    g_clear_pointer (&variant_data_32bit_first, g_variant_unref);
    g_clear_pointer (&variant_data_32bit_second, g_variant_unref);
    g_clear_pointer (&variant_table, g_variant_unref);
    return retval;
}


IBusComposeTableEx *
ibus_compose_table_load_cache (const gchar *compose_file)
{
    IBusComposeTableEx *retval = NULL;
    guint32 hash;
    char *path = NULL;
    char *contents = NULL;
    GStatBuf original_buf;
    GStatBuf cache_buf;
    gsize length = 0;
    GError *error = NULL;

    do {
        hash = g_str_hash (compose_file);
        if ((path = ibus_compose_hash_get_cache_path (hash)) == NULL)
            return NULL;
        if (!g_file_test (path, G_FILE_TEST_EXISTS))
            break;

        if (g_stat (path, &cache_buf))
            break;
        if (g_lstat (compose_file, &original_buf))
            break;
        if (original_buf.st_mtime > cache_buf.st_mtime)
            break;
        if (g_stat (compose_file, &original_buf))
            break;
        if (original_buf.st_mtime > cache_buf.st_mtime)
            break;
        if (!g_file_get_contents (path, &contents, &length, &error)) {
            g_warning ("Failed to get cache content %s: %s",
                       path, error->message);
            g_error_free (error);
            break;
        }

        retval = ibus_compose_table_deserialize (contents, length);
        if (retval == NULL) {
            g_warning ("Failed to load the cache file: %s", path);
        } else {
            retval->rawdata = contents;
            retval->id = hash;
        }
    } while (0);

    g_free (path);
    return retval;
}


void
ibus_compose_table_save_cache (IBusComposeTableEx *compose_table)
{
    char *path = NULL;
    GVariant *variant_table = NULL;
    const char *contents = NULL;
    GError *error = NULL;
    gsize length = 0;

    if ((path = ibus_compose_hash_get_cache_path (compose_table->id)) == NULL)
      return;

    variant_table = ibus_compose_table_serialize (compose_table, FALSE);
    if (variant_table == NULL) {
        g_warning ("Failed to serialize compose table %s", path);
        goto out_save_cache;
    }
    contents = g_variant_get_data (variant_table);
    length = g_variant_get_size (variant_table);
    if (!g_file_set_contents (path, contents, length, &error)) {
        g_warning ("Failed to save compose table %s: %s", path, error->message);
        g_error_free (error);
        goto out_save_cache;
    }

out_save_cache:
    g_variant_unref (variant_table);
    g_free (path);
}


static IBusComposeTableEx *
ibus_compose_table_new_with_list (GList   *compose_list,
                                  int      max_compose_len,
                                  int      n_index_stride,
                                  guint32  hash)
{
    /* @ibus_compose_seqs: Include both compose sequence and the value(compose
     *     output) as the tradition GTK. The value is one character only
     *     and within 16bit. I.e. All compose sequences and the values
     *     are 16bit.
     * @ibus_compose_seqs_32bit_second: Include the compose values only.
     *     The length of values by compose sequence is more than one characster
     *     or one of the values is outside 16bit but within 32bit.
     *     Some values could be more than one character and Emoji character
     *     could be outside 16bit.
     *     See also ibus/src/tests/ibus-compose.emoji file for e.g.
     * @ibus_compose_seqs_32bit_first: Include the compose sequence only in
     *     case the value is included in @ibus_compose_seqs_32bit_second.
     * @s_size_total: The number of compose sequences.
     * @s_size_16bit: The number of compose sequences whose value is one
     *     character only and within 16bit. I.e. the number of the compose
     *     sequence in @ibus_compose_seqs is @@s_size_16bit. And
     *     @s_size_total - @s_size_16bit is the number of the compose sequence
     *     in @ibus_compose_seqs_32bit_first.
     * @v_size_32bit: The total number of compose values. Each length of the
     *     values is more than one character or one of the value is
     *     outside 16bit but within 32bit. I.e. The size of
     *     @ibus_compose_seqs_32bit_second is @v_size_32bit.
     * @v_index_32bit: Each index of the compose values in
     *     @ibus_compose_seqs_32bit_second and this is not a fixed value in
     *     this API. If a compose sequence is found in
     *     @ibus_compose_seqs_32bit_first and the next value is 0, 0 is lined
     *     in @ibus_compose_seqs_32bit_first until @max_compose_len after
     *     the found compose sequence. And the next value is the length of
     *     the compose values and the next value is the @v_index_32bit, i.e.
     *     the index of @ibus_compose_seqs_32bit_second.
     *     E.g. the following line could be found in
     *     @ibus_compose_seqs_32bit_first:
     *         ..., "U17ff", "0", "0", "0", "0", 2, 100, ...
     *     @ibus_compose_seqs_32bit_second[100] is "ាំ"  and the character
     *     length is 2.
     *     @max_compose_len is 5 and @n_index_stride is 7.
     */
    gsize s_size_total, s_size_16bit, v_size_32bit, v_index_32bit;
    guint n = 0, m = 0;
    int i, j;
    gpointer rawdata = NULL;
    guint16 *ibus_compose_seqs = NULL;
    guint16 *ibus_compose_seqs_32bit_first = NULL;
    guint32 *ibus_compose_seqs_32bit_second = NULL;
    GList *list;
    IBusComposeData *compose_data = NULL;
    IBusComposeTableEx *retval = NULL;

    g_return_val_if_fail (compose_list != NULL, NULL);

    s_size_total = g_list_length (compose_list);
    s_size_16bit = s_size_total;
    v_size_32bit = 0;

    for (list = compose_list; list != NULL; list = list->next) {
        compose_data = list->data;
        if (unichar_length (compose_data->values) > 1 ||
            compose_data->values[0] >= 0xFFFF) {
            --s_size_16bit;
            v_size_32bit += unichar_length (compose_data->values);
        }
    }

    if (s_size_16bit) {
        if (G_UNLIKELY ((s_size_16bit * n_index_stride) >
                        (G_MAXSIZE / sizeof (guint16)))) {
            g_warning ("Too long allocation %lu x %u",
                       s_size_16bit, n_index_stride);
            return NULL;
        }
        rawdata = (gpointer)g_new (guint16, s_size_16bit * n_index_stride);
        if (G_UNLIKELY (!rawdata)) {
            g_warning ("Failed g_new");
            return NULL;
        }
        ibus_compose_seqs = (guint16*)rawdata;
    }
    if (s_size_total > s_size_16bit) {
        if (G_UNLIKELY (((s_size_total - s_size_16bit) * n_index_stride) >
                        (G_MAXSIZE / sizeof (guint16)))) {
            g_warning ("Too long allocation %lu x %u",
                       s_size_total - s_size_16bit, n_index_stride);
            g_free (ibus_compose_seqs);
            return NULL;
        }
        rawdata = (gpointer)g_new (
                guint16,
                (s_size_total - s_size_16bit) * n_index_stride);
        if (G_UNLIKELY ((sizeof (guint16) * (s_size_total - s_size_16bit)
                        * n_index_stride) / sizeof (guint32) + v_size_32bit 
                        > (G_MAXSIZE / sizeof (guint32)))) {
            g_warning ("Too long allocation %lu x %u x %lu",
                       s_size_total - s_size_16bit,
                       n_index_stride,
                       v_size_32bit);
            g_free (ibus_compose_seqs);
            g_free (rawdata);
            return NULL;
        }
        if (G_LIKELY (rawdata)) {
            rawdata = g_realloc (
                    rawdata,
                    sizeof (guint16) * (s_size_total - s_size_16bit)
                        * n_index_stride + sizeof (guint32) * v_size_32bit);
        }
        if (G_LIKELY (rawdata)) {
            ibus_compose_seqs_32bit_first = (guint16*)rawdata;
            ibus_compose_seqs_32bit_second =
                    (guint32*)(rawdata + sizeof (guint16)
                    * (s_size_total - s_size_16bit) * n_index_stride);
        }
        if (!ibus_compose_seqs_32bit_first || !ibus_compose_seqs_32bit_second) {
            g_warning ("Failed g_new");
            g_free (ibus_compose_seqs);
            g_free (rawdata);
            return NULL;
        }
    }

    v_index_32bit = 0;
    for (list = compose_list; list != NULL; list = list->next) {
        gboolean is_32bit = FALSE;
        compose_data = list->data;

        is_32bit = unichar_length (compose_data->values) > 1 ? TRUE :
                compose_data->values[0] >= 0xFFFF ? TRUE : FALSE;
        if (is_32bit) {
            g_assert (ibus_compose_seqs_32bit_first);
            g_assert (ibus_compose_seqs_32bit_second);
        }
        for (i = 0; i < max_compose_len; i++) {
            if (compose_data->sequence[i] == 0) {
                for (j = i; j < max_compose_len; j++) {
                    if (is_32bit) {
                        g_assert (m < (s_size_total - s_size_16bit)
                                  * n_index_stride);
                        ibus_compose_seqs_32bit_first[m++] = 0;
                    } else {
                        g_assert (n < s_size_16bit * n_index_stride);
                        ibus_compose_seqs[n++] = 0;
                    }
                }
                break;
            }
            if (is_32bit) {
                g_assert (m < (s_size_total - s_size_16bit) * n_index_stride);
                ibus_compose_seqs_32bit_first[m++] =
                        (guint16) compose_data->sequence[i];
            } else {
                g_assert (n < s_size_16bit * n_index_stride);
                ibus_compose_seqs[n++] = (guint16) compose_data->sequence[i];
            }
        }
        if (is_32bit) {
            for (j = 0; compose_data->values[j]; j++) {
                g_assert (v_index_32bit + j <  v_size_32bit);
                memcpy(&ibus_compose_seqs_32bit_second[v_index_32bit + j],
                       &compose_data->values[j],
                       sizeof *ibus_compose_seqs_32bit_second);
            }
            g_assert (m + 1 < (s_size_total - s_size_16bit) * n_index_stride);
            ibus_compose_seqs_32bit_first[m++] = j;
            ibus_compose_seqs_32bit_first[m++] = v_index_32bit;
            v_index_32bit += j;
        } else {
            g_assert (n + 1 < s_size_16bit * n_index_stride);
            ibus_compose_seqs[n++] = (guint16) compose_data->values[0];
            ibus_compose_seqs[n++] = 0;
        }
    }

    retval = g_new0 (IBusComposeTableEx, 1);
    retval->data = ibus_compose_seqs;
    retval->max_seq_len = max_compose_len;
    retval->n_seqs = s_size_16bit;
    retval->id = hash;
    retval->rawdata = rawdata;
    if (s_size_total > s_size_16bit) {
        retval->priv = g_new0 (IBusComposeTablePrivate, 1);
        retval->priv->data_first = ibus_compose_seqs_32bit_first;
        retval->priv->data_second = ibus_compose_seqs_32bit_second;
        retval->priv->first_n_seqs = s_size_total - s_size_16bit;
        retval->priv->second_size = v_size_32bit;
    }

    return retval;
}


IBusComposeTableEx *
ibus_compose_table_new_with_file (const gchar *compose_file,
                                  GSList      *compose_tables)
{
    GList *compose_list = NULL;
    IBusComposeTableEx *compose_table;
    int max_compose_len = 0;
    int n_index_stride = 0;

    g_assert (compose_file != NULL);

    compose_list = ibus_compose_list_parse_file (compose_file,
                                                 &max_compose_len);
    if (compose_list == NULL)
        return NULL;
    n_index_stride = max_compose_len + 2;
    compose_list = ibus_compose_list_check_duplicated_with_en (compose_list,
                                                               max_compose_len,
                                                               compose_tables);
    compose_list = g_list_sort_with_data (
            compose_list,
            (GCompareDataFunc) ibus_compose_data_compare,
            GINT_TO_POINTER (max_compose_len));

    compose_list = ibus_compose_list_check_duplicated_with_own (
            compose_list,
            max_compose_len);

    if (compose_list == NULL) {
        g_warning ("compose file %s does not include any keys besides keys "
                   "in en-us compose file", compose_file);
        return NULL;
    }

    if (g_getenv ("IBUS_COMPOSE_TABLE_PRINT") != NULL)
        ibus_compose_list_print (compose_list, max_compose_len, n_index_stride);

    compose_table = ibus_compose_table_new_with_list (
            compose_list,
            max_compose_len,
            n_index_stride,
            g_str_hash (compose_file));

    g_list_free_full (compose_list,
                      (GDestroyNotify) ibus_compose_list_element_free);

    return compose_table;
}


/* if ibus_compose_seqs[N - 1] is an outputed compose character,
 * ibus_compose_seqs[N * 2 - 1] is also an outputed compose character.
 * and ibus_compose_seqs[0] to ibus_compose_seqs[0 + N - 3] are the
 * sequences and call ibus_engine_simple_add_table:
 * ibus_engine_simple_add_table(engine, ibus_compose_seqs,
 *                              N - 2, G_N_ELEMENTS(ibus_compose_seqs) / N)
 * The compose sequences are allowed within G_MAXUINT16
 */
GSList *
ibus_compose_table_list_add_array (GSList        *compose_tables,
                                   const guint16 *data,
                                   gint           max_seq_len,
                                   gint           n_seqs)
{
    guint32 hash;
    IBusComposeTableEx *compose_table;
    int n_index_stride = max_seq_len + 2;
    int length = n_index_stride * n_seqs;
    int i;
    guint16 *ibus_compose_seqs = NULL;

    g_assert (length >= 0);
    g_return_val_if_fail (data != NULL, compose_tables);
    g_return_val_if_fail (max_seq_len <= IBUS_MAX_COMPOSE_LEN, compose_tables);

    hash = ibus_compose_table_data_hash (data, length);

    if (g_slist_find_custom (compose_tables,
                             GINT_TO_POINTER (hash),
                             ibus_compose_table_find) != NULL) {
        return compose_tables;
    }

    ibus_compose_seqs = g_new0 (guint16, length);
    for (i = 0; i < length; i++)
        ibus_compose_seqs[i] = data[i];

    compose_table = g_new0 (IBusComposeTableEx, 1);
    compose_table->data = ibus_compose_seqs;
    compose_table->max_seq_len = max_seq_len;
    compose_table->n_seqs = n_seqs;
    compose_table->id = hash;

    return g_slist_prepend (compose_tables, compose_table);
}


GSList *
ibus_compose_table_list_add_file (GSList      *compose_tables,
                                  const gchar *compose_file)
{
    guint32 hash;
    IBusComposeTableEx *compose_table;

    g_return_val_if_fail (compose_file != NULL, compose_tables);

    hash = g_str_hash (compose_file);
    if (g_slist_find_custom (compose_tables,
                             GINT_TO_POINTER (hash),
                             ibus_compose_table_find) != NULL) {
        return compose_tables;
    }

    compose_table = ibus_compose_table_load_cache (compose_file);
    if (compose_table != NULL)
        return g_slist_prepend (compose_tables, compose_table);

   if ((compose_table = ibus_compose_table_new_with_file (compose_file,
                                                          compose_tables))
           == NULL) {
       return compose_tables;
   }

    ibus_compose_table_save_cache (compose_table);
    return g_slist_prepend (compose_tables, compose_table);
}


GSList *
ibus_compose_table_list_add_table (GSList             *compose_tables,
                                   IBusComposeTableEx *compose_table)
{
    g_return_val_if_fail (compose_table != NULL, compose_tables);
    if (g_slist_find_custom (compose_tables,
                             GINT_TO_POINTER (compose_table->id),
                             ibus_compose_table_find) != NULL) {
        return compose_tables;
    }
    return g_slist_prepend (compose_tables, compose_table);
}


static int
compare_seq (const void *key, const void *value)
{
    int i = 0;
    const guint *keysyms = key;
    const guint16 *seq = value;

    while (keysyms[i]) {
        guint typed_key = keysyms[i];
        guint saved_key = (guint)seq[i];
        guint flag = ibus_compose_key_flag (saved_key);
        if (typed_key < (saved_key + flag))
            return (0xffff & typed_key) - saved_key;
        else if (typed_key > (saved_key + flag))
            return (0xffff & typed_key) - saved_key;

        i++;
    }

    return 0;
}


/**
 * ibus_compose_table_check:
 * @table: An #IBusComposeTableEx.
 * @compose_buffer: (array length=n_compose):
                    A candidate typed key sequence to generate compose chars.
 * @n_compose: The length of compose_buffer.
 * @compose_finish: If typed key sequence is finished for the compose chars.
 * @compose_match: If typed key sequence is matched partically.
 * @output: Matched compse chars.
 * @is_32bit: The type of #IBusComposeTableEx.
 *
 * If the current @comopse_buffer matches a compose sequence partically in
 * #IBusComposeTableEx, return %TRUE otherwise %FALSE.
 * If the matched compose sequence can be converted to a Unicode string,
 * %TRUE is set to @compose_match and the converted string is assgined to
 * @output to update the preedit text.
 * If the current @comopse_buffer matches a compose sequence fully in
 * #IBusComposeTableEx, %TRUE is set to @compose_finish and the generated
 * compose character is assigned to @output.
 *
 * #IBusComposeTableEx includes 16bit key sequences and the corresponded
 * compose characters. #IBusComposeTableEx has two types, one is
 * 16bit and one compose char and another is 32bit or more than two chars.
 * If #IBusComposeTableEx is 16bit and one compose char, @is_32bit is %TRUE.
 *
 * @compose_buffer is the typed key sequence but not limitted to 16bit.
 * E.g.
 * Shift + t is 0x100fef9 keysym with Arabic keyboard.
 * Shift + NumLock is 0xfef9 keysym with keypad:pointerkeys XKB option.
 * So compare_seq() compares the 32bit key sequence and 16bit one.
 */
gboolean
ibus_compose_table_check (const IBusComposeTableEx *table,
                          guint                    *compose_buffer,
                          int                       n_compose,
                          gboolean                 *compose_finish,
                          gboolean                 *compose_match,
                          GString                  *output,
                          gboolean                  is_32bit)
{
    int row_stride = table->max_seq_len + 2;
    const guint16 *data_first;
    int n_seqs;
    guint16 *seq;

    if (compose_finish)
        *compose_finish = FALSE;
    if (compose_match)
        *compose_match = FALSE;
    if (output)
        g_string_set_size (output, 0);

    if (n_compose > table->max_seq_len)
        return FALSE;

    if (is_32bit) {
        if (!table->priv)
            return FALSE;
        data_first = table->priv->data_first;
        n_seqs = table->priv->first_n_seqs;
    } else {
        data_first = table->data;
        n_seqs = table->n_seqs;
    }
    seq = bsearch (compose_buffer,
                   data_first, n_seqs,
                   sizeof (guint16) * row_stride,
                   compare_seq);

    if (seq == NULL)
        return FALSE;

    guint16 *prev_seq;

    /* Back up to the first sequence that matches to make sure
     * we find the exact match if their is one.
     */
    while (seq > data_first) {
        prev_seq = seq - row_stride;
        if (compare_seq (compose_buffer, prev_seq) != 0)
            break;
        seq = prev_seq;
    }

    /* complete sequence */
    if (n_compose == table->max_seq_len || seq[n_compose] == 0) {
        guint16 *next_seq;
        gunichar value = 0;
        int num = 0;
        int index = 0;
        char *output_str = NULL;
        GError *error = NULL;

        if (is_32bit) {
            num = seq[table->max_seq_len];
            index = seq[table->max_seq_len + 1];
            value =  table->priv->data_second[index];
        } else {
            value = seq[table->max_seq_len];
        }

        if (is_32bit) {
            output_str = g_ucs4_to_utf8 (table->priv->data_second + index,
                                         num, NULL, NULL, &error);
            if (output_str) {
                if (output)
                    g_string_append (output, output_str);
                g_free (output_str);
                if (compose_match)
                    *compose_match = TRUE;
            } else {
                g_warning ("Failed to output multiple characters: %s",
                           error->message);
                g_error_free (error);
            }
        } else {
            if (output)
                g_string_append_unichar (output, value);
            if (compose_match)
                *compose_match = TRUE;
        }

        /* We found a tentative match. See if there are any longer
         * sequences containing this subsequence
         */
        next_seq = seq + row_stride;
        if (next_seq < data_first + row_stride * n_seqs) {
            if (compare_seq (compose_buffer, next_seq) == 0)
                return TRUE;
        }

        if (compose_finish)
            *compose_finish = TRUE;
        compose_buffer[0] = 0;
    }
    return TRUE;
}


/* This function receives a sequence of Unicode characters and tries to
 * normalize it (NFC). We check for the case the the resulting string
 * has length 1 (single character).
 * NFC normalisation normally rearranges diacritic marks, unless these
 * belong to the same Canonical Combining Class.
 * If they belong to the same canonical combining class, we produce all
 * permutations of the diacritic marks, then attempt to normalize.
 */
static gboolean
check_normalize_nfc (gunichar* combination_buffer, int n_compose)
{
    gunichar combination_buffer_temp[IBUS_MAX_COMPOSE_ALGORITHM_LEN + 1];
    char *combination_utf8_temp = NULL;
    char *nfc_temp = NULL;
    int n_combinations;
    gunichar temp_swap;
    int i;

    n_combinations = 1;

    for (i = 1; i < n_compose; i++ )
        n_combinations *= i;

    /* Xorg reuses dead_tilde for the perispomeni diacritic mark.
     * We check if base character belongs to Greek Unicode block,
     * and if so, we replace tilde with perispomeni. */
    if (combination_buffer[0] >= 0x390 && combination_buffer[0] <= 0x3FF) {
        for (i = 1; i < n_compose; i++ )
            if (combination_buffer[i] == 0x303)
                combination_buffer[i] = 0x342;
    }

    memcpy (combination_buffer_temp,
            combination_buffer,
            IBUS_MAX_COMPOSE_ALGORITHM_LEN * sizeof (gunichar) );

    for (i = 0; i < n_combinations; i++ ) {
        g_unicode_canonical_ordering (combination_buffer_temp, n_compose);
        combination_utf8_temp = g_ucs4_to_utf8 (combination_buffer_temp, -1,
                                                NULL, NULL, NULL);
        nfc_temp = g_utf8_normalize (combination_utf8_temp, -1,
                                     G_NORMALIZE_NFC);

        if (g_utf8_strlen (nfc_temp, -1) == 1) {
            memcpy (combination_buffer,
                    combination_buffer_temp,
                    IBUS_MAX_COMPOSE_ALGORITHM_LEN * sizeof (gunichar) );

            g_free (combination_utf8_temp);
            g_free (nfc_temp);

            return TRUE;
        }

        g_free (combination_utf8_temp);
        g_free (nfc_temp);

        if (n_compose > 2) {
            int j = i % (n_compose - 1) + 1;
            int k = (i+1) % (n_compose - 1) + 1;
            if (j >= IBUS_MAX_COMPOSE_ALGORITHM_LEN) {
                g_warning ("j >= %d for combination_buffer_temp",
                           IBUS_MAX_COMPOSE_ALGORITHM_LEN);
                break;
            }
            if (k >= IBUS_MAX_COMPOSE_ALGORITHM_LEN) {
                g_warning ("k >= %d for combination_buffer_temp",
                           IBUS_MAX_COMPOSE_ALGORITHM_LEN);
                break;
            }
            temp_swap = combination_buffer_temp[j];
            combination_buffer_temp[j] = combination_buffer_temp[k];
            combination_buffer_temp[k] = temp_swap;
        } else {
            break;
        }
    }

    return FALSE;
}


gboolean
ibus_check_algorithmically (const guint   *compose_buffer,
                            int            n_compose,
                            gunichar      *output_char)

{
    int i;
    gunichar combination_buffer[IBUS_MAX_COMPOSE_ALGORITHM_LEN + 1];
    char *combination_utf8, *nfc;

    if (output_char)
        *output_char = 0;

    /* Check the IBUS_MAX_COMPOSE_ALGORITHM_LEN length only here instead of
     * IBUS_MAX_COMPOSE_LEN length.
     * Because this API calls check_normalize_nfc() which calculates the factorial
     * of `n_compose` and assigns the value to `n_combinations`.
     * I.e. 9! == 40320 <= SHRT_MAX == 32767
     * The factorial of exceeding INT_MAX spends a long time in check_normalize_nfc()
     * and causes a D-Bus timeout between GTK clients and IBusEngineSimple.
     * Currenlty IBUS_MAX_COMPOSE_LEN is much larger and supports the long compose
     * sequence however the max 9 would be enough for this mechanical compose.
     */
    if (n_compose > IBUS_MAX_COMPOSE_ALGORITHM_LEN)
        return FALSE;

    for (i = 0; i < n_compose && IS_DEAD_KEY (compose_buffer[i]); i++)
        ;
    if (i == n_compose)
        return TRUE;

    if (i > 0 && i == n_compose - 1) {
        combination_buffer[0] = ibus_keyval_to_unicode (compose_buffer[i]);
        combination_buffer[n_compose] = 0;
        i--;
        while (i >= 0) {
            combination_buffer[i+1] = ibus_keysym_to_unicode (compose_buffer[i],
                                                              TRUE,
                                                              NULL);
            if (!combination_buffer[i+1]) {
                combination_buffer[i+1] =
                        ibus_keyval_to_unicode (compose_buffer[i]);
            }
            i--;
        }

        /* If the buffer normalizes to a single character,
         * then modify the order of combination_buffer accordingly, if
         * necessary, and return TRUE.
         */
        if (check_normalize_nfc (combination_buffer, n_compose)) {
            combination_utf8 = g_ucs4_to_utf8 (combination_buffer, -1,
                                               NULL, NULL, NULL);
            nfc = g_utf8_normalize (combination_utf8, -1, G_NORMALIZE_NFC);

            if (output_char)
                *output_char = g_utf8_get_char (nfc);

            g_free (combination_utf8);
            g_free (nfc);

            return TRUE;
        }
    }

    return FALSE;
}


gunichar
ibus_keysym_to_unicode (guint     keysym,
                        gboolean  combining,
                        gboolean *need_space) {
#define CASE(keysym_suffix, unicode, sp)                                      \
        case IBUS_KEY_dead_##keysym_suffix:                                   \
            if (need_space)                                                   \
                *need_space = sp;                                             \
            return unicode
#define CASE_COMBINE(keysym_suffix, combined_unicode, isolated_unicode, sp)   \
        case IBUS_KEY_dead_##keysym_suffix:                                   \
            if (need_space)                                                   \
                *need_space = sp;                                             \
            if (combining)                                                    \
                return combined_unicode;                                      \
            else                                                              \
                return isolated_unicode
    switch (keysym) {
#ifdef IBUS_ENGLISH_DEAD_KEY
    CASE (a, 0x0363, 1);
    CASE (A, 0x0363, 1);
    CASE (i, 0x0365, 1);
    CASE (I, 0x0365, 1);
    CASE (u, 0x0367, 1);
    CASE (U, 0x0367, 1);
    CASE (e, 0x0364, 1);
    CASE (E, 0x0364, 1);
    CASE (o, 0x0366, 1);
    CASE (O, 0x0366, 1);
#else
    CASE (a, 0x3041, 0);
    CASE (A, 0x3042, 0);
    CASE (i, 0x3043, 0);
    CASE (I, 0x3044, 0);
    CASE (u, 0x3045, 0);
    CASE (U, 0x3046, 0);
    CASE (e, 0x3047, 0);
    CASE (E, 0x3048, 0);
    CASE (o, 0x3049, 0);
    CASE (O, 0x304A, 0);
#endif
    CASE_COMBINE (abovecomma,                   0x0313, 0x02BC, 0);
    CASE_COMBINE (abovedot,                     0x0307, 0x02D9, 0);
    CASE_COMBINE (abovereversedcomma,           0x0314, 0x02BD, 0);
    CASE_COMBINE (abovering,                    0x030A, 0x02DA, 0);
    CASE_COMBINE (aboveverticalline,            0x030D, 0x02C8, 0);
    CASE_COMBINE (acute,                        0x0301, 0x00B4, 0);
    CASE         (belowbreve,                   0x032E, 1);
    CASE_COMBINE (belowcircumflex,              0x032D, 0xA788, 0);
    CASE_COMBINE (belowcomma,                   0x0326, 0x002C, 0);
    CASE         (belowdiaeresis,               0x0324, 1);
    CASE_COMBINE (belowdot,                     0x0323, 0x002E, 0);
    CASE_COMBINE (belowmacron,                  0x0331, 0x02CD, 0);
    CASE_COMBINE (belowring,                    0x030A, 0x02F3, 0);
    CASE_COMBINE (belowtilde,                   0x0330, 0x02F7, 0);
    CASE_COMBINE (belowverticalline,            0x0329, 0x02CC, 0);
    CASE_COMBINE (breve,                        0x0306, 0x02D8, 0);
    CASE_COMBINE (capital_schwa,                0x1DEA, 0x1D4A, 0);
    CASE_COMBINE (caron,                        0x030C, 0x02C7, 0);
    CASE_COMBINE (cedilla,                      0x0327, 0x00B8, 0);
    CASE_COMBINE (circumflex,                   0x0302, 0x005E, 0);
    CASE         (currency,                     0x00A4, 0);
    // IBUS_KEY_dead_dasia == IBUS_KEY_dead_abovereversedcomma
    CASE_COMBINE (diaeresis,                    0x0308, 0x00A8, 0);
    CASE_COMBINE (doubleacute,                  0x030B, 0x02DD, 0);
    CASE_COMBINE (doublegrave,                  0x030F, 0x02F5, 0);
    CASE_COMBINE (grave,                        0x0300, 0x0060, 0);
    CASE         (greek,                        0x03BC, 0);
    CASE_COMBINE (hook,                         0x0309, 0x02C0, 0);
    CASE         (horn,                         0x031B, 1);
    CASE         (invertedbreve,                0x032F, 1);
    CASE_COMBINE (iota,                         0x0345, 0x037A, 0);
    CASE         (longsolidusoverlay,           0x0338, 1);
    CASE_COMBINE (lowline,                      0x0332, 0x005F, 0);
    CASE_COMBINE (macron,                       0x0304, 0x00AF, 0);
    CASE_COMBINE (ogonek,                       0x0328, 0x02DB, 0);
    // IBUS_KEY_dead_perispomeni == IBUS_KEY_dead_tilde
    // IBUS_KEY_dead_psili == IBUS_KEY_dead_abovecomma
    CASE_COMBINE (semivoiced_sound,             0x309A, 0x309C, 0);
    CASE_COMBINE (small_schwa,                  0x1DEA, 0x1D4A, 0);
    CASE         (stroke,                       0x0335, 1);
    CASE_COMBINE (tilde,                        0x0303, 0x007E, 0);
    CASE_COMBINE (voiced_sound,                 0x3099, 0x309B, 0);
    case IBUS_KEY_Multi_key:
        if (need_space)
            *need_space = FALSE;
        /* We only show the Compose key visibly when it is the
         * only glyph in the preedit, or when it occurs in the
         * middle of the sequence. Sadly, the official character,
         * U+2384, COMPOSITION SYMBOL, is bit too distracting, so
         * we use U+00B7, MIDDLE DOT.
         */
        return 0x00B7;
    default:;
        if (need_space)
            *need_space = FALSE;
    }
    return 0x0;
#undef CASE
#undef CASE_COMBINE
}
