#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <dlfcn.h>

#ifndef DEFAULT_IM_MODULE_TYPE
#define DEFAULT_IM_MODULE_TYPE "gtk3"
#endif

#define OPTION_TYPE_MESSAGE \
  N_("Set im-module TYPE to \"gtk2\",  \"gtk3\" or \"gtk4\". Default is " \
     "\"gtk3\".")

typedef const char * (* IBusIMGetContextIdFunc) (int *argc, char ***argv);

static char *im_module_type;


char *
ibus_im_module_get_id (int argc, char *argv[])
{
    static const GOptionEntry options[3] = {
        { "type", (char)0, (int)0, G_OPTION_ARG_STRING, &im_module_type,
          OPTION_TYPE_MESSAGE,
         "TYPE"},
        { NULL }
    };
    GOptionContext *option;
    GError *error = NULL;
    void *module;
    char *im_context_id;
    IBusIMGetContextIdFunc im_get_context_id;

    if (!(option = g_option_context_new (NULL))) {
        g_critical ("malloc GOptionContext is failed.");
        return NULL;
    }
    g_option_context_add_main_entries (option, options, GETTEXT_PACKAGE);
    g_option_context_parse (option, &argc, &argv, &error);
    if (error) {
        g_critical ("%s", error->message);
        g_clear_error (&error);
        return NULL;
    }
    g_option_context_free (option);
    if (!im_module_type)
        im_module_type = g_strdup (DEFAULT_IM_MODULE_TYPE);

    if (G_LIKELY (!g_strcmp0 (im_module_type, "gtk3"))) {
        module = dlopen (GTK3_IM_MODULEDIR "/im-ibus.so",
                         RTLD_LAZY);
    } else if (!g_strcmp0 (im_module_type, "gtk4")) {
        const char *module_path_env = g_getenv ("GTK_PATH");
        char *module_path;
        if (module_path_env) {
            module_path = g_build_filename (module_path_env,
                                            GTK4_IM_MODULEDIR "/libim-ibus.so",
                                            NULL);
        } else {
            module_path = g_strdup (GTK4_IM_MODULEDIR "/libim-ibus.so");
        }
        module = dlopen (module_path, RTLD_LAZY);
        g_free (module_path);
    } else if (!g_strcmp0 (im_module_type, "gtk2")) {
        module = dlopen (GTK2_IM_MODULEDIR "/im-ibus.so",
                         RTLD_LAZY);
    } else {
        module = dlopen (im_module_type, RTLD_LAZY);
    }
    if (!module) {
        g_warning ("Not found module: %s", dlerror ());
        return NULL;
    }

    im_get_context_id = dlsym (module, "im_get_context_id");
    if (!im_get_context_id) {
        g_warning ("Not found im_get_context_id: %s", dlerror ());
        dlclose (module);
        return NULL;
    }

    im_context_id = strdup (im_get_context_id (&argc, &argv));
    dlclose (module);
    return im_context_id;
}

#undef DEFAULT_IM_MODULE_TYPE

