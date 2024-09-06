/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2013 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2018-2023 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2008-2019 Red Hat, Inc.
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
#include "ibusinputcontext.h"
#include <gio/gio.h>
#include "ibusshare.h"
#include "ibusinternal.h"
#include "ibusmarshalers.h"
#include "ibusattribute.h"
#include "ibuslookuptable.h"
#include "ibusproplist.h"
#include "ibuserror.h"

#define IBUS_INPUT_CONTEXT_GET_PRIVATE(o)  \
   ((IBusInputContextPrivate *)ibus_input_context_get_instance_private (o))

enum {
    ENABLED,
    DISABLED,
    COMMIT_TEXT,
    FORWARD_KEY_EVENT,
    DELETE_SURROUNDING_TEXT,
    UPDATE_PREEDIT_TEXT,
    UPDATE_PREEDIT_TEXT_WITH_MODE,
    SHOW_PREEDIT_TEXT,
    HIDE_PREEDIT_TEXT,
    UPDATE_AUXILIARY_TEXT,
    SHOW_AUXILIARY_TEXT,
    HIDE_AUXILIARY_TEXT,
    UPDATE_LOOKUP_TABLE,
    SHOW_LOOKUP_TABLE,
    HIDE_LOOKUP_TABLE,
    PAGE_UP_LOOKUP_TABLE,
    PAGE_DOWN_LOOKUP_TABLE,
    CURSOR_UP_LOOKUP_TABLE,
    CURSOR_DOWN_LOOKUP_TABLE,
    REGISTER_PROPERTIES,
    UPDATE_PROPERTY,
    REQUIRE_SURROUNDING_TEXT,
    LAST_SIGNAL,
};

/* IBusInputContextPrivate */
struct _IBusInputContextPrivate {
    /* TRUE if the current engine needs surrounding text; FALSE otherwise */
    gboolean  needs_surrounding_text;

    /* cached surrounding text (see also IBusEnginePrivate and
       BusEngineProxy) */
    IBusText *surrounding_text;
    guint     surrounding_cursor_pos;
    guint     selection_anchor_pos;
};

typedef struct _IBusInputContextPrivate IBusInputContextPrivate;

static guint            context_signals[LAST_SIGNAL] = { 0 };

static IBusText *text_empty = NULL;

/* functions prototype */
static void     ibus_input_context_real_destroy (IBusProxy              *context);
static void     ibus_input_context_g_signal     (GDBusProxy             *proxy,
                                                 const gchar            *sender_name,
                                                 const gchar            *signal_name,
                                                 GVariant               *parameters);

G_DEFINE_TYPE_WITH_PRIVATE (IBusInputContext,
                            ibus_input_context,
                            IBUS_TYPE_PROXY)

