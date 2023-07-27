[CCode (cprefix = "GdkWayland", lower_case_cprefix = "gdk_wayland_", cheader_filename = "gdk/gdkwayland.h")]
namespace GdkWayland
{
    [CCode (type_id = "gdk_wayland_display_get_type ()")]
    public class Display : Gdk.Display {
        public void *get_wl_display();
    }

    [CCode (type_id = "gdk_wayland_window_get_type ()")]
    public class Window : Gdk.Window {
        public void *get_wl_surface();
        public void  set_use_custom_surface();
    }
}
