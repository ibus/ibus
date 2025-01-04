/* GTK - The GIMP Toolkit
 * Copyright (C) 1998, 2001 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#include "config.h"
#include <string.h>
#include <stdlib.h>

#include "ibusaccelgroup.h"
#include "ibuskeys.h"
#include "ibuskeysyms.h"


/* for _gtk_get_primary_accel_mod() */
#define _IBUS_GET_PRIMARY_ACCEL_MOD IBUS_CONTROL_MASK

/**
 * SECTION: ibusaccelgroup
 * @short_description: Groups of global keyboard accelerators for an
 *     entire GtkWindow
 * @title: Accelerator Groups
 * @stability: Unstable
 *
 * Provides ibus_accelerator_parse()
 */


/**
 * ibus_accelerator_valid:
 * @keyval: a GDK keyval
 * @modifiers: modifier mask
 *
 * Determines whether a given keyval and modifier mask constitute
 * a valid keyboard accelerator. For example, the #IBUS_KEY_a keyval
 * plus #IBUS_CONTROL_MASK is valid - this is a “Ctrl+a” accelerator.
 * But, you can't, for instance, use the #IBUS_KEY_Control_L keyval
 * as an accelerator.
 *
 * Returns: %TRUE if the accelerator is valid
 */
gboolean
ibus_accelerator_valid (guint           keyval,
                        IBusModifierType modifiers)
{
    static const guint invalid_accelerator_vals[] = {
        IBUS_KEY_Shift_L, IBUS_KEY_Shift_R, IBUS_KEY_ISO_Level2_Latch, IBUS_KEY_Shift_Lock,
        IBUS_KEY_Caps_Lock,
        IBUS_KEY_ISO_Lock,
        IBUS_KEY_Control_L, IBUS_KEY_Control_R,
        IBUS_KEY_Meta_L, IBUS_KEY_Meta_R,
        IBUS_KEY_Alt_L, IBUS_KEY_Alt_R,
        IBUS_KEY_Super_L, IBUS_KEY_Super_R,
        IBUS_KEY_Hyper_L, IBUS_KEY_Hyper_R,
        IBUS_KEY_ISO_Level3_Shift, IBUS_KEY_ISO_Level3_Latch, IBUS_KEY_ISO_Level3_Lock,
        IBUS_KEY_ISO_Level5_Shift, IBUS_KEY_ISO_Level5_Latch, IBUS_KEY_ISO_Level5_Lock,
        IBUS_KEY_ISO_Group_Shift, IBUS_KEY_ISO_Group_Latch, IBUS_KEY_ISO_Group_Lock,
        IBUS_KEY_ISO_Next_Group, IBUS_KEY_ISO_Next_Group_Lock,
        IBUS_KEY_ISO_Prev_Group, IBUS_KEY_ISO_Prev_Group_Lock,
        IBUS_KEY_ISO_First_Group, IBUS_KEY_ISO_First_Group_Lock,
        IBUS_KEY_ISO_Last_Group, IBUS_KEY_ISO_Last_Group_Lock,
        IBUS_KEY_Num_Lock,
        IBUS_KEY_Multi_key,
        IBUS_KEY_Scroll_Lock,
        IBUS_KEY_Sys_Req,
        IBUS_KEY_Tab, IBUS_KEY_ISO_Left_Tab, IBUS_KEY_KP_Tab,
        IBUS_KEY_First_Virtual_Screen,
        IBUS_KEY_Prev_Virtual_Screen,
        IBUS_KEY_Next_Virtual_Screen,
        IBUS_KEY_Last_Virtual_Screen,
        IBUS_KEY_Terminate_Server,
        IBUS_KEY_AudibleBell_Enable,
        0
    };
    static const guint invalid_unmodified_vals[] = {
        IBUS_KEY_Up, IBUS_KEY_Down, IBUS_KEY_Left, IBUS_KEY_Right,
        IBUS_KEY_KP_Up, IBUS_KEY_KP_Down, IBUS_KEY_KP_Left, IBUS_KEY_KP_Right,
        0
    };
    const guint *ac_val;

    modifiers &= IBUS_MODIFIER_MASK;

    if (keyval <= 0xFF)
        return keyval >= 0x20;

    ac_val = invalid_accelerator_vals;
    while (*ac_val) {
        if (keyval == *ac_val++)
            return FALSE;
    }

    if (!modifiers) {
        ac_val = invalid_unmodified_vals;
        while (*ac_val) {
            if (keyval == *ac_val++)
                return FALSE;
        }
    }

    return TRUE;
}