static void
ibus_input_context_class_init (IBusInputContextClass *class)
{
    IBusProxyClass *ibus_proxy_class = IBUS_PROXY_CLASS (class);
    GDBusProxyClass *g_dbus_proxy_class = G_DBUS_PROXY_CLASS (class);

    ibus_proxy_class->destroy = ibus_input_context_real_destroy;

    g_dbus_proxy_class->g_signal = ibus_input_context_g_signal;

    /* install signals */
    /**
     * IBusInputContext::enabled:
     * @context: An IBusInputContext.
     *
     * Emitted when an IME is enabled.
     */
    context_signals[ENABLED] =
        g_signal_new (I_("enabled"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusInputContext::disabled:
     * @context: An IBusInputContext.
     *
     * Emitted when an IME is disabled.
     */
    context_signals[DISABLED] =
        g_signal_new (I_("disabled"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusInputContext::commit-text:
     * @context: An IBusInputContext.
     * @text: Text to be committed.
     *
     * Emitted when the text is going to be committed.
     *
     * (Note: The text object is floating, and it will be released after the
     *  signal. If signal handler wants to keep the object, the handler should
     *  use g_object_ref_sink() to get the ownership of the object.)
     */
    context_signals[COMMIT_TEXT] =
        g_signal_new (I_("commit-text"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__OBJECT,
            G_TYPE_NONE,
            1,
            IBUS_TYPE_TEXT);

    /**
     * IBusInputContext::forward-key-event:
     * @context: An IBusInputContext.
     * @keyval: Key symbol of the keyboard event.
     * @keycode: Key symbol of the keyboard event.
     * @modifiers: Key modifier flags.
     *
     * Emitted to forward key event from IME to client of IME.
     */
    context_signals[FORWARD_KEY_EVENT] =
        g_signal_new (I_("forward-key-event"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__UINT_UINT_UINT,
            G_TYPE_NONE,
            3,
            G_TYPE_UINT,
            G_TYPE_UINT,
            G_TYPE_UINT);

    /**
     * IBusInputContext::delete-surrounding-text:
     * @context: An IBusInputContext.
     * @offset: the character offset from the cursor position of the text to be deleted.
     *   A negative value indicates a position before the cursor.
     * @n_chars: the number of characters to be deleted.
     *
     * Emitted to delete surrounding text event from IME to client of IME.
     */
    context_signals[DELETE_SURROUNDING_TEXT] =
        g_signal_new (I_("delete-surrounding-text"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__INT_UINT,
            G_TYPE_NONE,
            2,
            G_TYPE_INT,
            G_TYPE_UINT);

    /**
     * IBusInputContext::update-preedit-text:
     * @context: An IBusInputContext.
     * @text: Text to be updated.
     * @cursor_pos: Cursor position.
     * @visible: Whether the update is visible.
     *
     * Emitted to update preedit text.
     *
     * (Note: The text object is floating, and it will be released after the
     *  signal. If signal handler wants to keep the object, the handler should
     *  use g_object_ref_sink() to get the ownership of the object.)
     */
    context_signals[UPDATE_PREEDIT_TEXT] =
        g_signal_new (I_("update-preedit-text"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__OBJECT_UINT_BOOLEAN,
            G_TYPE_NONE,
            3,
            IBUS_TYPE_TEXT,
            G_TYPE_UINT,
            G_TYPE_BOOLEAN);

    /**
     * IBusInputContext::update-preedit-text-with-mode:
     * @context: An IBusInputContext.
     * @text: Text to be updated.
     * @cursor_pos: Cursor position.
     * @visible: Whether the update is visible.
     * @mode: Preedit mode.
     *
     * Emitted to update preedit text with the mode.
     *
     * (Note: The text object is floating, and it will be released after the
     *  signal. If signal handler wants to keep the object, the handler should
     *  use g_object_ref_sink() to get the ownership of the object.)
     */
    context_signals[UPDATE_PREEDIT_TEXT_WITH_MODE] =
        g_signal_new (I_("update-preedit-text-with-mode"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__OBJECT_UINT_BOOLEAN_UINT,
            G_TYPE_NONE,
            4,
            IBUS_TYPE_TEXT,
            G_TYPE_UINT,
            G_TYPE_BOOLEAN,
            G_TYPE_UINT);

    /**
     * IBusInputContext::show-preedit-text:
     * @context: An IBusInputContext.
     *
     * Emitted to show preedit text.
     */
    context_signals[SHOW_PREEDIT_TEXT] =
        g_signal_new (I_("show-preedit-text"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusInputContext::hide-preedit-text:
     * @context: An IBusInputContext.
     *
     * Emitted to hide preedit text.
     */
    context_signals[HIDE_PREEDIT_TEXT] =
        g_signal_new (I_("hide-preedit-text"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusInputContext::update-auxiliary-text:
     * @context: An IBusInputContext.
     * @text: An auxiliary text
     * @visible: The visibility of @text
     *
     * Emitted to hide auxilary text.
     *
     * (Note: The text object is floating, and it will be released after the
     *  signal. If signal handler wants to keep the object, the handler should
     *  use g_object_ref_sink() to get the ownership of the object.)
     */
    context_signals[UPDATE_AUXILIARY_TEXT] =
        g_signal_new (I_("update-auxiliary-text"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__OBJECT_BOOLEAN,
            G_TYPE_NONE, 2,
            IBUS_TYPE_TEXT,
            G_TYPE_BOOLEAN);

    /**
     * IBusInputContext::show-auxiliary-text:
     * @context: An IBusInputContext.
     *
     * Emitted to show auxiliary text.
     */
    context_signals[SHOW_AUXILIARY_TEXT] =
        g_signal_new (I_("show-auxiliary-text"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusInputContext::hide-auxiliary-text:
     * @context: An IBusInputContext.
     *
     * Emitted to hide auxiliary text.
     */
    context_signals[HIDE_AUXILIARY_TEXT] =
        g_signal_new (I_("hide-auxiliary-text"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusInputContext::update-lookup-table:
     * @context: An IBusInputContext.
     * @table: An IBusLookupTable to be updated.
     * @visible: Whether the table should be visible.
     *
     * Emitted to update lookup table.
     *
     * (Note: The table object is floating, and it will be released after the
     *  signal. If signal handler wants to keep the object, the handler should
     *  use g_object_ref_sink() to get the ownership of the object.)
     */
    context_signals[UPDATE_LOOKUP_TABLE] =
        g_signal_new (I_("update-lookup-table"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__OBJECT_BOOLEAN,
            G_TYPE_NONE, 2,
            IBUS_TYPE_LOOKUP_TABLE,
            G_TYPE_BOOLEAN);

    /**
     * IBusInputContext::show-lookup-table:
     * @context: An IBusInputContext.
     *
     * Emitted to show lookup table.
     */
    context_signals[SHOW_LOOKUP_TABLE] =
        g_signal_new (I_("show-lookup-table"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusInputContext::hide-lookup-table:
     * @context: An IBusInputContext.
     *
     * Emitted to hide lookup table.
     */
    context_signals[HIDE_LOOKUP_TABLE] =
        g_signal_new (I_("hide-lookup-table"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusInputContext::page-up-lookup-table:
     * @context: An IBusInputContext.
     *
     * Emitted to view the previous page of lookup table.
     */
    context_signals[PAGE_UP_LOOKUP_TABLE] =
        g_signal_new (I_("page-up-lookup-table"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusInputContext::page-down-lookup-table:
     * @context: An IBusInputContext.
     *
     * Emitted to view the next page of lookup table.
     */
    context_signals[PAGE_DOWN_LOOKUP_TABLE] =
        g_signal_new (I_("page-down-lookup-table"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusInputContext::cursor-up-lookup-table:
     * @context: An IBusInputContext.
     *
     * Emitted to select previous candidate of lookup table.
     */
    context_signals[CURSOR_UP_LOOKUP_TABLE] =
        g_signal_new (I_("cursor-up-lookup-table"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * IBusInputContext::cursor-down-lookup-table:
     * @context: An IBusInputContext.
     *
     * Emitted to select next candidate of lookup table.
     */
    context_signals[CURSOR_DOWN_LOOKUP_TABLE] =
        g_signal_new (I_("cursor-down-lookup-table"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE,
            0);

    /**
     * IBusInputContext::register-properties:
     * @context: An IBusInputContext.
     * @props: An IBusPropList that contains properties.
     *
     * Emitted to register the properties in @props.
     *
     * (Note: The props object is floating, and it will be released after the
     *  signal. If signal handler wants to keep the object, the handler should
     *  use g_object_ref_sink() to get the ownership of the object.)
     */
    context_signals[REGISTER_PROPERTIES] =
        g_signal_new (I_("register-properties"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__OBJECT,
            G_TYPE_NONE,
            1,
            IBUS_TYPE_PROP_LIST);

    /**
     * IBusInputContext::update-property:
     * @context: An IBusInputContext.
     * @prop: The IBusProperty to be updated.
     *
     * Emitted to update the property @prop.
     *
     * (Note: The prop object is floating, and it will be released after the
     *  signal. If signal handler wants to keep the object, the handler should
     *  use g_object_ref_sink() to get the ownership of the object.)
     */
    context_signals[UPDATE_PROPERTY] =
        g_signal_new (I_("update-property"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__OBJECT,
            G_TYPE_NONE,
            1,
            IBUS_TYPE_PROPERTY);

    /**
     * IBusInputContext::require-surrounding-text:
     * @context: An IBusInputContext.
     *
     * Emitted to receive the RequireSurroundingText signal from the daemon.
     */
    context_signals[REQUIRE_SURROUNDING_TEXT] =
        g_signal_new (I_("require-surrounding-text"),
            G_TYPE_FROM_CLASS (class),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            _ibus_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    text_empty = ibus_text_new_from_static_string ("");
    g_object_ref_sink (text_empty);
}

static void
ibus_input_context_init (IBusInputContext *context)
{
    IBusInputContextPrivate *priv;

    priv = IBUS_INPUT_CONTEXT_GET_PRIVATE (context);
    priv->surrounding_text = g_object_ref_sink (text_empty);
}

static void
ibus_input_context_real_destroy (IBusProxy *context)
{
    IBusInputContextPrivate *priv;
    priv = IBUS_INPUT_CONTEXT_GET_PRIVATE (IBUS_INPUT_CONTEXT (context));

    if (priv->surrounding_text) {
        g_object_unref (priv->surrounding_text);
        priv->surrounding_text = NULL;
    }

    IBUS_PROXY_CLASS(ibus_input_context_parent_class)->destroy (context);
}

static void
ibus_input_context_g_signal (GDBusProxy  *proxy,
                             const gchar *sender_name,
                             const gchar *signal_name,
                             GVariant    *parameters)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (proxy));

    IBusInputContext *context;
    context = IBUS_INPUT_CONTEXT (proxy);

    static const struct {
        const gchar *signal_name;
        guint signal_id;
    } signals [] = {
        { "ShowPreeditText",        SHOW_PREEDIT_TEXT        },
        { "HidePreeditText",        HIDE_PREEDIT_TEXT        },
        { "ShowAuxiliaryText",      SHOW_AUXILIARY_TEXT      },
        { "HideAuxiliaryText",      HIDE_AUXILIARY_TEXT      },
        { "ShowLookupTable",        SHOW_LOOKUP_TABLE        },
        { "HideLookupTable",        HIDE_LOOKUP_TABLE        },
        { "PageUpLookupTable",      PAGE_UP_LOOKUP_TABLE     },
        { "PageDownLookupTable",    PAGE_DOWN_LOOKUP_TABLE   },
        { "CursorUpLookupTable",    CURSOR_UP_LOOKUP_TABLE   },
        { "CursorDownLookupTable",  CURSOR_DOWN_LOOKUP_TABLE },
    };

    if (g_strcmp0 (signal_name, "CommitText") == 0) {
        GVariant *variant = NULL;
        g_variant_get (parameters, "(v)", &variant);
        IBusText *text = IBUS_TEXT (ibus_serializable_deserialize (variant));
        g_variant_unref (variant);
        g_signal_emit (context, context_signals[COMMIT_TEXT], 0, text);

        if (g_object_is_floating (text)) {
            g_object_ref_sink (text);
            g_object_unref (text);
        }
        return;
    }
    if (g_strcmp0 (signal_name, "UpdatePreeditText") == 0) {
        GVariant *variant = NULL;
        gint32 cursor_pos;
        gboolean visible;
        g_variant_get (parameters, "(vub)", &variant, &cursor_pos, &visible);
        IBusText *text = IBUS_TEXT (ibus_serializable_deserialize (variant));
        g_variant_unref (variant);

        g_signal_emit (context,
                       context_signals[UPDATE_PREEDIT_TEXT],
                       0,
                       text,
                       cursor_pos,
                       visible);

        if (g_object_is_floating (text)) {
            g_object_ref_sink (text);
            g_object_unref (text);
        }
        return;
    }
    if (g_strcmp0 (signal_name, "UpdatePreeditTextWithMode") == 0) {
        GVariant *variant = NULL;
        gint32 cursor_pos;
        gboolean visible;
        guint mode = 0;
        g_variant_get (parameters,
                       "(vubu)", &variant, &cursor_pos, &visible, &mode);
        IBusText *text = IBUS_TEXT (ibus_serializable_deserialize (variant));
        g_variant_unref (variant);

        g_signal_emit (context,
                       context_signals[UPDATE_PREEDIT_TEXT_WITH_MODE],
                       0,
                       text,
                       cursor_pos,
                       visible,
                       mode);

        if (g_object_is_floating (text)) {
            g_object_ref_sink (text);
            g_object_unref (text);
        }
        return;
    }

    /* lookup signal in table */
    gint i;
    for (i = 0;
         i < G_N_ELEMENTS (signals) && g_strcmp0 (signal_name, signals[i].signal_name) != 0;
         i++);

    if (i < G_N_ELEMENTS (signals)) {
        g_signal_emit (context, context_signals[signals[i].signal_id], 0);
        return;
    }

    if (g_strcmp0 (signal_name, "UpdateAuxiliaryText") == 0) {
        GVariant *variant = NULL;
        gboolean visible;
        g_variant_get (parameters, "(vb)", &variant, &visible);
        IBusText *text = IBUS_TEXT (ibus_serializable_deserialize (variant));
        g_variant_unref (variant);

        g_signal_emit (context,
                       context_signals[UPDATE_AUXILIARY_TEXT],
                       0,
                       text,
                       visible);
        if (g_object_is_floating (text)) {
            g_object_ref_sink (text);
            g_object_unref (text);
        }
        return;
    }

    if (g_strcmp0 (signal_name, "UpdateLookupTable") == 0) {
        GVariant *variant = NULL;
        gboolean visible;
        g_variant_get (parameters, "(vb)", &variant, &visible);

        IBusLookupTable *table = IBUS_LOOKUP_TABLE (ibus_serializable_deserialize (variant));
        g_variant_unref (variant);

        g_signal_emit (context,
                       context_signals[UPDATE_LOOKUP_TABLE],
                       0,
                       table,
                       visible);
        if (g_object_is_floating (table)) {
            g_object_ref_sink (table);
            g_object_unref (table);
        }
        return;

    }

    if (g_strcmp0 (signal_name, "RegisterProperties") == 0) {
        GVariant *variant = NULL;
        g_variant_get (parameters, "(v)", &variant);

        IBusPropList *prop_list = IBUS_PROP_LIST (ibus_serializable_deserialize (variant));
        g_variant_unref (variant);

        g_signal_emit (context,
                       context_signals[REGISTER_PROPERTIES],
                       0,
                       prop_list);

        if (g_object_is_floating (prop_list)) {
            g_object_ref_sink (prop_list);
            g_object_unref (prop_list);
        }
        return;
    }

    if (g_strcmp0 (signal_name, "UpdateProperty") == 0) {
        GVariant *variant = NULL;
        g_variant_get (parameters, "(v)", &variant);
        IBusProperty *prop = IBUS_PROPERTY (ibus_serializable_deserialize (variant));
        g_variant_unref (variant);

        g_signal_emit (context, context_signals[UPDATE_PROPERTY], 0, prop);

        if (g_object_is_floating (prop)) {
            g_object_ref_sink (prop);
            g_object_unref (prop);
        }
        return;
    }

    if (g_strcmp0 (signal_name, "ForwardKeyEvent") == 0) {
        guint32 keyval;
        guint32 keycode;
        guint32 state;

        g_variant_get (parameters, "(uuu)", &keyval, &keycode, &state);

        /* Forward key event back with IBUS_FORWARD_MASK. And process_key_event will
         * not process key event with IBUS_FORWARD_MASK again. */
        g_signal_emit (context,
                       context_signals[FORWARD_KEY_EVENT],
                       0,
                       keyval,
                       keycode,
                       state | IBUS_FORWARD_MASK);
        return;
    }

    if (g_strcmp0 (signal_name, "DeleteSurroundingText") == 0) {
        gint offset_from_cursor;
        guint nchars;

        g_variant_get (parameters, "(iu)", &offset_from_cursor, &nchars);

        g_signal_emit (context,
                       context_signals[DELETE_SURROUNDING_TEXT],
                       0,
                       offset_from_cursor,
                       nchars);
        return;
    }

    IBusInputContextPrivate *priv;
    priv = IBUS_INPUT_CONTEXT_GET_PRIVATE (IBUS_INPUT_CONTEXT (context));

    if (g_strcmp0 (signal_name, "Enabled") == 0) {
        priv->needs_surrounding_text = FALSE;
        g_signal_emit (context, context_signals[ENABLED], 0);
        return;
    }

    if (g_strcmp0 (signal_name, "Disabled") == 0) {
        priv->needs_surrounding_text = FALSE;
        g_signal_emit (context, context_signals[DISABLED], 0);
        return;
    }

    if (g_strcmp0 (signal_name, "RequireSurroundingText") == 0) {
        priv->needs_surrounding_text = TRUE;
        g_signal_emit (context, context_signals[REQUIRE_SURROUNDING_TEXT], 0);
        return;
    }

    G_DBUS_PROXY_CLASS (ibus_input_context_parent_class)->g_signal (
                                proxy, sender_name, signal_name, parameters);
}

IBusInputContext *
ibus_input_context_new (const gchar     *path,
                        GDBusConnection *connection,
                        GCancellable    *cancellable,
                        GError         **error)
{
    g_assert (path != NULL);
    g_assert (G_IS_DBUS_CONNECTION (connection));
    const gchar *service_name = IBUS_SERVICE_IBUS;

    GInitable *initable;

    GDBusProxyFlags flags = G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START;

    if (g_object_get_data (G_OBJECT (connection), "ibus-portal-connection"))
        service_name = IBUS_SERVICE_PORTAL;

    initable = g_initable_new (IBUS_TYPE_INPUT_CONTEXT,
                               cancellable,
                               error,
                               "g-connection",      connection,
                               "g-name",            service_name,
                               "g-flags",           flags,
                               "g-interface-name",  IBUS_INTERFACE_INPUT_CONTEXT,
                               "g-object-path",     path,
                               "g-default-timeout", ibus_get_timeout (),
                               NULL);
    if (initable != NULL)
        return IBUS_INPUT_CONTEXT (initable);
    return NULL;
}

void
ibus_input_context_new_async (const gchar         *path,
                              GDBusConnection     *connection,
                              GCancellable        *cancellable,
                              GAsyncReadyCallback  callback,
                              gpointer             user_data)
{
    g_assert (path != NULL);
    g_assert (G_IS_DBUS_CONNECTION (connection));
    g_assert (callback != NULL);
    const gchar *service_name = IBUS_SERVICE_IBUS;

    GDBusProxyFlags flags = G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START;

    if (g_object_get_data (G_OBJECT (connection), "ibus-portal-connection"))
        service_name = IBUS_SERVICE_PORTAL;

    g_async_initable_new_async (IBUS_TYPE_INPUT_CONTEXT,
                                G_PRIORITY_DEFAULT,
                                cancellable,
                                callback,
                                user_data,
                                "g-connection",      connection,
                                "g-name",            service_name,
                                "g-flags",           flags,
                                "g-interface-name",  IBUS_INTERFACE_INPUT_CONTEXT,
                                "g-object-path",     path,
                                "g-default-timeout", ibus_get_timeout (),
                                NULL);
}

IBusInputContext *
ibus_input_context_new_async_finish (GAsyncResult  *res,
                                     GError       **error)
{
    GObject *object = NULL;
    GObject *source_object = NULL;

    source_object = g_async_result_get_source_object (res);
    g_assert (source_object != NULL);

    object = g_async_initable_new_finish (G_ASYNC_INITABLE (source_object),
                                          res,
                                          error);
    g_object_unref (source_object);

    if (object != NULL) {
        return IBUS_INPUT_CONTEXT (object);
    }
    else {
        return NULL;
    }
}

IBusInputContext *
ibus_input_context_get_input_context (const gchar     *path,
                                      GDBusConnection *connection)
{
    IBusInputContext *context = NULL;
    GError *error = NULL;

    context = ibus_input_context_new (path, connection, NULL, &error);
    if (context == NULL) {
        g_warning ("ibus_input_context_get_input_context: %s", error->message);
        g_error_free (error);
        return NULL;
    }

    /* Do not call "org.freedesktop.IBus.Service.Destroy" when the input
     * context object is disposed. */
    IBUS_PROXY (context)->own = FALSE;
    return context;
}

void
ibus_input_context_get_input_context_async (const gchar         *path,
                                            GDBusConnection     *connection,
                                            GCancellable        *cancellable,
                                            GAsyncReadyCallback  callback,
                                            gpointer             user_data)
{
    ibus_input_context_new_async (path,
                                  connection,
                                  cancellable,
                                  callback,
                                  user_data);
}

IBusInputContext *
ibus_input_context_get_input_context_async_finish (GAsyncResult  *res,
                                                   GError       **error)
{
    IBusInputContext *context = NULL;

    context = ibus_input_context_new_async_finish (res, error);
    if (context == NULL) {
        return NULL;
    }

    /* Do not call "org.freedesktop.IBus.Service.Destroy" when the input
     * context object is disposed. */
    IBUS_PROXY (context)->own = FALSE;
    return context;
}

void
ibus_input_context_process_hand_writing_event (IBusInputContext   *context,
                                               const gdouble      *coordinates,
                                               guint               coordinates_len)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (context));
    g_return_if_fail (coordinates != NULL);
    g_return_if_fail (coordinates_len >= 4); /* The array should contain at least one line. */
    g_return_if_fail ((coordinates_len & 1) == 0);

    guint i;
    GVariantBuilder builder;
    g_variant_builder_init (&builder, G_VARIANT_TYPE ("ad"));
    for (i = 0; i < coordinates_len; i++) {
        g_variant_builder_add (&builder, "d", coordinates[i]);
    }

    g_dbus_proxy_call ((GDBusProxy *) context,
                       "ProcessHandWritingEvent",           /* method_name */
                       g_variant_new ("(ad)", &builder),    /* parameters */
                       G_DBUS_CALL_FLAGS_NONE,              /* flags */
                       -1,                                  /* timeout */
                       NULL,                                /* cancellable */
                       NULL,                                /* callback */
                       NULL                                 /* user_data */
                       );
}

void
ibus_input_context_cancel_hand_writing (IBusInputContext   *context,
                                        guint               n_strokes)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (context));

    g_dbus_proxy_call ((GDBusProxy *) context,
                       "CancelHandWriting",                 /* method_name */
                       g_variant_new ("(u)", n_strokes),    /* parameters */
                       G_DBUS_CALL_FLAGS_NONE,              /* flags */
                       -1,                                  /* timeout */
                       NULL,                                /* cancellable */
                       NULL,                                /* callback */
                       NULL                                 /* user_data */
                       );
}

void
ibus_input_context_process_key_event_async (IBusInputContext   *context,
                                            guint32             keyval,
                                            guint32             keycode,
                                            guint32             state,
                                            gint                timeout_msec,
                                            GCancellable       *cancellable,
                                            GAsyncReadyCallback callback,
                                            gpointer            user_data)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (context));

    g_dbus_proxy_call ((GDBusProxy *) context,
                       "ProcessKeyEvent",                   /* method_name */
                       g_variant_new ("(uuu)",
                            keyval, keycode, state),        /* parameters */
                       G_DBUS_CALL_FLAGS_NONE,              /* flags */
                       timeout_msec,                        /* timeout */
                       cancellable,                         /* cancellable */
                       callback,                            /* callback */
                       user_data                            /* user_data */
                       );
}

gboolean
ibus_input_context_process_key_event_async_finish (IBusInputContext  *context,
                                                   GAsyncResult      *res,
                                                   GError           **error)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (context));
    g_assert (G_IS_ASYNC_RESULT (res));
    g_assert (error == NULL || *error == NULL);

    gboolean processed = FALSE;

    GVariant *variant = g_dbus_proxy_call_finish ((GDBusProxy *) context,
                                                   res, error);
    if (variant != NULL) {
        g_variant_get (variant, "(b)", &processed);
        g_variant_unref (variant);
    }

    return processed;
}

gboolean
ibus_input_context_process_key_event (IBusInputContext *context,
                                      guint32           keyval,
                                      guint32           keycode,
                                      guint32           state)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (context));

    GVariant *result = g_dbus_proxy_call_sync ((GDBusProxy *) context,
                            "ProcessKeyEvent",              /* method_name */
                            g_variant_new ("(uuu)",
                                 keyval, keycode, state),   /* parameters */
                            G_DBUS_CALL_FLAGS_NONE,         /* flags */
                            -1,                             /* timeout */
                            NULL,                           /* cancellable */
                            NULL);

    if (result != NULL) {
        gboolean processed = FALSE;

        g_variant_get (result, "(b)", &processed);
        g_variant_unref (result);
        return processed;
    }

    return FALSE;
}

void
ibus_input_context_set_cursor_location (IBusInputContext *context,
                                        gint32            x,
                                        gint32            y,
                                        gint32            w,
                                        gint32            h)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (context));

    g_dbus_proxy_call ((GDBusProxy *) context,
                       "SetCursorLocation",                 /* method_name */
                       g_variant_new ("(iiii)", x, y, w, h),/* parameters */
                       G_DBUS_CALL_FLAGS_NONE,              /* flags */
                       -1,                                  /* timeout */
                       NULL,                                /* cancellable */
                       NULL,                                /* callback */
                       NULL                                 /* user_data */
                       );
}

void
ibus_input_context_set_cursor_location_relative (IBusInputContext *context,
                                                 gint32            x,
                                                 gint32            y,
                                                 gint32            w,
                                                 gint32            h)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (context));

    g_dbus_proxy_call ((GDBusProxy *) context,
                       "SetCursorLocationRelative",         /* method_name */
                       g_variant_new ("(iiii)", x, y, w, h),/* parameters */
                       G_DBUS_CALL_FLAGS_NONE,              /* flags */
                       -1,                                  /* timeout */
                       NULL,                                /* cancellable */
                       NULL,                                /* callback */
                       NULL                                 /* user_data */
                       );
}

void
ibus_input_context_set_capabilities (IBusInputContext   *context,
                                     guint32             capabilites)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (context));
    g_dbus_proxy_call ((GDBusProxy *) context,
                       "SetCapabilities",                   /* method_name */
                       g_variant_new ("(u)", capabilites),  /* parameters */
                       G_DBUS_CALL_FLAGS_NONE,              /* flags */
                       -1,                                  /* timeout */
                       NULL,                                /* cancellable */
                       NULL,                                /* callback */
                       NULL                                 /* user_data */
                       );
}

void
ibus_input_context_property_activate (IBusInputContext *context,
                                      const gchar      *prop_name,
                                      guint32           state)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (context));
    g_dbus_proxy_call ((GDBusProxy *) context,
                       "PropertyActivate",                  /* method_name */
                       g_variant_new ("(su)",
                                prop_name, state),          /* parameters */
                       G_DBUS_CALL_FLAGS_NONE,              /* flags */
                       -1,                                  /* timeout */
                       NULL,                                /* cancellable */
                       NULL,                                /* callback */
                       NULL                                 /* user_data */
                       );
}

void
ibus_input_context_property_show (IBusInputContext *context,
                                  const gchar     *prop_name)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (context));
    g_dbus_proxy_call ((GDBusProxy *) context,
                       "PropertyShow",                      /* method_name */
                       g_variant_new ("(s)", prop_name),    /* parameters */
                       G_DBUS_CALL_FLAGS_NONE,              /* flags */
                       -1,                                  /* timeout */
                       NULL,                                /* cancellable */
                       NULL,                                /* callback */
                       NULL                                 /* user_data */
                       );
}

void
ibus_input_context_property_hide (IBusInputContext *context,
                                       const gchar      *prop_name)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (context));
    g_dbus_proxy_call ((GDBusProxy *) context,
                       "PropertyHide",                      /* method_name */
                       g_variant_new ("(s)", prop_name),    /* parameters */
                       G_DBUS_CALL_FLAGS_NONE,              /* flags */
                       -1,                                  /* timeout */
                       NULL,                                /* cancellable */
                       NULL,                                /* callback */
                       NULL                                 /* user_data */
                       );
}

void
ibus_input_context_set_surrounding_text (IBusInputContext   *context,
                                         IBusText           *text,
                                         guint32             cursor_pos,
                                         guint32             anchor_pos)
{
    IBusInputContextPrivate *priv;

    g_assert (IBUS_IS_INPUT_CONTEXT (context));
    g_assert (IBUS_IS_TEXT (text));

    priv = IBUS_INPUT_CONTEXT_GET_PRIVATE (context);

    /* This API should send "SetSurroundingText" D-Bus method when
     * input contexts are switched between tabs in a text application
     * so that engines can receive the updated surrounding texts after
     * focus-in events happen.
     *
     * GNOME shell uses a single input context and the address of the input
     * contexts are always same. So check the address of texts if the input
     * contexts on applications are switched.
     */
    if (cursor_pos != priv->surrounding_cursor_pos ||
        anchor_pos != priv->selection_anchor_pos ||
        priv->surrounding_text == NULL ||
        text != priv->surrounding_text ||
        g_strcmp0 (text->text, priv->surrounding_text->text) != 0) {
        if (priv->surrounding_text)
            g_object_unref (priv->surrounding_text);
        priv->surrounding_text = (IBusText *) g_object_ref_sink (text);
        priv->surrounding_cursor_pos = cursor_pos;
        priv->selection_anchor_pos = anchor_pos;

        if (priv->needs_surrounding_text) {
            GVariant *variant = ibus_serializable_serialize ((IBusSerializable *)text);
            g_dbus_proxy_call ((GDBusProxy *) context,
                               "SetSurroundingText",        /* method_name */
                               g_variant_new ("(vuu)",
                                              variant,
                                              cursor_pos,
                                              anchor_pos),  /* parameters */
                                G_DBUS_CALL_FLAGS_NONE,     /* flags */
                                -1,                         /* timeout */
                                NULL,                       /* cancellable */
                                NULL,                       /* callback */
                                NULL                        /* user_data */
                                );
        }
    } else {
       g_object_unref(text);
    }
}

gboolean
ibus_input_context_needs_surrounding_text (IBusInputContext *context)
{
    IBusInputContextPrivate *priv;
    priv = IBUS_INPUT_CONTEXT_GET_PRIVATE (IBUS_INPUT_CONTEXT (context));
    return priv->needs_surrounding_text;
}

void
ibus_input_context_set_content_type (IBusInputContext *context,
                                     guint             purpose,
                                     guint             hints)
{
    GVariant *cached_content_type;
    GVariant *content_type;

    g_assert (IBUS_IS_INPUT_CONTEXT (context));

    cached_content_type =
        g_dbus_proxy_get_cached_property ((GDBusProxy *)context,
                                          "ContentType");
    content_type = g_variant_new ("(uu)", purpose, hints);

    g_variant_ref_sink (content_type);
    if (!cached_content_type ||
        !g_variant_equal (content_type, cached_content_type)) {
        g_dbus_proxy_call ((GDBusProxy *)context,
                           "org.freedesktop.DBus.Properties.Set",
                           g_variant_new ("(ssv)",
                                          IBUS_INTERFACE_INPUT_CONTEXT,
                                          "ContentType",
                                          content_type),
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL, /* cancellable */
                           NULL, /* callback */
                           NULL  /* user_data */
                           );
        /* Need to update the cache by manual since there is a timing issue. */
        g_dbus_proxy_set_cached_property ((GDBusProxy *)context,
                                          "ContentType",
                                          content_type);
    }

    if (cached_content_type)
        g_variant_unref (cached_content_type);
    g_variant_unref (content_type);
}

void
ibus_input_context_get_engine_async (IBusInputContext   *context,
                                     gint                timeout_msec,
                                     GCancellable       *cancellable,
                                     GAsyncReadyCallback callback,
                                     gpointer            user_data)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (context));
    g_dbus_proxy_call ((GDBusProxy *) context,
                       "GetEngine",               /* method_name */
                       NULL,                      /* parameters */
                       G_DBUS_CALL_FLAGS_NONE,    /* flags */
                       timeout_msec,
                       cancellable,
                       callback,
                       user_data);
}

IBusEngineDesc *
ibus_input_context_get_engine_async_finish (IBusInputContext   *context,
                                            GAsyncResult       *res,
                                            GError            **error)
{
    GVariant *variant;
    GVariant *engine_desc_variant;
    IBusEngineDesc *desc;

    g_assert (IBUS_IS_INPUT_CONTEXT (context));
    g_assert (G_IS_ASYNC_RESULT (res));
    g_assert (error == NULL || *error == NULL);

    variant = g_dbus_proxy_call_finish ((GDBusProxy *) context, res, error);
    if (variant == NULL) {
        return NULL;
    }

    engine_desc_variant = g_variant_get_child_value (variant, 0);
    desc = IBUS_ENGINE_DESC (
            ibus_serializable_deserialize (engine_desc_variant));
    g_variant_unref (engine_desc_variant);
    g_variant_unref (variant);

    return desc;
}

IBusEngineDesc *
ibus_input_context_get_engine (IBusInputContext *context)
{
    GVariant *result = NULL;
    GError *error = NULL;
    GVariant *engine_desc_variant;
    IBusEngineDesc *desc;

    g_assert (IBUS_IS_INPUT_CONTEXT (context));

    result = g_dbus_proxy_call_sync ((GDBusProxy *) context,
                                     "GetEngine",               /* method_name */
                                     NULL,                      /* parameters */
                                     G_DBUS_CALL_FLAGS_NONE,    /* flags */
                                     -1,                        /* timeout */
                                     NULL,                      /* cancellable */
                                     &error                     /* error */
                                     );
    if (result == NULL) {
        if (g_error_matches (error, IBUS_ERROR, IBUS_ERROR_NO_ENGINE)) {
            g_debug ("%s.GetEngine: %s",
                     IBUS_INTERFACE_INPUT_CONTEXT,
                     error->message);
        }
        else {
            g_warning ("%s.GetEngine: %s",
                       IBUS_INTERFACE_INPUT_CONTEXT,
                       error->message);
        }
        g_error_free (error);
        return NULL;
    }

    engine_desc_variant = g_variant_get_child_value (result, 0);
    desc = IBUS_ENGINE_DESC (
            ibus_serializable_deserialize (engine_desc_variant));
    g_variant_unref (engine_desc_variant);
    g_variant_unref (result);

    return desc;
}

void
ibus_input_context_set_engine (IBusInputContext *context,
                               const gchar *name)
{
    g_assert (IBUS_IS_INPUT_CONTEXT (context));
    g_assert (name);
    g_dbus_proxy_call ((GDBusProxy *) context,
                       "SetEngine",                         /* method_name */
                       g_variant_new ("(s)", name),         /* parameters */
                       G_DBUS_CALL_FLAGS_NONE,              /* flags */
                       -1,                                  /* timeout */
                       NULL,                                /* cancellable */
                       NULL,                                /* callback */
                       NULL                                 /* user_data */
                       );
}

void
ibus_input_context_set_client_commit_preedit (IBusInputContext *context,
                                              gboolean          client_commit)
{
    GVariant *cached_var_client_commit;
    GVariant *var_client_commit;

    g_assert (IBUS_IS_INPUT_CONTEXT (context));

    cached_var_client_commit =
        g_dbus_proxy_get_cached_property ((GDBusProxy *)context,
                                          "ClientCommitPreedit");
    var_client_commit = g_variant_new ("(b)", client_commit);

    g_variant_ref_sink (var_client_commit);
    if (!cached_var_client_commit ||
        !g_variant_equal (var_client_commit, cached_var_client_commit)) {
        g_dbus_proxy_call ((GDBusProxy *)context,
                           "org.freedesktop.DBus.Properties.Set",
                           g_variant_new ("(ssv)",
                                          IBUS_INTERFACE_INPUT_CONTEXT,
                                          "ClientCommitPreedit",
                                          var_client_commit),
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL, /* cancellable */
                           NULL, /* callback */
                           NULL  /* user_data */
                           );
        /* Need to update the cache by manual since there is a timing issue. */
        g_dbus_proxy_set_cached_property ((GDBusProxy *)context,
                                          "ClientCommitPreedit",
                                          var_client_commit);
    }

    if (cached_var_client_commit)
        g_variant_unref (cached_var_client_commit);
    g_variant_unref (var_client_commit);
}

void
ibus_input_context_set_post_process_key_event (IBusInputContext *context,
                                               gboolean          enable)
{
    GVariant *cached_var_post;
    GVariant *var_post;

    g_assert (IBUS_IS_INPUT_CONTEXT (context));

    cached_var_post =
        g_dbus_proxy_get_cached_property ((GDBusProxy *)context,
                                          "EffectivePostProcessKeyEvent");
    var_post = g_variant_new ("(b)", enable);
    g_variant_ref_sink (var_post);
    if (!cached_var_post ||
        !g_variant_equal (var_post, cached_var_post)) {
        g_dbus_proxy_call ((GDBusProxy *)context,
                           "org.freedesktop.DBus.Properties.Set",
                           g_variant_new ("(ssv)",
                                          IBUS_INTERFACE_INPUT_CONTEXT,
                                          "EffectivePostProcessKeyEvent",
                                          var_post),
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL, /* cancellable */
                           NULL, /* callback */
                           NULL  /* user_data */
                           );
        /* Need to update the cache by manual since there is a timing issue. */
        g_dbus_proxy_set_cached_property ((GDBusProxy *)context,
                                          "EffectivePostProcessKeyEvent",
                                          var_post);
    }

    if (cached_var_post)
        g_variant_unref (cached_var_post);
    g_variant_unref (var_post);
}


static void
ibus_input_context_fwd_text_to_commit (IBusInputContext *context,
                                       IBusText         *text)
{
    g_signal_emit (context, context_signals[COMMIT_TEXT], 0, text);
}


static void
ibus_input_context_fwd_text_to_forward_key_event (IBusInputContext *context,
                                                  IBusText         *text)
{
    gchar **array = NULL;
    guint keyval, keycode, state;
    array = g_strsplit (text->text, ",", -1);
    keyval = g_ascii_strtoull (array[0], NULL, 10);
    keycode = g_ascii_strtoull (array[1], NULL, 10);
    state = g_ascii_strtoull (array[2], NULL, 10);
    g_strfreev (array);
    g_signal_emit (context,
                   context_signals[FORWARD_KEY_EVENT],
                   0,
                   keyval,
                   keycode,
                   state | IBUS_FORWARD_MASK);
}


static void
ibus_input_context_fwd_text_to_require_surrounding (IBusInputContext *context,
                                                    IBusText         *text)
{
    IBusInputContextPrivate *priv;
    g_assert (IBUS_IS_INPUT_CONTEXT (context));
    priv = IBUS_INPUT_CONTEXT_GET_PRIVATE (IBUS_INPUT_CONTEXT (context));
    priv->needs_surrounding_text = TRUE;
    g_signal_emit (context, context_signals[REQUIRE_SURROUNDING_TEXT], 0);
}


static void
ibus_input_context_fwd_text_to_delete_surrounding (IBusInputContext *context,
                                                   IBusText         *text)
{
    gchar **array = NULL;
    gint offset_from_cursor;
    guint nchars;
    array = g_strsplit (text->text, ",", -1);
    offset_from_cursor = g_ascii_strtoll (array[0], NULL, 10);
    nchars = g_ascii_strtoull (array[1], NULL, 10);
    g_strfreev (array);
    g_signal_emit (context,
                   context_signals[DELETE_SURROUNDING_TEXT],
                   0,
                   offset_from_cursor,
                   nchars);
}


static void
ibus_input_context_fwd_text_to_hide_preedit (IBusInputContext *context,
                                             IBusText         *text)
{
    g_signal_emit (context, context_signals[HIDE_PREEDIT_TEXT], 0, text);
}


static void
ibus_input_context_fwd_text_to_show_preedit (IBusInputContext *context,
                                             IBusText         *text)
{
    g_signal_emit (context, context_signals[SHOW_PREEDIT_TEXT], 0, text);
}


static void
ibus_input_context_fwd_text_to_update_preedit (IBusInputContext *context,
                                               IBusText         *text,
                                               IBusText         *position,
                                               char              type)
{
    gchar **array = NULL;
    guint32 cursor_pos;
    gboolean visible;
    guint mode = 0;

    array = g_strsplit (position->text, ",", -1);
    cursor_pos = g_ascii_strtoull (array[0], NULL, 10);
    visible = g_ascii_strtoull (array[1], NULL, 10) ? TRUE : FALSE;
    if (type == 'u') {
                g_signal_emit (context,
                               context_signals[UPDATE_PREEDIT_TEXT],
                               0,
                               text,
                               cursor_pos,
                               visible);
    } else {
                mode = g_ascii_strtoull (array[2], NULL, 10);
                g_signal_emit (context,
                               context_signals[UPDATE_PREEDIT_TEXT_WITH_MODE],
                               0,
                               text,
                               cursor_pos,
                               visible,
                               mode);
    }
    g_strfreev (array);
}


void
ibus_input_context_post_process_key_event (IBusInputContext *context)
{
    GVariant *cached_var_post;
    gboolean enable = FALSE;
    GVariant *result;
    GError *error = NULL;
    GVariant *variant = NULL;
    GVariantIter iter;
    gsize size;
    char type = 0;
    GVariant *vtext = NULL;

    g_assert (IBUS_IS_INPUT_CONTEXT (context));

    cached_var_post =
        g_dbus_proxy_get_cached_property ((GDBusProxy *)context,
                                          "EffectivePostProcessKeyEvent");
    if (cached_var_post)
        g_variant_get (cached_var_post, "(b)", &enable);
    if (!enable) {
        g_warning ("%s: ibus_input_context_set_post_process_key_event() "
                   "needs to be called before.",
                   G_STRFUNC);
        if (cached_var_post)
            g_variant_unref (cached_var_post);
        return;
    }
    g_variant_unref (cached_var_post);
    result = g_dbus_proxy_call_sync (
            (GDBusProxy *)context,
            "org.freedesktop.DBus.Properties.Get",
            g_variant_new ("(ss)",
                           IBUS_INTERFACE_INPUT_CONTEXT,
                           "PostProcessKeyEvent"),
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            NULL,
            &error);
    if (error) {
        g_warning ("%s: %s", G_STRFUNC, error->message);
        g_error_free (error);
        return;
    }

    g_variant_get (result, "(v)", &variant);
    g_assert (variant);
    g_variant_iter_init (&iter, variant);
    size = g_variant_iter_n_children (&iter);
    while (size > 0 && g_variant_iter_loop (&iter, "(yv)", &type, &vtext)) {
        IBusText *text =
                (IBusText *)ibus_serializable_deserialize_object (vtext);
        if (!IBUS_IS_TEXT (text)) {
            g_warning ("%s: %s", G_STRFUNC, "text is not IBusText");
            break;
        }
        switch (type) {
        case 'c':
            ibus_input_context_fwd_text_to_commit (context, text);
            break;
        case 'd':
            ibus_input_context_fwd_text_to_delete_surrounding (context, text);
            break;
        case 'f':
            ibus_input_context_fwd_text_to_forward_key_event (context, text);
            break;
        case 'h':
            ibus_input_context_fwd_text_to_hide_preedit (context, text);
            break;
        case 'r':
            ibus_input_context_fwd_text_to_require_surrounding (context, text);
            break;
        case 's':
            ibus_input_context_fwd_text_to_show_preedit (context, text);
            break;
        case 'u':
        case 'm': {
            IBusText *position;
            g_clear_pointer (&vtext, g_variant_unref);
            if (!g_variant_iter_loop (&iter, "(yv)", &type, &vtext)) {
                g_warning ("%s: %s", G_STRFUNC,
                           "Type 'u' requires next type 'u'");
                break;
            }
            if (type != 'u' && type != 'm') {
                g_warning ("%s: %s", G_STRFUNC,
                           "The next of type 'u' should be type 'u'");
                break;
            }
            position =
                    (IBusText *)ibus_serializable_deserialize_object (vtext);
            if (!IBUS_IS_TEXT (position)) {
                g_warning ("%s: %s", G_STRFUNC, "text is not IBusText");
                break;
            }
            ibus_input_context_fwd_text_to_update_preedit (context,
                                                           text,
                                                           position,
                                                           type);
            if (g_object_is_floating (position)) {
                g_object_ref_sink (position);
                g_object_unref (position);
            }
            break;
        }
        default:
            g_warning ("%s: Type '%c' is not supported.", G_STRFUNC, type);
        }
        if (g_object_is_floating (text)) {
            g_object_ref_sink (text);
            g_object_unref (text);
        }
        g_clear_pointer (&vtext, g_variant_unref);
    }

    g_variant_unref (variant);
    g_variant_unref (result);
}

#define DEFINE_FUNC(name, Name)                                         \
    void                                                                \
    ibus_input_context_##name (IBusInputContext *context)               \
    {                                                                   \
        g_assert (IBUS_IS_INPUT_CONTEXT (context));                     \
        g_dbus_proxy_call ((GDBusProxy *) context,                      \
                           #Name,                   /* method_name */   \
                           NULL,                    /* parameters */    \
                           G_DBUS_CALL_FLAGS_NONE,  /* flags */         \
                           -1,                      /* timeout */       \
                           NULL,                    /* cancellable */   \
                           NULL,                    /* callback */      \
                           NULL                     /* user_data */     \
                           );                                           \
    }

DEFINE_FUNC(focus_in, FocusIn);
DEFINE_FUNC(focus_out, FocusOut);
DEFINE_FUNC(reset, Reset);
DEFINE_FUNC(page_up, PageUp);
DEFINE_FUNC(page_down, PageDown);
DEFINE_FUNC(cursor_up, CursorUp);
DEFINE_FUNC(cursor_down, CursorDown);
#undef DEFINE_FUNC
