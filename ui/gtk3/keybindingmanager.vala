/* vim:set et sts=4 sw=4:
valac --pkg gtk+-2.0 --pkg x11 --pkg gdk-x11-2.0 --pkg gee-1.0 keybinding-manager.vala
*/

/**
 * This class is in charge to grab keybindings on the X11 display
 * and filter X11-events and passing on such events to the registered
 * handler methods.
 *
 * @author Oliver Sauder <os@esite.ch>
 */

public class KeybindingManager : GLib.Object {
    /**
     * list of binded keybindings
     */
    private GLib.List<Keybinding> m_bindings = new GLib.List<Keybinding>();

    private static KeybindingManager m_instance = null;

    /**
     * Helper class to store keybinding
     */
    private class Keybinding {
        public Keybinding(uint keysym,
                          Gdk.ModifierType modifiers,
                          KeybindingHandlerFunc handler) {
            this.keysym = keysym;
            this.modifiers = modifiers;
            this.handler = handler;
        }

        public uint keysym { get; set; }
        public Gdk.ModifierType modifiers { get; set; }
        public unowned KeybindingHandlerFunc handler { get; set; }
    }

    /**
     * Keybinding func needed to bind key to handler
     *
     * @param event passing on gdk event
     */
    public delegate void KeybindingHandlerFunc(Gdk.Event event);


    private  KeybindingManager(bool is_wayland_im) {
        if (!is_wayland_im)
            Gdk.Event.handler_set(event_handler);
    }

#if ENABLE_XIM
    /**
     * Bind accelerator to given handler
     *
     * @param keysym
     * @param modifiers
     * @param handler handler called when given accelerator is pressed
     */
    public bool bind(uint keysym,
                     Gdk.ModifierType modifiers,
                     KeybindingHandlerFunc handler) {
        if (!grab_keycode (keysym, modifiers))
            return false;

        // store binding
        Keybinding binding = new Keybinding(keysym, modifiers, handler);
        m_bindings.append(binding);

        return true;
    }

    /**
     * Unbind given accelerator.
     *
     * @param keysym
     * @param modifiers
     */
    public void unbind(uint keysym,
                       Gdk.ModifierType modifiers) {
        // unbind all keys with given accelerator
        GLib.List<Keybinding> remove_bindings = new GLib.List<Keybinding>();
        foreach(Keybinding binding in m_bindings) {
            if (binding.keysym == keysym && binding.modifiers == modifiers) {
                ungrab_keycode (binding.keysym,
                                binding.modifiers);
                remove_bindings.append(binding);
            }
        }

        // remove unbinded keys
        foreach (Keybinding binding in remove_bindings)
            m_bindings.remove (binding);
    }
#endif

    public static KeybindingManager get_instance (bool is_wayland_im=false) {
        if (m_instance == null)
            m_instance = new KeybindingManager (is_wayland_im);
        return m_instance;
    }

    public static Gdk.ModifierType get_primary_modifier (uint binding_mask) {
        const Gdk.ModifierType[] masks = {
            Gdk.ModifierType.MOD5_MASK,
            Gdk.ModifierType.MOD4_MASK,
            Gdk.ModifierType.MOD3_MASK,
            Gdk.ModifierType.MOD2_MASK,
            Gdk.ModifierType.MOD1_MASK,
            Gdk.ModifierType.CONTROL_MASK,
            Gdk.ModifierType.SHIFT_MASK,
            Gdk.ModifierType.LOCK_MASK
        };
        for (int i = 0; i < masks.length; i++) {
            Gdk.ModifierType mask = masks[i];
            if ((binding_mask & mask) == mask)
                return mask;
        }
        return 0;
    }

    public static bool primary_modifier_still_pressed(Gdk.Event event,
                                                      uint primary_modifier) {
        Gdk.EventKey keyevent = event.key;
        if (primary_modifier == 0)
            return false;

        Gdk.Device device = event.get_device();
        Gdk.Device pointer;
        if (device.get_source() == Gdk.InputSource.KEYBOARD)
            pointer = device.get_associated_device();
        else
            pointer = device;

        double[] axes = null;
        uint modifier = 0;
        pointer.get_state(keyevent.window, axes, out modifier);
        if ((primary_modifier & modifier) == primary_modifier)
            return true;

        return false;
    }

