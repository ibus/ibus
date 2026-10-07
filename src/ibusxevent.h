/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2018 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2018 Red Hat, Inc.
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

#ifndef __IBUS_X_EVENT_H_
#define __IBUS_X_EVENT_H_

/**
 * SECTION: ibusxevent
 * @short_description: Extension Event wrapper object
 * @title: IBusExtensionEvent
 * @stability: Unstable
 *
 * An IBusXEvent provides a wrapper of XEvent.
 *
 * see_also: #IBusComponent, #IBusEngineDesc
 */

#include "ibusserializable.h"

/*
 * Type macros.
 */

/* define GOBJECT macros */
#define IBUS_TYPE_EXTENSION_EVENT                                       \
    (ibus_extension_event_get_type ())
#define IBUS_EXTENSION_EVENT(obj)                                       \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                 \
                                 IBUS_TYPE_EXTENSION_EVENT,             \
                                 IBusExtensionEvent))
#define IBUS_EXTENSION_EVENT_CLASS(klass)                               \
    (G_TYPE_CHECK_CLASS_CAST ((klass),                                  \
                              IBUS_TYPE_EXTENSION_EVENT,                \
                              IBusExtensionEventClass))
#define IBUS_IS_EXTENSION_EVENT(obj)                                    \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IBUS_TYPE_EXTENSION_EVENT))
#define IBUS_IS_EXTENSION_EVENT_CLASS(klass)                            \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), IBUS_TYPE_EXTENSION_EVENT))
#define IBUS_EXTENSION_EVENT_GET_CLASS(obj)                             \
    (G_TYPE_INSTANCE_GET_CLASS ((obj),                                  \
                                IBUS_TYPE_EXTENSION_EVENT,              \
                                IBusExtensionEventClass))

#define IBUS_TYPE_X_EVENT                                               \
    (ibus_x_event_get_type ())
#define IBUS_X_EVENT(obj)                                               \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), IBUS_TYPE_X_EVENT, IBusXEvent))
#define IBUS_X_EVENT_CLASS(klass)                                       \
    (G_TYPE_CHECK_CLASS_CAST ((klass), IBUS_TYPE_X_EVENT, IBusXEventClass))
#define IBUS_IS_X_EVENT(obj)                                            \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IBUS_TYPE_X_EVENT))
#define IBUS_IS_X_EVENT_CLASS(klass)                                    \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), IBUS_TYPE_X_EVENT))
#define IBUS_X_EVENT_GET_CLASS(obj)                                     \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), IBUS_TYPE_X_EVENT, IBusXEventClass))

G_BEGIN_DECLS

typedef struct _IBusProcessKeyEventData IBusProcessKeyEventData;
typedef struct _IBusExtensionEvent IBusExtensionEvent;
typedef struct _IBusExtensionEventClass IBusExtensionEventClass;
typedef struct _IBusExtensionEventPrivate IBusExtensionEventPrivate;
typedef struct _IBusXEvent IBusXEvent;
typedef struct _IBusXEventClass IBusXEventClass;
typedef struct _IBusXEventPrivate IBusXEventPrivate;

/**
 * IBusProcessKeyEventData:
 *
 * IBuProcessKeyEventData properties.
 */
struct _IBusProcessKeyEventData {
    /*< public >*/
    guint keyval;
    guint keycode;
    guint state;
};

/**
 * IBusExtensionEvent:
 *
 * IBusExtensionEvent properties.
 */
struct _IBusExtensionEvent {
    /*< private >*/
    IBusSerializable parent;
    IBusExtensionEventPrivate *priv;

    /* instance members */
    /*< public >*/
};

struct _IBusExtensionEventClass {
    /*< private >*/
    IBusSerializableClass parent;

    /* class members */
    /*< public >*/

    /*< private >*/
    /* padding */
    gpointer pdummy[10];
};


GType              ibus_extension_event_get_type    (void);

/**
 * ibus_extension_event_new:
 * @first_property_name: Name of the first property.
 * @...: the NULL-terminated arguments of the properties and values.
 *
 * Create a new #IBusExtensionEvent.
 *
 * Returns: A newly allocated #IBusExtensionEvent. E.g.
 * ibus_extension_event_new ("name", "emoji", "is-enabled", TRUE, NULL);
 */
