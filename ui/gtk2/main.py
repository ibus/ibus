# vim:set et sts=4 sw=4:
#
# ibus - The Input Bus
#
# Copyright(c) 2007-2015 Peng Huang <shawn.p.huang@gmail.com>
# Copyright(c) 2007-2015 Red Hat, Inc.
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

# try Cython
# try:
#     import pyximport
#     pyximport.install(pyimport=True,build_in_temp=False)
# except:
#     pass

import os
import sys
import getopt
import ibus
import gtk
import gettext
import panel
import pynotify
from i18n import DOMAINNAME, _, N_

class UIApplication:
    def __init__ (self, replace):
        pynotify.init("ibus")
        self.__bus = ibus.Bus()
        self.__bus.connect("disconnected", gtk.main_quit)
        self.__bus.connect("registry-changed", self.__registry_changed_cb)

        match_rule = "type='signal',\
                      sender='org.freedesktop.IBus',\
                      path='/org/freedesktop/IBus'"
        self.__bus.add_match(match_rule)

        self.__panel = panel.Panel(self.__bus)
        flag = ibus.BUS_NAME_FLAG_ALLOW_REPLACEMENT
        if replace:
            flag = flag | ibus.BUS_NAME_FLAG_REPLACE_EXISTING
        self.__bus.request_name(ibus.IBUS_SERVICE_PANEL, flag)
        self.__bus.get_dbusconn().add_signal_receiver(self.__name_acquired_cb,
                                                      signal_name="NameAcquired")
        self.__bus.get_dbusconn().add_signal_receiver(self.__name_lost_cb,
                                                      signal_name="NameLost")
        self.__notify = pynotify.Notification("IBus", \
                            _("Some input methods have been installed, removed or updated. " \
                            "Please restart ibus input platform."), \
                            "ibus")
        self.__notify.set_timeout(10 * 1000)
        self.__notify.add_action("restart", _("Restart Now"), self.__restart_cb, None)
        self.__notify.add_action("ignore", _("Later"), lambda *args: None, None)

    def __restart_cb(self, notify, action, data):
        if action == "restart":
            self.__bus.exit(True)

    def __registry_changed_cb(self, bus):
        self.__notify.show()

    def __name_acquired_cb(self, name):
        self.__panel.show()

    def __name_lost_cb(self, name):
        self.__panel.hide()

    def run(self):
        try:
            gtk.main()
        except KeyboardInterrupt:
            pass

def launch_panel(replace):
    settings = gtk.settings_get_default()
    if settings.get_property('gtk-fallback-icon-theme') == None:
        settings.set_property('gtk-fallback-icon-theme', 'gnome')
    # gtk.settings_get_default().props.gtk_theme_name = "/home/phuang/.themes/aud-Default/gtk-2.0/gtkrc"
    # gtk.rc_parse("./themes/default/gtkrc")
    UIApplication(replace).run()

def print_help(out, v = 0):
    print >> out, "-h, --help             show this message."
    print >> out, "-d, --daemonize        daemonize ibus"
    print >> out, "-r, --replace          replace existing ibus UI"
    sys.exit(v)

def main():
    daemonize = False
    replace = False
    shortopt = "hdr"
    longopt = ["help", "daemonize", "replace"]
    try:
        opts, args = getopt.getopt(sys.argv[1:], shortopt, longopt)
    except getopt.GetoptError, err:
        print_help(sys.stderr, 1)

    for o, a in opts:
        if o in("-h", "--help"):
            print_help(sys.stdout)
        elif o in("-d", "--daemonize"):
            daemonize = True
        elif o in("-r", "--replace"):
            replace = True
        else:
            print >> sys.stderr, "Unknown argument: %s" % o
            print_help(sys.stderr, 1)

    if daemonize:
        if os.fork():
            sys.exit()

    launch_panel(replace)

if __name__ == "__main__":
    import i18n
    i18n.init_textdomain(DOMAINNAME)
    main()