    public static uint keyval_to_modifier (uint keyval) {
        switch(keyval) {
            case 0xffe3: /* Control_L */
            case 0xffe4: /* Control_R */
                return Gdk.ModifierType.CONTROL_MASK;
            case 0xffe1: /* Shift_L */
            case 0xffe2: /* Shift_R */
                return Gdk.ModifierType.SHIFT_MASK;
            case 0xffe5: /* Caps_Lock */
                return Gdk.ModifierType.LOCK_MASK;
            case 0xffe9: /* Alt_L */
            case 0xffea: /* Alt_R */
                return Gdk.ModifierType.MOD1_MASK;
            case 0xffe7: /* Meta_L */
            case 0xffe8: /* Meta_R */
                return Gdk.ModifierType.META_MASK;
            case 0xffeb: /* Super_L */
            case 0xffec: /* Super_R */
                return Gdk.ModifierType.SUPER_MASK;
            case 0xffed: /* Hyper_L */
            case 0xffee: /* Hyper_R */
                return Gdk.ModifierType.HYPER_MASK;
            default:
                return 0;
        }
    }

    private void event_handler(Gdk.Event event) {
        do {
            if (event.any.window != Gdk.get_default_root_window()) {
                break;
            }

            if (event.type == Gdk.EventType.KEY_PRESS) {
                uint modifiers = event.key.state & IBus.MODIFIER_FILTER;
                uint keyval = event.key.keyval;
                if (keyval >= IBus.KEY_A && keyval <= IBus.KEY_Z &&
                    (modifiers & Gdk.ModifierType.SHIFT_MASK) != 0) {
                    keyval = keyval - IBus.KEY_A + IBus.KEY_a;
                }
                foreach (var binding in m_bindings) {
                    if (keyval != binding.keysym ||
                        modifiers != binding.modifiers)
                        continue;
                    binding.handler(event);
                    return;
                }
            }
        } while (false);
        Gtk.main_do_event(event);
    }

#if ENABLE_XIM
    // Get union of given modifiers and all the combination of the
    // modifiers in ignored_modifiers.
    XI.GrabModifiers[] get_grab_modifiers(uint modifiers) {
        const int[] ignored_modifiers = {
            X.KeyMask.LockMask,
            X.KeyMask.Mod2Mask,
            X.KeyMask.Mod5Mask
        };
        int[] masks = {};
        for (int i = 0; i < ignored_modifiers.length; i++) {
            int modifier = ignored_modifiers[i];
            masks += modifier;

            int length = masks.length;
            for (int j = 0; j < length - 1; j++) {
                masks += masks[j] | modifier;
            }
        }
        masks += 0;

        XI.GrabModifiers[] ximodifiers = {};
        foreach (var mask in masks) {
            ximodifiers += XI.GrabModifiers() {
                modifiers = mask | modifiers,
                status = 0
            };
        }

        return ximodifiers;
    }

    bool grab_keycode(uint keyval, uint modifiers) {
        Gdk.X11.Display display = BindingCommon.get_xdisplay();
        if (display == null)
            return false;
        unowned X.Display xdisplay = display.get_xdisplay();
        int keycode = xdisplay.keysym_to_keycode(keyval);
        if (keycode == 0) {
            warning("Can not convert keyval=%u to keycode!", keyval);
            return false;
        }

        XI.EventMask evmask = XI.EventMask() {
            deviceid = XI.AllMasterDevices,
            mask = new uchar[(XI.LASTEVENT + 7) / 8]
        };
        XI.set_mask(evmask.mask, XI.EventType.KeyPress);
        XI.set_mask(evmask.mask, XI.EventType.KeyRelease);

        int retval = XI.grab_keycode (xdisplay,
                                      XI.AllMasterDevices,
                                      keycode,
                                      xdisplay.default_root_window(),
                                      X.GrabMode.Async,
                                      X.GrabMode.Async,
                                      true,
                                      evmask,
                                      get_grab_modifiers(modifiers));
            
        return retval == 0;
    }

    bool ungrab_keycode(uint keyval, uint modifiers) {
        Gdk.X11.Display display = BindingCommon.get_xdisplay();
        if (display == null)
            return false;
        unowned X.Display xdisplay = display.get_xdisplay();
        int keycode = xdisplay.keysym_to_keycode(keyval);
        if (keycode == 0) {
            warning("Can not convert keyval=%u to keycode!", keyval);
            return false;
        }

        int retval = XI.ungrab_keycode (xdisplay,
                                        XI.AllMasterDevices,
                                        keycode,
                                        xdisplay.default_root_window(),
                                        get_grab_modifiers(modifiers));

        return retval == 0;
    }
#endif
}

/*
public static int main (string[] args)
{
    Gtk.init (ref args);

    KeybindingManager manager = new KeybindingManager();
    manager.bind("<Ctrl><Alt>V", test);

    Gtk.main ();
    return 0;
}

private static void test()
{
    debug("hotkey pressed");
}
*/
