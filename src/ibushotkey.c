/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* IBus - The Input Bus
 * Copyright (C) 2008-2010 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2018 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2008-2018 Red Hat, Inc.
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
#include "ibushotkey.h"
#include "ibusmarshalers.h"
#include "ibuskeysyms.h"
#include "ibusinternal.h"
#include "ibusshare.h"

#define IBUS_HOTKEY_PROFILE_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), IBUS_TYPE_HOTKEY_PROFILE, IBusHotkeyProfilePrivate))

enum {
    TRIGGER,
    LAST_SIGNAL,
};

typedef struct _IBusHotkey IBusHotkey;
typedef struct _IBusHotkeyEvent IBusHotkeyEvent;
typedef struct _IBusHotkeyProfilePrivate IBusHotkeyProfilePrivate;

struct _IBusHotkey {
    guint   keyval;
    guint   modifiers;
};

struct _IBusHotkeyEvent {
    GQuark event;
    GList *hotkeys;
};

struct _IBusHotkeyProfilePrivate {
    GTree *hotkeys;
    GArray *events;
    guint   mask;
};



/* functions prototype */
static IBusHotkey   *ibus_hotkey_new                (guint                   keyval,
                                                     guint                   modifiers);
static IBusHotkey   *ibus_hotkey_copy               (const IBusHotkey       *src);
static void          ibus_hotkey_free               (IBusHotkey             *hotkey);
static void          ibus_hotkey_profile_class_init (IBusHotkeyProfileClass *class);
static void          ibus_hotkey_profile_init       (IBusHotkeyProfile      *profile);
static void          ibus_hotkey_profile_destroy    (IBusHotkeyProfile      *profile);
static gboolean      ibus_hotkey_profile_serialize  (IBusHotkeyProfile      *profile,
                                                     GVariantBuilder        *builder);
static gint          ibus_hotkey_profile_deserialize(IBusHotkeyProfile      *profile,
                                                     GVariant               *variant);
static gboolean      ibus_hotkey_profile_copy       (IBusHotkeyProfile      *dest,
                                                     const IBusHotkeyProfile*src);
static void          ibus_hotkey_profile_trigger    (IBusHotkeyProfile      *profile,
                                                     GQuark                  event,
                                                     gpointer                user_data);

// Normalize modifiers by setting necessary modifier bits according to keyval.
static guint         normalize_modifiers            (guint                   keyval,
                                                     guint                   modifiers);
static gboolean      is_modifier                    (guint                   keyval);

static IBusSerializableClass *parent_class = NULL;

static guint profile_signals[LAST_SIGNAL] = { 0 };

GType
ibus_hotkey_get_type (void)
{
    static GType type = 0;

    if (type == 0) {
        type = g_boxed_type_register_static ("IBusHotkey",
                                             (GBoxedCopyFunc) ibus_hotkey_copy,
                                             (GBoxedFreeFunc) ibus_hotkey_free);
    }

    return type;
}

static IBusHotkey *
ibus_hotkey_new (guint keyval,
                 guint modifiers)
{
    IBusHotkey *hotkey = g_slice_new (IBusHotkey);

    hotkey->keyval = keyval;
    hotkey->modifiers = modifiers;

    return hotkey;
}

static void
ibus_hotkey_free (IBusHotkey *hotkey)
{
    g_slice_free (IBusHotkey, hotkey);
}


static IBusHotkey *
ibus_hotkey_copy (const IBusHotkey *src)
{
    return ibus_hotkey_new (src->keyval, src->modifiers);
}

static gint
ibus_hotkey_cmp_with_data (IBusHotkey *hotkey1,
                           IBusHotkey *hotkey2,
                           gpointer    user_data)
{
    gint retval;

    if (hotkey1 == hotkey2)
        return 0;

    retval = hotkey1->keyval - hotkey2->keyval;
    if (retval == 0)
        retval = hotkey1->modifiers - hotkey2->modifiers;

    return retval;
}



GType
ibus_hotkey_profile_get_type (void)
{
    static GType type = 0;

    static const GTypeInfo type_info = {
        sizeof (IBusHotkeyProfileClass),
        (GBaseInitFunc)     NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc)    ibus_hotkey_profile_class_init,
        NULL,               /* class finalize */
        NULL,               /* class data */
        sizeof (IBusHotkeyProfile),
        0,
        (GInstanceInitFunc) ibus_hotkey_profile_init,
    };

    if (type == 0) {
        type = g_type_register_static (IBUS_TYPE_SERIALIZABLE,
                                       "IBusHotkeyProfile",
                                       &type_info,
                                       0);
    }

    return type;
}

