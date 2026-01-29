/* wayland-client.vapi
 *
 * Copyright 2022 Corentin Noël <corentin.noel@collabora.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 * 	Corentin Noël <corentin.noel@collabora.com>
 */

[CCode (cprefix = "wl_", lower_case_cprefix = "wl_", cheader_filename = "wayland-client.h")]
namespace Wl {
	[SimpleType]
	[IntegerType (rank = 6)]
	[CCode (cname = "wl_fixed_t", lower_case_cprefix = "wl_fixed_", default_value = "0", cheader_filename = "wayland-util.h", has_type_id = false)]
	public struct fixed_t {
		[CCode (cname = "wl_fixed_from_double")]
		public fixed_t.from_double (double d);
		[CCode (cname = "wl_fixed_from_int")]
		public fixed_t.from_int (int i);
		public double to_double ();
		public int to_int ();
	}

	[Compact]
	[CCode (cname = "struct wl_array", free_function = "wl_array_release")]
	public class Array {
		public size_t size;
		public size_t alloc;
		public void* data;
		public void init ();
		public void* add (size_t size);
		public int copy (Wl.Array source);
	}

	[Compact]
	[CCode (cname = "struct wl_buffer", free_function = "wl_buffer_destroy")]
	public class Buffer {
		public int add_listener (Wl.BufferListener listener, void* data);
		public void set_user_data (void* user_data);
		public void* get_user_data ();
		public uint32 get_version ();
	}

	[Compact]
	[CCode (cname = "struct wl_display", free_function = "wl_display_disconnect")]
	public class Display {
		[CCode (cname = "wl_display_connect")]
		public Display.connect (string name);
		[CCode (cname = "wl_display_connect_to_fd")]
		public Display.connect_to_fd (int fd);
		public void set_user_data (void* user_data);
		public void* get_user_data ();
		public uint32 get_version ();
		public int get_fd ();
		public int dispatch ();
		public int dispatch_queue (Wl.EventQueue queue);
		public int dispatch_queue_pending (Wl.EventQueue queue);
		public int get_error ();
		public int dispatch_pending ();
		public uint32 get_protocol_error (out unowned Wl.Interface interface, out uint32 id);
		public int flush ();
		public int roundtrip_queue (Wl.EventQueue queue);
		public int roundtrip ();
		public Wl.EventQueue create_queue ();
		public int prepare_read_queue (Wl.EventQueue queue);
		public int prepare_read ();
		public void cancel_read ();
		public int read_events ();
		public unowned Wl.Callback sync ();
		public Wl.Registry get_registry ();
		public int add_listener (Wl.DisplayListener listener, void* data);
	}

	[Compact]
	[CCode (cname = "struct wl_compositor", free_function = "wl_compositor_destroy")]
	public class Compositor {
		public void set_user_data (void* user_data);
		public void* get_user_data ();
		public uint32 get_version ();
		public Wl.Surface create_surface ();
		public Wl.Region create_region ();
	}

	[Compact]
	[CCode (cname = "struct wl_callback", free_function = "wl_callback_destroy")]
	public class Callback {
		public int add_listener (Wl.CallbackListener listener, void* data);
		public void set_user_data (void* user_data);
		public void* get_user_data ();
		public uint32 get_version ();
	}

	[Compact]
	[CCode (cname = "struct wl_data_offer", free_function = "wl_data_offer_destroy")]
	public class DataOffer {
		public int add_listener (Wl.DataOfferListener listener, void* data);
		public void set_user_data (void* user_data);
		public void* get_user_data ();
		public uint32 get_version ();
		public void accept (uint32 serial, string mime_type);
		public void receive (string mime_type, uint32 fd);
		public void finish ();
		public void set_actions (uint32 dnd_actions, uint32 preferred_action);
	}

	[Compact]
	[CCode (cname = "struct wl_data_source", free_function = "wl_data_source_destroy")]
	public class DataSource {
		public int add_listener (Wl.DataSourceListener listener, void* data);
		public void set_user_data (void* user_data);
		public void* get_user_data ();
		public uint32 get_version ();
		public void offer (string mime_type);
		public void set_actions (uint32 dnd_actions);
	}

