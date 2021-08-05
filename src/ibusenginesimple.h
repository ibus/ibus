/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2015 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright (C) 2015-2019 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2008-2017 Red Hat, Inc.
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

#if !defined (__IBUS_H_INSIDE__) && !defined (IBUS_COMPILATION)
#error "Only <ibus.h> can be included directly"
#endif

#ifndef __IBUS_ENGINE_SIMPLE_H__
#define __IBUS_ENGINE_SIMPLE_H__

/**
 * SECTION: ibusenginesimple
 * @short_description: Input method engine supporting table-based input method
 * @title: IBusEngineSimple
 * @stability: Stable
 *
 * An IBusEngineSimple provides table-based input method logic.
 *
 * see_also: #IBusEngine
 */

#include "ibusengine.h"
#include "ibuskeysyms.h"

G_BEGIN_DECLS

#define IBUS_MAX_COMPOSE_LEN 0xFF

/*
 * Type macros.
 */

/* define GOBJECT macros */
#define IBUS_TYPE_ENGINE_SIMPLE             \
    (ibus_engine_simple_get_type ())
#define IBUS_ENGINE_SIMPLE(obj)             \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), IBUS_TYPE_ENGINE_SIMPLE, IBusEngineSimple))
#define IBUS_ENGINE_SIMPLE_CLASS(klass)     \
    (G_TYPE_CHECK_CLASS_CAST ((klass), IBUS_TYPE_ENGINE_SIMPLE, IBusEngineSimpleClass))
#define IBUS_IS_ENGINE_SIMPLE(obj)          \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IBUS_TYPE_ENGINE_SIMPLE))
#define IBUS_IS_ENGINE_SIMPLE_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), IBUS_TYPE_ENGINE_SIMPLE))
#define IBUS_ENGINE_SIMPLE_GET_CLASS(obj)   \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), IBUS_TYPE_ENGINE_SIMPLE, IBusEngineSimpleClass))

typedef struct _IBusEngineSimple IBusEngineSimple;
typedef struct _IBusEngineSimpleClass IBusEngineSimpleClass;
typedef struct _IBusEngineSimplePrivate IBusEngineSimplePrivate;

/**
 * IBusEngineSimple:
 *
 * IBusEngineSimple properties.
 */
struct _IBusEngineSimple {
    /*< private >*/
    IBusEngine parent;
    IBusEngineSimplePrivate *priv;

    /* instance members */
    /*< public >*/
};

struct _IBusEngineSimpleClass {
    /*< private >*/
    IBusEngineClass parent;

    /* class members */
    /*< public >*/
    /* signals */
    
    /*< private >*/
    /* padding */
    gpointer pdummy[8];
};

static const guint16 IBUS_COMPOSE_IGNORE_KEYLIST[] = {
    IBUS_KEY_Shift_L,
    IBUS_KEY_Shift_R,
    IBUS_KEY_Control_L,
    IBUS_KEY_Control_R,
    IBUS_KEY_Caps_Lock,
    IBUS_KEY_Shift_Lock,
    IBUS_KEY_Meta_L,
    IBUS_KEY_Meta_R,
    IBUS_KEY_Alt_L,
    IBUS_KEY_Alt_R,
    IBUS_KEY_Super_L,
    IBUS_KEY_Super_R,
    IBUS_KEY_Hyper_L,
    IBUS_KEY_Hyper_R,
    IBUS_KEY_Mode_switch,
    IBUS_KEY_ISO_Level3_Shift
};

GType   ibus_engine_simple_get_type       (void);

/**
 * ibus_engine_simple_add_table:
 * @simple: An IBusEngineSimple.
 * @data: (element-type guint16) (array): The table which must be available
 *      during the whole life of the simple engine.
 * @max_seq_len: Maximum length of a sequence in the table (cannot be greater
 *      than %IBUS_MAX_COMPOSE_LEN)
 * @n_seqs: number of sequences in the table
 *
 * Adds an additional table to search to the engine. Each row of the table
 * consists of max_seq_len key symbols followed by two guint16 interpreted as
 * the high and low words of a gunicode value. Tables are searched starting from
 * the last added.
 *
 * The table must be sorted in dictionary order on the numeric value of the key
 * symbol fields. (Values beyond the length of the sequence should be zero.)
 */
void             ibus_engine_simple_add_table   (IBusEngineSimple  *simple,
                                                 const guint16     *data,
                                                 gint               max_seq_len,
                                                 gint               n_seqs);

/**
 * ibus_engine_simple_add_table_by_locale:
 * @simple: An IBusEngineSimple.
 * @locale: (allow-none): The locale name. If the locale is %NULL,
 *                        the current locale is used.
 *
 * Call ibus_engine_simple_add_table() internally by locale.
 *
 * Returns: %TRUE if the @locale is matched to the table.
 */
gboolean         ibus_engine_simple_add_table_by_locale
                                                (IBusEngineSimple  *simple,
                                                 const gchar       *locale);

/**
 * ibus_engine_simple_add_compose_file:
 * @simple: An IBusEngineSimple.
 * @file: The compose file.
 *
 * Call ibus_engine_simple_add_table() internally by locale.
 *
 * Returns: %TRUE if the @file is loaded.
 */
gboolean         ibus_engine_simple_add_compose_file
                                                (IBusEngineSimple  *simple,
                                                 const gchar       *file);
G_END_DECLS

#endif // __IBUS_ENGINE_SIMPLE_H__
