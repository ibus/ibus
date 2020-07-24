/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */

#include <string.h>
#include <ibus.h>
#include "config.h"

#define GCONF_PREFIX "/desktop/ibus"

struct _IBusConfigGConf {
    IBusConfigService parent;
    GConfClient *client;
};

struct _IBusConfigGConfClass {
    IBusConfigServiceClass parent;

};

/* functions prototype */
static void         ibus_config_gconf_class_init    (IBusConfigGConfClass   *class);
static void         ibus_config_gconf_init          (IBusConfigGConf        *config);
static void         ibus_config_gconf_destroy       (IBusConfigGConf        *config);
static gboolean     ibus_config_gconf_set_value     (IBusConfigService      *config,
                                                     const gchar            *section,
                                                     const gchar            *name,
                                                     GVariant               *value,
                                                     GError                **error);
static GVariant    *ibus_config_gconf_get_value     (IBusConfigService      *config,
                                                     const gchar            *section,
                                                     const gchar            *name,
                                                     GError                **error);
static GVariant    *ibus_config_gconf_get_values    (IBusConfigService      *config,
                                                     const gchar            *section,
                                                     GError                **error);
static gboolean     ibus_config_gconf_unset_value   (IBusConfigService      *config,
                                                     const gchar            *section,
                                                     const gchar            *name,
                                                     GError                **error);
static GConfValue   *_to_gconf_value                (GVariant               *value);
static GVariant     *_from_gconf_value              (const GConfValue       *gvalue);

G_DEFINE_TYPE (IBusConfigGConf, ibus_config_gconf, IBUS_TYPE_CONFIG_SERVICE)

static void
ibus_config_gconf_class_init (IBusConfigGConfClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (class);

    IBUS_OBJECT_CLASS (object_class)->destroy = (IBusObjectDestroyFunc) ibus_config_gconf_destroy;
    IBUS_CONFIG_SERVICE_CLASS (object_class)->set_value   = ibus_config_gconf_set_value;
    IBUS_CONFIG_SERVICE_CLASS (object_class)->get_value   = ibus_config_gconf_get_value;
    IBUS_CONFIG_SERVICE_CLASS (object_class)->get_values  = ibus_config_gconf_get_values;
    IBUS_CONFIG_SERVICE_CLASS (object_class)->unset_value = ibus_config_gconf_unset_value;
}

static void
_value_changed_cb (GConfClient     *client,
                   const gchar     *key,
                   GConfValue      *value,
                   IBusConfigGConf *config)
{
    gchar *p, *section, *name;

    g_return_if_fail (key != NULL);

    p = g_strdup (key);
    section = p + sizeof (GCONF_PREFIX);
    name = rindex (p, '/') + 1;
    *(name - 1) = '\0';
    
    GVariant *variant;
    if (value) {
        variant = _from_gconf_value (value);
    }
    else {
        /* Use a empty typle for a unset value */
        variant = g_variant_new_tuple (NULL, 0);
    }
    g_return_if_fail (variant != NULL);
    ibus_config_service_value_changed ((IBusConfigService *) config,
                                       section,
                                       name,
                                       variant);
    g_variant_unref (variant);
    g_free (p);
}

static void
ibus_config_gconf_init (IBusConfigGConf *config)
{
    config->client = gconf_client_get_default ();
    gconf_client_add_dir (config->client,
                          GCONF_PREFIX,
                          GCONF_CLIENT_PRELOAD_RECURSIVE,
                          NULL);
    g_signal_connect (config->client, "value-changed", G_CALLBACK (_value_changed_cb), config);
}

static void
ibus_config_gconf_destroy (IBusConfigGConf *config)
{
    if (config->client) {
        g_signal_handlers_disconnect_by_func (config->client, G_CALLBACK (_value_changed_cb), config);
        g_object_unref (config->client);
        config->client = NULL;
    }

    IBUS_OBJECT_CLASS (ibus_config_gconf_parent_class)->destroy ((IBusObject *)config);
}

