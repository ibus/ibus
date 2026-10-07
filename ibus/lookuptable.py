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
        "LookupTable",
    )

import dbus
from common import *
from serializable import *
from exception import *

class LookupTable(Serializable):
    __gtype_name__ = "PYIBusLookupTable"
    __NAME__ = "IBusLookupTable"
    def __init__(self, page_size=5, cursor_pos=0, coursor_visible=True, round=False,
        orientation=ORIENTATION_SYSTEM, candidates=None, labels=None):
        super(LookupTable, self).__init__()
        self.__cursor_pos = cursor_pos
        self.__cursor_visible = True
        self.__round = round
        self.__orientation = orientation
        if candidates == None:
            self.__candidates = list()
        else:
            self.__candidates = candidates
        self.set_page_size(page_size)
        self.set_labels(labels)

    def set_page_size(self, page_size):
        self.__page_size = page_size

    def get_page_size(self):
        return self.__page_size

    def get_current_page_size(self):
        nr_candidate = len(self.__candidates)
        nr_page, last_page_size = divmod(nr_candidate, self.__page_size)
        if self.__cursor_pos / self.__page_size == nr_page:
            return last_page_size
        else:
            return self.__page_size

    def set_labels(self, labels):
        if labels == None:
            self.__labels = list()
        else:
            self.__labels = labels

    def get_labels(self):
        return self.__labels

    def show_cursor(self, show=True):
        self.__cursor_visible = show

    def is_cursor_visible(self):
        return self.__cursor_visible

    def get_current_page_start(self):
        return (self.__cursor_pos / self.__page_size) * self.__page_size

    def set_cursor_pos(self, pos):
        if pos >= len(self.__candidates) or pos < 0:
            return False
        self.__cursor_pos = pos
        return True

    def get_cursor_pos(self):
        return self.__cursor_pos

    def get_cursor_pos_in_current_page(self):
        page, pos_in_page = divmod(self.__cursor_pos, self.__page_size)
        return pos_in_page

    def set_cursor_pos_in_current_page(self, pos):
        if pos < 0 or pos >= self.__page_size:
            return False
        pos += self.get_current_page_start()
        if pos >= len(self.__candidates):
            return False
        self.__cursor_pos = pos
        return True
    def set_orientation(self, orientation):
        self.__orientation = orientation

    def get_orientation(self):
        return self.__orientation

    def page_up(self):
        if self.__cursor_pos < self.__page_size:
            if self.__round:
                nr_candidates = len(self.__candidates)
                max_page = nr_candidates / self.__page_size
                self.__cursor_pos += max_page * self.__page_size
                if self.__cursor_pos > nr_candidates - 1:
                    self.__cursor_pos = nr_candidates - 1
                return True
            else:
                return False

        self.__cursor_pos -= self.__page_size
        return True

    def page_down(self):
        current_page = self.__cursor_pos / self.__page_size
        nr_candidates = len(self.__candidates)
        max_page = nr_candidates / self.__page_size

        if current_page >= max_page:
            if self.__round:
                self.__cursor_pos %= self.__page_size
                return True
            else:
                return False

        pos = self.__cursor_pos + self.__page_size
        if pos >= nr_candidates:
            pos = nr_candidates - 1
        self.__cursor_pos = pos

        return True

    def cursor_up(self):
        if self.__cursor_pos == 0:
            if self.__round:
                self.__cursor_pos = len(self.__candidates) - 1
                return True
            else:
                return False

        self.__cursor_pos -= 1
        return True

    def cursor_down(self):
        if self.__cursor_pos == len(self.__candidates) - 1:
            if self.__round:
                self.__cursor_pos = 0
                return True
            else:
                return False

        self.__cursor_pos += 1
        return True

    def clean(self):
        self.__candidates = list()
        self.__cursor_pos = 0

    def append_candidate(self, text):
        self.__candidates.append(text)

    def get_candidate(self, index):
        return self.__candidates[index]

    def append_label(self, text):
        self.__labels.append(text)

    def get_label(self, index):
        return self.__labels[index]

    def get_candidates_in_current_page(self):
        page = self.__cursor_pos / self.__page_size
        start_index = page * self.__page_size
        end_index = min((page + 1) * self.__page_size, len(self.__candidates))
        return self.__candidates[start_index:end_index]

    def get_current_candidate(self):
        return self.__candidates [self.__cursor_pos]

    def get_number_of_candidates(self):
        return len(self.__candidates)

    def __len__(self):
        return self.get_number_of_candidates()

    def serialize(self, struct):
        super(LookupTable, self).serialize(struct)
        struct.append(dbus.UInt32(self.__page_size))
        struct.append(dbus.UInt32(self.__cursor_pos))
        struct.append(dbus.Boolean(self.__cursor_visible))
        struct.append(dbus.Boolean(self.__round))
        struct.append(dbus.Int32(self.__orientation))
        candidates = map(lambda c: serialize_object(c), self.__candidates)
        struct.append(dbus.Array(candidates, signature="v"))
        labels = map(lambda c: serialize_object(c), self.__labels)
        struct.append(dbus.Array(labels, signature="v"))


    def get_current_page_as_lookup_table(self):
        candidates = self.get_candidates_in_current_page()
        return LookupTable(self.__page_size,
                           self.__cursor_pos % self.__page_size,
                           self.__cursor_visible,
                           self.__round,
                           self.__orientation,
                           candidates,
                           self.__labels)

    def deserialize(self, struct):
        super(LookupTable, self).deserialize(struct)

        self.__page_size = struct.pop(0)
        self.__cursor_pos = struct.pop(0)
        self.__cursor_visible = struct.pop(0)
        self.__round = struct.pop(0)
        self.__orientation = struct.pop(0)
        self.__candidates = map(deserialize_object, struct.pop(0))
        self.__labels = map(deserialize_object, struct.pop(0))

def test():
    t = LookupTable()
    # attrs = AttrList()
    # attrs.append(AttributeBackground(RGB(233, 0,1), 0, 3))
    # attrs.append(AttributeUnderline(1, 3, 5))
    t.append_candidate("Hello")
    value = serialize_object(t)
    t = deserialize_object(value)
    t = t.get_current_page_as_lookup_table()
    value = serialize_object(t)
    t = deserialize_object(value)

if __name__ == "__main__":
    test()
