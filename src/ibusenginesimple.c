/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2014 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2015-2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2014-2017 Red Hat, Inc.
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
#  include "config.h"
#endif

#include "ibuscomposetable.h"
#include "ibusemoji.h"
#include "ibusenginesimple.h"
#include "ibusenginesimpleprivate.h"

#include "ibuskeys.h"
#include "ibuskeysyms.h"
#include "ibusutil.h"

#include <memory.h>
#include <stdlib.h>

#define IBUS_ENGINE_SIMPLE_GET_PRIVATE(o)  \
   ((IBusEngineSimplePrivate *)ibus_engine_simple_get_instance_private (o))

#define SET_COMPOSE_BUFFER_ELEMENT_NEXT(buffer, index, value) {         \
    if ((index) >= COMPOSE_BUFFER_SIZE &&                               \
        COMPOSE_BUFFER_SIZE < IBUS_MAX_COMPOSE_LEN) {                   \
        COMPOSE_BUFFER_SIZE = ((index) + 10) < IBUS_MAX_COMPOSE_LEN     \
                              ? ((index) + 10) : IBUS_MAX_COMPOSE_LEN;  \
        (buffer) = g_renew (guint, (buffer), COMPOSE_BUFFER_SIZE + 1);  \
    }                                                                   \
    if ((index) < COMPOSE_BUFFER_SIZE) {                                \
        (buffer)[(index)] = (value);                                    \
        (index) += 1;                                                   \
    }                                                                   \
}

#define SET_COMPOSE_BUFFER_ELEMENT_END(buffer, index, value) {          \
    if ((index) > COMPOSE_BUFFER_SIZE)                                  \
        (index) = COMPOSE_BUFFER_SIZE;                                  \
    (buffer)[(index)] = (value);                                        \
}

#define CHECK_COMPOSE_BUFFER_LENGTH(index) {                            \
    if ((index) > COMPOSE_BUFFER_SIZE)                                  \
        (index) = COMPOSE_BUFFER_SIZE;                                  \
}

typedef struct {
    GHashTable *dict;
    int         max_seq_len;
} IBusEngineDict;

struct _IBusEngineSimplePrivate {
    guint              *compose_buffer;
    GString            *tentative_match;
    int                 tentative_match_len;
    char               *tentative_emoji;

    guint               hex_mode_enabled : 1;
    guint               in_hex_sequence : 1;
    guint               in_emoji_sequence : 1;
    guint               in_compose_sequence : 1;
    guint               modifiers_dropped : 1;
    IBusEngineDict     *emoji_dict;
    IBusLookupTable    *lookup_table;
    gboolean            lookup_table_visible;
    IBusText           *updated_preedit;
};

guint COMPOSE_BUFFER_SIZE = 20;
G_LOCK_DEFINE_STATIC (global_tables);
static GSList *global_tables;
static IBusText *updated_preedit_empty;
static IBusComposeTableEx *en_compose_table;

/* functions prototype */
static void     ibus_engine_simple_destroy      (IBusEngineSimple   *simple);
static void     ibus_engine_simple_focus_in     (IBusEngine         *engine);
static void     ibus_engine_simple_focus_out    (IBusEngine         *engine);
static void     ibus_engine_simple_reset        (IBusEngine         *engine);
static gboolean ibus_engine_simple_process_key_event
                                                (IBusEngine         *engine,
                                                 guint               keyval,
                                                 guint               keycode,
                                                 guint               modifiers);
static void     ibus_engine_simple_page_down   (IBusEngine          *engine);
static void     ibus_engine_simple_page_up     (IBusEngine          *engine);
static void     ibus_engine_simple_candidate_clicked
                                               (IBusEngine          *engine,
                                                guint                index,
                                                guint                button,
                                                guint                state);
static void     ibus_engine_simple_commit_char (IBusEngineSimple    *simple,
                                                gunichar             ch);
static void     ibus_engine_simple_commit_str  (IBusEngineSimple    *simple,
                                                const char          *str);
static void     ibus_engine_simple_update_preedit_text
                                               (IBusEngineSimple    *simple);

G_DEFINE_TYPE_WITH_PRIVATE (IBusEngineSimple,
                            ibus_engine_simple,
                            IBUS_TYPE_ENGINE)

static void
ibus_engine_simple_class_init (IBusEngineSimpleClass *class)
{
    IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (class);
    IBusEngineClass *engine_class = IBUS_ENGINE_CLASS (class);
    GBytes *data;
    GError *error = NULL;
    const char *contents;
    gsize length = 0;

    ibus_object_class->destroy =
        (IBusObjectDestroyFunc) ibus_engine_simple_destroy;

    engine_class->focus_in  = ibus_engine_simple_focus_in;
    engine_class->focus_out = ibus_engine_simple_focus_out;
    engine_class->reset     = ibus_engine_simple_reset;
    engine_class->process_key_event
                            = ibus_engine_simple_process_key_event;
    engine_class->page_down = ibus_engine_simple_page_down;
    engine_class->page_up   = ibus_engine_simple_page_up;
    engine_class->candidate_clicked
                            = ibus_engine_simple_candidate_clicked;
    updated_preedit_empty = ibus_text_new_from_string ("");
    g_object_ref_sink (updated_preedit_empty);

    data = g_resources_lookup_data ("/org/freedesktop/ibus/compose/sequences",
                                    G_RESOURCE_LOOKUP_FLAGS_NONE,
                                    &error);
    if (error) {
        g_warning ("Not found compose resource %s", error->message);
        g_clear_error (&error);
        return;
    }
    contents = g_bytes_get_data (data, &length);
    en_compose_table = ibus_compose_table_deserialize (contents, length);
}

static void
ibus_engine_simple_init (IBusEngineSimple *simple)
{
    IBusEngineSimplePrivate *priv;

    priv = simple->priv = IBUS_ENGINE_SIMPLE_GET_PRIVATE (simple);
    priv->compose_buffer = g_new0 (guint, COMPOSE_BUFFER_SIZE + 1);
    priv->hex_mode_enabled =
        g_getenv("IBUS_ENABLE_CTRL_SHIFT_U") != NULL ||
        g_getenv("IBUS_ENABLE_CONTROL_SHIFT_U") != NULL;
    priv->tentative_match = g_string_new ("");
    priv->tentative_match_len = 0;
    priv->updated_preedit =
            (IBusText *)g_object_ref_sink (updated_preedit_empty);
    if (!en_compose_table) {
        g_warning ("Failed to load EN compose table");
    } else {
        global_tables = ibus_compose_table_list_add_table (global_tables,
                                                           en_compose_table);
    }
}


