# vim:set et sts=4 sw=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2010 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2007-2010 Red Hat, Inc.
#
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, see <https://www.gnu.org/licenses/>.

SHIFT_MASK = 1 << 0
LOCK_MASK = 1 << 1
CONTROL_MASK = 1 << 2
ALT_MASK = 1 << 3
MOD1_MASK = 1 << 3
MOD2_MASK = 1 << 4
MOD3_MASK = 1 << 5
MOD4_MASK = 1 << 6
MOD5_MASK = 1 << 7
BUTTON1_MASK = 1 << 8
BUTTON2_MASK = 1 << 9
BUTTON3_MASK = 1 << 10
BUTTON4_MASK = 1 << 11
BUTTON5_MASK = 1 << 12

HANDLED_MASK = 1 << 24
IGNORED_MASK = 1 << 25
FORWARD_MASK = 1 << 25

SUPER_MASK = 1 << 26
HYPER_MASK = 1 << 27
META_MASK = 1 << 28

RELEASE_MASK = 1 << 30

MODIFIER_MASK = 0x5c001fff

MODIFIER_NAME_TABLE = (
    ("Shift", SHIFT_MASK),
    ("CapsLock", LOCK_MASK),
    ("Ctrl", CONTROL_MASK),
    ("Alt", MOD1_MASK),
    ("SUPER", SUPER_MASK),
    ("Hyper", HYPER_MASK),
    ("Meta", META_MASK),
    ("Release", RELEASE_MASK),
)
