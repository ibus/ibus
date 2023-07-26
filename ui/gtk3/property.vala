/* vim:set et sts=4 sw=4:
 *
 * ibus - The Input Bus
 *
 * Copyright(c) 2011-2015 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright(c) 2023 Takao Fujiwara <takao.fujiwara1@gmail.com>
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

public class PropertyManager {
    private IBus.PropList m_props;

    public void ProperyManager() {
    }

    public void set_properties(IBus.PropList props) {
        m_props = props;
    }

    public int create_menu_items(Gtk.Menu menu) {
        if (m_props == null)
            return 0;
        return create_menu_items_internal(m_props, menu);
    }

    private int create_menu_items_internal(IBus.PropList props, Gtk.Menu menu) {
        int i = 0;
        PropRadioMenuItem last_radio = null;
        while (true) {
            IBus.Property prop = props.get(i);
            if (prop == null)
                break;
            debug("ins prop = %s", prop.get_key());

            i++;
            IPropItem item = null;
            switch(prop.get_prop_type()) {
                case IBus.PropType.NORMAL:
                    item = new PropImageMenuItem(prop);
                    break;
                case IBus.PropType.TOGGLE:
                    item = new PropCheckMenuItem(prop);
                    break;
                case IBus.PropType.RADIO:
                    {
                        PropRadioMenuItem radio =
                            new PropRadioMenuItem(prop, last_radio);
                        item = radio;
                        last_radio = radio;
                    }
                    break;
                case IBus.PropType.MENU:
                    {
                        var menuitem = new PropImageMenuItem(prop);
                        item = menuitem;
                        var  submenu = new Gtk.Menu();
                        if(create_menu_items_internal(prop.get_sub_props(), submenu) > 0)
                            menuitem.set_submenu(submenu);
                    }
                    break;
                case IBus.PropType.SEPARATOR:
                    item = new PropSeparatorMenuItem(prop);
                    break;
                default:
                    warning("unknown property type %d", (int)prop.get_prop_type());
                    break;
            }
            if (prop.get_prop_type() != IBus.PropType.RADIO)
                last_radio = null;
            if (item != null) {
                menu.append(item as Gtk.MenuItem);
                item.property_activate.connect((k, s) => property_activate(k, s));
            }
        }
        return i;
    }

    public void update_property(IBus.Property prop) {
        assert(prop != null);
        if (m_props != null)
            m_props.update_property(prop);
    }

    public signal void property_activate(string key, int state);
}

public interface IPropItem : GLib.Object {
    public abstract void update_property(IBus.Property prop);
    public signal void property_activate(string key, int state);
}

public class PropImageMenuItem : Gtk.MenuItem, IPropItem {
    private IBus.Property m_property;
    public PropImageMenuItem(IBus.Property property) {
        assert(property != null);

        m_property = property;
        set_no_show_all(true);
        sync();
    }

    public void update_property(IBus.Property property) {
        if (m_property.get_key() != property.get_key())
            return;

        m_property.set_label(property.get_label());
        m_property.set_visible(property.get_visible());
        m_property.set_sensitive(property.get_sensitive());
        m_property.set_tooltip(property.get_tooltip());
        m_property.set_state(property.get_state());
        sync();
    }

    private void sync() {
        set_label(m_property.get_label().get_text());
        set_visible(m_property.get_visible());
        set_sensitive(m_property.get_sensitive());
    }

    public override void activate() {
        property_activate(m_property.get_key(), m_property.get_state());
    }
}

public class PropCheckMenuItem : Gtk.CheckMenuItem, IPropItem {
    private IBus.Property m_property;
    public PropCheckMenuItem(IBus.Property property) {
        assert(property != null);

        m_property = property;
        set_no_show_all(true);
        sync();
    }

    public void update_property(IBus.Property property) {
        if (m_property.get_key() != property.get_key())
            return;

        m_property.set_label(property.get_label());
        m_property.set_icon(property.get_icon());
        m_property.set_visible(property.get_visible());
        m_property.set_sensitive(property.get_sensitive());
        m_property.set_tooltip(property.get_tooltip());
        m_property.set_state(property.get_state());
        sync();
    }

    private void sync() {
        set_label(m_property.get_label().get_text());
        set_visible(m_property.get_visible());
        set_sensitive(m_property.get_sensitive());
        set_active(m_property.get_state() == IBus.PropState.CHECKED);
    }

    public override void toggled() {
        IBus.PropState new_state =
            get_active() ? IBus.PropState.CHECKED : IBus.PropState.UNCHECKED;
        if (m_property.get_state() != new_state) {
            m_property.set_state(new_state);
            property_activate(m_property.get_key(), m_property.get_state());
        }
    }
}

public class PropRadioMenuItem : Gtk.RadioMenuItem, IPropItem {
    private IBus.Property m_property;
    public PropRadioMenuItem(IBus.Property property,
        PropRadioMenuItem ?group_source) {
        assert(property != null);

        m_property = property;
        set_no_show_all(true);

        if (group_source != null)
            set_group(group_source.get_group());

        /* Call sync() after call set_group() because
         * gtk_radio_menu_item_set_group() sets active = 0. */
        sync();
    }

    public void update_property(IBus.Property property) {
        if (m_property.get_key() != property.get_key())
            return;

        m_property.set_label(property.get_label());
        m_property.set_icon(property.get_icon());
        m_property.set_visible(property.get_visible());
        m_property.set_sensitive(property.get_sensitive());
        m_property.set_tooltip(property.get_tooltip());
        m_property.set_state(property.get_state());
        sync();
    }

    private void sync() {
        set_label(m_property.get_label().get_text());
        set_visible(m_property.get_visible());
        set_sensitive(m_property.get_sensitive());
        set_active(m_property.get_state() == IBus.PropState.CHECKED);
    }

    public override void toggled() {
        IBus.PropState new_state =
            get_active() ? IBus.PropState.CHECKED : IBus.PropState.UNCHECKED;
        if (m_property.get_state() != new_state) {
            m_property.set_state(new_state);
            property_activate(m_property.get_key(), m_property.get_state());
        }
    }
}

public class PropSeparatorMenuItem : Gtk.SeparatorMenuItem, IPropItem {
    private IBus.Property m_property;
    public PropSeparatorMenuItem(IBus.Property property) {
        assert(property != null);
        m_property = property;
    }

    public void update_property(IBus.Property property) {
    }
}