	[Compact]
	[CCode (cname = "struct wl_data_device", free_function = "wl_data_device_destroy")]
	public class DataDevice {
		public int add_listener (Wl.DataDeviceListener listener, void* data);
		public void set_user_data (void* user_data);
		public void* get_user_data ();
		public uint32 get_version ();
		public void start_drag (Wl.DataSource source, Wl.Surface origin, Wl.Surface icon, uint32 serial);
		public void set_selection (Wl.DataSource source, uint32 serial);
		[DestroysInstance]
		public void release ();
	}

	[Compact]
	[CCode (cname = "struct wl_data_device_manager", free_function = "wl_data_device_manager_destroy")]
	public class DataDeviceManager {
		public void set_user_data (void* user_data);
		public void* get_user_data ();
		public uint32 get_version ();
		public Wl.DataSource create_data_source ();
		public Wl.DataDevice get_data_device (Wl.Seat seat);
	}

	[Compact]
	[CCode (cname = "struct wl_event_queue", free_function = "wl_event_queue_destroy")]
	public class EventQueue {
	}

	[Compact]
	[CCode (cname = "struct wl_interface")]
	public class Interface {
		public unowned string name;
		public int version;
		[CCode (array_length_cname = "method_count")]
		public unowned Wl.Message[] methods;
		[CCode (array_length_cname = "event_count")]
		public unowned Wl.Message[] events;
	}

	[Compact]
	[CCode (cname = "struct wl_keyboard", free_function = "wl_keyboard_destroy")]
	public class Keyboard : Wl.Proxy {
		public int add_listener (Wl.KeyboardListener listener, void* data);
		public void set_user_data (void* user_data);
		public void* get_user_data ();
		public uint32 get_version ();
		[DestroysInstance]
		public void release ();
	}

	[Compact]
	[CCode (cname = "struct wl_message")]
	public class Message {
		public unowned string name;
		public unowned string signature;
		[CCode (array_length = false)]
		public unowned Wl.Interface?[] types;
	}

	[Compact]
	[CCode (cname = "struct wl_output", free_function = "wl_output_destroy")]
	public class Output : Wl.Proxy {
		public int add_listener (Wl.OutputListener listener, void* data);
		public void set_user_data (void* user_data);
		public void* get_user_data ();
		public uint32 get_version ();
		[DestroysInstance]
		public void release ();
	}

	[Compact]
	[CCode (cname = "struct wl_pointer", free_function = "wl_pointer_destroy")]
	public class Pointer : Wl.Proxy {
		public int add_listener (Wl.PointerListener listener, void* data);
		public void set_user_data (void* user_data);
		public void* get_user_data ();
		public uint32 get_version ();
		public void set_cursor (uint32 serial, Wl.Surface surface, int32 hotspot_x, int32 hotspot_y);
		[DestroysInstance]
		public void release ();
	}

	[Compact]
	[CCode (cname = "struct wl_proxy", free_function = "wl_proxy_destroy")]
	public class Proxy {
		public void set_user_data (void* user_data);
		public void* get_user_data ();
		public uint32 get_version ();
		public uint32 get_id ();
		public void set_tag ([CCode (array_length = false)] string[] tag);
		[CCode (array_length = false)]
		public unowned string[] get_tag ();
		public unowned string get_class ();
	}

	[Compact]
	[CCode (cname = "struct wl_region", free_function = "wl_region_destroy")]
	public class Region : Wl.Proxy {
		public void set_user_data (void* user_data);
		public void* get_user_data ();
		public uint32 get_version ();
		public void add (int32 x, int32 y, int32 width, int32 height);
		public void subtract (int32 x, int32 y, int32 width, int32 height);
	}

	[Compact]
	[CCode (cname = "struct wl_registry", free_function = "wl_registry_destroy")]
	public class Registry : Wl.Proxy {
		public int add_listener (Wl.RegistryListener listener, void* data);
		public void set_user_data (void* user_data);
		public void* get_user_data ();
		public uint32 get_version ();
		[CCode (simple_generics = true)]
		public T? bind<T> (uint32 name, ref Wl.Interface interface, uint32 version);
	}

	[Compact]
	[CCode (cname = "struct wl_seat", free_function = "wl_seat_destroy")]
	public class Seat : Wl.Proxy {
		public int add_listener (Wl.SeatListener listener, void* data);
		public void set_user_data (void* user_data);
		public void* get_user_data ();
		public uint32 get_version ();
		public Wl.Pointer get_pointer ();
		public Wl.Keyboard get_keyboard ();
		public Wl.Touch get_touch ();
		[DestroysInstance]
		public void release ();
	}