IBusExtensionEvent *ibus_extension_event_new        (const gchar
                                                           *first_property_name,
                                                     ...);

/**
 * ibus_extension_event_get_version:
 * @event: An #IBusExtensionEvent.
 *
 * Returns: Version of #IBusExtensionEvent
 */
guint              ibus_extension_event_get_version (IBusExtensionEvent *event);

/**
 * ibus_extension_event_get_purpose:
 * @event: An #IBusExtensionEvent.
 *
 * Returns: name of the extension for #IBusXEvent
 */
const gchar *      ibus_extension_event_get_name    (IBusExtensionEvent *event);

/**
 * ibus_extension_event_is_enabled:
 * @event: An #IBusExtensionEvent.
 *
 * Returns: %TRUE if the extension is enabled for #IBusExtensionEvent
 */
gboolean           ibus_extension_event_is_enabled  (IBusExtensionEvent *event);

/**
 * ibus_extension_event_is_extension:
 * @event: An #IBusExtensionEvent.
 *
 * Returns: %TRUE if the #IBusExtensionEvent is called by an extension.
 * %FALSE if the #IBusExtensionEvent is called by an active engine or
 * panel.
 * If this value is %TRUE, the event is send to ibus-daemon, an active
 * engine. If it's %FALSE, the event is sned to ibus-daemon, panels.
 */
gboolean           ibus_extension_event_is_extension
                                                    (IBusExtensionEvent *event);

/**
 * ibus_extension_event_get_params:
 * @event: An #IBusExtensionEvent.
 *
 * Returns: Parameters to enable the extension for #IBusXEvent
 */
const gchar *      ibus_extension_event_get_params  (IBusExtensionEvent *event);



typedef enum {
    IBUS_X_EVENT_NOTHING           = -1,
    IBUS_X_EVENT_KEY_PRESS         = 0,
    IBUS_X_EVENT_KEY_RELEASE       = 1,
    IBUS_X_EVENT_OTHER             = 2,
    IBUS_X_EVENT_EVENT_LAST        /* helper variable for decls */
} IBusXEventType;

/**
 * IBusXEvent:
 * @type: event type
 *
 * IBusXEvent properties.
 */
struct _IBusXEvent {
    /*< private >*/
    IBusSerializable parent;
    IBusXEventPrivate *priv;

    /* instance members */
    /*< public >*/
    IBusXEventType event_type;
    guint          window;
    gint8          send_event;
    gulong         serial;
};

struct _IBusXEventClass {
    /*< private >*/
    IBusSerializableClass parent;

    /* class members */
    /*< public >*/

    /*< private >*/
    /* padding */
    gpointer pdummy[10];
};

GType        ibus_x_event_get_type       (void);

/**
 * ibus_x_event_new:
 * @first_property_name: Name of the first property.
 * @...: the NULL-terminated arguments of the properties and values.
 *
 * Create a new #IBusXEvent.
 *
 * Returns: A newly allocated #IBusXEvent. E.g.
 * ibus_x_event_new ("event-type", IBUS_X_EVENT_KEY_PRESS, NULL);
 */
IBusXEvent *   ibus_x_event_new            (const gchar
                                                           *first_property_name,
                                            ...);

/**
 * ibus_x_event_get_version:
 * @event: An #IBusXEvent.
 *
 * Returns: Version of #IBusXEvent
 */
guint          ibus_x_event_get_version    (IBusXEvent         *event);

/**
 * ibus_x_event_get_event_type:
 * @event: An #IBusXEvent.
 *
 * Returns: IBusXEventType of #IBusXEvent
 */
IBusXEventType ibus_x_event_get_event_type (IBusXEvent         *event);

/**
 * ibus_x_event_get_window:
 * @event: An #IBusXEvent.
 *
 * Returns: XID of #IBusXEvent
 */
guint32        ibus_x_event_get_window     (IBusXEvent         *event);

