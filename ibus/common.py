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
        "IBUS_IFACE_IBUS",
        "IBUS_SERVICE_IBUS",
        "IBUS_PATH_IBUS",
        "IBUS_IFACE_CONFIG",
        "IBUS_IFACE_PANEL",
        "IBUS_IFACE_ENGINE",
        "IBUS_IFACE_ENGINE_FACTORY",
        "IBUS_IFACE_INPUT_CONTEXT",
        "IBUS_IFACE_NOTIFICATIONS",
        "ORIENTATION_HORIZONTAL",
        "ORIENTATION_VERTICAL",
        "ORIENTATION_SYSTEM",
        "BUS_NAME_FLAG_ALLOW_REPLACEMENT",
        "BUS_NAME_FLAG_REPLACE_EXISTING",
        "BUS_NAME_FLAG_DO_NOT_QUEUE",
        "BUS_REQUEST_NAME_REPLY_PRIMARY_OWNER",
        "BUS_REQUEST_NAME_REPLY_IN_QUEUE",
        "BUS_REQUEST_NAME_REPLY_EXISTS",
        "BUS_REQUEST_NAME_REPLY_ALREADY_OWNER",
        "default_reply_handler",
        "default_error_handler",
        "DEFAULT_ASYNC_HANDLERS",
        "CONFIG_GENERAL_SHORTCUT_TRIGGER_DEFAULT",
        "CONFIG_GENERAL_SHORTCUT_ENABLE_DEFAULT",
        "CONFIG_GENERAL_SHORTCUT_DISABLE_DEFAULT",
        "CONFIG_GENERAL_SHORTCUT_NEXT_ENGINE_DEFAULT",
        "CONFIG_GENERAL_SHORTCUT_PREV_ENGINE_DEFAULT",
        "main",
        "main_quit",
        "main_iteration",
        "get_address",
        "get_socket_path",
    )

import os
import sys
#from xdg import BaseDirectory
import ctypes
import _config

# __display = os.environ["DISPLAY"]
# __hostname, __display_screen = __display.split(":", 1)
# if not __hostname.strip():
#     __hostname = "unix"
# __display_number = __display_screen.split(".")[0]
# 
# __username = None
# try:
#     __username = os.getlogin()
# except:
#     pass
# if not __username:
#     __username = os.getenv ("LOGNAME")
# if not __username:
#     __username = os.getenv ("USER")
# if not __username:
#     __username = os.getenv ("LNAME")
# if not __username:
#     __username = os.getenv ("USERNAME")
# libibus = ctypes.CDLL("libibus.so")
# id = ctypes.c_char_p(libibus.ibus_get_local_machine_id()).value
# 
# IBUS_SOCKET_FILE = os.path.join(BaseDirectory.xdg_config_home,
#                                 "ibus", "bus",
#                                 "%s-%s-%s"% (id, __hostname, __display_number))
# def get_address():
#     libibus = ctypes.CDLL("libibus.so")
#     address = ctypes.c_char_p(libibus.ibus_get_address()).value
#     return address
# 
#     address = os.getenv("IBUS_ADDRESS")
#     if address:
#         return address
#     try:
#         for l in file(IBUS_SOCKET_FILE):
#             if not l.startswith("IBUS_ADDRESS="):
#                 continue
#             address = l[13:]
#             address = address.strip()
#             break
#     except:
#         return None
#     return address

libibus = ctypes.CDLL(_config.LIBIBUS_SONAME)
get_address = libibus.ibus_get_address
get_address.restype=ctypes.c_char_p

get_socket_path = libibus.ibus_get_socket_path
get_socket_path.restype=ctypes.c_char_p

# __session_id = os.getenv ("IBUS_SESSION_ID")
# 
# IBUS_ADDR = "unix:path=/tmp/ibus-%s%s/ibus-%s-%s" % (__username,
#                                                      "-" + __session_id if __session_id else "",
#                                                      __hostname,
#                                                      __display_number)
# IBUS_ADDR  = "tcp:host=localhost,port=7799"

IBUS_IFACE_IBUS     = "org.freedesktop.IBus"
IBUS_PATH_IBUS      = "/org/freedesktop/IBus"
IBUS_SERVICE_IBUS   = "org.freedesktop.IBus"

IBUS_IFACE_PANEL            = "org.freedesktop.IBus.Panel"
IBUS_IFACE_CONFIG           = "org.freedesktop.IBus.Config"
IBUS_IFACE_ENGINE           = "org.freedesktop.IBus.Engine"
IBUS_IFACE_ENGINE_FACTORY   = "org.freedesktop.IBus.Factory"
IBUS_IFACE_INPUT_CONTEXT    = "org.freedesktop.IBus.InputContext"
IBUS_IFACE_NOTIFICATIONS    = "org.freedesktop.IBus.Notifications"

# define pre-edit commit mode when the focus is lost
IBUS_ENGINE_PREEDIT_CLEAR       = 0
IBUS_ENGINE_PREEDIT_COMMIT      = 1

# define orientation
ORIENTATION_HORIZONTAL  = 0
ORIENTATION_VERTICAL    = 1
ORIENTATION_SYSTEM      = 2

# define bus name flag
BUS_NAME_FLAG_ALLOW_REPLACEMENT   = (1 << 0)
BUS_NAME_FLAG_REPLACE_EXISTING    = (1 << 1)
BUS_NAME_FLAG_DO_NOT_QUEUE        = (1 << 2)

# define bus request name reply
BUS_REQUEST_NAME_REPLY_PRIMARY_OWNER = 1
BUS_REQUEST_NAME_REPLY_IN_QUEUE      = 2
BUS_REQUEST_NAME_REPLY_EXISTS        = 3
BUS_REQUEST_NAME_REPLY_ALREADY_OWNER = 4

def default_reply_handler( *args):
    pass

def default_error_handler(e):
    print >> sys.stderr, e

DEFAULT_ASYNC_HANDLERS = {
    "reply_handler" : default_reply_handler,
    "error_handler" : default_error_handler
}

CONFIG_GENERAL_SHORTCUT_TRIGGER_DEFAULT = [
    "Control+space",
    "Zenkaku_Hankaku",
    "Hangul"]
CONFIG_GENERAL_SHORTCUT_ENABLE_DEFAULT = []
CONFIG_GENERAL_SHORTCUT_DISABLE_DEFAULT = []
CONFIG_GENERAL_SHORTCUT_NEXT_ENGINE_DEFAULT = []
CONFIG_GENERAL_SHORTCUT_PREV_ENGINE_DEFAULT = []

__mainloop = None

def __init_main_loop():
    global __mainloop
    if __mainloop == None:
        from gi.repository import GObject as gobject
        __mainloop = gobject.MainLoop()

def main():
    __init_main_loop()
    __mainloop.run()

def main_quit():
    global __mainloop
    if __mainloop:
        __mainloop.quit()
    
def main_iteration(may_block=False):
    __init_main_loop()
    return __mainloop.get_context().iteration(may_block)