	[Compact]
	[CCode (cname = "struct wl_shell", free_function = "wl_shell_destroy")]
	public class Shell : Wl.Proxy {
		public void set_user_data (void* user_data);
		public void* get_user_data ();
		public uint32 get_version ();
		public Wl.ShellSurface get_shell_surface (Wl.Surface surface);
	}

	[Compact]
	[CCode (cname = "struct wl_shell_surface", free_function = "wl_shell_surface_destroy")]
	public class ShellSurface : Wl.Proxy {
		public int add_listener (Wl.ShellSurfaceListener listener, void* data);
		public void set_user_data (void* user_data);
		public void* get_user_data ();
		public uint32 get_version ();
		public void pong (uint32 serial);
		public void move (Wl.Seat seat, uint32 serial);
		public void resize (Wl.Seat seat, uint32 serial, uint32 edges);
		public void set_toplevel ();
		public void set_transient (Wl.Surface surface, int32 x, int32 y, uint32 flags);
		public void set_fullscreen (uint32 method, uint32 framerate, Wl.Output output);
		public void set_popup (Wl.Seat seat, uint32 serial, Wl.Surface parent, int32 x, int32 y, uint32 flags);
		public void set_maximized (Wl.Output output);
		public void set_title (string title);
		public void set_class (string class_);
	}

	[Compact]
	[CCode (cname = "struct wl_shm", free_function = "wl_shm_destroy")]
	public class Shm : Wl.Proxy {
		public int add_listener (Wl.ShmListener listener, void* data);
		public void set_user_data (void* user_data);
		public void* get_user_data ();
		public uint32 get_version ();
	}

	[Compact]
	[CCode (cname = "struct wl_shm_pool", free_function = "wl_shm_pool_destroy")]
	public class ShmPool : Wl.Proxy {
		public void set_user_data (void* user_data);
		public void* get_user_data ();
		public uint32 get_version ();
		public Wl.Buffer create_buffer (int32 offset, int32 width, int32 height, int32 stride, uint32 format);
		public void resize (int32 size);
	}

	[Compact]
	[CCode (cname = "struct wl_subcompositor", free_function = "wl_subcompositor_destroy")]
	public class Subcompositor : Wl.Proxy {
		public void set_user_data (void* user_data);
		public void* get_user_data ();
		public uint32 get_version ();
		public Wl.Subsurface get_subsurface (Wl.Surface surface, Wl.Surface parent);
	}

	[Compact]
	[CCode (cname = "struct wl_subsurface", free_function = "wl_subsurface_destroy")]
	public class Subsurface : Wl.Proxy {
		public void set_user_data (void* user_data);
		public void* get_user_data ();
		public uint32 get_version ();
		public void set_position (int32 x, int32 y);
		public void place_above (Wl.Surface sibling);
		public void place_below (Wl.Surface sibling);
		public void set_sync ();
		public void set_desync ();
	}

	[Compact]
	[CCode (cname = "struct wl_surface", free_function = "wl_surface_destroy")]
	public class Surface : Wl.Proxy {
		public int add_listener (Wl.SurfaceListener listener, void* data);
		public void set_user_data (void* user_data);
		public void* get_user_data ();
		public uint32 get_version ();
		public void attach (Wl.Buffer buffer, int32 x, int32 y);
		public void damage (int32 x, int32 y, int32 width, int32 height);
		public unowned Wl.Callback frame ();
		public void set_opaque_region (Wl.Region? region);
		public void set_input_region (Wl.Region? region);
		public void commit ();
		public void set_buffer_transform (int32 transform);
		public void set_buffer_scale (int32 transform);
		public void damage_buffer (int32 x, int32 y, int32 width, int32 height);
		public void offset (int32 x, int32 y);
	}

	[Compact]
	[CCode (cname = "struct wl_touch", free_function = "wl_touch_destroy")]
	public class Touch : Wl.Proxy {
		public int add_listener (Wl.TouchListener listener, void* data);
		public void set_user_data (void* user_data);
		public void* get_user_data ();
		public uint32 get_version ();
		[DestroysInstance]
		public void release ();
	}

	[CCode (cname = "struct wl_buffer_listener", has_type_id = false)]
	public struct BufferListener {
		public Wl.BufferListenerRelease release;
	}

