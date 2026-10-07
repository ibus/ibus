/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* IBus - The Input Bus
 * Copyright (C) 2008-2013 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2008-2013 Red Hat, Inc.
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

#if !defined (__IBUS_H_INSIDE__) && !defined (IBUS_COMPILATION)
#error "Only <ibus.h> can be included directly"
#endif

#ifndef __IBUS_HOTKEY_H_
#define __IBUS_HOTKEY_H_

/**
 * SECTION: ibushotkey
 * @short_description: Hotkeys and associated events.
 * @stability: Stable
 *
 * An IBusHotkeyProfile associates a hotkey and an event.
 */

#include "ibusserializable.h"

/*
 * Type macros.
 */
/* define IBusHotkeyProfile macros */
#define IBUS_TYPE_HOTKEY_PROFILE             \
    (ibus_hotkey_profile_get_type ())
#define IBUS_HOTKEY_PROFILE(obj)             \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), IBUS_TYPE_HOTKEY_PROFILE, IBusHotkeyProfile))
#define IBUS_HOTKEY_PROFILE_CLASS(klass)     \
    (G_TYPE_CHECK_CLASS_CAST ((klass), IBUS_TYPE_HOTKEY_PROFILE, IBusHotkeyProfileClass))
#define IBUS_IS_HOTKEY_PROFILE(obj)          \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IBUS_TYPE_HOTKEY_PROFILE))
#define IBUS_IS_HOTKEY_PROFILE_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), IBUS_TYPE_HOTKEY_PROFILE))
#define IBUS_HOTKEY_PROFILE_GET_CLASS(obj)   \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), IBUS_TYPE_HOTKEY_PROFILE, IBusHotkeyProfileClass))

G_BEGIN_DECLS

typedef struct _IBusHotkeyProfile IBusHotkeyProfile;
typedef struct _IBusHotkeyProfileClass IBusHotkeyProfileClass;

/**
 * IBusHotkeyProfile:
 *
 * An opaque data type representing an IBusHotkeyProfile.
 */
struct _IBusHotkeyProfile {
    IBusSerializable parent;

    /* members */
};

struct _IBusHotkeyProfileClass {
    IBusSerializableClass parent;

    void (* trigger) (IBusHotkeyProfile *profile,
                      GQuark             event,
                      gpointer           user_data);
};

/* hotkey profile functions */
GType            ibus_hotkey_profile_get_type   (void);
/**
 * ibus_hotkey_profile_new:
 *
 * Creates a new #IBusHotkeyProfile.
 *
 * Returns: A newly allocated #IBusHotkeyProfile.
 */
IBusHotkeyProfile
                *ibus_hotkey_profile_new        (void);

/**
 * ibus_hotkey_profile_add_hotkey :
 * @profile: An IBusHotkeyProfile.
 * @keyval: Keycode of the hotkey.
 * @modifiers: Modifiers of the hotkey.
 * @event: The event to be associated.
 *
 * Adds a hotkey and its associated event to an #IBusHotkeyProfile.
 *
 * Returns: Always %TRUE.
 */
gboolean         ibus_hotkey_profile_add_hotkey (IBusHotkeyProfile  *profile,
                                                 guint               keyval,
                                                 guint               modifiers,
                                                 GQuark              event);

/**
 * ibus_hotkey_profile_add_hotkey_from_string:
 * @profile: An IBusHotkeyProfile.
 * @str: Key in string representation.  '+' is the separator.
 * @event: The event to be associated.
 *
 * Adds a hotkey and its associated event to an #IBusHotkeyProfile.
 * The hotkey is in string format, such like
 * <constant>Control+Shift+A</constant>.
 *
 * Returns: FALSE if @str contains invalid symbol; TRUE otherwise.
 */
gboolean         ibus_hotkey_profile_add_hotkey_from_string
                                                (IBusHotkeyProfile  *profile,
                                                 const gchar        *str,
                                                 GQuark              event);

/**
 * ibus_hotkey_profile_remove_hotkey:
 * @profile: An IBusHotkeyProfile.
 * @keyval: Keycode of the hotkey.
 * @modifiers: Modifiers of the hotkey.
 *
 * Removes the hotkey for an #IBusHotkeyProfile.
 *
 * Returns: %FALSE if the key is not in @profile, %TRUE otherwise.
 */
gboolean         ibus_hotkey_profile_remove_hotkey
                                                (IBusHotkeyProfile  *profile,
                                                 guint               keyval,
                                                 guint               modifiers);

/**
 * ibus_hotkey_profile_remove_hotkey_by_event:
 * @profile: An IBusHotkeyProfile.
 * @event: The associated event.
 *
 * Removes the hotkey for an #IBusHotkeyProfile by event.
 *
 * Returns: %FALSE if no such event in @profile, %TRUE otherwise.
 */
gboolean         ibus_hotkey_profile_remove_hotkey_by_event
                                                (IBusHotkeyProfile  *profile,
                                                 GQuark              event);

/**
 * ibus_hotkey_profile_filter_key_event:
 * @profile: An IBusHotkeyProfile.
 * @keyval: Keycode of the hotkey.
 * @modifiers: Modifiers of the hotkey.
 * @prev_keyval: Keycode of the hotkey.
 * @prev_modifiers: Modifiers of the hotkey.
 * @user_data: user data for signal "trigger".
 *
 * Emits a <constant>::trigger</constant> signal when a hotkey is in a profile.
 *
 * Returns: 0 if releasing a hotkey and the hotkey is not in the profile;
 * an associated event otherwise.
 *
 * See also: ::trigger
 */
GQuark           ibus_hotkey_profile_filter_key_event
                                                (IBusHotkeyProfile  *profile,
                                                 guint               keyval,
                                                 guint               modifiers,
                                                 guint               prev_keyval,
                                                 guint               prev_modifiers,
                                                 gpointer            user_data);

/**
 * ibus_hotkey_profile_lookup_hotkey:
 * @profile: An IBusHotkeyProfile.
 * @keyval: Keycode of the hotkey.
 * @modifiers: Modifiers of the hotkey.
 *
 * Returns: The event associated to the hotkey or 0 if the hotkey is not in the
 * profile.
 */
GQuark           ibus_hotkey_profile_lookup_hotkey
                                                (IBusHotkeyProfile  *profile,
                                                 guint               keyval,
                                                 guint               modifiers);

G_END_DECLS
#endif
