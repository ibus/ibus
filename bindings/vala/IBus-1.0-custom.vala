namespace IBus {
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