	[CCode (cname = "struct wl_callback_listener", has_type_id = false)]
	public struct CallbackListener {
		public Wl.CallbackListenerDone done;
	}

	[CCode (cname = "struct wl_data_device_listener", has_type_id = false)]
	public struct DataDeviceListener {
		public Wl.DataDeviceListenerDataOffer data_offer;
		public Wl.DataDeviceListenerEnter enter;
		public Wl.DataDeviceListenerLeave leave;
		public Wl.DataDeviceListenerMotion motion;
		public Wl.DataDeviceListenerDrop drop;
		public Wl.DataDeviceListenerSelection selection;
	}

	[CCode (cname = "struct wl_data_offer_listener", has_type_id = false)]
	public struct DataOfferListener {
		public Wl.DataOfferListenerOffer offer;
		public Wl.DataOfferListenerSourceActions source_actions;
		public Wl.DataOfferListenerAction action;
	}

	[CCode (cname = "struct wl_data_source_listener", has_type_id = false)]
	public struct DataSourceListener {
		public Wl.DataSourceListenerTarget target;
		public Wl.DataSourceListenerSend send;
		public Wl.DataSourceListenerCancelled cancelled;
		public Wl.DataSourceListenerDndDropPerformed dnd_drop_performed;
		public Wl.DataSourceListenerDndFinished dnd_finished;
		public Wl.DataSourceListenerAction action;
	}

	[CCode (cname = "struct wl_display_listener", has_type_id = false)]
	public struct DisplayListener {
		public Wl.DisplayListenerError error;
		public Wl.DisplayListenerDeleteID delete_id;
	}

	[CCode (cname = "struct wl_keyboard_listener", has_type_id = false)]
	public struct KeyboardListener {
		public Wl.KeyboardListenerKeymap keymap;
		public Wl.KeyboardListenerEnter enter;
		public Wl.KeyboardListenerLeave leave;
		public Wl.KeyboardListenerKey key;
		public Wl.KeyboardListenerModifiers modifiers;
		public Wl.KeyboardListenerRepeatInfo repeat_info;
	}

	[CCode (cname = "struct wl_output_listener", has_type_id = false)]
	public struct OutputListener {
		public Wl.OutputListenerGeometry geometry;
		public Wl.OutputListenerMode mode;
		public Wl.OutputListenerDone done;
		public Wl.OutputListenerScale scale;
		public Wl.OutputListenerName name;
		public Wl.OutputListenerDescription description;
	}

	[CCode (cname = "struct wl_pointer_listener", has_type_id = false)]
	public struct PointerListener {
		public Wl.PointerListenerEnter enter;
		public Wl.PointerListenerLeave leave;
		public Wl.PointerListenerMotion motion;
		public Wl.PointerListenerButton button;
		public Wl.PointerListenerAxis axis;
		public Wl.PointerListenerFrame frame;
		public Wl.PointerListenerAxisSource axis_source;
		public Wl.PointerListenerAxisStop axis_stop;
		public Wl.PointerListenerAxisDiscrete axis_discrete;
	}

	[CCode (cname = "struct wl_registry_listener", has_type_id = false)]
	public struct RegistryListener {
		public Wl.RegistryListenerGlobal global;
		public Wl.RegistryListenerGlobalRemove global_remove;
	}

	[CCode (cname = "struct wl_seat_listener", has_type_id = false)]
	public struct SeatListener {
		public Wl.SeatListenerCapabilities capabilities;
		public Wl.SeatListenerName name;
	}

	[CCode (cname = "struct wl_surface_listener", has_type_id = false)]
	public struct SurfaceListener {
		public Wl.SurfaceListenerEnter enter;
		public Wl.SurfaceListenerLeave leave;
	}

	[CCode (cname = "struct wl_shell_surface_listener", has_type_id = false)]
	public struct ShellSurfaceListener {
		public Wl.ShellSurfaceListenerPing ping;
		public Wl.ShellSurfaceListenerConfigure configure;
		public Wl.ShellSurfaceListenerPopupDone popup_done;
	}

	[CCode (cname = "struct wl_shm_listener", has_type_id = false)]
	public struct ShmListener {
		public Wl.ShmListenerFormat format;
	}

