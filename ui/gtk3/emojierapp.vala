/* vim:set et sts=4 sw=4:
 *
 * ibus - The Input Bus
 *
 * Copyright (c) 2017 Peng Wu <alexepico@gmail.com>
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

string emoji_font = null;
string annotation_lang = null;
bool partial_match = false;
int partial_match_length = -1;
int partial_match_condition = -1;

public class EmojiApplication : Application {
    private IBusEmojier? m_emojier;
    GLib.Settings m_settings_emoji =
            new GLib.Settings("org.freedesktop.ibus.panel.emoji");


    private EmojiApplication() {
        Object(application_id: "org.freedesktop.ibus.panel.emojier",
               flags: ApplicationFlags.HANDLES_COMMAND_LINE);
        set_inactivity_timeout(100000);
    }


    private void show_dialog(ApplicationCommandLine command_line) {
        m_emojier = new IBusEmojier();
        Gdk.Event event = Gtk.get_current_event();
        // Plasma and GNOME3 desktop returns null event
        if (event == null) {
            event = new Gdk.Event(Gdk.EventType.KEY_PRESS);
            event.key.time = Gdk.CURRENT_TIME;
            // event.get_seat() refers event.any.window
            event.key.window = Gdk.get_default_root_window();
            event.key.window.ref();
        }
        string emoji = m_emojier.run("", event);
        if (emoji == null) {
            m_emojier = null;
            command_line.print("%s\n", _("Canceled to choose an emoji."));
            return;
        }
        Gtk.Clipboard clipboard = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD);
        clipboard.set_text(emoji, -1);
        clipboard.store();

        var emojier_favorites = m_settings_emoji.get_strv("favorites");
        bool has_favorite = false;
        foreach (unowned string favorite in emojier_favorites) {
            if (favorite == emoji) {
                has_favorite = true;
                break;
            }
        }
        if (!has_favorite) {
            emojier_favorites += emoji;
            m_settings_emoji.set_strv("favorites", emojier_favorites);
        }

        m_emojier = null;
        command_line.print("%s\n", _("Copied an emoji to your clipboard."));
    }


    public void activate_dialog(ApplicationCommandLine command_line) {
        this.hold ();
        show_dialog(command_line);
        this.release ();
    }


    private int _command_line (ApplicationCommandLine command_line) {
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

        activate_dialog(command_line);

        return Posix.EXIT_SUCCESS;
    }


    public override int command_line (ApplicationCommandLine command_line) {
        // keep the application running until we are done with this commandline
        this.hold();
        int result = _command_line(command_line);
        this.release();
        return result;
    }


    public static int main (string[] args) {
        GLib.Intl.bindtextdomain(Config.GETTEXT_PACKAGE,
                                 Config.GLIB_LOCALE_DIR);
        GLib.Intl.bind_textdomain_codeset(Config.GETTEXT_PACKAGE, "UTF-8");
        GLib.Intl.textdomain(Config.GETTEXT_PACKAGE);

        IBus.init();

        Gtk.init(ref args);

        EmojiApplication app = new EmojiApplication();
        int status = app.run(args);
        return status;
    }
}