static inline gboolean
is_alt (const gchar *string)
{
    return ((string[0] == '<') &&
            (string[1] == 'a' || string[1] == 'A') &&
            (string[2] == 'l' || string[2] == 'L') &&
            (string[3] == 't' || string[3] == 'T') &&
            (string[4] == '>'));
}

static inline gboolean
is_ctl (const gchar *string)
{
    return ((string[0] == '<') &&
            (string[1] == 'c' || string[1] == 'C') &&
            (string[2] == 't' || string[2] == 'T') &&
            (string[3] == 'l' || string[3] == 'L') &&
            (string[4] == '>'));
}

static inline gboolean
is_modx (const gchar *string)
{
    return ((string[0] == '<') &&
            (string[1] == 'm' || string[1] == 'M') &&
            (string[2] == 'o' || string[2] == 'O') &&
            (string[3] == 'd' || string[3] == 'D') &&
            (string[4] >= '1' && string[4] <= '5') &&
            (string[5] == '>'));
}

static inline gboolean
is_ctrl (const gchar *string)
{
    return ((string[0] == '<') &&
            (string[1] == 'c' || string[1] == 'C') &&
            (string[2] == 't' || string[2] == 'T') &&
            (string[3] == 'r' || string[3] == 'R') &&
            (string[4] == 'l' || string[4] == 'L') &&
            (string[5] == '>'));
}

static inline gboolean
is_shft (const gchar *string)
{
    return ((string[0] == '<') &&
            (string[1] == 's' || string[1] == 'S') &&
            (string[2] == 'h' || string[2] == 'H') &&
            (string[3] == 'f' || string[3] == 'F') &&
            (string[4] == 't' || string[4] == 'T') &&
            (string[5] == '>'));
}

static inline gboolean
is_shift (const gchar *string)
{
    return ((string[0] == '<') &&
            (string[1] == 's' || string[1] == 'S') &&
            (string[2] == 'h' || string[2] == 'H') &&
            (string[3] == 'i' || string[3] == 'I') &&
            (string[4] == 'f' || string[4] == 'F') &&
            (string[5] == 't' || string[5] == 'T') &&
            (string[6] == '>'));
}

static inline gboolean
is_control (const gchar *string)
{
    return ((string[0] == '<') &&
            (string[1] == 'c' || string[1] == 'C') &&
            (string[2] == 'o' || string[2] == 'O') &&
            (string[3] == 'n' || string[3] == 'N') &&
            (string[4] == 't' || string[4] == 'T') &&
            (string[5] == 'r' || string[5] == 'R') &&
            (string[6] == 'o' || string[6] == 'O') &&
            (string[7] == 'l' || string[7] == 'L') &&
            (string[8] == '>'));
}

static inline gboolean
is_release (const gchar *string)
{
    return ((string[0] == '<') &&
            (string[1] == 'r' || string[1] == 'R') &&
            (string[2] == 'e' || string[2] == 'E') &&
            (string[3] == 'l' || string[3] == 'L') &&
            (string[4] == 'e' || string[4] == 'E') &&
            (string[5] == 'a' || string[5] == 'A') &&
            (string[6] == 's' || string[6] == 'S') &&
            (string[7] == 'e' || string[7] == 'E') &&
            (string[8] == '>'));
}

static inline gboolean
is_meta (const gchar *string)
{
    return ((string[0] == '<') &&
            (string[1] == 'm' || string[1] == 'M') &&
            (string[2] == 'e' || string[2] == 'E') &&
            (string[3] == 't' || string[3] == 'T') &&
            (string[4] == 'a' || string[4] == 'A') &&
            (string[5] == '>'));
}