	[CCode (cname = "struct wl_touch_listener", has_type_id = false)]
	public struct TouchListener {
		public Wl.TouchListenerDown down;
		public Wl.TouchListenerUp up;
		public Wl.TouchListenerMotion motion;
		public Wl.TouchListenerFrame frame;
		public Wl.TouchListenerCancel cancel;
		public Wl.TouchListenerShape shape;
		public Wl.TouchListenerOrientation orientation;
	}

	[CCode (cprefix = "WL_DATA_OFFER_ERROR_", cname = "enum wl_data_offer_error", has_type_id = false)]
	public enum DataOfferError {
		INVALID_FINISH,
		INVALID_ACTION_MASK,
		INVALID_ACTION,
		INVALID_OFFER
	}

	[CCode (cprefix = "WL_DATA_SOURCE_ERROR_", cname = "enum wl_data_source_error", has_type_id = false)]
	public enum DataSourceError {
		INVALID_ACTION_MASK,
		INVALID_SOURCE
	}

	[CCode (cprefix = "WL_DATA_DEVICE_ERROR_", cname = "enum wl_data_device_error", has_type_id = false)]
	public enum DataDeviceError {
		ROLE
	}

	[CCode (cprefix = "WL_DISPLAY_ERROR_", cname = "enum wl_display_error", has_type_id = false)]
	public enum DisplayError {
		INVALID_OBJECT,
		INVALID_METHOD,
		NO_MEMORY,
		IMPLEMENTATION
	}

	[CCode (cprefix = "WL_POINTER_ERROR_", cname = "enum wl_pointer_error", has_type_id = false)]
	public enum PointerError {
		ROLE
	}

	[CCode (cprefix = "WL_SEAT_ERROR_", cname = "enum wl_seat_error", has_type_id = false)]
	public enum SeatError {
		MISSING_CAPABILITY
	}

	[CCode (cprefix = "WL_SUBCOMPOSITOR_ERROR_", cname = "enum wl_subcompositor_error", has_type_id = false)]
	public enum SubcompositorError {
		BAD_SURFACE
	}

	[CCode (cprefix = "WL_SUBSURFACE_ERROR_", cname = "enum wl_subsurface_error", has_type_id = false)]
	public enum SubsurfaceError {
		BAD_SURFACE
	}

	[CCode (cprefix = "WL_SURFACE_ERROR_", cname = "enum wl_surface_error", has_type_id = false)]
	public enum SurfaceError {
		INVALID_SCALE,
		INVALID_TRANSFORM,
		INVALID_SIZE,
		INVALID_OFFSET
	}

	[CCode (cprefix = "WL_SHM_ERROR_", cname = "enum wl_shm_error", has_type_id = false)]
	public enum ShmError {
		INVALID_FORMAT,
		INVALID_STRIDE,
		INVALID_FD
	}

	[CCode (cprefix = "WL_SHELL_ERROR_", cname = "enum wl_shell_error", has_type_id = false)]
	public enum ShellError {
		ROLE
	}

	[CCode (cprefix = "WL_DATA_DEVICE_MANAGER_DND_ACTION_", cname = "enum wl_data_device_manager_dnd_action", has_type_id = false)]
	public enum DataDeviceManagerDndAction {
		NONE,
		COPY,
		MOVE,
		ASK
	}

	[CCode (cprefix = "WL_KEYBOARD_KEYMAP_FORMAT_", cname = "enum wl_keyboard_keymap_format", has_type_id = false)]
	public enum KeyboardKeymapFormat {
		NO_KEYMAP,
		XKB_V1
	}

	[CCode (cprefix = "WL_KEYBOARD_KEY_STATE_", cname = "enum wl_keyboard_key_state", has_type_id = false)]
	public enum KeyboardKeyState {
		RELEASED,
		PRESSED
	}

	[CCode (cprefix = "WL_OUTPUT_SUBPIXEL_", cname = "enum wl_output_subpixel", has_type_id = false)]
	public enum OutputSubpixel {
		UNKNOWN,
		NONE,
		HORIZONTAL_RGB,
		HORIZONTAL_BGR,
		VERTICAL_RGB,
		VERTICAL_BGR
	}

	[CCode (cprefix = "WL_OUTPUT_TRANSFORM_", cname = "enum wl_output_transform", has_type_id = false)]
	public enum OutputTransform {
		NORMAL,
		[CCode (cname = "WL_OUTPUT_TRANSFORM_90")]
		_90,
		[CCode (cname = "WL_OUTPUT_TRANSFORM_180")]
		_180,
		[CCode (cname = "WL_OUTPUT_TRANSFORM_270")]
		_270,
		FLIPPED,
		FLIPPED_90,
		FLIPPED_180,
		FLIPPED_270
	}