static GConfValue *
_to_gconf_value (GVariant *value)
{
    GConfValue *gv = NULL;

    switch (g_variant_classify (value)) {
    case G_VARIANT_CLASS_STRING:
        {
            gv = gconf_value_new (GCONF_VALUE_STRING);
            gconf_value_set_string (gv, g_variant_get_string (value, NULL));
        }
        break;
    case G_VARIANT_CLASS_INT32:
        {
            gv = gconf_value_new (GCONF_VALUE_INT);
            gconf_value_set_int (gv, g_variant_get_int32 (value));
        }
        break;
    case G_VARIANT_CLASS_BOOLEAN:
        {
            gv = gconf_value_new (GCONF_VALUE_BOOL);
            gconf_value_set_bool (gv, g_variant_get_boolean (value));
        }
        break;
    case G_VARIANT_CLASS_DOUBLE:
        {
            gv = gconf_value_new (GCONF_VALUE_FLOAT);
            gconf_value_set_float (gv, g_variant_get_double (value));
        }
        break;
    case G_VARIANT_CLASS_ARRAY:
        {
            const GVariantType *element_type = g_variant_type_element (g_variant_get_type (value));

            GConfValueType type = GCONF_VALUE_INVALID;
            if (g_variant_type_equal (element_type, G_VARIANT_TYPE_STRING))
                type = GCONF_VALUE_STRING;
            else if (g_variant_type_equal (element_type, G_VARIANT_TYPE_INT32))
                type = GCONF_VALUE_INT;
            else if (g_variant_type_equal (element_type, G_VARIANT_TYPE_BOOLEAN))
                type = GCONF_VALUE_BOOL;
            else if (g_variant_type_equal (element_type, G_VARIANT_TYPE_DOUBLE))
                type = GCONF_VALUE_FLOAT;
            else
                g_return_val_if_reached (NULL);

            gv = gconf_value_new (GCONF_VALUE_LIST);
            gconf_value_set_list_type (gv, type);

            GSList *elements = NULL;
            GVariantIter iter;
            GVariant *child;
            g_variant_iter_init (&iter, value);
            while ((child = g_variant_iter_next_value (&iter)) != NULL) {
                elements = g_slist_append (elements, _to_gconf_value (child));
                g_variant_unref (child);
            }
            gconf_value_set_list_nocopy (gv, elements);
        }
        break;
    default:
        g_return_val_if_reached (NULL);
    }

    return gv;
}

static GVariant *
_from_gconf_value (const GConfValue *gv)
{
    g_assert (gv != NULL);

    switch (gv->type) {
    case GCONF_VALUE_STRING:
        return g_variant_new_string (gconf_value_get_string (gv));
    case GCONF_VALUE_INT:
        return g_variant_new_int32 (gconf_value_get_int (gv));
    case GCONF_VALUE_FLOAT:
        return g_variant_new_double (gconf_value_get_float (gv));
    case GCONF_VALUE_BOOL:
        return g_variant_new_boolean (gconf_value_get_bool (gv));
    case GCONF_VALUE_LIST:
        {
            GVariantBuilder builder;
            switch (gconf_value_get_list_type (gv)) {
            case GCONF_VALUE_STRING:
                g_variant_builder_init (&builder, G_VARIANT_TYPE("as")); break;
            case GCONF_VALUE_INT:
                g_variant_builder_init (&builder, G_VARIANT_TYPE("ai")); break;
            case GCONF_VALUE_FLOAT:
                g_variant_builder_init (&builder, G_VARIANT_TYPE("ad")); break;
            case GCONF_VALUE_BOOL:
                g_variant_builder_init (&builder, G_VARIANT_TYPE("ab")); break;
            default:
                g_assert_not_reached ();
            }

            GSList *list = gconf_value_get_list (gv);
            GSList *p = list;
            while (p != NULL) {
                switch (gconf_value_get_list_type (gv)) {
                case GCONF_VALUE_STRING:
                    g_variant_builder_add (&builder, "s", gconf_value_get_string ((GConfValue *)p->data));
                    break;
                case GCONF_VALUE_INT:
                    g_variant_builder_add (&builder, "i", gconf_value_get_int ((GConfValue *)p->data));
                    break;
                case GCONF_VALUE_FLOAT:
                    g_variant_builder_add (&builder, "d", gconf_value_get_float ((GConfValue *)p->data));
                    break;
                case GCONF_VALUE_BOOL:
                    g_variant_builder_add (&builder, "b", gconf_value_get_bool ((GConfValue *)p->data));
                    break;
                default:
                    g_assert_not_reached ();
                }
                p = p->next;
            }
            return g_variant_builder_end (&builder);
        }
    default:
        g_assert_not_reached ();
    }
}