/**
 * ibus_x_event_get_send_event:
 * @event: An #IBusXEvent.
 *
 * Returns: send_event of #IBusXEvent
 */
gint8          ibus_x_event_get_send_event (IBusXEvent         *event);

/**
 * ibus_x_event_get_serial:
 * @event: An #IBusXEvent.
 *
 * Returns: serial of #IBusXEvent
 */
gulong         ibus_x_event_get_serial     (IBusXEvent         *event);

/**
 * ibus_x_event_get_time:
 * @event: An #IBusXEvent.
 *
 * Returns: time of #IBusXEvent
 */
guint32        ibus_x_event_get_time       (IBusXEvent         *event);

/**
 * ibus_x_event_get_state:
 * @event: An #IBusXEvent.
 *
 * Returns: state of #IBusXEvent
 */
guint          ibus_x_event_get_state      (IBusXEvent         *event);

/**
 * ibus_x_event_get_keyval:
 * @event: An #IBusXEvent.
 *
 * Returns: keyval of #IBusXEvent
 */
guint          ibus_x_event_get_keyval     (IBusXEvent         *event);

/**
 * ibus_x_event_get_length:
 * @event: An #IBusXEvent.
 *
 * Returns: length of #IBusXEvent
 */
gint           ibus_x_event_get_length     (IBusXEvent         *event);

/**
 * ibus_x_event_get_string:
 * @event: An #IBusXEvent.
 *
 * Returns: string of #IBusXEvent
 */
const gchar *  ibus_x_event_get_string     (IBusXEvent         *event);

/**
 * ibus_x_event_get_hardware_keycode:
 * @event: An #IBusXEvent.
 *
 * Returns: hardware keycode of #IBusXEvent
 */
guint16        ibus_x_event_get_hardware_keycode
                                           (IBusXEvent         *event);

/**
 * ibus_x_event_get_group:
 * @event: An #IBusXEvent.
 *
 * Returns: group of #IBusXEvent
 */
guint8         ibus_x_event_get_group      (IBusXEvent         *event);

/**
 * ibus_x_event_get_is_modifier:
 * @event: An #IBusXEvent.
 *
 * Returns: is_modifier of #IBusXEvent
 */
gboolean       ibus_x_event_get_is_modifier
                                           (IBusXEvent         *event);

/**
 * ibus_x_event_get_subwindow:
 * @event: An #IBusXEvent.
 *
 * Returns: subwindow of #IBusXEvent
 */
guint32        ibus_x_event_get_subwindow  (IBusXEvent         *event);

/**
 * ibus_x_event_get_root:
 * @event: An #IBusXEvent.
 *
 * Returns: root window of #IBusXEvent
 */
guint32        ibus_x_event_get_root       (IBusXEvent         *event);

/**
 * ibus_x_event_get_x:
 * @event: An #IBusXEvent.
 *
 * Returns: x of #IBusXEvent
 */
gint           ibus_x_event_get_x          (IBusXEvent         *event);

/**
 * ibus_x_event_get_y:
 * @event: An #IBusXEvent.
 *
 * Returns: y of #IBusXEvent
 */
gint           ibus_x_event_get_y          (IBusXEvent         *event);

/**
 * ibus_x_event_get_x_root:
 * @event: An #IBusXEvent.
 *
 * Returns: x-root of #IBusXEvent
 */
gint           ibus_x_event_get_x_root     (IBusXEvent         *event);

/**
 * ibus_x_event_get_y_root:
 * @event: An #IBusXEvent.
 *
 * Returns: y-root of #IBusXEvent
 */
gint           ibus_x_event_get_y_root     (IBusXEvent         *event);

/**
 * ibus_x_event_get_same_screen:
 * @event: An #IBusXEvent.
 *
 * Returns: same_screen of #IBusXEvent
 */
gboolean       ibus_x_event_get_same_screen
                                           (IBusXEvent         *event);

/**
 * ibus_x_event_get_purpose:
 * @event: An #IBusXEvent.
 *
 * Returns: purpose of #IBusXEvent
 */
const gchar *  ibus_x_event_get_purpose    (IBusXEvent         *event);

G_END_DECLS
#endif