	[CCode (cprefix = "WL_OUTPUT_MODE_", cname = "enum wl_output_mode", has_type_id = false)]
	public enum OutputMode {
		CURRENT,
		PREFERRED
	}

	[CCode (cprefix = "WL_POINTER_BUTTON_STATE_", cname = "enum wl_pointer_button_state", has_type_id = false)]
	public enum PointerButtonState {
		RELEASED,
		PRESSED
	}

	[CCode (cprefix = "WL_POINTER_AXIS_", cname = "enum wl_pointer_axis", has_type_id = false)]
	public enum PointerAxis {
		VERTICAL_SCROLL,
		HORIZONTAL_SCROLL
	}

	[CCode (cprefix = "WL_POINTER_AXIS_SOURCE_", cname = "enum wl_pointer_axis_source", has_type_id = false)]
	public enum PointerAxisSource {
		WHEEL,
		FINGER,
		CONTINUOUS,
		WHEEL_TILT
	}

	[CCode (cprefix = "WL_SEAT_CAPABILITY_", cname = "enum wl_seat_capability", has_type_id = false)]
	public enum SeatCapability {
		POINTER,
		KEYBOARD,
		TOUCH
	}

	[CCode (cprefix = "WL_SHELL_SURFACE_RESIZE_", cname = "enum wl_shell_surface_resize", has_type_id = false)]
	public enum ShellSurfaceResize {
		NONE,
		TOP,
		BOTTOM,
		LEFT,
		TOP_LEFT,
		BOTTOM_LEFT,
		RIGHT,
		TOP_RIGHT,
		BOTTOM_RIGHT
	}

	[CCode (cprefix = "WL_SHELL_SURFACE_TRANSIENT_", cname = "enum wl_shell_surface_transient", has_type_id = false)]
	public enum ShellSurfaceTranscient {
		INACTIVE
	}

	[CCode (cprefix = "WL_SHELL_SURFACE_FULLSCREEN_METHOD_", cname = "enum wl_shell_surface_fullscreen_method", has_type_id = false)]
	public enum ShellSurfaceFullscreenMethod {
		DEFAULT,
		SCALE,
		DRIVER,
		FILL
	}

	[CCode (cprefix = "WL_SHM_FORMAT_", cname = "enum wl_shm_format", has_type_id = false)]
	public enum ShmFormat {
		ARGB8888,
		XRGB8888,
		C8,
		RGB332,
		BGR233,
		XRGB4444,
		XBGR4444,
		RGBX4444,
		BGRX4444,
		ARGB4444,
		ABGR4444,
		RGBA4444,
		BGRA4444,
		XRGB1555,
		XBGR1555,
		RGBX5551,
		BGRX5551,
		ARGB1555,
		ABGR1555,
		RGBA5551,
		BGRA5551,
		RGB565,
		BGR565,
		RGB888,
		BGR888,
		XBGR8888,
		RGBX8888,
		BGRX8888,
		ABGR8888,
		RGBA8888,
		BGRA8888,
		XRGB2101010,
		XBGR2101010,
		RGBX1010102,
		BGRX1010102,
		ARGB2101010,
		ABGR2101010,
		RGBA1010102,
		BGRA1010102,
		YUYV,
		YVYU,
		UYVY,
		VYUY,
		AYUV,
		NV12,
		NV21,
		NV16,
		NV61,
		YUV410,
		YVU410,
		YUV411,
		YVU411,
		YUV420,
		YVU420,
		YUV422,
		YVU422,
		YUV444,
		YVU444,
		R8,
		R16,
		RG88,
		GR88,
		RG1616,
		GR1616,
		XRGB16161616F,
		XBGR16161616F,
		ARGB16161616F,
		ABGR16161616F,
		XYUV8888,
		VUY888,
		VUY101010,
		Y210,
		Y212,
		Y216,
		Y410,
		Y412,
		Y416,
		XVYU2101010,
		XVYU12_16161616,
		XVYU16161616,
		Y0L0,
		X0L0,
		Y0L2,
		X0L2,
		YUV420_8BIT,
		YUV420_10BIT,
		XRGB8888_A8,
		XBGR8888_A8,
		RGBX8888_A8,
		BGRX8888_A8,
		RGB888_A8,
		BGR888_A8,
		RGB565_A8,
		BGR565_A8,
		NV24,
		NV42,
		P210,
		P010,
		P012,
		P016,
		AXBXGXRX106106106106,
		NV15,
		Q410,
		Q401,
		XRGB16161616,
		XBGR16161616,
		ARGB16161616,
		ABGR16161616
	}

