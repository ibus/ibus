/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus - The Input Bus
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

#ifndef __IBUS_DEBUG_H_
#define __IBUS_DEBUG_H_

/**
 * SECTION: ibusdebug
 * @short_description: Debug message output.
 * @stability: Stable
 *
 * This section lists functions that generate debug and warning messages.
 */

/**
 * ibus_warning:
 * @msg: A printf formatted message to be print.
 * @...: Necessary arguments for @msg.
 *
 * A convenient wrapper for g_warning.
 * The output format will be
 * <programlisting>
 *     source_file:line, message...
 * </programlisting>
 */
#define ibus_warning(msg, args...) \
    g_warning("%s:%d, " msg, __FILE__, __LINE__, ##args)

#endif

