/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* IBus - The Input Bus
 * Copyright (C) 2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2025 Red Hat, Inc.
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

#ifndef __IBUS_ATTRIBUTE_LIST_PRIVATE_H_
#define __IBUS_ATTRIBUTE_LIST_PRIVATE_H_

#define __IBUS_H_INSIDE__
#include "ibusattrlist.h"
#undef __IBUS_H_INSIDE__

/**
 * ibus_attr_list_copy_format_to_rgba:
 * @attr_list: An #IBusAttrList instance.
 * @error: An #GError pointer.
 *
 * Returns: (transfer none): A newly allocated #IBusAttrList which includes the
 * RGBA #IBusAttribute with the type of @IBUS_ATTR_TYPE_UNDERLINE,
 * @IBUS_ATTR_TYPE_FOREGROUND, @IBUS_ATTR_TYPE_BACKGROUND, or %NULL.
 * Workaround: returned value will be unref with ibus_text_set_attributes().
 *
 * Since: 1.5.33
 * Stability: Unstable
 */
IBusAttrList        *ibus_attr_list_copy_format_to_rgba
                                                (IBusAttrList *attr_list,
                                                 GError      **error);

/**
 * ibus_attr_list_copy_format_to_hint:
 * @attr_list: An #IBusAttrList instance.
 * @error: An #GError pointer.
 *
 * Returns: (transfer none): A newly allocated #IBusAttrList which includes the
 * hint #IBusAttribute with the type of @IBUS_ATTR_TYPE_HINT, or %NULL.
 * Workaround: returned value will be unref with ibus_text_set_attributes().
 *
 * Since: 1.5.33
 * Stability: Unstable
 */
IBusAttrList        *ibus_attr_list_copy_format_to_hint
                                                (IBusAttrList *attr_list,
                                                 GError      **error);
#endif