	[CCode (has_target = false, has_typedef = false)]
	public delegate void BufferListenerRelease (void *data, Wl.Buffer wl_buffer);

	[CCode (has_target = false, has_typedef = false)]
	public delegate void CallbackListenerDone (void *data, Wl.Callback wl_callback, uint32 callback_data);

	[CCode (has_target = false, has_typedef = false)]
	public delegate void DataDeviceListenerDataOffer (void *data, Wl.DataDevice wl_data_device, Wl.DataOffer id);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void DataDeviceListenerEnter (void *data, Wl.DataDevice wl_data_device, uint32 serial, Wl.Surface surface, Wl.fixed_t x, Wl.fixed_t y, Wl.DataOffer id);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void DataDeviceListenerLeave (void *data, Wl.DataDevice wl_data_device);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void DataDeviceListenerMotion (void *data, Wl.DataDevice wl_data_device, uint32 time, Wl.fixed_t x, Wl.fixed_t y);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void DataDeviceListenerDrop (void *data, Wl.DataDevice wl_data_device);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void DataDeviceListenerSelection (void *data, Wl.DataDevice wl_data_device, Wl.DataOffer id);

	[CCode (has_target = false, has_typedef = false)]
	public delegate void DataOfferListenerOffer (void *data, Wl.DataOffer wl_data_offer, string mime_type);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void DataOfferListenerSourceActions (void *data, Wl.DataOffer wl_data_offer, uint32 source_actions);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void DataOfferListenerAction (void *data, Wl.DataOffer wl_data_offer, uint32 dnd_action);

	[CCode (has_target = false, has_typedef = false)]
	public delegate void DataSourceListenerTarget (void *data, Wl.DataSource wl_data_source, string mime_type);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void DataSourceListenerSend (void *data, Wl.DataSource wl_data_source, string mime_type, int32 fd);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void DataSourceListenerCancelled (void *data, Wl.DataSource wl_data_source);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void DataSourceListenerDndDropPerformed (void *data, Wl.DataSource wl_data_source);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void DataSourceListenerDndFinished (void *data, Wl.DataSource wl_data_source);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void DataSourceListenerAction (void *data, Wl.DataSource wl_data_source, uint32 dnd_action);

	[CCode (has_target = false, has_typedef = false)]
	public delegate void DisplayListenerError (void *data, Wl.Display wl_display, void *object_id, uint32 code, string message);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void DisplayListenerDeleteID (void *data, Wl.Display wl_display, uint32 id);

	[CCode (has_target = false, has_typedef = false)]
	public delegate void KeyboardListenerKeymap (void *data, Wl.Keyboard wl_keyboard, uint32 format, int32 fd, uint32 size);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void KeyboardListenerEnter (void *data, Wl.Keyboard wl_keyboard, uint32 serial, Wl.Surface surface, Wl.Array keys);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void KeyboardListenerLeave (void *data, Wl.Keyboard wl_keyboard, uint32 serial, Wl.Surface surface);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void KeyboardListenerKey (void *data, Wl.Keyboard wl_keyboard, uint32 serial, uint32 time, uint32 key, uint32 state);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void KeyboardListenerModifiers (void *data, Wl.Keyboard wl_keyboard, uint32 serial, uint32 mods_depressed, uint32 mods_latched, uint32 mods_locked, uint32 group);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void KeyboardListenerRepeatInfo (void *data, Wl.Keyboard wl_keyboard, int32 rate, int32 delay);

	[CCode (has_target = false, has_typedef = false)]
	public delegate void OutputListenerGeometry (void *data, Wl.Output wl_output, int32 x, int32 y, int32 physical_width, int32 physical_height, int32 subpixel, string make, string model, int32 transform);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void OutputListenerMode (void *data, Wl.Output wl_output, uint32 flags, int32 width, int32 height, int32 refresh);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void OutputListenerDone (void *data, Wl.Output wl_output);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void OutputListenerScale (void *data, Wl.Output wl_output, int32 factor);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void OutputListenerName (void *data, Wl.Output wl_output, string name);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void OutputListenerDescription (void *data, Wl.Output wl_output, string description);