static void
ibus_engine_simple_destroy (IBusEngineSimple *simple)
{
    IBusEngineSimplePrivate *priv = simple->priv;

    if (priv->emoji_dict) {
        if (priv->emoji_dict->dict)
            g_clear_pointer (&priv->emoji_dict->dict, g_hash_table_destroy);
        g_slice_free (IBusEngineDict, priv->emoji_dict);
        priv->emoji_dict = NULL;
    }

    g_clear_object (&priv->lookup_table);
    g_clear_pointer (&priv->compose_buffer, g_free);
    g_clear_pointer (&priv->tentative_emoji, g_free);
    g_string_free (priv->tentative_match, TRUE);
    priv->tentative_match = NULL;
    priv->tentative_match_len = 0;
    g_clear_object (&priv->updated_preedit);

    IBUS_OBJECT_CLASS(ibus_engine_simple_parent_class)->destroy (
        IBUS_OBJECT (simple));
}

static void
ibus_engine_simple_focus_in (IBusEngine *engine)
{
    IBUS_ENGINE_CLASS (ibus_engine_simple_parent_class)->focus_in (engine);
}

static void
ibus_engine_simple_focus_out (IBusEngine *engine)
{
    ibus_engine_simple_reset (engine);
    IBUS_ENGINE_CLASS (ibus_engine_simple_parent_class)->focus_out (engine);
}

static void
ibus_engine_simple_reset (IBusEngine *engine)
{
    IBusEngineSimple *simple = (IBusEngineSimple *)engine;
    IBusEngineSimplePrivate *priv = simple->priv;

    priv->compose_buffer[0] = 0;

    if (priv->tentative_match->len > 0 || priv->in_hex_sequence) {
        priv->in_hex_sequence = FALSE;
        g_string_set_size (priv->tentative_match, 0);
        priv->tentative_match_len = 0;
    } else if (priv->tentative_emoji || priv->in_emoji_sequence) {
        priv->in_emoji_sequence = FALSE;
        g_clear_pointer (&priv->tentative_emoji, g_free);
    } else if (!priv->in_hex_sequence && !priv->in_emoji_sequence) {
        g_string_set_size (priv->tentative_match, 0);
        priv->tentative_match_len = 0;
    }
    ibus_engine_hide_preedit_text ((IBusEngine *)simple);
    g_object_unref (priv->updated_preedit);
    priv->updated_preedit =
            (IBusText *)g_object_ref_sink (updated_preedit_empty);
}

static void
ibus_engine_simple_commit_char (IBusEngineSimple *simple,
                                gunichar          ch)
{
    g_return_if_fail (g_unichar_validate (ch));

    IBusEngineSimplePrivate *priv = simple->priv;

    if (priv->in_hex_sequence ||
        priv->tentative_match_len > 0 ||
        priv->compose_buffer[0] != 0) {
        g_string_set_size (priv->tentative_match, 0);
        priv->tentative_match_len = 0;
        priv->in_hex_sequence = FALSE;
        priv->in_compose_sequence = FALSE;
        priv->compose_buffer[0] = 0;
        /* Don't call ibus_engine_simple_update_preedit_text() inside
         * not to call it as duplilcated.
         */
    }
    if (priv->tentative_emoji || priv->in_emoji_sequence) {
        priv->in_emoji_sequence = FALSE;
        g_clear_pointer (&priv->tentative_emoji, g_free);
    }
    ibus_engine_commit_text ((IBusEngine *)simple,
            ibus_text_new_from_unichar (ch));
}


static void
ibus_engine_simple_commit_str (IBusEngineSimple *simple,
                               const char       *str)
{
    IBusEngineSimplePrivate *priv = simple->priv;
    char *backup_str;

    g_return_if_fail (str && *str);

    backup_str = g_strdup (str);

    if (priv->in_hex_sequence ||
        priv->tentative_match_len > 0 ||
        priv->compose_buffer[0] != 0) {
        g_string_set_size (priv->tentative_match, 0);
        priv->tentative_match_len = 0;
        priv->in_hex_sequence = FALSE;
        priv->in_compose_sequence = FALSE;
        priv->compose_buffer[0] = 0;
        /* Don't call ibus_engine_simple_update_preedit_text() inside
         * not to call it as duplilcated.
         */
    }
    if (priv->tentative_emoji || priv->in_emoji_sequence) {
        priv->in_emoji_sequence = FALSE;
        g_clear_pointer (&priv->tentative_emoji, g_free);
    }

    ibus_engine_commit_text ((IBusEngine *)simple,
            ibus_text_new_from_string (backup_str));
    g_free (backup_str);
}

static void
ibus_engine_simple_update_preedit_text (IBusEngineSimple *simple)
{
    IBusEngineSimplePrivate *priv = simple->priv;
    GString *s = g_string_new ("");
    int i, j;

    if (priv->in_hex_sequence || priv->in_emoji_sequence) {
        int hexchars = 0;

        if (priv->in_hex_sequence)
            g_string_append_c (s, 'u');
        else
            g_string_append_c (s, '@');

        while (priv->compose_buffer[hexchars] != 0) {
            g_string_append_unichar(
                    s,
                    ibus_keyval_to_unicode (priv->compose_buffer[hexchars++])
            );
        }
    } else if (priv->tentative_emoji && *priv->tentative_emoji) {
        IBusText *text = ibus_text_new_from_string (priv->tentative_emoji);
        int len = strlen (priv->tentative_emoji);
        ibus_text_append_attribute (text,
                IBUS_ATTR_TYPE_UNDERLINE, IBUS_ATTR_UNDERLINE_SINGLE, 0, len);
        g_object_ref_sink (text);
        ibus_engine_update_preedit_text ((IBusEngine *)simple, text, len, TRUE);
        g_object_unref (priv->updated_preedit);
        priv->updated_preedit = text;
        g_string_free (s, TRUE);
        return;
    } else if (priv->in_compose_sequence) {
        if (priv->tentative_match->len > 0 && priv->compose_buffer[0] != 0) {
            g_string_append (s, priv->tentative_match->str);
        } else {
            for (i = 0; priv->compose_buffer[i]; ++i) {
                guint keysym = priv->compose_buffer[i];
                gboolean show_keysym = TRUE;
                gboolean need_space = FALSE;
                gunichar ch;

                if (keysym == IBUS_KEY_Multi_key) {
                    /* We only show the Compose key visibly when it is the
                     * only glyph in the preedit, or when it occurs in the
                     * middle of the sequence. Sadly, the official character,
                     * U+2384, COMPOSITION SYMBOL, is bit too distracting, so
                     * we use U+00B7, MIDDLE DOT.
                     */
                    for (j = i + 1; priv->compose_buffer[j]; j++) {
                        if (priv->compose_buffer[j] != IBUS_KEY_Multi_key) {
                            show_keysym = FALSE;
                            break;
                        }
                    }
                    if (!show_keysym)
                        continue;
                    ch = ibus_keysym_to_unicode (keysym, FALSE, NULL);
                    g_string_append_unichar (s, ch);
                } else if (IS_DEAD_KEY (keysym)) {
                    ch = ibus_keysym_to_unicode (keysym, FALSE, &need_space);
                    if (ch) {
                        if (need_space)
                            g_string_append_c (s, ' ');
                        g_string_append_unichar (s, ch);
                    }
                } else {
                    ch = ibus_keyval_to_unicode (keysym);
                    if (ch) {
                        g_string_append_unichar(s, ch);
                    /* Can send Unicode char as keysym with <Uxxxx> format
                     * in comopse sequences and should not warn this case.
                     */
                    } else if (g_unichar_validate (keysym)) {
                        ch = keysym;
                        g_string_append_unichar(s, ch);
                    }
                }
                if (!ch) {
                    g_warning (
                        "Not found alternative character of compose key 0x%X",
                        priv->compose_buffer[i]);
                }
            }
        }
    }

    if (s->len == 0) {
        /* #2536 IBusEngine can inherit IBusEngineSimple for comopse keys.
         * If the previous preedit is zero, the current preedit does not
         * need to be hidden here at least because ibus-daemon could have
         * another preedit for the child IBusEnigne likes m17n and caclling
         * ibus_engine_hide_preedit_text() here could cause a reset of
         * the cursor position in ibus-daemon.
         */
        if (strlen (priv->updated_preedit->text)) {
            ibus_engine_hide_preedit_text ((IBusEngine *)simple);
            g_object_unref (priv->updated_preedit);
            priv->updated_preedit =
                    (IBusText *)g_object_ref_sink (updated_preedit_empty);
        }
    } else if (s->len >= G_MAXINT) {
        g_warning ("%s is too long compose length: %lu", s->str, s->len);
    } else {
        guint len = (guint)g_utf8_strlen (s->str, -1);
        IBusText *text = ibus_text_new_from_string (s->str);
        ibus_text_append_attribute (text,
                IBUS_ATTR_TYPE_UNDERLINE, IBUS_ATTR_UNDERLINE_SINGLE, 0, len);
        g_object_ref_sink (text);
        ibus_engine_update_preedit_text ((IBusEngine *)simple, text, len, TRUE);
        g_object_unref (priv->updated_preedit);
        priv->updated_preedit = text;
    }
    g_string_free (s, TRUE);
}


