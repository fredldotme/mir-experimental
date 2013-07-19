/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include "mir/compositor/renderer.h"

#include "mir_test_framework/display_server_test_fixture.h"
#include "mir_test/fake_event_hub_input_configuration.h"

#include <gtest/gtest.h>

#include <thread>
#include <cstdio>
#include <fcntl.h>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace mia = mir::input::android;
namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;
namespace ms = mir::surfaces;

namespace
{

char const* const mir_test_socket = mtf::test_socket_file().c_str();

class NullRenderer : public mc::Renderer
{
public:
    virtual void render(std::function<void(std::shared_ptr<void> const&)>,
                                   mc::CompositingCriteria const&, ms::BufferStream&)
    {
        /* 
         * Do nothing, so that the Renderable's buffers are not consumed
         * by the server, thus causing the client to block when asking for
         * the second buffer (assuming double-buffering).
         */
        std::this_thread::yield();
    }

    void clear() {}
};

void null_surface_callback(MirSurface*, void*)
{
}

class Flag
{
public:
    explicit Flag(std::string const& flag_file)
        : flag_file{flag_file}
    {
        std::remove(flag_file.c_str());
    }

    void set()
    {
        close(open(flag_file.c_str(), O_CREAT, S_IWUSR | S_IRUSR));
    }

    bool is_set()
    {
        int fd = -1;
        if ((fd = open(flag_file.c_str(), O_RDONLY, S_IWUSR | S_IRUSR)) != -1)
        {
            close(fd);
            return true;
        }
        return false;
    }

    void wait()
    {
        while (!is_set())
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

private:
    std::string const flag_file;
};

}

TEST_F(BespokeDisplayServerTestFixture, server_can_shut_down_when_clients_are_blocked)
{
    Flag next_buffer_done1{"next_buffer_done1_c5d49978.tmp"};
    Flag next_buffer_done2{"next_buffer_done2_c5d49978.tmp"};
    Flag next_buffer_done3{"next_buffer_done3_c5d49978.tmp"};
    Flag server_done{"server_done_c5d49978.tmp"};

    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<mc::Renderer> the_renderer() override
        {
            return renderer([] { return std::make_shared<NullRenderer>(); });
        }
    } server_config;

    launch_server_process(server_config);

    struct ClientConfig : TestingClientConfiguration
    {
        ClientConfig(Flag& next_buffer_done,
                     Flag& server_done)
            : next_buffer_done(next_buffer_done),
              server_done(server_done)
        {
        }

        void exec()
        {
            MirConnection* connection = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);

            ASSERT_TRUE(connection != NULL);

            MirSurfaceParameters const request_params =
            {
                __PRETTY_FUNCTION__,
                640, 480,
                mir_pixel_format_abgr_8888,
                mir_buffer_usage_hardware
            };

            MirSurface* surf = mir_connection_create_surface_sync(connection, &request_params);

            /* Ask for the first buffer (should succeed) */
            mir_surface_swap_buffers_sync(surf);
            /* Ask for the first second buffer (should block) */
            mir_surface_swap_buffers(surf, null_surface_callback, nullptr);

            next_buffer_done.set();
            server_done.wait();

            /* 
             * TODO: Releasing the connection to a shut down server blocks
             * the client. We should handle unexpected server shutdown more
             * gracefully on the client side.
             *
             * mir_connection_release(connection);
             */
        }

        Flag& next_buffer_done;
        Flag& server_done;
    };

    ClientConfig client_config1{next_buffer_done1, server_done};
    ClientConfig client_config2{next_buffer_done2, server_done};
    ClientConfig client_config3{next_buffer_done3, server_done};

    launch_client_process(client_config1);
    launch_client_process(client_config2);
    launch_client_process(client_config3);

    run_in_test_process([&]
    {
        /* Wait until the clients are blocked on getting the second buffer */
        next_buffer_done1.wait();
        next_buffer_done2.wait();
        next_buffer_done3.wait();

        /* Shutting down the server should not block */
        shutdown_server_process();

        /* Notify the clients that we are done (we only need to set the flag once) */
        server_done.set();
    });
}