	[CCode (has_target = false, has_typedef = false)]
	public delegate void PointerListenerEnter (void *data, Wl.Pointer wl_pointer, uint32 serial, Wl.Surface surface, Wl.fixed_t surface_x, Wl.fixed_t surface_y);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void PointerListenerLeave (void *data, Wl.Pointer wl_pointer, uint32 serial, Wl.Surface surface);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void PointerListenerMotion (void *data, Wl.Pointer wl_pointer, uint32 time, Wl.fixed_t surface_x, Wl.fixed_t surface_y);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void PointerListenerButton (void *data, Wl.Pointer wl_pointer, uint32 serial, uint32 time, uint32 button, uint32 state);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void PointerListenerAxis (void *data, Wl.Pointer wl_pointer, uint32 time, uint32 axis, Wl.fixed_t @value);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void PointerListenerFrame (void *data, Wl.Pointer wl_pointer);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void PointerListenerAxisSource (void *data, Wl.Pointer wl_pointer, uint32 axis_source);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void PointerListenerAxisStop (void *data, Wl.Pointer wl_pointer, uint32 time, uint32 axis);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void PointerListenerAxisDiscrete (void *data, Wl.Pointer wl_pointer, uint32 axis, int32 discrete);

	[CCode (has_target = false, has_typedef = false)]
	public delegate void RegistryListenerGlobal (void *data, Wl.Registry wl_registry, uint32 name, string @interface, uint32 version);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void RegistryListenerGlobalRemove (void *data, Wl.Registry wl_registry, uint32 name);

	[CCode (has_target = false, has_typedef = false)]
	public delegate void SeatListenerCapabilities (void *data, Wl.Seat wl_seat, uint32 capabilities);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void SeatListenerName (void *data, Wl.Seat wl_seat, string name);

	[CCode (has_target = false, has_typedef = false)]
	public delegate void ShellSurfaceListenerPing (void *data, Wl.ShellSurface wl_shell_surface, uint32 serial);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void ShellSurfaceListenerConfigure (void *data, Wl.ShellSurface wl_shell_surface, uint32 edges, int32 width, int32 height);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void ShellSurfaceListenerPopupDone (void *data, Wl.ShellSurface wl_shell_surface);

	[CCode (has_target = false, has_typedef = false)]
	public delegate void ShmListenerFormat (void *data, Wl.Shm wl_shm, uint32 format);

	[CCode (has_target = false, has_typedef = false)]
	public delegate void SurfaceListenerEnter (void *data, Wl.Surface wl_surface, Wl.Output output);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void SurfaceListenerLeave (void *data, Wl.Surface wl_surface, Wl.Output output);

	[CCode (has_target = false, has_typedef = false)]
	public delegate void TouchListenerDown (void *data, Wl.Touch wl_touch, uint32 serial, uint32 time, Wl.Surface surface, int32 id, Wl.fixed_t x, Wl.fixed_t y);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void TouchListenerUp (void *data, Wl.Touch wl_touch, uint32 serial, uint32 time, int32 id);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void TouchListenerMotion (void *data, Wl.Touch wl_touch, uint32 time, int32 id, Wl.fixed_t x, Wl.fixed_t y);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void TouchListenerFrame (void *data, Wl.Touch wl_touch);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void TouchListenerCancel (void *data, Wl.Touch wl_touch);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void TouchListenerShape (void *data, Wl.Touch wl_touch, int32 id, Wl.fixed_t major, Wl.fixed_t minor);
	[CCode (has_target = false, has_typedef = false)]
	public delegate void TouchListenerOrientation (void *data, Wl.Touch wl_touch, int32 id, Wl.fixed_t orientation);

	[CCode (cname = "WAYLAND_VERSION_MAJOR")]
	public const int VERSION_MAJOR;
	[CCode (cname = "WAYLAND_VERSION_MINOR")]
	public const int VERSION_MINOR;
	[CCode (cname = "WAYLAND_VERSION_MICRO")]
	public const int VERSION_MICRO;
	[CCode (cname = "WAYLAND_VERSION")]
	public const string VERSION;
}