/* In addition to the table-driven sequences, we allow Unicode hex
 * codes to be entered. The method chosen here is similar to the
 * one recommended in ISO 14755, but not exactly the same, since we
 * don't want to steal 16 valuable key combinations.
 *
 * A hex Unicode sequence must be started with Ctrl-Shift-U, followed
 * by a sequence of hex digits entered with Ctrl-Shift still held.
 * Releasing one of the modifiers or pressing space while the modifiers
 * are still held commits the character. It is possible to erase
 * digits using backspace.
 *
 * As an extension to the above, we also allow to start the sequence
 * with Ctrl-Shift-U, then release the modifiers before typing any
 * digits, and enter the digits without modifiers.
 */
#define HEX_MOD_MASK (IBUS_CONTROL_MASK | IBUS_SHIFT_MASK)

static gboolean
check_hex (IBusEngineSimple *simple,
           int               n_compose)
{
    IBusEngineSimplePrivate *priv = simple->priv;

    int i;
    GString *str;
    gulong n;
    char *nptr = NULL;
    char buf[7];

    CHECK_COMPOSE_BUFFER_LENGTH (n_compose);

    g_string_set_size (priv->tentative_match, 0);
    priv->tentative_match_len = 0;

    str = g_string_new (NULL);

    i = 0;
    while (i < n_compose) {
        gunichar ch;

        ch = ibus_keyval_to_unicode (priv->compose_buffer[i]);

        if (ch == 0) {
            g_string_free (str, TRUE);
            return FALSE;
        }

        if (!g_unichar_isxdigit (ch)) {
            g_string_free (str, TRUE);
            return FALSE;
        }

        buf[g_unichar_to_utf8 (ch, buf)] = '\0';

        g_string_append (str, buf);

        ++i;
    }

    n = strtoul (str->str, &nptr, 16);

    /* if strtoul fails it probably means non-latin digits were used;
     * we should in principle handle that, but we probably don't.
     */
    if (nptr - str->str < str->len) {
        g_string_free (str, TRUE);
        return FALSE;
    } else {
        g_string_free (str, TRUE);
    }

    if (g_unichar_validate (n)) {
        g_string_set_size (priv->tentative_match, 0);
        g_string_append_unichar (priv->tentative_match, n);
        priv->tentative_match_len = n_compose;
    }

    return TRUE;
}

static IBusEngineDict *
load_emoji_dict (void)
{
    IBusEngineDict *emoji_dict;
    GList *keys;
    int max_length = 0;

    emoji_dict = g_slice_new0 (IBusEngineDict);
    emoji_dict->dict = ibus_emoji_dict_load (IBUS_DATA_DIR "/dicts/emoji.dict");
    if (!emoji_dict->dict)
        return emoji_dict;

    keys = g_hash_table_get_keys (emoji_dict->dict);
    for (; keys; keys = keys->next) {
        int length = strlen (keys->data);
        if (max_length < length)
            max_length = length;
    }
    emoji_dict->max_seq_len = max_length;

    return emoji_dict;
}

static gboolean
check_emoji_table (IBusEngineSimple       *simple,
                   int                     n_compose,
                   int                     index)
{
    IBusEngineSimplePrivate *priv = simple->priv;
    IBusEngineDict *emoji_dict = priv->emoji_dict;
    GString *str = NULL;
    int i;
    char buf[7];
    GSList *words = NULL;

    g_assert (IBUS_IS_ENGINE_SIMPLE (simple));

    CHECK_COMPOSE_BUFFER_LENGTH (n_compose);

    if (priv->lookup_table == NULL) {
        priv->lookup_table = ibus_lookup_table_new (10, 0, TRUE, TRUE);
        g_object_ref_sink (priv->lookup_table);
    }
    if (emoji_dict == NULL)
        emoji_dict = priv->emoji_dict = load_emoji_dict ();

    if (emoji_dict == NULL || emoji_dict->dict == NULL)
        return FALSE;

    if (n_compose > emoji_dict->max_seq_len)
        return FALSE;

    str = g_string_new (NULL);
    priv->lookup_table_visible = FALSE;

    i = 0;
    while (i < n_compose) {
        gunichar ch;

        ch = ibus_keyval_to_unicode (priv->compose_buffer[i]);

        if (ch == 0) {
            g_string_free (str, TRUE);
            return FALSE;
        }

        if (!g_unichar_isprint (ch)) {
            g_string_free (str, TRUE);
            return FALSE;
        }

        buf[g_unichar_to_utf8 (ch, buf)] = '\0';

        g_string_append (str, buf);

        ++i;
    }

    if (str->str) {
        words = g_hash_table_lookup (emoji_dict->dict, str->str);
    }
    g_string_free (str, TRUE);

    if (words != NULL) {
        int i = 0;
        ibus_lookup_table_clear (priv->lookup_table);
        priv->lookup_table_visible = TRUE;

        while (words) {
            if (i == index) {
                g_clear_pointer (&priv->tentative_emoji, g_free);
                priv->tentative_emoji = g_strdup (words->data);
            }
            IBusText *text = ibus_text_new_from_string (words->data);
            ibus_lookup_table_append_candidate (priv->lookup_table, text);
            words = words->next;
            i++;
        }
        return TRUE;
    }

    return FALSE;
}