static inline gboolean
is_super (const gchar *string)
{
    return ((string[0] == '<') &&
            (string[1] == 's' || string[1] == 'S') &&
            (string[2] == 'u' || string[2] == 'U') &&
            (string[3] == 'p' || string[3] == 'P') &&
            (string[4] == 'e' || string[4] == 'E') &&
            (string[5] == 'r' || string[5] == 'R') &&
            (string[6] == '>'));
}

static inline gboolean
is_hyper (const gchar *string)
{
    return ((string[0] == '<') &&
            (string[1] == 'h' || string[1] == 'H') &&
            (string[2] == 'y' || string[2] == 'Y') &&
            (string[3] == 'p' || string[3] == 'P') &&
            (string[4] == 'e' || string[4] == 'E') &&
            (string[5] == 'r' || string[5] == 'R') &&
            (string[6] == '>'));
}

static inline gboolean
is_primary (const gchar *string)
{
    return ((string[0] == '<') &&
            (string[1] == 'p' || string[1] == 'P') &&
            (string[2] == 'r' || string[2] == 'R') &&
            (string[3] == 'i' || string[3] == 'I') &&
            (string[4] == 'm' || string[4] == 'M') &&
            (string[5] == 'a' || string[5] == 'A') &&
            (string[6] == 'r' || string[6] == 'R') &&
            (string[7] == 'y' || string[7] == 'Y') &&
            (string[8] == '>'));
}

static inline gboolean
is_keycode (const gchar *string)
{
    return (string[0] == '0' &&
            string[1] == 'x' &&
            g_ascii_isxdigit (string[2]) &&
            g_ascii_isxdigit (string[3]));
}

/**
 * ibus_accelerator_parse:
 * @accelerator: string representing an accelerator
 * @accelerator_key: (out) (allow-none): return location for accelerator
 *     keyval, or %NULL
 * @accelerator_mods: (out) (allow-none): return location for accelerator
 *     modifier mask, %NULL
 *
 * Parses a string representing an accelerator. The format looks like
 * “&lt;Control&gt;a” or “&lt;Shift&gt;&lt;Alt&gt;F1” or “&lt;Release%gt;z”
 * (the last one is for key release).
 *
 * The parser is fairly liberal and allows lower or upper case, and also
 * abbreviations such as “&lt;Ctl&gt;” and “&lt;Ctrl&gt;”. Key names are
 * parsed using gdk_keyval_from_name(). For character keys the name is not the
 * symbol, but the lowercase name, e.g. one would use “&lt;Ctrl&gt;minus”
 * instead of “&lt;Ctrl&gt;-”.
 *
 * If the parse fails, @accelerator_key and @accelerator_mods will
 * be set to 0 (zero).
 *
 * Since: 1.5.18
 */
