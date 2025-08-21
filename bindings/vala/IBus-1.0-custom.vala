namespace IBus {
        [CCode (cheader_filename = "ibusattrlistprivate.h",
                type_id = "ibus_attr_list_get_type ()")]
        public class AttrList : IBus.Serializable {
                [Version (since = "1.5.33")]
                public unowned IBus.AttrList
                copy_format_to_hint () throws GLib.Error;
                [Version (since = "1.5.33")]
                public unowned IBus.AttrList
                copy_format_to_rgba (IBus.RGBA? selected_fg,
                                     IBus.RGBA? selected_bg) throws GLib.Error;
        }
        public class EmojiData : IBus.Serializable {
		[CCode (cname = "ibus_emoji_data_new",
                        has_construct_function = true)]
		public EmojiData (string first_property_name, ...);
	}
	public class ExtensionEvent : IBus.Serializable {
		[CCode (cname = "ibus_extension_event_new",
                        has_construct_function = true)]
		public ExtensionEvent (string first_property_name, ...);
	}
        public class InputContext : IBus.Proxy {
                [Version (since = "1.5.33")]
                public void set_selected_color (IBus.RGBA fg_color,
                                                IBus.RGBA bg_color);
	}
	public class Message : IBus.Serializable {
		[CCode (cname = "ibus_message_new",
                        has_construct_function = true)]
		public Message (uint domain,
                                uint code,
                                string? title,
                                string description,
                                ...);
	}
	public class PanelService : IBus.Service {
                public void
                panel_extension_register_keys(string first_property_name, ...);
                [Version (since = "1.5.33")]
                public void set_selected_color (IBus.RGBA fg_color,
                                                IBus.RGBA bg_color);
	}
	// For some reason, ibus_text_new_from_static_string is hidden in GIR
	// https://github.com/ibus/ibus/commit/37e6e587
	[CCode (type_id = "ibus_text_get_type ()", cheader_filename = "ibus.h")]
	public class Text : IBus.Serializable {
		[CCode (cname = "ibus_text_new_from_static_string",
                        has_construct_function = false)]
		public Text.from_static_string (string str);
	}
	public class XEvent : IBus.Serializable {
		[CCode (cname = "ibus_x_event_new",
                 has_construct_function = true)]
		public XEvent (string first_property_name, ...);
	}
}
