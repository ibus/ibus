/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* ibus
 * Copyright (C) 2008 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2008 Red Hat, Inc.
 *
 * gdk-private.c: Copied some code from gtk2
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

#include "config.h"

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>

#ifdef HAVE_X11_XKBLIB_H
#  define HAVE_XKB
#  include <X11/XKBlib.h>
#endif

void
translate_key_event (GdkDisplay *display,
		     GdkEvent   *event,
		     XEvent     *xevent)
{
  GdkKeymap *keymap = gdk_keymap_get_for_display (display);

  event->key.type = xevent->xany.type == KeyPress ? GDK_KEY_PRESS : GDK_KEY_RELEASE;
  event->key.time = xevent->xkey.time;

  event->key.state = (GdkModifierType) xevent->xkey.state;

#ifdef HAVE_XKB
  event->key.group = XkbGroupForCoreState (xevent->xkey.state);
#else
  event->key.group = 0;
#endif

  event->key.hardware_keycode = xevent->xkey.keycode;

#ifdef ENABLE_GTK3
  event->key.keyval = GDK_KEY_VoidSymbol;
#else
  event->key.keyval = GDK_VoidSymbol;
#endif

  gdk_keymap_translate_keyboard_state (keymap,
				       event->key.hardware_keycode,
				       event->key.state,
				       event->key.group,
				       &event->key.keyval,
				       NULL, NULL, NULL);
  event->key.is_modifier = 0;

  /* Fill in event->string crudely, since various programs
   * depend on it.
   */
  event->key.string = NULL;
  event->key.length = 0;

  return;
}