TEST_F(BespokeDisplayServerTestFixture, server_releases_resources_on_shutdown_with_connected_clients)
{
    Flag surface_created1{"surface_created1_7e9c69fc.tmp"};
    Flag surface_created2{"surface_created2_7e9c69fc.tmp"};
    Flag surface_created3{"surface_created3_7e9c69fc.tmp"};
    Flag server_done{"server_done_7e9c69fc.tmp"};
    Flag resources_freed_success{"resources_free_success_7e9c69fc.tmp"};
    Flag resources_freed_failure{"resources_free_failure_7e9c69fc.tmp"};

    /* Use the real input manager, but with a fake event hub */
    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<mi::InputConfiguration> the_input_configuration() override
        {
            if (!input_configuration)
            {
                input_configuration =
                    std::make_shared<mtd::FakeEventHubInputConfiguration>(
                        std::initializer_list<std::shared_ptr<mi::EventFilter> const>{},
                        the_input_region(),
                        std::shared_ptr<mi::CursorListener>(),
                        the_input_report());
            }

            return input_configuration;
        }

        std::shared_ptr<mi::InputManager> the_input_manager() override
        {
            return DefaultServerConfiguration::the_input_manager();
        }

        std::shared_ptr<mir::shell::InputTargeter> the_input_targeter() override
        {
            return DefaultServerConfiguration::the_input_targeter();
        }
        std::shared_ptr<mir::surfaces::InputRegistrar> the_input_registrar() override
        {
            return DefaultServerConfiguration::the_input_registrar();
        }

        std::shared_ptr<mi::InputConfiguration> input_configuration;
    };

    auto server_config = std::make_shared<ServerConfig>();
    launch_server_process(*server_config);

    struct ClientConfig : TestingClientConfiguration
    {
        ClientConfig(Flag& surface_created,
                     Flag& server_done)
            : surface_created(surface_created),
              server_done(server_done)
        {
        }

        void exec()
        {
            MirConnection* connection = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);

            ASSERT_TRUE(connection != NULL);

            MirSurfaceParameters const request_params =
            {
                __PRETTY_FUNCTION__,
                640, 480,
                mir_pixel_format_abgr_8888,
                mir_buffer_usage_hardware
            };

            mir_connection_create_surface_sync(connection, &request_params);

            surface_created.set();
            server_done.wait();
        }

        Flag& surface_created;
        Flag& server_done;
    };

    ClientConfig client_config1{surface_created1, server_done};
    ClientConfig client_config2{surface_created2, server_done};
    ClientConfig client_config3{surface_created3, server_done};

    launch_client_process(client_config1);
    launch_client_process(client_config2);
    launch_client_process(client_config3);

    run_in_test_process([&]
    {
        /* Wait until the clients have created a surface */
        surface_created1.wait();
        surface_created2.wait();
        surface_created3.wait();

        /* Shut down the server */
        shutdown_server_process();

        /* Wait until we have checked the resources in the server process */
        while (!resources_freed_failure.is_set() && !resources_freed_success.is_set())
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        /* Fail if the resources have not been freed */
        if (resources_freed_failure.is_set())
            ADD_FAILURE();

        /* Notify the clients that we are done (we only need to set the flag once) */
        server_done.set();
    });

    /*
     * Check that all resources are freed after destroying the server config.
     * Note that these checks are run multiple times: in the server process,
     * in each of the client processes and in the test process. We only care about
     * the results in the server process (in the other cases the checks will
     * succeed anyway, since we are not using the config object).
     */
    std::weak_ptr<mir::graphics::Display> display = server_config->the_display();
    std::weak_ptr<mir::compositor::Compositor> compositor = server_config->the_compositor();
    std::weak_ptr<mir::frontend::Communicator> communicator = server_config->the_communicator();
    std::weak_ptr<mir::input::InputManager> input_manager = server_config->the_input_manager();

    server_config.reset();

    EXPECT_EQ(0, display.use_count());
    EXPECT_EQ(0, compositor.use_count());
    EXPECT_EQ(0, communicator.use_count());
    EXPECT_EQ(0, input_manager.use_count());

    if (display.use_count() != 0 ||
        compositor.use_count() != 0 ||
        communicator.use_count() != 0 ||
        input_manager.use_count() != 0)
    {
        resources_freed_failure.set();
    }
    else
    {
        resources_freed_success.set();
    }
}