void
ibus_accelerator_parse (const gchar      *accelerator,
                        guint            *accelerator_key,
                        IBusModifierType *accelerator_mods)
{
    guint keyval;
    IBusModifierType mods;
    gint len;
    gboolean error;

    if (accelerator_key)
        *accelerator_key = 0;
    if (accelerator_mods)
        *accelerator_mods = 0;
    g_return_if_fail (accelerator != NULL);

    error = FALSE;
    keyval = 0;
    mods = 0;
    len = strlen (accelerator);
    while (len) {
        if (*accelerator == '<') {
            if (len >= 9 && is_release (accelerator)) {
                accelerator += 9;
                len -= 9;
                mods |= IBUS_RELEASE_MASK;
            } else if (len >= 9 && is_primary (accelerator)) {
                accelerator += 9;
                len -= 9;
                mods |= _IBUS_GET_PRIMARY_ACCEL_MOD;
            } else if (len >= 9 && is_control (accelerator)) {
                accelerator += 9;
                len -= 9;
                mods |= IBUS_CONTROL_MASK;
            } else if (len >= 7 && is_shift (accelerator)) {
                accelerator += 7;
                len -= 7;
                mods |= IBUS_SHIFT_MASK;
            } else if (len >= 6 && is_shft (accelerator)) {
                accelerator += 6;
                len -= 6;
                mods |= IBUS_SHIFT_MASK;
            } else if (len >= 6 && is_ctrl (accelerator)) {
                accelerator += 6;
                len -= 6;
                mods |= IBUS_CONTROL_MASK;
            } else if (len >= 6 && is_modx (accelerator)) {
                static const guint mod_vals[] = {
                    IBUS_MOD1_MASK, IBUS_MOD2_MASK, IBUS_MOD3_MASK,
                    IBUS_MOD4_MASK, IBUS_MOD5_MASK
                };

                len -= 6;
                accelerator += 4;
                mods |= mod_vals[*accelerator - '1'];
                accelerator += 2;
            } else if (len >= 5 && is_ctl (accelerator)) {
                accelerator += 5;
                len -= 5;
                mods |= IBUS_CONTROL_MASK;
            } else if (len >= 5 && is_alt (accelerator)) {
                accelerator += 5;
                len -= 5;
                mods |= IBUS_MOD1_MASK;
            } else if (len >= 6 && is_meta (accelerator)) {
                accelerator += 6;
                len -= 6;
                mods |= IBUS_META_MASK;
            } else if (len >= 7 && is_hyper (accelerator)) {
                accelerator += 7;
                len -= 7;
                mods |= IBUS_HYPER_MASK;
            } else if (len >= 7 && is_super (accelerator)) {
                accelerator += 7;
                len -= 7;
                mods |= IBUS_SUPER_MASK;
            } else {
                gchar last_ch;

                last_ch = *accelerator;
                while (last_ch && last_ch != '>') {
                    last_ch = *accelerator;
                    accelerator += 1;
                    len -= 1;
                }
            }
        } else {
            if (len >= 4 && is_keycode (accelerator)) {
                /* There was a keycode in the string, but
                 * we cannot store it, so we have an error */
                error = TRUE;
                goto out;
            } else {
	        keyval = ibus_keyval_from_name (accelerator);
	        if (keyval == IBUS_KEY_VoidSymbol) {
	            error = TRUE;
	            goto out;
		}
	    }

            accelerator += len;
            len -= len;
        }
    }

out:
    if (error)
        keyval = mods = 0;

    if (accelerator_key)
        *accelerator_key = ibus_keyval_to_lower (keyval);
    if (accelerator_mods)
        *accelerator_mods = mods;
}

/**
 * ibus_accelerator_name:
 * @accelerator_key: accelerator keyval
 * @accelerator_mods: accelerator modifier mask
 *
 * Converts an accelerator keyval and modifier mask into a string
 * parseable by gtk_accelerator_parse(). For example, if you pass in
 * #IBUS_KEY_q and #IBUS_CONTROL_MASK, this function returns “&lt;Control&gt;q”.
 *
 * If you need to display accelerators in the user interface,
 * see gtk_accelerator_get_label().
 *
 * Returns: a newly-allocated accelerator name
 */