static gboolean
no_sequence_matches (IBusEngineSimple *simple,
                     int               n_compose,
                     guint             keyval,
                     guint             keycode,
                     guint             modifiers)
{
    IBusEngineSimplePrivate *priv = simple->priv;

    gunichar ch;

    CHECK_COMPOSE_BUFFER_LENGTH (n_compose);

    priv->in_compose_sequence = FALSE;

    /* No compose sequences found, check first if we have a partial
     * match pending.
     */
    if (priv->tentative_match_len > 0) {
        guint *compose_buffer;
        int len = priv->tentative_match_len;
        int i;
        char *str;

        compose_buffer = alloca (sizeof (guint) * COMPOSE_BUFFER_SIZE);
        memcpy (compose_buffer,
                priv->compose_buffer,
                sizeof (guint) * COMPOSE_BUFFER_SIZE);

        str = g_strdup (priv->tentative_match->str);
        ibus_engine_simple_commit_str (simple, str);
        g_free (str);
        ibus_engine_simple_update_preedit_text (simple);

        for (i=0; i < n_compose - len - 1; i++) {
            ibus_engine_simple_process_key_event (
                    (IBusEngine *)simple,
                    compose_buffer[len + i],
                    0, 0);
        }

        return ibus_engine_simple_process_key_event (
                (IBusEngine *)simple, keyval, keycode, modifiers);
    } else if (priv->tentative_emoji && *priv->tentative_emoji) {
        ibus_engine_simple_commit_str (simple, priv->tentative_emoji);
        priv->compose_buffer[0] = 0;
        ibus_engine_simple_update_preedit_text (simple);
    } else {
        if (n_compose == 2 && IS_DEAD_KEY (priv->compose_buffer[0])) {
            gboolean need_space = FALSE;
            GString *s = g_string_new ("");
            /* dead keys are never *really* dead */
            ch = ibus_keysym_to_unicode (priv->compose_buffer[0],
                                         FALSE, &need_space);
            if (ch) {
                if (need_space)
                    g_string_append_c (s, ' ');
                g_string_append_unichar (s, ch);
            }
            ch = ibus_keyval_to_unicode (priv->compose_buffer[1]);
            if (ch != 0 && !g_unichar_iscntrl (ch))
                g_string_append_unichar (s, ch);
            ibus_engine_simple_commit_str (simple, s->str);
            g_string_free (s, TRUE);
            ibus_engine_simple_update_preedit_text (simple);
            return TRUE;
        }

        priv->compose_buffer[0] = 0;
        if (n_compose > 1) {
            /* Invalid sequence */
            /* FIXME beep_window (event->window); */
            ibus_engine_simple_update_preedit_text (simple);
            return TRUE;
        }

        ibus_engine_simple_update_preedit_text (simple);
        ch = ibus_keyval_to_unicode (keyval);
        /* IBUS_CHANGE: RH#769133, #2588
         * Since we use ibus xkb engines as the disable IM mode,
         * do not commit the characters locally without in_hex_sequence.
         * If IBus tries to commit a character, it should be forwarded to
         * the application at once with IBUS_IGNORED_MASK before the actual
         * commit because any characters can be control characters even if
         * they are not ASCII characters, e.g. game cursor keys with a
         * language keyboard layout likes VIM cursor mode  "hjkl" keys.
         */
        if (ch != 0 && !g_unichar_iscntrl (ch) &&
            priv->in_hex_sequence) {
            return TRUE;
        } else {
            return FALSE;
        }
    }
    return FALSE;
}

static gboolean
is_hex_keyval (guint keyval)
{
  gunichar ch = ibus_keyval_to_unicode (keyval);

  return g_unichar_isxdigit (ch);
}

static gboolean
is_graph_keyval (guint keyval)
{
  gunichar ch = ibus_keyval_to_unicode (keyval);

  return g_unichar_isgraph (ch);
}

static void
ibus_engine_simple_update_lookup_and_aux_table (IBusEngineSimple *simple)
{
    IBusEngineSimplePrivate *priv;
    guint index, candidates;
    char *aux_label = NULL;
    IBusText *text = NULL;

    g_return_if_fail (IBUS_IS_ENGINE_SIMPLE (simple));

    priv = simple->priv;
    index = ibus_lookup_table_get_cursor_pos (priv->lookup_table) + 1;
    candidates = ibus_lookup_table_get_number_of_candidates(priv->lookup_table);
    aux_label = g_strdup_printf ("(%u / %u)", index, candidates);
    text = ibus_text_new_from_string (aux_label);
    g_free (aux_label);

    ibus_engine_update_auxiliary_text (IBUS_ENGINE (simple),
                                       text,
                                       priv->lookup_table_visible);
    ibus_engine_update_lookup_table (IBUS_ENGINE (simple),
                                     priv->lookup_table,
                                     priv->lookup_table_visible);
}

static gboolean
ibus_engine_simple_if_in_range_of_lookup_table (IBusEngineSimple *simple,
                                                guint             keyval)
{
    IBusEngineSimplePrivate *priv;
    int index, candidates, cursor_pos, cursor_in_page, page_size;

    priv = simple->priv;

    if (priv->lookup_table == NULL || !priv->lookup_table_visible)
        return FALSE;
    if (keyval < IBUS_KEY_0 || keyval > IBUS_KEY_9)
        return FALSE;
    if (keyval == IBUS_KEY_0)
        keyval = IBUS_KEY_9 + 1;
    index = keyval - IBUS_KEY_1;
    candidates =
            ibus_lookup_table_get_number_of_candidates (priv->lookup_table);
    cursor_pos = ibus_lookup_table_get_cursor_pos (priv->lookup_table);
    cursor_in_page = ibus_lookup_table_get_cursor_in_page (priv->lookup_table);
    page_size = ibus_lookup_table_get_page_size (priv->lookup_table);
    if (index > ((candidates - (cursor_pos - cursor_in_page)) % page_size))
        return FALSE;
    return TRUE;
}