static gboolean
ibus_config_gconf_set_value (IBusConfigService      *config,
                             const gchar            *section,
                             const gchar            *name,
                             GVariant               *value,
                             GError                **error)
{
    gchar *key;
    GConfValue *gv;

    gv = _to_gconf_value (value);
    if (gv == NULL) {
        gchar *str = g_variant_print (value, TRUE);
        g_set_error (error,
                     G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                     "Can not set config value [%s:%s] to %s.",
                     section, name, str);
        g_free (str);
        return FALSE;
    }
    key = g_strdup_printf (GCONF_PREFIX"/%s/%s", section, name);
    gconf_client_set (((IBusConfigGConf *)config)->client, key, gv, error);

    g_free (key);
    gconf_value_free (gv);

    if (*error != NULL) {
        return FALSE;
    }
    return TRUE;
}

static GVariant *
ibus_config_gconf_get_value (IBusConfigService      *config,
                             const gchar            *section,
                             const gchar            *name,
                             GError                **error)
{
    gchar *key = g_strdup_printf (GCONF_PREFIX"/%s/%s", section, name);

    GConfValue *gv = gconf_client_get (((IBusConfigGConf *) config)->client, key, NULL);

    g_free (key);

    if (gv == NULL) {
        g_set_error (error,
                     G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                     "Config value [%s:%s] does not exist.", section, name);
        return NULL;
    }

    GVariant *variant = _from_gconf_value (gv);
    gconf_value_free (gv);

    return g_variant_ref_sink (variant);
}

static GVariant *
ibus_config_gconf_get_values (IBusConfigService      *config,
                              const gchar            *section,
                              GError                **error)
{
    gchar *dir = g_strdup_printf (GCONF_PREFIX"/%s", section);
    gint len = strlen(dir) + 1;
    GSList *entries = gconf_client_all_entries (((IBusConfigGConf *) config)->client, dir, NULL);
    g_free (dir);

    GSList *p;
    GVariantBuilder builder;

    g_variant_builder_init (&builder, G_VARIANT_TYPE ("a{sv}"));
    for (p = entries; p != NULL; p = p->next) {
        GConfEntry *entry = (GConfEntry *)p->data;
        if (entry->key != NULL && entry->value != NULL) {
            const gchar *name = entry->key + len;
            GVariant *value = _from_gconf_value (entry->value);
            g_variant_builder_add (&builder, "{sv}", name, value);
        }
        gconf_entry_free (entry);
    }
    g_slist_free (entries);

    return g_variant_builder_end (&builder);
}

static gboolean
ibus_config_gconf_unset_value (IBusConfigService      *config,
                               const gchar            *section,
                               const gchar            *name,
                               GError                **error)
{
    gchar *key = g_strdup_printf (GCONF_PREFIX"/%s/%s", section, name);

    gboolean retval = gconf_client_unset (((IBusConfigGConf *)config)->client, key, error);
    g_free (key);

    return retval;
}

IBusConfigGConf *
ibus_config_gconf_new (GDBusConnection *connection)
{
    IBusConfigGConf *config;
    config = (IBusConfigGConf *) g_object_new (IBUS_TYPE_CONFIG_GCONF,
                                               "object-path", IBUS_PATH_CONFIG,
                                               "connection", connection,
                                               NULL);
    return config;
}
