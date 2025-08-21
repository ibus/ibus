/* vim:set et sts=4 sw=4:
 *
 * ibus - The Input Bus
 *
 * Copyright(c) 2011-2014 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright(c) 2018 Takao Fujiwara <takao.fujiwara1@gmail.com>
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

class ThemedRGBA {
    public Gdk.RGBA? normal_fg { get; set; }
    public Gdk.RGBA? normal_bg { get; set; }
    public Gdk.RGBA? selected_fg { get; set; }
    public Gdk.RGBA? selected_bg { get; set; }

    private Gtk.StyleContext m_style_context;
    private Gtk.Settings m_settings;

    public ThemedRGBA(Gtk.StyleContext style_context) {
        reset_rgba();

        /* Use the color of Gtk.TextView instead of Gtk.Label
         * because the selected label "color" is not configured
         * in "Adwaita" theme and the selected label "background-color"
         * is not configured in "Maia" theme.
         */
        m_style_context = style_context;

        get_rgba();

        m_settings = Gtk.Settings.get_default();
        // m_style_context.changed signal won't be called for the isolated
        // Gtk.TextView widget.
        m_settings.notify["gtk-theme-name"].connect(() => { get_rgba(); });
    }

    ~ThemedRGBA() {
        reset_rgba();
        m_settings = null;
    }

    private void reset_rgba() {
        this.normal_fg = null;
        this.normal_bg = null;
        this.selected_fg = null;
        this.selected_bg = null;
    }

    private void get_rgba() {
        reset_rgba();
        Gdk.RGBA color = { 0, };

        /* "Adwaita", "Maia-gtk" themes define "theme_fg_color".
         * "Xfce-stellar" theme does not define "theme_fg_color".
         * "Xfce-base" theme does not define any theme colors.
         * https://github.com/ibus/ibus/issues/1871
         */
        if (m_style_context.lookup_color("theme_fg_color", out color)) {
            ;
        } else if (m_style_context.lookup_color("fg_normal", out color)) {
            ;
        } else {
            color.parse("#2e3436");
        }
        this.normal_fg = color.copy();

        if (m_style_context.lookup_color("theme_selected_fg_color",
                                         out color)) {
            ;
        } else if (m_style_context.lookup_color("fg_selected",
                                                out color)) {
            ;
        } else {
            color.parse("#ffffff");
        }
        this.selected_fg = color.copy();

        if (m_style_context.lookup_color("theme_bg_color", out color)) {
            ;
        } else if (m_style_context.lookup_color("base_normal", out color)) {
            ;
        } else {
            color.parse("#f6f5f4");
        }
        this.normal_bg = color.copy();

        if (m_style_context.lookup_color("theme_selected_bg_color",
                                         out color)) {
            ;
        } else if (m_style_context.lookup_color("base_selected", out color)) {
            ;
        } else {
            color.parse("#3584e4");
        }
        this.selected_bg = color.copy();
    }
}

class IconWidget: Gtk.Image {
    /**
     * IconWidget:
     * @icon_name_or_path: Can be a name or path but not stock id
     *     because gtk_icon_theme_load_icon() cannot fallback the
     *     stock id to a real file name against
     *     gtk_image_new_from_stock().
     * @size: #Gtk.IconSize
     */
    public IconWidget(string icon_name_or_path, Gtk.IconSize size) {
        Gdk.Pixbuf pixbuf = null;
        int fixed_width, fixed_height;
        Gtk.icon_size_lookup(size, out fixed_width, out fixed_height);

        try {
            if (icon_name_or_path[0] == '/') {
                pixbuf = new Gdk.Pixbuf.from_file(icon_name_or_path);
            } else {
                var theme = Gtk.IconTheme.get_default();
                pixbuf = theme.load_icon(icon_name_or_path, fixed_width, 0);
            }
        } catch (GLib.Error e) {
            try {
                var theme = Gtk.IconTheme.get_default();
                pixbuf = theme.load_icon("ibus-engine", fixed_width, 0);
            } catch (GLib.Error e) {
                set_from_icon_name("image-missing", size);
                return;
            }
        }

        if (pixbuf == null)
            return;
        float width = (float)pixbuf.get_width();
        float height = (float)pixbuf.get_height();
        float scale = fixed_width / (width > height ? width : height);
        width *= scale;
        height *= scale;

        pixbuf = pixbuf.scale_simple((int)width, (int)height, Gdk.InterpType.BILINEAR);
        set_from_pixbuf(pixbuf);
        show();
    }
}