static void
ibus_engine_simple_set_number_on_lookup_table (IBusEngineSimple *simple,
                                               guint             keyval,
                                               int               n_compose)
{
    IBusEngineSimplePrivate *priv;
    int index, cursor_pos, cursor_in_page, real_index;

    priv = simple->priv;

    if (keyval == IBUS_KEY_0)
        keyval = IBUS_KEY_9 + 1;
    index = keyval - IBUS_KEY_1;
    cursor_pos = ibus_lookup_table_get_cursor_pos (priv->lookup_table);
    cursor_in_page = ibus_lookup_table_get_cursor_in_page (priv->lookup_table);
    real_index = cursor_pos - cursor_in_page + index;

    ibus_lookup_table_set_cursor_pos (priv->lookup_table, real_index);
    check_emoji_table (simple, n_compose, real_index);
    priv->lookup_table_visible = FALSE;
    ibus_engine_simple_update_lookup_and_aux_table (simple);

    if (priv->tentative_emoji && *priv->tentative_emoji) {
        ibus_engine_simple_commit_str (simple, priv->tentative_emoji);
        priv->compose_buffer[0] = 0;
    } else {
        g_clear_pointer (&priv->tentative_emoji, g_free);
        priv->in_emoji_sequence = FALSE;
        priv->compose_buffer[0] = 0;
    }

    ibus_engine_simple_update_preedit_text (simple);
}


static gboolean
ibus_engine_simple_check_all_compose_table (IBusEngineSimple *simple,
                                            int               n_compose)
{
    IBusEngineSimplePrivate *priv = simple->priv;
    GSList *tmp_list;
    gboolean compose_finish = FALSE;
    gboolean compose_match = FALSE;
    GString *output = g_string_new ("");
    gboolean success = FALSE;
    gboolean is_32bit = FALSE;
    gunichar output_char = '\0';

    /* GtkIMContextSimple output the first compose char in case of
     * n_compose == 2 but it does not work in fi_FI copmose to output U+1EDD
     * with the following sequence:
     * <dead_hook> <dead_horn> <o> : "ờ" U1EDD
     */

    G_LOCK (global_tables);
    tmp_list = global_tables;
    while (tmp_list) {
        is_32bit = FALSE;
        if (ibus_compose_table_check (
            (IBusComposeTableEx *)tmp_list->data,
            priv->compose_buffer,
            n_compose,
            &compose_finish,
            &compose_match,
            output,
            is_32bit)) {
            success = TRUE;
            break;
        }
        is_32bit = TRUE;
        if (ibus_compose_table_check (
            (IBusComposeTableEx *)tmp_list->data,
            priv->compose_buffer,
            n_compose,
            &compose_finish,
            &compose_match,
            output,
            is_32bit)) {
            success = TRUE;
            break;
        }
        tmp_list = tmp_list->next;
    }
    G_UNLOCK (global_tables);

    if (success) {
        priv->in_compose_sequence = TRUE;
        if (compose_finish) {
            if (compose_match) {
                if (is_32bit) {
                    ibus_engine_simple_commit_str (simple, output->str);
                } else {
                    ibus_engine_simple_commit_char (simple,
                                                    g_utf8_get_char (output->str));
                }
            }
            ibus_engine_simple_update_preedit_text (simple);
            g_string_free (output, TRUE);
            return TRUE;
        } else if (compose_match) {
            g_string_assign (priv->tentative_match, output->str);
            priv->tentative_match_len = n_compose;
            ibus_engine_simple_update_preedit_text (simple);
            g_string_free (output, TRUE);
            return TRUE;
        }
    }
    g_string_free (output, TRUE);
    output = NULL;

    if (ibus_check_algorithmically (priv->compose_buffer,
                                    n_compose,
                                    &output_char)) {
        priv->in_compose_sequence = TRUE;
        if (output_char) {
            if (success) {
                g_string_append_unichar (priv->tentative_match, output_char);
                priv->tentative_match_len = n_compose;
            } else  {
                ibus_engine_simple_commit_char (simple, output_char);
                priv->compose_buffer[0] = 0;
            }
            ibus_engine_simple_update_preedit_text (simple);
            return TRUE;
        }
        success = TRUE;
    }
    if (success) {
        ibus_engine_simple_update_preedit_text (simple);
        return TRUE;
    }

    return FALSE;
}


