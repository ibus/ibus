namespace IBus {
        public class WaylandIM : IBus.Object {
		[CCode (cname = "ibus_wayland_im_new", has_construct_function = true)]
		public WaylandIM (string first_property_name, ...);
	}
}
