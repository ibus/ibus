/* vim:set et sts=4 sw=4:
 *
 * ibus - The Input Bus
 *
 * Copyright (c) 2017 Peng Wu <alexepico@gmail.com>
 * Copyright (c) 2017-2021 Takao Fujiwara <takao.fujiwara1@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
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
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 */

string emoji_font = null;
string annotation_lang = null;
bool partial_match = false;
int partial_match_length = -1;
int partial_match_condition = -1;

public class EmojiApplication : Gtk.Application {
    private IBusEmojier? m_emojier;
    private GLib.Settings m_settings_emoji =
            new GLib.Settings("org.freedesktop.ibus.panel.emoji");
    private ApplicationCommandLine? m_command_line = null;


    private EmojiApplication() {
        Object(application_id: "org.freedesktop.IBus.Panel.Emojier",
               flags: ApplicationFlags.HANDLES_COMMAND_LINE);
        set_inactivity_timeout(100000);
    }


    private void save_selected_string(string? selected_string,
                                      bool    cancelled) {
        if (cancelled) {
            m_command_line.print("%s\n", _("Canceled to choose an emoji."));
            return;
        }
        GLib.return_if_fail(selected_string != null);
        Gtk.Clipboard clipboard = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD);
        clipboard.set_text(selected_string, -1);
        clipboard.store();