static gboolean
ibus_engine_simple_process_key_event (IBusEngine *engine,
                                      guint       keyval,
                                      guint       keycode,
                                      guint       modifiers)
{
    IBusEngineSimple *simple = (IBusEngineSimple *)engine;
    IBusEngineSimplePrivate *priv = simple->priv;
    int n_compose = 0;
    int n_compose_prev;
    gboolean have_hex_mods;
    gboolean is_hex_start = FALSE;
    gboolean is_emoji_start = FALSE;
    gboolean is_hex_end;
    gboolean is_space;
    gboolean is_backspace;
    gboolean is_escape;
    guint hex_keyval;
    guint printable_keyval;
    int i;

    while (n_compose <= COMPOSE_BUFFER_SIZE && priv->compose_buffer[n_compose] != 0)
        n_compose++;
    if (n_compose > COMPOSE_BUFFER_SIZE) {
        g_warning ("copmose table buffer is full.");
        n_compose = COMPOSE_BUFFER_SIZE;
    }
    n_compose_prev = n_compose;

    if (modifiers & IBUS_RELEASE_MASK) {
        if (priv->in_hex_sequence &&
            (keyval == IBUS_KEY_Control_L || keyval == IBUS_KEY_Control_R ||
             keyval == IBUS_KEY_Shift_L || keyval == IBUS_KEY_Shift_R)) {
            if (priv->tentative_match->len > 0) {
                char *str = g_strdup (priv->tentative_match->str);
                ibus_engine_simple_commit_str (simple, str);
                g_free (str);
                ibus_engine_simple_update_preedit_text (simple);
            } else if (n_compose == 0) {
                priv->modifiers_dropped = TRUE;
            } else {
                /* invalid hex sequence */
                /* FIXME beep_window (event->window); */
                g_string_set_size (priv->tentative_match, 0);
                g_clear_pointer (&priv->tentative_emoji, g_free);
                priv->in_hex_sequence = FALSE;
                priv->in_emoji_sequence = FALSE;
                priv->compose_buffer[0] = 0;

                ibus_engine_simple_update_preedit_text (simple);
            }

            return TRUE;
        }
        /* Handle Shift + Space */
        else if (priv->in_emoji_sequence &&
            (keyval == IBUS_KEY_Control_L || keyval == IBUS_KEY_Control_R)) {
            if (priv->tentative_emoji && *priv->tentative_emoji) {
                ibus_engine_simple_commit_str (simple, priv->tentative_emoji);
                priv->compose_buffer[0] = 0;
                ibus_engine_simple_update_preedit_text (simple);
            } else if (n_compose == 0) {
                priv->modifiers_dropped = TRUE;
            } else {
                /* invalid hex sequence */
                /* FIXME beep_window (event->window); */
                g_string_set_size (priv->tentative_match, 0);
                g_clear_pointer (&priv->tentative_emoji, g_free);
                priv->in_hex_sequence = FALSE;
                priv->in_emoji_sequence = FALSE;
                priv->compose_buffer[0] = 0;

                ibus_engine_simple_update_preedit_text (simple);
            }
        }
        else
            return FALSE;
    }

    /* Ignore modifier key presses */
    for (i = 0; i < G_N_ELEMENTS (IBUS_COMPOSE_IGNORE_KEYLIST); i++)
        if (keyval == IBUS_COMPOSE_IGNORE_KEYLIST[i])
            return FALSE;

    if ((priv->in_hex_sequence || priv->in_emoji_sequence)
        && priv->modifiers_dropped) {
        have_hex_mods = TRUE;
    } else {
        have_hex_mods = (modifiers & (HEX_MOD_MASK)) == HEX_MOD_MASK;
    }

    is_hex_start = (keyval == IBUS_KEY_U) && priv->hex_mode_enabled;
    is_hex_end = (keyval == IBUS_KEY_space ||
                  keyval == IBUS_KEY_KP_Space ||
                  keyval == IBUS_KEY_Return ||
                  keyval == IBUS_KEY_ISO_Enter ||
                  keyval == IBUS_KEY_KP_Enter);
    is_space = (keyval == IBUS_KEY_space || keyval == IBUS_KEY_KP_Space);
    is_backspace = keyval == IBUS_KEY_BackSpace;
    is_escape = keyval == IBUS_KEY_Escape;
    hex_keyval = is_hex_keyval (keyval) ? keyval : 0;
    printable_keyval = is_graph_keyval (keyval) ? keyval : 0;

    /* gtkimcontextsimple causes a buffer overflow in priv->compose_buffer.
     * Add the check code here.
     */
    if (n_compose > COMPOSE_BUFFER_SIZE &&
        (priv->in_hex_sequence || priv->in_emoji_sequence)) {
        if (is_backspace) {
            priv->compose_buffer[--n_compose] = 0;
            n_compose_prev = n_compose;
        }
        else if (is_hex_end) {
            /* invalid hex sequence */
            /* FIXME beep_window (event->window); */
            g_string_set_size (priv->tentative_match, 0);
            g_clear_pointer (&priv->tentative_emoji, g_free);
            priv->in_hex_sequence = FALSE;
            priv->in_emoji_sequence = FALSE;
            priv->compose_buffer[0] = 0;
        }
        else if (is_escape) {
            ibus_engine_simple_reset (engine);
            if (priv->lookup_table != NULL && priv->lookup_table_visible) {
                priv->lookup_table_visible = FALSE;
                ibus_engine_simple_update_lookup_and_aux_table (simple);
            }
            return TRUE;
        }

        if (have_hex_mods)
            ibus_engine_simple_update_preedit_text (simple);

        return TRUE;
    }

    /* If we are already in a non-hex sequence, or
     * this keystroke is not hex modifiers + hex digit, don't filter
     * key events with accelerator modifiers held down. We only treat
     * Control and Alt as accel modifiers here, since Super, Hyper and
     * Meta are often co-located with Mode_Switch, Multi_Key or
     * ISO_Level3_Switch.
     */
    if (!have_hex_mods ||
        (n_compose > 0 && !priv->in_hex_sequence && !priv->in_emoji_sequence) ||
        (n_compose == 0 && !priv->in_hex_sequence && !is_hex_start &&
         !priv->in_emoji_sequence && !is_emoji_start) ||
        (priv->in_hex_sequence && !hex_keyval &&
         !is_hex_start && !is_hex_end && !is_escape && !is_backspace) ||
        (priv->in_emoji_sequence && !printable_keyval &&
         !is_emoji_start && !is_hex_end && !is_escape && !is_backspace)) {
        guint no_text_input_mask = IBUS_MOD1_MASK | IBUS_MOD4_MASK \
                                   | IBUS_CONTROL_MASK | IBUS_SUPER_MASK;
        if (modifiers & no_text_input_mask ||
            ((priv->in_hex_sequence || priv->in_emoji_sequence) &&
             priv->modifiers_dropped &&
             (keyval == IBUS_KEY_Return ||
              keyval == IBUS_KEY_ISO_Enter ||
              keyval == IBUS_KEY_KP_Enter))) {
              return FALSE;
        }
    }

    /* Handle backspace */
    if (priv->in_hex_sequence && have_hex_mods && is_backspace) {
        if (n_compose > 0) {
            priv->compose_buffer[--n_compose] = 0;
            n_compose_prev = n_compose;
            check_hex (simple, n_compose);
        } else {
            priv->in_hex_sequence = FALSE;
        }

        ibus_engine_simple_update_preedit_text (simple);

        return TRUE;
    }
    if (priv->in_emoji_sequence && have_hex_mods && is_backspace) {
        if (n_compose > 0) {
            priv->compose_buffer[--n_compose] = 0;
            n_compose_prev = n_compose;
            check_emoji_table (simple, n_compose, -1);
            ibus_engine_simple_update_lookup_and_aux_table (simple);
        } else {
            priv->in_emoji_sequence = FALSE;
        }

        ibus_engine_simple_update_preedit_text (simple);

        return TRUE;
    }
    if (!priv->in_hex_sequence && !priv->in_emoji_sequence && is_backspace) {
        if (n_compose > 0) {
            priv->compose_buffer[--n_compose] = 0;
            n_compose_prev = n_compose;
            g_string_set_size (priv->tentative_match, 0);
            priv->tentative_match_len = 0;
            ibus_engine_simple_check_all_compose_table (simple, n_compose);
            return TRUE;
        }
    }

    /* Check for hex sequence restart */
    if (priv->in_hex_sequence && have_hex_mods && is_hex_start) {
        if (priv->tentative_match->len > 0 ) {
            char *str = g_strdup (priv->tentative_match->str);
            ibus_engine_simple_commit_str (simple, str);
            g_free (str);
            ibus_engine_simple_update_preedit_text (simple);
        }
        else {
            /* invalid hex sequence */
            if (n_compose > 0) {
                /* FIXME beep_window (event->window); */
                g_string_set_size (priv->tentative_match, 0);
                priv->in_hex_sequence = FALSE;
                priv->compose_buffer[0] = 0;
            }
        }
    }
    if (priv->in_emoji_sequence && have_hex_mods && is_emoji_start) {
        if (priv->tentative_emoji && *priv->tentative_emoji) {
            ibus_engine_simple_commit_str (simple, priv->tentative_emoji);
            priv->compose_buffer[0] = 0;
            ibus_engine_simple_update_preedit_text (simple);
        }
        else {
            if (n_compose > 0) {
                g_clear_pointer (&priv->tentative_emoji, g_free);
                priv->in_emoji_sequence = FALSE;
                priv->compose_buffer[0] = 0;
            }
        }
    }

    /* Check for hex sequence start */
    if (!priv->in_hex_sequence && !priv->in_emoji_sequence &&
        have_hex_mods && is_hex_start) {
        priv->compose_buffer[0] = 0;
        priv->in_hex_sequence = TRUE;
        priv->in_emoji_sequence = FALSE;
        priv->modifiers_dropped = FALSE;
        g_string_set_size (priv->tentative_match, 0);
        g_clear_pointer (&priv->tentative_emoji, g_free);

        // g_debug ("Start HEX MODE");

        ibus_engine_simple_update_preedit_text (simple);

        return TRUE;
    } else if (!priv->in_hex_sequence && !priv->in_emoji_sequence &&
               have_hex_mods && is_emoji_start) {
        priv->compose_buffer[0] = 0;
        priv->in_hex_sequence = FALSE;
        priv->in_emoji_sequence = TRUE;
        priv->modifiers_dropped = FALSE;
        g_string_set_size (priv->tentative_match, 0);
        g_clear_pointer (&priv->tentative_emoji, g_free);

        // g_debug ("Start HEX MODE");

        ibus_engine_simple_update_preedit_text (simple);

        return TRUE;
    }

    if (priv->in_hex_sequence) {
        if (hex_keyval) {
            SET_COMPOSE_BUFFER_ELEMENT_NEXT (priv->compose_buffer,
                                             n_compose,
                                             hex_keyval);
        } else if (is_escape) {
            // FIXME
            ibus_engine_simple_reset (engine);

            return TRUE;
        } else if (!is_hex_end) {
            /* non-hex character in hex sequence */
            /* FIXME beep_window (event->window); */
            return TRUE;
        }
    } else if (priv->in_emoji_sequence) {
        if (printable_keyval) {
            if (!ibus_engine_simple_if_in_range_of_lookup_table (simple,
                        printable_keyval)) {
                /* digit keyval can be an index on the current lookup table
                 * but it also can be a part of an emoji annotation.
                 * E.g. "1" and "2" are  indexes of emoji "1".
                 * "100" is an annotation of the emoji "100".
                 */
                SET_COMPOSE_BUFFER_ELEMENT_NEXT (priv->compose_buffer,
                                                 n_compose,
                                                 printable_keyval);
            }
        }
        else if (is_space && (modifiers & IBUS_SHIFT_MASK)) {
            SET_COMPOSE_BUFFER_ELEMENT_NEXT (priv->compose_buffer,
                                             n_compose,
                                             IBUS_KEY_space);
        }
        else if (is_escape) {
            ibus_engine_simple_reset (engine);
            if (priv->lookup_table != NULL && priv->lookup_table_visible) {
                priv->lookup_table_visible = FALSE;
                ibus_engine_simple_update_lookup_and_aux_table (simple);
            }
            return TRUE;
        }
    } else {
        if (is_escape) {
            if (n_compose > 0) {
                ibus_engine_simple_reset (engine);
                return TRUE;
            }
        }
        SET_COMPOSE_BUFFER_ELEMENT_NEXT (priv->compose_buffer,
                                         n_compose,
                                         keyval);
    }

    SET_COMPOSE_BUFFER_ELEMENT_END (priv->compose_buffer,
                                    n_compose,
                                    0);

    if (priv->in_hex_sequence) {
        /* If the modifiers are still held down, consider the sequence again */
        if (have_hex_mods) {
            /* space or return ends the sequence, and we eat the key */
            if (n_compose > 0 && is_hex_end) {
                if (priv->tentative_match->len > 0) {
                    char *str = g_strdup (priv->tentative_match->str);
                    ibus_engine_simple_commit_str (simple, str);
                    g_free (str);
                    ibus_engine_simple_update_preedit_text (simple);
                    return TRUE;
                } else {
                    /* invalid hex sequence */
                    /* FIXME beep_window (event->window); */
                    g_string_set_size (priv->tentative_match, 0);
                    priv->in_hex_sequence = FALSE;
                    priv->compose_buffer[0] = 0;
                }
            }
            else if (!check_hex (simple, n_compose))
                /* FIXME beep_window (event->window); */
                ;
            ibus_engine_simple_update_preedit_text (simple);

            return TRUE;
        }
    }
    else if (priv->in_emoji_sequence) {
        if (have_hex_mods && n_compose > 0) {
            gboolean update_lookup_table = FALSE;

            if (priv->lookup_table_visible) {
                switch (keyval) {
                case IBUS_KEY_space:
                case IBUS_KEY_KP_Space:
                    if ((modifiers & IBUS_SHIFT_MASK) == 0) {
                        ibus_lookup_table_cursor_down (priv->lookup_table);
                        update_lookup_table = TRUE;
                    }
                    break;
                case IBUS_KEY_Down:
                    ibus_lookup_table_cursor_down (priv->lookup_table);
                    update_lookup_table = TRUE;
                    break;
                case IBUS_KEY_Up:
                    ibus_lookup_table_cursor_up (priv->lookup_table);
                    update_lookup_table = TRUE;
                    break;
                case IBUS_KEY_Page_Down:
                    ibus_lookup_table_page_down (priv->lookup_table);
                    update_lookup_table = TRUE;
                    break;
                case IBUS_KEY_Page_Up:
                    ibus_lookup_table_page_up (priv->lookup_table);
                    update_lookup_table = TRUE;
                    break;
                default:;
                }
            }

            if (!update_lookup_table) {
                if (ibus_engine_simple_if_in_range_of_lookup_table (simple,
                            keyval)) {
                        ibus_engine_simple_set_number_on_lookup_table (
                                simple,
                                keyval,
                                n_compose);
                        return TRUE;
                }
                else if (is_hex_end && !is_space) {
                    if (priv->lookup_table) {
                        int index = (int) ibus_lookup_table_get_cursor_pos (
                                priv->lookup_table);
                        check_emoji_table (simple, n_compose, index);
                        priv->lookup_table_visible = FALSE;
                        update_lookup_table = TRUE;
                    }
                } else if (check_emoji_table (simple, n_compose, -1)) {
                    update_lookup_table = TRUE;
                } else {
                    priv->lookup_table_visible = FALSE;
                    update_lookup_table = TRUE;
                }
            }

            if (update_lookup_table)
                ibus_engine_simple_update_lookup_and_aux_table (simple);
            if (is_hex_end && !is_space) {
                if (priv->tentative_emoji && *priv->tentative_emoji) {
                    ibus_engine_simple_commit_str (simple,
                            priv->tentative_emoji);
                    priv->compose_buffer[0] = 0;
                    ibus_engine_simple_update_preedit_text (simple);
                } else {
                    g_clear_pointer (&priv->tentative_emoji, g_free);
                    priv->in_emoji_sequence = FALSE;
                    priv->compose_buffer[0] = 0;
                }
            }

            ibus_engine_simple_update_preedit_text (simple);

            return TRUE;
        }
    } else { /* Then, check for compose sequences */
        if (ibus_engine_simple_check_all_compose_table (simple, n_compose)) {
            return TRUE;
        } else if (n_compose_prev > 0 && (n_compose - n_compose_prev) > 0) {
            /* Show the previous preedit text. */
            guint backup_char = 0;

            n_compose = n_compose_prev;
            g_assert (n_compose < (COMPOSE_BUFFER_SIZE + 1));
            /* FIXME beep_window (event->window); */
            backup_char = priv->compose_buffer[n_compose];
            priv->compose_buffer[n_compose] = 0;
            if (ibus_engine_simple_check_all_compose_table (simple, n_compose))
                return TRUE;
            priv->compose_buffer[n_compose] = backup_char;
        }
    }

    /* The current compose_buffer doesn't match anything */
    return no_sequence_matches (simple, n_compose, keyval, keycode, modifiers);
}

