# vim:set et sts=4 sw=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2010 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2007-2010 Red Hat, Inc.
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

__all__ = (
        "ConfigBase",
        "IBUS_SERVICE_CONFIG",
        "IBUS_PATH_CONFIG"
    )

IBUS_SERVICE_CONFIG = "org.freedesktop.IBus.Config"
IBUS_PATH_CONFIG = "/org/freedesktop/IBus/Config"

from gi.repository import GObject as gobject
import object
import interface
import dbus
from dbus.proxies import ProxyObject

class ConfigBase(object.Object):
    def __init__(self, bus):
        super(ConfigBase, self).__init__()
        self.__proxy = ConfigProxy(self, bus.get_dbusconn())

    def get_value(self, section, name):
        pass

    def get_values(self, section):
        pass

    def set_value(self, section, name, value):
        pass

    def unset_value(self, section, name):
        pass

    def value_changed(self, section, name, value):
        self.__proxy.ValueChanged(section, name, value)


class ConfigProxy(interface.IConfig):
    def __init__ (self, config, dbusconn):
        super(ConfigProxy, self).__init__(dbusconn, IBUS_PATH_CONFIG)
        self.__dbusconn = dbusconn
        self.__config = config

    def GetValue(self, section, name):
        return self.__config.get_value(section, name)

    def GetValues(self, section):
        return self.__config.get_values(section)

    def SetValue(self, section, name, value):
        return self.__config.set_value(section, name, value)

    def UnsetValue(self, section, name):
        return self.__config.unset_value(section, name)

    def Destroy(self):
        self.__config.destroy()

class Config(object.Object):
    __gtype_name__ = "PYIBusConfig"
    __gsignals__ = {
        "reloaded" : (
            gobject.SIGNAL_RUN_LAST,
            gobject.TYPE_NONE,
            ()
        ),
        "value-changed" : (
            gobject.SIGNAL_RUN_LAST,
            gobject.TYPE_NONE,
            (gobject.TYPE_STRING, gobject.TYPE_STRING, gobject.TYPE_PYOBJECT)
        ),
    }

    def __init__(self, bus):
        super(Config, self).__init__()
        self.__bus = bus
        self.__bus_name = None

        self.__bus.add_match("type='signal',\
                              sender='org.freedesktop.DBus',\
                              member='NameOwnerChanged',\
                              arg0='org.freedesktop.IBus.Config'")
        self.__bus.get_dbusconn().add_signal_receiver(self.__name_owner_changed_cb, signal_name="NameOwnerChanged")

        try:
            self.__init_config()
        except:
            self.__config = None

    def __name_owner_changed_cb(self, name, old_name, new_name):
        if name == "org.freedesktop.IBus.Config":
            if new_name == "":
                self.__config = None
            else:
                self.__init_config(new_name)

    def __init_config(self, bus_name=None):
        if bus_name == None:
            bus_name = self.__bus.get_name_owner(IBUS_SERVICE_CONFIG)

        match_rule = "type='signal',\
                      sender='%s',\
                      member='ValueChanged',\
                      path='/org/freedesktop/IBus/Config'"

        if self.__bus_name:
            self.__bus.remove_match(match_rule % self.__bus_name)
            self.__bus_name = None

        _config = self.__bus.get_dbusconn().get_object(bus_name, IBUS_PATH_CONFIG)
        self.__config = dbus.Interface(_config, dbus_interface="org.freedesktop.IBus.Config")
        self.__config.connect_to_signal("ValueChanged", self.__value_changed_cb)

        self.__bus_name = bus_name
        self.__bus.add_match(match_rule % self.__bus_name)
        self.emit("reloaded")

    def __value_changed_cb(self, section, name, value):
        self.emit("value-changed", section, name, value)

    def get_value(self, section, name, default_value):
        try:
            return self.__config.GetValue(section, name)
        except:
            return default_value

    def get_values(self, section):
        try:
            return self.__config.GetValues(section)
        except:
            return None

    def set_value(self, section, name, value):
        try:
            return self.__config.SetValue(section, name, value)
        except:
            return

    def set_list(self, section, name, value, signature):
        return self.set_value(section, name, dbus.Array(value, signature=signature))

    def unset_value(self, section, name):
        try:
            return self.__config.UnsetValue(section, name)
        except:
            return