        var emojier_favorites = m_settings_emoji.get_strv("favorites");
        bool has_favorite = false;
        foreach (unowned string favorite in emojier_favorites) {
            if (favorite == selected_string) {
                has_favorite = true;
                break;
            }
        }
        if (!has_favorite) {
            emojier_favorites += selected_string;
            m_settings_emoji.set_strv("favorites", emojier_favorites);
        }
        m_command_line.print("%s\n", _("Copied an emoji to your clipboard."));
    }


    private void show_dialog(ApplicationCommandLine command_line) {
        m_command_line = command_line;
        m_emojier.reset();
        m_emojier.set_annotation("");
        m_emojier.show_all();
    }


    public void candidate_clicked_lookup_table(uint index,
                                               uint button,
                                               uint state) {
        if (m_command_line == null)
            return;
        if (button == IBusEmojier.BUTTON_CLOSE_BUTTON) {
            m_emojier.hide();
            save_selected_string(null, true);
            m_command_line = null;
            return;
        }
        if (m_emojier == null)
            return;
        bool show_candidate = false;
        uint ncandidates = m_emojier.get_number_of_candidates();
        if (ncandidates > 0 && ncandidates >= index) {
            m_emojier.set_cursor_pos(index);
            show_candidate = m_emojier.has_variants(index, false);
        } else {
            return;
        }
        if (show_candidate) {
            return;
        }
        string emoji = m_emojier.get_current_candidate();
        m_emojier.hide();
        save_selected_string(emoji, false);
        m_command_line = null;
    }


    public void activate_dialog(ApplicationCommandLine command_line) {
        this.hold ();
        show_dialog(command_line);
        this.release ();
    }


    private int _command_line(ApplicationCommandLine command_line) {
        // Set default font size
        IBusEmojier.set_emoji_font(m_settings_emoji.get_string("font"));

        const OptionEntry[] options = {
            { "font", 0, 0, OptionArg.STRING, out emoji_font,
              /* TRANSLATORS: "FONT" should be capital and translatable.
               * It's used for an argument command --font=FONT
               */
              N_("\"FONT\" for emoji characters on emoji dialog"),
              N_("FONT") },
            { "lang", 0, 0, OptionArg.STRING, out annotation_lang,
              /* TRANSLATORS: "LANG" should be capital and translatable.
               * It's used for an argument command --lang=LANG
               */
              N_("\"LANG\" for annotations on emoji dialog. E.g. \"en\""),
              N_("LANG") },
            { "partial-match", 0, 0, OptionArg.NONE, out partial_match,
              N_("Emoji annotations can be match partially"),
              null },
            { "partial-match-length", 0, 0, OptionArg.INT,
              out partial_match_length,
              N_("Match with the length of the specified integer"),
              null },
            { "partial-match-condition", 0, 0, OptionArg.INT,
              out partial_match_condition,
              N_("Match with the condition of the specified integer"),
              null },
            { null }
        };

        var option = new OptionContext();
        option.add_main_entries(options, Config.GETTEXT_PACKAGE);

        // We have to make an extra copy of the array,
        // since .parse assumes that it can remove strings
        // from the array without freeing them.
        string[] args = command_line.get_arguments();
        string*[] _args = new string[args.length];
        for (int i = 0; i < args.length; i++) {
            _args[i] = args[i];
        }

        // Need to initialize for the second instance.
        emoji_font = null;
        annotation_lang = null;

        try {
            unowned string[] tmp = _args;
            option.parse(ref tmp);
        } catch (OptionError e) {
            stderr.printf("%s\n", e.message);
            return Posix.EXIT_FAILURE;
        }

        if (m_emojier != null && m_emojier.is_running()) {
            Gdk.Event event = new Gdk.Event(Gdk.EventType.KEY_PRESS);
            event.key.time = Gdk.CURRENT_TIME;
            m_emojier.present_centralize(event);
            return Posix.EXIT_SUCCESS;
        }

        if (annotation_lang == null)
            annotation_lang = m_settings_emoji.get_string("lang");
        IBusEmojier.set_annotation_lang(annotation_lang);
        IBusEmojier.set_partial_match(partial_match);
        if (partial_match_length > 0) {
            IBusEmojier.set_partial_match_length(partial_match_length);
        } else {
            IBusEmojier.set_partial_match_length(
                    m_settings_emoji.get_int("partial-match-length"));
        }
        if (partial_match_condition > 2) {
            warning("Need condition between 0 and 2.");
            IBusEmojier.set_partial_match_condition(
                    m_settings_emoji.get_int("partial-match-condition"));
        }
        else if (partial_match_condition >= 0) {
            IBusEmojier.set_partial_match_condition(partial_match_condition);
        } else {
            IBusEmojier.set_partial_match_condition(
                    m_settings_emoji.get_int("partial-match-condition"));
        }

        if (emoji_font != null)
            IBusEmojier.set_emoji_font(emoji_font);

        IBusEmojier.set_favorites(
                m_settings_emoji.get_strv("favorites"),
                m_settings_emoji.get_strv("favorite-annotations"));

        IBusEmojier.load_unicode_dict();

        if (m_emojier == null) {
            bool is_wayland = false;
#if USE_GDK_WAYLAND
            Type instance_type = Gdk.Display.get_default().get_type();
            Type wayland_type = typeof(GdkWayland.Display);
            is_wayland = instance_type.is_a(wayland_type);
#else
            warning("Checking Wayland is disabled");
#endif
            m_emojier = new IBusEmojier(is_wayland);
            // For title handling in gnome-shell
            add_window(m_emojier);
            m_emojier.candidate_clicked.connect((i, b, s) => {
                candidate_clicked_lookup_table(i, b, s);
            });
            m_emojier.cancel.connect(() => {
                if (m_command_line == null)
                    return;
                m_emojier.hide();
                save_selected_string(null, true);
                m_command_line = null;
            });
            m_emojier.commit_text.connect(() => {
                if (m_command_line == null)
                    return;
                m_emojier.hide();
                string selected_string = m_emojier.get_selected_string();
                save_selected_string(selected_string, false);
                m_command_line = null;
            });
        }

        activate_dialog(command_line);

        return Posix.EXIT_SUCCESS;
    }


    public override int command_line(ApplicationCommandLine command_line) {
        // keep the application running until we are done with this commandline
        this.hold();
        int result = _command_line(command_line);
        this.release();
        return result;
    }


    public override void shutdown() {
        base.shutdown();
        remove_window(m_emojier);
        m_emojier = null;
    }


    public static int main (string[] args) {
        GLib.Intl.bindtextdomain(Config.GETTEXT_PACKAGE, Config.LOCALEDIR);
        GLib.Intl.bind_textdomain_codeset(Config.GETTEXT_PACKAGE, "UTF-8");
        GLib.Intl.textdomain(Config.GETTEXT_PACKAGE);

        IBus.init();

        Gtk.init(ref args);

        EmojiApplication app = new EmojiApplication();
        int status = app.run(args);
        return status;
    }
}
