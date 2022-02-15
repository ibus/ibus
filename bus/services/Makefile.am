# vim:set noet ts=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2022 Takao Fujiwara <takao.fujiwara1@gmail.com>
# Copyright (c) 2022 Red Hat, Inc.
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
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
# USA

NULL =

# Originally this D-Bus service file was added for GNOME but seems not to
# be used at present.
#
# TODO: Check if flatpack uses this file and we could delete
# org.freedesktop.IBus.service file in the future.
dbusservice_in_files = org.freedesktop.IBus.service.in
dbusservice_DATA = $(dbusservice_in_files:.service.in=.service)
dbusservicedir=${datadir}/dbus-1/services

systemdservice_in_files = \
    org.freedesktop.IBus.session.generic.service.in \
    org.freedesktop.IBus.session.GNOME.service.in \
    $(NULL)

%.service: %.service.in
	$(AM_V_GEN) sed -e "s|\@bindir\@|$(bindir)|" \
	                -e "s|\@xim_cli_arg\@|$(XIM_CLI_ARG)|" $< > $@.tmp && \
	            mv $@.tmp $@

if ENABLE_SYSTEMD
systemdservice_DATA = $(systemdservice_in_files:.service.in=.service)
systemdservicedir = $(SYSTEMD_USER_UNIT_DIR)

# in install-data-hook.
install-data-hook:
	$(MKDIR_P) "$(DESTDIR)$(SYSTEMD_USER_UNIT_DIR)/gnome-session.target.wants/"
	( \
		cd "$(DESTDIR)$(SYSTEMD_USER_UNIT_DIR)/gnome-session.target.wants" ; \
		$(LN_S) -f "../org.freedesktop.IBus.session.GNOME.service" .; \
	)
	$(NULL)
endif

EXTRA_DIST =                    \
    $(dbusservice_in_files)     \
    $(systemdservice_in_files)  \
    $(NULL)

CLEANFILES =                    \
    $(dbusservice_DATA)         \
    $(systemdservice_DATA)      \
    $(NULL)

-include $(top_srcdir)/git.mk
