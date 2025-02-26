# vim:set et sts=4 sw=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2012 Daiki Ueno <ueno@unixuser.org>
# Copyright (c) 2011 Peng Huang <shawn.p.huang@gmail.com>
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

from gi.repository import GObject

from ..overrides import override

# for newer pygobject: https://bugzilla.gnome.org/show_bug.cgi?id=686828
# from ..module import get_introspection_module
# IBus = get_introspection_module('IBus')
from ..importer import modules
IBus = modules['IBus']._introspection_module

__all__ = []

class Attribute(IBus.Attribute):
    def __new__(cls, type=0, value=0, start_index=0, end_index=0):
        return IBus.Attribute.new(type, value, start_index, end_index)

Attribute = override(Attribute)
__all__.append('Attribute')

class Component(IBus.Component):
    # Backward compatibility: allow non-keyword arguments
    def __init__(self,
                 name='',
                 description='',
                 version='',
                 license='',
                 author='',
                 homepage='',
                 command_line='',
                 textdomain='',
                 **kwargs):
        super(Component, self).__init__(name=name,
                                        description=description,
                                        version=version,
                                        license=license,
                                        author=author,
                                        homepage=homepage,
                                        command_line=command_line,
                                        textdomain=textdomain,
                                        **kwargs)

    # Backward compatibility: allow keyword arguments
    def add_engine(self, engine=None, **kwargs):
        if engine is None:
            engine = EngineDesc(**kwargs)
        super(Component, self).add_engine(engine)

Component = override(Component)
__all__.append('Component')

class Config(IBus.Config):
    # Backward compatibility: accept default arg
    def get_value(self, section, name, default=None):
        value = super(Config, self).get_value(section, name)
        if value is None:
            return default
        return value

    # Backward compatibility: unset value if value is None
    # Note that we don't call GLib.Variant.unpack here
    def set_value(self, section, name, value):
        if value is None:
            self.unset(section, name)
        else:
            super(Config, self).set_value(section, name, value)

Config = override(Config)
__all__.append('Config')

class EngineDesc(IBus.EngineDesc):
    # Backward compatibility: allow non-keyword arguments
    def __init__(self,
                 name='',
                 longname='',
                 description='',
                 language='',
                 license='',
                 author='',
                 icon='',
                 layout='us',
                 hotkeys='',
                 rank=0,
                 symbol='',
                 setup='',
                 layout_variant='',
                 layout_option='',
                 version='',
                 textdomain='',
                 **kwargs):
        super(EngineDesc, self).__init__(name=name,
                                         longname=longname,
                                         description=description,
                                         language=language,
                                         license=license,
                                         author=author,
                                         icon=icon,
                                         layout=layout,
                                         hotkeys=hotkeys,
                                         rank=rank,
                                         symbol=symbol,
                                         setup=setup,
                                         layout_variant=layout_variant,
                                         layout_option=layout_option,
                                         version=version,
                                         textdomain=textdomain,
                                         **kwargs)

EngineDesc = override(EngineDesc)
__all__.append('EngineDesc')

class Factory(IBus.Factory):
    # Backward compatibility: allow non-keyword arguments
    def __init__(self, bus=None, **kwargs):
        if bus is not None:
            kwargs.setdefault('connection', bus.get_connection())
            kwargs.setdefault('object_path', IBus.PATH_FACTORY)
        super(Factory, self).__init__(**kwargs)

Factory = override(Factory)
__all__.append('Factory')

class Keymap(IBus.Keymap):
    # Backward compatibility: allow non-keyword arguments
    def __new__(cls, name):
        return IBus.Keymap.new(name)

    def __init__(*args, **kwargs):
        pass

Keymap = override(Keymap)
__all__.append('Keymap')

class LookupTable(IBus.LookupTable):
    # Backward compatibility: allow non-keyword arguments
    def __new__(cls,
                page_size=5,
                cursor_pos=0,
                cursor_visible=True,
                round=False,
                orientation=IBus.Orientation.SYSTEM,
                candidates=[],
                labels=[]):
        table = IBus.LookupTable.new(page_size,
                                     cursor_pos,
                                     cursor_visible,
                                     round)
        table.set_orientation(orientation)
        for candidate in candidates:
            table.append_candidate(candidate)
        for index, label in enumerate(labels):
            table.set_label(index, label)
        return table

    def __init__(self, *args, **kwargs):
        pass

    # Backward compatibility: rename
    def show_cursor(self, visible):
        self.set_cursor_visible(visible)

    # Backward compatibility: rename
    def clean(self):
        self.clear()

LookupTable = override(LookupTable)
__all__.append('LookupTable')

class Property(IBus.Property):
    # Backward compatibility: allow non-keyword arguments
    def __init__(self,
                 key='',
                 type=IBus.PropType.NORMAL,
                 label='',
                 icon='',
                 tooltip='',
                 sensitive=True,
                 visible=True,
                 state=IBus.PropState.UNCHECKED,
                 symbol='',
                 **kwargs):
        prop_type = kwargs.pop('prop_type', type)
        if label != None and not isinstance(label, IBus.Text):
            label = Text(label)
        if tooltip != None and not isinstance(tooltip, IBus.Text):
            tooltip = Text(tooltip)
        if symbol != None and not isinstance(symbol, IBus.Text):
            symbol = Text(symbol)
        super(Property, self).__init__(key=key,
                                       prop_type=prop_type,
                                       label=label,
                                       icon=icon,
                                       tooltip=tooltip,
                                       sensitive=sensitive,
                                       visible=visible,
                                       state=state,
                                       symbol=symbol,
                                       **kwargs)

Property = override(Property)
__all__.append('Property')

class Text(IBus.Text):
    # Backward compatibility: allow non-keyword arguments
    def __new__(cls, string='', attrs=None):
        text = IBus.Text.new_from_string(string)
        if attrs is not None:
            text.set_attributes(attrs)
        return text

    def __init__(self, *args, **kwargs):
        pass

Text = override(Text)
__all__.append('Text')

IBus.init()
