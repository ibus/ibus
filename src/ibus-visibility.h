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

#if (defined(_WIN32) || defined(__CYGWIN__)) && !defined(IBUS_STATIC_COMPILATION)
#  define _IBUS_EXPORT __declspec(dllexport)
#  define _IBUS_IMPORT __declspec(dllimport)
#elif __GNUC__ >= 4
#  define _IBUS_EXPORT __attribute__((visibility("default")))
#  define _IBUS_IMPORT
#else
#  define _IBUS_EXPORT
#  define _IBUS_IMPORT
#endif
#ifdef IBUS_COMPILATION
#  define _IBUS_API _IBUS_EXPORT
#else
#  define _IBUS_API _IBUS_IMPORT
#endif

#define _IBUS_EXTERN _IBUS_API extern

#define IBUS_ENCODE_VERSION(major, minor, micro) ((major) << 16 | \
                                                  (minor) << 8  | \
                                                  (micro))

#define IBUS_VERSION_CUR_STABLE (IBUS_ENCODE_VERSION (IBUS_MAJOR_VERSION, \
                                                      IBUS_MINOR_VERSION, \
                                                      IBUS_MICRO_VERSION))

#ifndef IBUS_VERSION_MIN_REQUIRED
#define IBUS_VERSION_MIN_REQUIRED (IBUS_VERSION_CUR_STABLE)
#elif IBUS_VERSION_MIN_REQUIRED == 0
#undef IBUS_VERSION_MIN_REQUIRED
#define IBUS_VERSION_MIN_REQUIRED (IBUS_VERSION_CUR_STABLE + 1)
#endif

#ifdef IBUS_DISABLE_DEPRECATION_WARNINGS
#define IBUS_DEPRECATED _IBUS_EXTERN
#define IBUS_DEPRECATED_FOR(f) _IBUS_EXTERN
#else
#define IBUS_DEPRECATED G_DEPRECATED _IBUS_EXTERN
#define IBUS_DEPRECATED_FOR(f) G_DEPRECATED_FOR(f) _IBUS_EXTERN
#endif


#define IBUS_VERSION_1_5_33     (IBUS_ENCODE_VERSION (1, 5, 33))

#if IBUS_VERSION_MIN_REQUIRED >= IBUS_VERSION_1_5_33
#define IBUS_DEPRECATED_IN_1_5_33 IBUS_DEPRECATED
#define IBUS_DEPRECATED_IN_1_5_33_FOR(f) IBUS_DEPRECATED_FOR (f)
#else
#define IBUS_DEPRECATED_IN_1_5_33 _IBUS_EXTERN
#define IBUS_DEPRECATED_IN_1_5_33_FOR(f) _IBUS_EXTERN
#endif