static void
ibus_engine_simple_page_down (IBusEngine *engine)
{
    IBusEngineSimple *simple = (IBusEngineSimple *)engine;
    IBusEngineSimplePrivate *priv = simple->priv;
    if (priv->lookup_table == NULL)
        return;
    ibus_lookup_table_page_down (priv->lookup_table);
    ibus_engine_simple_update_lookup_and_aux_table (simple);
}

static void
ibus_engine_simple_page_up (IBusEngine *engine)
{
    IBusEngineSimple *simple = (IBusEngineSimple *)engine;
    IBusEngineSimplePrivate *priv = simple->priv;
    if (priv->lookup_table == NULL)
        return;
    ibus_lookup_table_page_up (priv->lookup_table);
    ibus_engine_simple_update_lookup_and_aux_table (simple);
}

static void
ibus_engine_simple_candidate_clicked (IBusEngine *engine,
                                      guint       index,
                                      guint       button,
                                      guint       state)
{
    IBusEngineSimple *simple = (IBusEngineSimple *)engine;
    IBusEngineSimplePrivate *priv = simple->priv;
    guint keyval;
    int n_compose = 0;

    if (priv->lookup_table == NULL || !priv->lookup_table_visible)
        return;
    if (index == 9)
        keyval = IBUS_KEY_0;
    else
        keyval = IBUS_KEY_1 + index;
    while (priv->compose_buffer[n_compose] != 0)
        n_compose++;
    CHECK_COMPOSE_BUFFER_LENGTH (n_compose);
    ibus_engine_simple_set_number_on_lookup_table (simple, keyval, n_compose);
}

