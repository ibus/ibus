/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include <sys/socket.h>
#include <unistd.h>

#include "component.h"
#include "connection.h"
#include "engineproxy.h"
#include "factoryproxy.h"
#include "global.h"

/* bus_engine_proxy_new() for an engine whose factory is not running yet
 * waits for the factory, a timeout and a cancellation at the same time.
 * Exactly one of them may complete the request, whatever order the main
 * loop dispatches them in. */
typedef struct {
    int              peer_fd;
    GDBusConnection *dbus_connection;
    BusConnection   *connection;
    BusFactoryProxy *factory;
    BusComponent    *component;
    IBusEngineDesc  *desc;
    GCancellable    *cancellable;
    guint            n_callbacks;
    GError          *error;
} Fixture;

static void
fixture_set_up (Fixture       *fixture,
                gconstpointer  user_data)
{
    GError *error = NULL;
    int fds[2];

    g_assert_cmpint (socketpair (AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, fds),
                     ==, 0);
    fixture->peer_fd = fds[1];

    GSocket *socket = g_socket_new_from_fd (fds[0], &error);
    g_assert_no_error (error);
    GSocketConnection *stream =
            g_socket_connection_factory_create_connection (socket);
    fixture->dbus_connection =
            g_dbus_connection_new_sync (G_IO_STREAM (stream),
                                        NULL,
                                        G_DBUS_CONNECTION_FLAGS_NONE,
                                        NULL,
                                        NULL,
                                        &error);
    g_assert_no_error (error);
    g_object_unref (stream);
    g_object_unref (socket);

    fixture->connection = bus_connection_new (fixture->dbus_connection);
    g_object_ref_sink (fixture->connection);
    fixture->factory = bus_factory_proxy_new (fixture->connection);
    g_assert_nonnull (fixture->factory);

    IBusComponent *component =
            ibus_component_new ("org.freedesktop.IBus.TestEngineProxy",
                                "", "", "", "", "",
                                "/bin/sh -c exit",
                                "");
    g_object_ref_sink (component);
    fixture->desc = ibus_engine_desc_new ("test-engine-proxy",
                                          "", "", "", "", "", "", "");
    ibus_component_add_engine (component, fixture->desc);
    g_object_ref (fixture->desc);
    fixture->component = bus_component_new (component, NULL);
    g_object_ref_sink (fixture->component);
    g_object_unref (component);

    fixture->cancellable = g_cancellable_new ();
}

static void
fixture_tear_down (Fixture       *fixture,
                   gconstpointer  user_data)
{
    /* Reap the engine process started by bus_engine_proxy_new(). */
    while (bus_component_is_running (fixture->component))
        g_main_context_iteration (NULL, TRUE);

    g_clear_error (&fixture->error);
    g_object_unref (fixture->cancellable);
    /* The factory goes away before its component, as when an engine exits. */
    ibus_proxy_destroy ((IBusProxy *) fixture->factory);
    g_object_unref (fixture->factory);
    ibus_object_destroy ((IBusObject *) fixture->component);
    g_object_unref (fixture->component);
    g_object_unref (fixture->desc);
    ibus_object_destroy ((IBusObject *) fixture->connection);
    g_object_unref (fixture->connection);
    g_dbus_connection_close_sync (fixture->dbus_connection, NULL, NULL);
    g_object_unref (fixture->dbus_connection);
    close (fixture->peer_fd);
}

static void
engine_proxy_new_cb (GObject      *source_object,
                     GAsyncResult *res,
                     Fixture      *fixture)
{
    GError *error = NULL;
    BusEngineProxy *engine = bus_engine_proxy_new_finish (res, &error);

    g_assert_null (engine);
    g_assert_nonnull (error);
    fixture->n_callbacks++;
    g_clear_error (&fixture->error);
    fixture->error = error;
}

/* Runs the main loop until the request completes, then until nothing is
 * pending, so that a second completion of the same request would show up. */
static void
wait_for_completion (Fixture *fixture)
{
    while (fixture->n_callbacks == 0)
        g_main_context_iteration (NULL, TRUE);
    while (g_main_context_iteration (NULL, FALSE))
        ;
}

static gboolean
cancel_idle_cb (GCancellable *cancellable)
{
    g_cancellable_cancel (cancellable);
    return G_SOURCE_REMOVE;
}

