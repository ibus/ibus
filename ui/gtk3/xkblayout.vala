/* vim:set et sts=4 sw=4:
 *
 * ibus - The Input Bus
 *
 * Copyright(c) 2014 Red Hat, Inc.
 * Copyright(c) 2014 Peng Huang <shawn.p.huang@gmail.com>
 * Copyright(c) 2014-2025 Takao Fujiwara <tfujiwar@redhat.com>
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

class XKBLayout
{
    private const string XKB_COMMAND = "setxkbmap";
    private const string XKB_LAYOUT_ARG = "-layout";
    private const string XMODMAP_COMMAND = "xmodmap";
    private const string[] XMODMAP_KNOWN_FILES = {".xmodmap", ".xmodmaprc",
                                                  ".Xmodmap", ".Xmodmaprc"};
    private string[] m_get_layout_args = {};
    private string[] m_xkb_latin_layouts = {};
    private string m_default_layout = "";
    private string m_default_variant = "";
    private string m_default_option = "";
    private bool m_use_xmodmap = true;

    public XKBLayout() {
    }


    public void set_latin_layouts(string[] xkb_latin_layouts) {
        m_xkb_latin_layouts = xkb_latin_layouts;
    }


    public void get_layout(out string layout,
                           out string variant,
                           out string option) {
        layout = "";
        variant = "";
        var o = Environment.get_variable("XKB_DEFAULT_OPTIONS");
        option = o != null ? o : "";

        search_get_layout_program();
        if (m_get_layout_args[0] == null) {
            warning("Not found localectl or setxkbmap command in PATH");
            return;
        }
        string[] exec_command = {};
        foreach (unowned string arg in m_get_layout_args)
            exec_command += arg;
        string standard_output = null;
        string standard_error = null;
        int exit_status = 0;

        try {
            GLib.Process.spawn_sync(null,
                                    exec_command,
                                    null,
                                    GLib.SpawnFlags.SEARCH_PATH,
                                    null,
                                    out standard_output,
                                    out standard_error,
                                    out exit_status);
        } catch (GLib.SpawnError err) {
            stderr.printf("IBUS_ERROR: %s\n", err.message);
        }
        if (exit_status != 0)
            stderr.printf("IBUS_ERROR: %s\n", standard_error ?? "");
        if (standard_output == null)
            return;

        if (exec_command[0] == "localectl") {
            parse_localectl_status_str(standard_output,
                                       ref layout,
                                       ref variant,
                                       ref option);
        } else if (exec_command[0] == XKB_COMMAND) {
            parse_xkbmap_query_str(standard_output,
                                   ref layout,
                                   ref variant,
                                   ref option);
        }
    }


    private void search_get_layout_program() {
        if (m_get_layout_args[0] != null)
            return;
        string get_layout_command = null;
        // setxkbmap can get the session XKB options in Xorg.
        if (BindingCommon.default_is_xdisplay())
            get_layout_command = "setxkbmap -query";
        else if (GLib.Environment.find_program_in_path("localectl") != null)
            get_layout_command = "localectl status";
        else if (GLib.Environment.find_program_in_path("setxkbmap") != null)
            get_layout_command = "setxkbmap -query";
        if (get_layout_command != null)
            m_get_layout_args = get_layout_command.split(" ");
    }


    private void parse_localectl_status_str(string standard_output,
                                            ref string layout,
                                            ref string variant,
                                            ref string option) {
        foreach (string line in standard_output.split("\n")) {
            const string[] elements = { "X11 Layout:", "X11 Variant:" };
            foreach (unowned string element in elements) {
                string retval = "";
                int index = line.index_of(element);
                if (index >= 0) {
                    retval = line[index + element.length:line.length];
                    if (retval != null)
                        retval = retval.strip();
                    if (element == elements[0])
                        layout = retval;
                    else if (element == elements[1])
                        variant = retval;
                }
            }
        }
    }


    private void parse_xkbmap_query_str(string standard_output,
                                        ref string layout,
                                        ref string variant,
                                        ref string option) {
        foreach (string line in standard_output.split("\n")) {
            const string[] elements = { "layout:", "variant:", "options:" };
            foreach (unowned string element in elements) {
                string retval = "";
                if (line.has_prefix(element)) {
                    retval = line[element.length:line.length];
                    if (retval != null)
                        retval = retval.strip();
                    if (element == elements[0])
                        layout = retval;
                    else if (element == elements[1])
                        variant = retval;
                    else if (element == elements[2])
                        option = retval;
                }
            }
        }
    }


    public void set_layout(IBus.EngineDesc engine) {
        string layout = engine.get_layout();
        string variant = engine.get_layout_variant();
        string option = engine.get_layout_option();

        assert (layout != null);

        /* If the layout is "default", return here so that the current
         * keymap is not changed.
         * Some engines do not wish to change the current keymap.
         */
        if (layout == "default" &&
            (variant == "default" || variant == "") &&
            (option == "default" || option == "")) {
            return;
        }

        bool need_us_layout = false;
        if (variant != "eng")
            need_us_layout = layout in m_xkb_latin_layouts;
        if (!need_us_layout && variant != null)
            need_us_layout =
                    "%s(%s)".printf(layout, variant) in m_xkb_latin_layouts;

        if (m_default_layout == "") {
            get_layout (out m_default_layout,
                        out m_default_variant,
                        out m_default_option);
        }

        if (layout == "default" || layout == "") {
            layout = m_default_layout;
            variant = m_default_variant;
        }

        if (layout == "") {
            warning("Could not get the correct layout");
            return;
        }

        if (option == "default" || option == "") {
            option = m_default_option;
        } else {
            if (!(option in m_default_option.split(","))) {
                option = "%s,%s".printf(m_default_option, option);
            } else {
                option = m_default_option;
            }
        }

        if (need_us_layout) {
            layout += ",us";
            if (variant != null) {
                variant += ",";
            }
        }

        string[] args = {};
        args += XKB_COMMAND;
        args += XKB_LAYOUT_ARG;
        args += layout;
        if (variant != null && variant != "" && variant != "default") {
            args += "-variant";
            args += variant;
        }
        if (option != null && option != "" && option != "default") {
            /*TODO: Need to get the session XKB options */
            args += "-option";
            args += "-option";
            args += option;
        }

        string standard_error = null;
        int exit_status = 0;
        try {
            if (!GLib.Process.spawn_sync(null,
                                         args,
                                         null,
                                         GLib.SpawnFlags.SEARCH_PATH,
                                         null,
                                         null,
                                         out standard_error,
                                         out exit_status))
                warning("Switch xkb layout to %s failed.",
                        engine.get_layout());
        } catch (GLib.SpawnError e) {
            warning("Execute setxkbmap failed: %s", e.message);
            return;
        }

        if (exit_status != 0)
            warning("Execute setxkbmap failed: %s",
                    standard_error ?? "(null)");

        run_xmodmap();
    }


    public void run_xmodmap() {
        if (!m_use_xmodmap) {
            return;
        }

        string homedir = GLib.Environment.get_home_dir();
        foreach (string xmodmap_file in XMODMAP_KNOWN_FILES) {
            string xmodmap_filepath = GLib.Path.build_filename(homedir,
                                                               xmodmap_file);

            if (!GLib.FileUtils.test(xmodmap_filepath, GLib.FileTest.EXISTS)) {
                continue;
            }

            string[] args = {XMODMAP_COMMAND, xmodmap_filepath};

            /* Call async here because if both setxkbmap and xmodmap is
             * sync, it seems a DBus timeout happens and xmodmap causes
             * a loop for a while and users would think panel icon is
             * frozen in case the global engine mode is disabled.
             *
             * Do not return here even if the previous async is running
             * so that all xmodmap can be done after setxkbmap is called.
             */
            try {
                GLib.Process.spawn_async(null,
                                         args,
                                         null,
                                         GLib.SpawnFlags.SEARCH_PATH,
                                         null,
                                         null);
            } catch (GLib.SpawnError e) {
                warning("Execute xmodmap is failed: %s\n", e.message);
                return;
            }

            break;
        }
    }


    public void set_use_xmodmap(bool use_xmodmap) {
        m_use_xmodmap = use_xmodmap;
    }
}