gchar*
ibus_accelerator_name (guint            accelerator_key,
                       IBusModifierType accelerator_mods)
{
   static const gchar text_release[] = "<Release>";
   static const gchar text_primary[] = "<Primary>";
   static const gchar text_shift[] = "<Shift>";
   static const gchar text_control[] = "<Control>";
   static const gchar text_mod1[] = "<Alt>";
   static const gchar text_mod2[] = "<Mod2>";
   static const gchar text_mod3[] = "<Mod3>";
   static const gchar text_mod4[] = "<Mod4>";
   static const gchar text_mod5[] = "<Mod5>";
   static const gchar text_meta[] = "<Meta>";
   static const gchar text_super[] = "<Super>";
   static const gchar text_hyper[] = "<Hyper>";
   IBusModifierType saved_mods;
   guint l;
   const gchar *keyval_name;
   gchar *accelerator;

    accelerator_mods &= IBUS_MODIFIER_MASK;

    keyval_name = ibus_keyval_name (ibus_keyval_to_lower (accelerator_key));
    if (!keyval_name)
        keyval_name = "";

    saved_mods = accelerator_mods;
    l = 0;
    if (accelerator_mods & IBUS_RELEASE_MASK)
        l += sizeof (text_release) - 1;
    if (accelerator_mods & _IBUS_GET_PRIMARY_ACCEL_MOD) {
        l += sizeof (text_primary) - 1;
        /* consume the default accel */
        accelerator_mods &= ~_IBUS_GET_PRIMARY_ACCEL_MOD;
    }
    if (accelerator_mods & IBUS_SHIFT_MASK)
        l += sizeof (text_shift) - 1;
    if (accelerator_mods & IBUS_CONTROL_MASK)
        l += sizeof (text_control) - 1;
    if (accelerator_mods & IBUS_MOD1_MASK)
        l += sizeof (text_mod1) - 1;
    if (accelerator_mods & IBUS_MOD2_MASK)
        l += sizeof (text_mod2) - 1;
    if (accelerator_mods & IBUS_MOD3_MASK)
        l += sizeof (text_mod3) - 1;
    if (accelerator_mods & IBUS_MOD4_MASK)
        l += sizeof (text_mod4) - 1;
    if (accelerator_mods & IBUS_MOD5_MASK)
        l += sizeof (text_mod5) - 1;
    l += strlen (keyval_name);
    if (accelerator_mods & IBUS_META_MASK)
        l += sizeof (text_meta) - 1;
    if (accelerator_mods & IBUS_HYPER_MASK)
        l += sizeof (text_hyper) - 1;
    if (accelerator_mods & IBUS_SUPER_MASK)
        l += sizeof (text_super) - 1;

    g_return_val_if_fail ((accelerator = g_new (gchar, l + 1)), NULL);

    accelerator_mods = saved_mods;
    l = 0;
    accelerator[l] = 0;
    if (accelerator_mods & IBUS_RELEASE_MASK) {
        strcpy (accelerator + l, text_release);
        l += sizeof (text_release) - 1;
    }
    if (accelerator_mods & _IBUS_GET_PRIMARY_ACCEL_MOD) {
        strcpy (accelerator + l, text_primary);
        l += sizeof (text_primary) - 1;
        /* consume the default accel */
        accelerator_mods &= ~_IBUS_GET_PRIMARY_ACCEL_MOD;
    }
    if (accelerator_mods & IBUS_SHIFT_MASK) {
        strcpy (accelerator + l, text_shift);
        l += sizeof (text_shift) - 1;
    }
    if (accelerator_mods & IBUS_CONTROL_MASK) {
        strcpy (accelerator + l, text_control);
        l += sizeof (text_control) - 1;
    }
    if (accelerator_mods & IBUS_MOD1_MASK) {
        strcpy (accelerator + l, text_mod1);
        l += sizeof (text_mod1) - 1;
    }
    if (accelerator_mods & IBUS_MOD2_MASK) {
        strcpy (accelerator + l, text_mod2);
        l += sizeof (text_mod2) - 1;
    }
    if (accelerator_mods & IBUS_MOD3_MASK) {
        strcpy (accelerator + l, text_mod3);
        l += sizeof (text_mod3) - 1;
    }
    if (accelerator_mods & IBUS_MOD4_MASK) {
        strcpy (accelerator + l, text_mod4);
        l += sizeof (text_mod4) - 1;
    }
    if (accelerator_mods & IBUS_MOD5_MASK) {
        strcpy (accelerator + l, text_mod5);
        l += sizeof (text_mod5) - 1;
    }
    if (accelerator_mods & IBUS_META_MASK) {
        strcpy (accelerator + l, text_meta);
        l += sizeof (text_meta) - 1;
    }
    if (accelerator_mods & IBUS_HYPER_MASK) {
        strcpy (accelerator + l, text_hyper);
        l += sizeof (text_hyper) - 1;
    }
    if (accelerator_mods & IBUS_SUPER_MASK) {
        strcpy (accelerator + l, text_super);
        l += sizeof (text_super) - 1;
    }
    strcpy (accelerator + l, keyval_name);

    return accelerator;
}