static void
ibus_hotkey_profile_class_init (IBusHotkeyProfileClass *class)
{
    IBusObjectClass *object_class = IBUS_OBJECT_CLASS (class);
    IBusSerializableClass *serializable_class = IBUS_SERIALIZABLE_CLASS (class);

    parent_class = (IBusSerializableClass *) g_type_class_peek_parent (class);

    g_type_class_add_private (class, sizeof (IBusHotkeyProfilePrivate));

    object_class->destroy = (IBusObjectDestroyFunc) ibus_hotkey_profile_destroy;

    serializable_class->serialize   = (IBusSerializableSerializeFunc) ibus_hotkey_profile_serialize;
    serializable_class->deserialize = (IBusSerializableDeserializeFunc) ibus_hotkey_profile_deserialize;
    serializable_class->copy        = (IBusSerializableCopyFunc) ibus_hotkey_profile_copy;

    class->trigger = ibus_hotkey_profile_trigger;

    /* install signals */
    /**
     * IBusHotkeyProfile::trigger:
     * @profile: An IBusHotkeyProfile.
     * @event: An event in GQuark.
     * @user_data: User data for callback.
     *
     * Emitted when a hotkey is pressed and the hotkey is in profile.
     * Implement the member function trigger() in extended class to receive this signal.
     *
     * <note><para>The last parameter, user_data is not actually a valid parameter. It is displayed because of GtkDoc bug.</para></note>
     */
    profile_signals[TRIGGER] =
        g_signal_new (I_("trigger"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
            G_STRUCT_OFFSET (IBusHotkeyProfileClass, trigger),
            NULL, NULL,
            _ibus_marshal_VOID__UINT_POINTER,
            G_TYPE_NONE,
            2,
            G_TYPE_UINT,
            G_TYPE_POINTER);
}

static void
ibus_hotkey_profile_init (IBusHotkeyProfile *profile)
{
    IBusHotkeyProfilePrivate *priv;
    priv = IBUS_HOTKEY_PROFILE_GET_PRIVATE (profile);

    priv->hotkeys = g_tree_new_full ((GCompareDataFunc) ibus_hotkey_cmp_with_data,
                                     NULL,
                                     (GDestroyNotify) ibus_hotkey_free,
                                     NULL);
    priv->events = g_array_new (TRUE, TRUE, sizeof (IBusHotkeyEvent));

    priv->mask = IBUS_SHIFT_MASK |
                 IBUS_CONTROL_MASK |
                 IBUS_MOD1_MASK |
                 IBUS_SUPER_MASK |
                 IBUS_HYPER_MASK |
                 IBUS_RELEASE_MASK;
}

static void
ibus_hotkey_profile_destroy (IBusHotkeyProfile *profile)
{
    IBusHotkeyProfilePrivate *priv;
    priv = IBUS_HOTKEY_PROFILE_GET_PRIVATE (profile);

    /* free events */
    if (priv->events) {
        IBusHotkeyEvent *p;
        gint i;
        p = (IBusHotkeyEvent *)g_array_free (priv->events, FALSE);
        priv->events = NULL;

        for (i = 0; p[i].event != 0; i++) {
            g_list_free (p[i].hotkeys);
        }
        g_free (p);
    }

    if (priv->hotkeys) {
        g_tree_destroy (priv->hotkeys);
        priv->hotkeys = NULL;
    }

    IBUS_OBJECT_CLASS (parent_class)->destroy ((IBusObject *)profile);
}

static gboolean
ibus_hotkey_profile_serialize (IBusHotkeyProfile *profile,
                               GVariantBuilder   *builder)
{
    gboolean retval;

    retval = parent_class->serialize ((IBusSerializable *) profile, builder);
    g_return_val_if_fail (retval, FALSE);

    return TRUE;
}

static gint
ibus_hotkey_profile_deserialize (IBusHotkeyProfile *profile,
                                 GVariant          *variant)
{
    gint retval;

    retval = parent_class->deserialize ((IBusSerializable *) profile, variant);
    g_return_val_if_fail (retval, 0);

    return retval;
}

static gboolean
ibus_hotkey_profile_copy (IBusHotkeyProfile       *dest,
                          const IBusHotkeyProfile *src)
{
    gboolean retval;

    retval = parent_class->copy ((IBusSerializable *)dest,
                                 (IBusSerializable *)src);
    g_return_val_if_fail (retval, FALSE);

    g_return_val_if_fail (IBUS_IS_HOTKEY_PROFILE (dest), FALSE);
    g_return_val_if_fail (IBUS_IS_HOTKEY_PROFILE (src), FALSE);

    return TRUE;
}

IBusHotkeyProfile *
ibus_hotkey_profile_new (void)
{
    IBusHotkeyProfile *profile = g_object_new (IBUS_TYPE_HOTKEY_PROFILE, NULL);

    return profile;
}

static void
ibus_hotkey_profile_trigger (IBusHotkeyProfile *profile,
                             GQuark             event,
                             gpointer           user_data)
{
    // g_debug ("%s is triggerred", g_quark_to_string (event));
}

static guint
normalize_modifiers (guint keyval,
                     guint modifiers)
{
    switch(keyval) {
    case IBUS_KEY_Control_L:
    case IBUS_KEY_Control_R:
        return modifiers | IBUS_CONTROL_MASK;
    case IBUS_KEY_Shift_L:
    case IBUS_KEY_Shift_R:
        return modifiers | IBUS_SHIFT_MASK;
    case IBUS_KEY_Alt_L:
    case IBUS_KEY_Alt_R:
    // Chrome OS does not have Meta key. Instead, shift+alt generates Meta keyval.
    case IBUS_KEY_Meta_L:
    case IBUS_KEY_Meta_R:
        return modifiers | IBUS_MOD1_MASK;
    case IBUS_KEY_Super_L:
    case IBUS_KEY_Super_R:
        return modifiers | IBUS_SUPER_MASK;
    case IBUS_KEY_Hyper_L:
    case IBUS_KEY_Hyper_R:
        return modifiers | IBUS_HYPER_MASK;
    default:
        return modifiers;
    }
}

static gboolean
is_modifier (guint keyval)
{
    switch(keyval) {
    case IBUS_KEY_Control_L:
    case IBUS_KEY_Control_R:
    case IBUS_KEY_Shift_L:
    case IBUS_KEY_Shift_R:
    case IBUS_KEY_Alt_L:
    case IBUS_KEY_Alt_R:
    case IBUS_KEY_Meta_L:
    case IBUS_KEY_Meta_R:
    case IBUS_KEY_Super_L:
    case IBUS_KEY_Super_R:
    case IBUS_KEY_Hyper_L:
    case IBUS_KEY_Hyper_R:
        return TRUE;
    default:
        return FALSE;
    }
}

gboolean
ibus_hotkey_profile_add_hotkey (IBusHotkeyProfile *profile,
                                guint              keyval,
                                guint              modifiers,
                                GQuark             event)
{
    IBusHotkeyProfilePrivate *priv;
    priv = IBUS_HOTKEY_PROFILE_GET_PRIVATE (profile);

    IBusHotkey *hotkey = ibus_hotkey_new (keyval,
                                          normalize_modifiers (keyval, modifiers & priv->mask));

    /* has the same hotkey in profile */
    if (g_tree_lookup (priv->hotkeys, hotkey) != NULL) {
        ibus_hotkey_free (hotkey);
        g_return_val_if_reached (FALSE);
    }

    g_tree_insert (priv->hotkeys, (gpointer) hotkey, GUINT_TO_POINTER (event));

    IBusHotkeyEvent *p = NULL;
    gint i;
    for ( i = 0; i < priv->events->len; i++) {
        p = &g_array_index (priv->events, IBusHotkeyEvent, i);
        if (p->event == event)
            break;
    }

    if (i >= priv->events->len) {
        g_array_set_size (priv->events, i + 1);
        p = &g_array_index (priv->events, IBusHotkeyEvent, i);
        p->event = event;
    }

    p->hotkeys = g_list_append (p->hotkeys, hotkey);

    return TRUE;
}


gboolean
ibus_hotkey_profile_add_hotkey_from_string (IBusHotkeyProfile *profile,
                                            const gchar       *str,
                                            GQuark             event)
{
    guint keyval;
    guint modifiers;

    if (ibus_key_event_from_string (str, &keyval, &modifiers) == FALSE) {
        return FALSE;
    }

    return ibus_hotkey_profile_add_hotkey (profile, keyval, modifiers, event);
}

gboolean
ibus_hotkey_profile_remove_hotkey (IBusHotkeyProfile *profile,
                                   guint              keyval,
                                   guint              modifiers)
{
    IBusHotkeyProfilePrivate *priv;
    priv = IBUS_HOTKEY_PROFILE_GET_PRIVATE (profile);

    modifiers = normalize_modifiers (keyval, modifiers & priv->mask);

    IBusHotkey hotkey = {
        .keyval = keyval,
        .modifiers = modifiers
    };

    IBusHotkey *p1;
    GQuark event;
    gboolean retval;

    retval = g_tree_lookup_extended (priv->hotkeys,
                                     &hotkey,
                                     (gpointer)&p1,
                                     (gpointer)&event);

    if (!retval)
        return FALSE;

    gint i;
    IBusHotkeyEvent *p2 = NULL;
    for ( i = 0; i < priv->events->len; i++) {
        p2 = &g_array_index (priv->events, IBusHotkeyEvent, i);
        if (p2->event == event)
            break;
    }

    g_assert (p2 && p2->event == event);

    p2->hotkeys = g_list_remove (p2->hotkeys, p1);
    if (p2->hotkeys == NULL) {
        g_array_remove_index_fast (priv->events, i);
    }

    g_tree_remove (priv->hotkeys, p1);

    return TRUE;
}

gboolean
ibus_hotkey_profile_remove_hotkey_by_event (IBusHotkeyProfile *profile,
                                            GQuark             event)
{
    IBusHotkeyProfilePrivate *priv;
    priv = IBUS_HOTKEY_PROFILE_GET_PRIVATE (profile);

    gint i;
    IBusHotkeyEvent *p = NULL;
    for ( i = 0; i < priv->events->len; i++) {
        p = &g_array_index (priv->events, IBusHotkeyEvent, i);
        if (p->event == event)
            break;
    }

    if (p == NULL || p->event != event)
        return FALSE;

    GList *list;
    for (list = p->hotkeys; list != NULL; list = list->next) {
        g_tree_remove (priv->hotkeys, (IBusHotkey *)list->data);
    }

    g_list_free (p->hotkeys);
    g_array_remove_index_fast (priv->events, i);

    return TRUE;
}

GQuark
ibus_hotkey_profile_filter_key_event (IBusHotkeyProfile *profile,
                                      guint              keyval,
                                      guint              modifiers,
                                      guint              prev_keyval,
                                      guint              prev_modifiers,
                                      gpointer           user_data)
{
    IBusHotkeyProfilePrivate *priv;
    priv = IBUS_HOTKEY_PROFILE_GET_PRIVATE (profile);

    modifiers = normalize_modifiers (keyval, modifiers & priv->mask);
    prev_modifiers = normalize_modifiers (prev_keyval, prev_modifiers & priv->mask);

    IBusHotkey hotkey = {
        .keyval = keyval,
        .modifiers = modifiers,
    };

    if (modifiers & IBUS_RELEASE_MASK) {
        /* previous key event must be a press key event */
        if (prev_modifiers & IBUS_RELEASE_MASK)
            return 0;

        /* modifiers should be same except the release bit */
        if (modifiers != (prev_modifiers | IBUS_RELEASE_MASK))
            return 0;

        /* If it is release key event,
         * we need check if keyval is equal to the prev keyval.
         * If keyval is not equal to the prev keyval,
         * but both keyvals are modifier keys,
         * we will still search it in hotkeys.
         * It is for matching some key sequences like:
         * Shift Down, Alt Down, Shift Up => Shift+Alt+Release - Shift hotkey
         **/
        if ((keyval != prev_keyval) &&
            (is_modifier (keyval) == FALSE ||
             is_modifier (prev_keyval) == FALSE))
            return 0;
    }

    GQuark event = (GQuark) GPOINTER_TO_UINT (g_tree_lookup (priv->hotkeys, &hotkey));

    if (event != 0) {
        g_signal_emit (profile, profile_signals[TRIGGER], event, user_data);
    }

    return event;
}

GQuark
ibus_hotkey_profile_lookup_hotkey (IBusHotkeyProfile *profile,
                                   guint              keyval,
                                   guint              modifiers)
{
    IBusHotkeyProfilePrivate *priv;
    priv = IBUS_HOTKEY_PROFILE_GET_PRIVATE (profile);

    modifiers = normalize_modifiers (keyval, modifiers & priv->mask);

    IBusHotkey hotkey = {
        .keyval = keyval,
        .modifiers = modifiers,
    };

    return (GQuark) GPOINTER_TO_UINT (g_tree_lookup (priv->hotkeys, &hotkey));
}
