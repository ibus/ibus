# vim:set et sts=4 sw=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2012 Daiki Ueno <ueno@unixuser.org>
# Copyright (c) 2014-2016 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2018-2026 Takao Fujiwara <takao.fujiwara1@gmail.com>
# Copyright (c) 2012-2018 Red Hat, Inc.
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

NULL =

overridesdir = $(pyoverridesdir)
overrides_PYTHON =                              \
    gi/overrides/IBus.py                        \
    $(NULL)

TESTS =

if ENABLE_TESTS
TESTS += test-deserialize.py test-override-ibus.py
endif

# IBus.Keymap() accesses keymap files
TESTS_ENVIRONMENT = \
    top_srcdir=$(top_srcdir) \
    PYTHONPATH=$(abs_top_srcdir)/tests:$${PYTHONPATH:+:$$PYTHONPATH} \
    PYTHONMALLOC=malloc \
    LD_LIBRARY_PATH=$(abs_top_builddir)/src/.libs:$$LD_LIBRARY_PATH \
    GI_TYPELIB_PATH=$(abs_top_builddir)/src:$$GI_TYPELIB_PATH \
    IBUS_KEYMAP_PATH=$(abs_top_srcdir)/data/keymaps \
    $(NULL)

LOG_COMPILER = $(top_srcdir)/src/tests/runtest $(PYTHON)

EXTRA_DIST =                                    \
    gi/__init__.py                              \
    gi/overrides/__init__.py                    \
    test-deserialize.py                         \
    test-override-ibus.py                       \
    $(NULL)

install-data-hook:
	@$(NORMAL_INSTALL)
	$(NULL)
uninstall-hook:
	@$(NORMAL_UNINSTALL)
	$(NULL)

-include $(top_srcdir)/git.mk
