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

__all__ = (
        "Object",
    )

import gobject

class Object(gobject.GObject):
    __gtype_name__ = "PYIBusObject"
    __gsignals__ = {
        'destroy' : (
            gobject.SIGNAL_RUN_LAST,
            gobject.TYPE_NONE,
            ())
    }

    def __init__(self):
        super(Object, self).__init__()
        self.__destroyed = False
        self.__handlers = []

    def destroy(self):
        if not self.__destroyed:
            self.emit("destroy")
            self.__destroyed = True

    def do_destroy(self):
        self.__disconnect_all()

    def connect(self, signal_name, handler, *args):
        id = super(Object, self).connect(signal_name, handler, *args)
        self.__handlers.append(id)

    def __disconnect_all(self):
        map(self.disconnect, self.__handlers)
        self.__handlers = []