void
ibus_engine_simple_add_table (IBusEngineSimple *simple,
                              const guint16    *data,
                              gint              max_seq_len,
                              gint              n_seqs)
{
    g_return_if_fail (IBUS_IS_ENGINE_SIMPLE (simple));

    global_tables = ibus_compose_table_list_add_array (global_tables,
                                                       data,
                                                       max_seq_len,
                                                       n_seqs);
}

gboolean
ibus_engine_simple_add_table_by_locale (IBusEngineSimple *simple,
                                        const gchar      *locale)
{
    /* Now ibus_engine_simple_add_compose_file() always returns TRUE. */
    gboolean retval = TRUE;
    char *path = NULL;
    const char *home;
#if GLIB_CHECK_VERSION (2, 58, 0)
    const char * const *langs;
    const char * const *lang = NULL;
#else
    const char *_locale;
    char **langs = NULL;
    char **lang = NULL;
#endif
    char * const sys_langs[] = { "el_gr", "fi_fi", "pt_br", NULL };
    char * const *sys_lang = NULL;

    if (locale == NULL) {
        path = g_build_filename (g_get_user_config_dir (),
                                 "ibus", "Compose", NULL);
        if (g_file_test (path, G_FILE_TEST_EXISTS)) {
            ibus_engine_simple_add_compose_file (simple, path);
            g_free (path);
            return retval;
        }
        g_clear_pointer(&path, g_free);

        path = g_build_filename (g_get_user_config_dir (),
                                 "gtk-4.0", "Compose", NULL);
        if (g_file_test (path, G_FILE_TEST_EXISTS)) {
            ibus_engine_simple_add_compose_file (simple, path);
            g_free (path);
            return retval;
        }
        g_clear_pointer(&path, g_free);

        path = g_build_filename (g_get_user_config_dir (),
                                 "gtk-3.0", "Compose", NULL);
        if (g_file_test (path, G_FILE_TEST_EXISTS)) {
            ibus_engine_simple_add_compose_file (simple, path);
            g_free (path);
            return retval;
        }
        g_clear_pointer(&path, g_free);

        home = g_get_home_dir ();
        if (home == NULL)
            return retval;

        path = g_build_filename (home, ".XCompose", NULL);
        if (g_file_test (path, G_FILE_TEST_EXISTS)) {
            ibus_engine_simple_add_compose_file (simple, path);
            g_free (path);
            return retval;
        }
        g_clear_pointer(&path, g_free);

#if GLIB_CHECK_VERSION (2, 58, 0)
        langs = g_get_language_names_with_category ("LC_CTYPE");
#else
        _locale = g_getenv ("LC_CTYPE");
        if (_locale == NULL)
            _locale = g_getenv ("LANG");
        if (_locale == NULL)
            _locale = "C";

        /* FIXME: https://bugzilla.gnome.org/show_bug.cgi?id=751826 */
        langs = g_get_locale_variants (_locale);
#endif

        for (lang = langs; *lang; lang++) {
            if (g_str_has_prefix (*lang, "en_US"))
                break;
            if (**lang == 'C')
                break;

            /* Other languages just include en_us compose table. */
            for (sys_lang = sys_langs; *sys_lang; sys_lang++) {
                if (g_ascii_strncasecmp (*lang, *sys_lang,
                                         strlen (*sys_lang)) == 0) {
                    path = g_build_filename (X11_LOCALEDATADIR,
                                             *lang, "Compose", NULL);
                    break;
                }
            }

            if (path == NULL)
                continue;

            if (g_file_test (path, G_FILE_TEST_EXISTS))
                break;
            g_clear_pointer(&path, g_free);
        }

#if !GLIB_CHECK_VERSION (2, 58, 0)
        g_strfreev (langs);
#endif

        if (path != NULL)
            ibus_engine_simple_add_compose_file (simple, path);
        g_clear_pointer(&path, g_free);
    } else {
        path = g_build_filename (X11_LOCALEDATADIR, locale, "Compose", NULL);
        do {
            if (g_file_test (path, G_FILE_TEST_EXISTS))
                break;
            g_clear_pointer(&path, g_free);
        } while (0);
        if (path == NULL)
            return retval;
        ibus_engine_simple_add_compose_file (simple, path);
    }

    return retval;
}

gboolean
ibus_engine_simple_add_compose_file (IBusEngineSimple *simple,
                                     const gchar      *compose_file)
{
    g_return_val_if_fail (IBUS_IS_ENGINE_SIMPLE (simple), TRUE);

    global_tables = ibus_compose_table_list_add_file (global_tables,
                                                      compose_file);
    return TRUE;
}