/**
 * test_cancel_then_factory:
 * @fixture: A #Fixture
 * @user_data: A user data
 *
 * This scenario details a use-after-free bug (issue #2957) caused by
 * premature task/data deallocation during async cancellation or timeout
 * handling in engineproxy.c.
 *
 * Call bus_engine_proxy_new() without #BusFactory and this case is the similar
 * logic with the case when ibus-daemon receives "SetGlobalEngine" D-Bus
 * method.
 *
 * Scenario 1: Premature Deallocation via Cancellation (#2957)
 * 1. bus_engine_proxy_new() connects cancelled_cb() and notify_factory_cb(),
 *    allocating `data` and `data->task`.
 * 2. g_cancellable_cancel() triggers engineproxy.c:cancelled_cb().
 * 3. engineproxy.c:cancelled_cb() schedules cancelled_idle_cb() as a GLIB
 *    idle source.
 * 4. bus_component_set_factory() triggers engineproxy.c:notify_factory_cb(),
 *    which connects create_engine_ready_cb() to `data->factory` using the
 *    "CreateEngine" D-Bus method.
 * 5. engineproxy.c:cancelled_idle_cb() sets the "Operation was cancelled"
 *    #GError and frees `data->task` and `data`.
 * 6. The "CreateEngine" D-Bus method is cancelled with the
 *    "Operation was cancelled" #GError and create_engine_ready_cb() is called.
 * 7. create_engine_ready_cb() attempts to assign the error to `data->task`,
 *    leading to a use-after-free because `data->task` was already freed
 *    in Step 5.
 */
static void
test_cancel_then_factory (Fixture       *fixture,
                          gconstpointer  user_data)
{
    bus_engine_proxy_new (fixture->desc,
                          g_gdbus_timeout,
                          fixture->cancellable,
                          (GAsyncReadyCallback) engine_proxy_new_cb,
                          fixture);

    /* The factory registers after the cancellation, before the main loop
     * runs again. */
    g_cancellable_cancel (fixture->cancellable);
    bus_component_set_factory (fixture->component, fixture->factory);

    wait_for_completion (fixture);
    g_assert_cmpuint (fixture->n_callbacks, ==, 1);
    g_assert_error (fixture->error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
}

/**
 * test_cancel_then_timeout:
 * @fixture: A #Fixture
 * @user_data: A user data
 *
 * The cancellation and the expired timeout have the same priority and
 * are dispatched in the same main loop iteration, the cancellation
 * first.
 *
 * ibus-daemon context: Triggering Rapid Cancellations
 * 1. gdbusconnection.c:call_in_idle_cb() is called with G_PRIORITY_DEFAULT.
 * 2. gdbusconnection.c:call_in_idle_cb() calls
 *    ibusimpl.c:bus_ibus_impl_service_method_call() ->
 *    _ibus_set_global_engine() -> bus_input_context_set_engine_by_desc()
 *    -> bus_engine_proxy_new() with `@timeout=g_gdbus_timeout` in
 *    ibus-daemon.
 * 3. If the "SetGlobalEngine" D-Bus method is called twice in rapid
 *    succession, bus_input_context_set_engine_by_desc() calls
 *    cancel_set_engine_by_desc().
 * 4. cancel_set_engine_by_desc() calls g_cancellable_cancel().
 *
 * Scenario 2: Priority Inversion & Timeout Race Condition
 * 1. cancel_idle_cb() calls g_cancellable_cancel() (matching the behavior
 *    of inputcontext.c:cancel_set_engine_by_desc() in ibus-daemon).
 * 2. bus_engine_proxy_new() connects cancelled_cb() and timeout_cb()
 *    with `@timeout=0`, initializing `data->task`.
 * 3. g_cancellable_cancel() calls engineproxy.c:cancelled_cb().
 * 4. engineproxy.c:cancelled_cb() schedules cancelled_idle_cb() at a HIGH
 *    priority GLib idle handler.
 * 5. engineproxy.c:timeout_cb() is called before cancelled_idle_cb()
 *    despite the lower priority, freeing `data->task` and `data`.
 * 6. engineproxy.c:cancelled_idle_cb() is called subsequently.
 * 7. cancelled_idle_cb() attempts to set an error on `data->task`,
 *    resulting in a use-after-free crash on the freed pointer.
 */
static void
test_cancel_then_timeout (Fixture       *fixture,
                          gconstpointer  user_data)
{
    g_idle_add_full (G_PRIORITY_DEFAULT,
                     (GSourceFunc) cancel_idle_cb,
                     fixture->cancellable,
                     NULL);
    bus_engine_proxy_new (fixture->desc,
                          0,
                          fixture->cancellable,
                          (GAsyncReadyCallback) engine_proxy_new_cb,
                          fixture);

    wait_for_completion (fixture);
    g_assert_cmpuint (fixture->n_callbacks, ==, 1);
    g_assert_error (fixture->error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
}

int
main (int argc, char *argv[])
{
    g_test_init (&argc, &argv, NULL);
    g_test_add ("/engine-proxy/cancel-then-factory",
                Fixture, NULL,
                fixture_set_up, test_cancel_then_factory, fixture_tear_down);
    g_test_add ("/engine-proxy/cancel-then-timeout",
                Fixture, NULL,
                fixture_set_up, test_cancel_then_timeout, fixture_tear_down);
    return g_test_run ();
}
