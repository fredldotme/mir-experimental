/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/server_configuration.h"

#include "mir/compositor/buffer_allocation_strategy.h"
#include "mir/frontend/protobuf_asio_communicator.h"
#include "mir/frontend/application_proxy.h"
#include "mir/graphics/renderer.h"
#include "mir/graphics/platform.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/compositor/buffer_bundle_manager.h"
#include "mir/compositor/buffer_ipc_package.h"
#include "mir/compositor/double_buffer_allocation_strategy.h"
#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir/surfaces/surface_controller.h"
#include "mir/surfaces/surface_stack.h"


namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace ms = mir::surfaces;

namespace
{

// TODO replace with a real renderer appropriate to the platform default
class StubRenderer : public mg::Renderer
{
public:
    virtual void render(mg::Renderable&)
    {
    }
};

class StubBuffer : public mc::Buffer
{
    geom::Size size() const { return geom::Size(); }

    geom::Stride stride() const { return geom::Stride(); }

    geom::PixelFormat pixel_format() const { return geom::PixelFormat(); }

    std::shared_ptr<mc::BufferIPCPackage> get_ipc_package() const { return std::make_shared<mc::BufferIPCPackage>(); }

    void bind_to_texture() {}

};

// TODO replace with a real buffer allocator appropriate to the platform default
class StubGraphicBufferAllocator : public mc::GraphicBufferAllocator
{
 public:
    std::unique_ptr<mc::Buffer> alloc_buffer(geom::Size, geom::PixelFormat)
    {
        return std::unique_ptr<mc::Buffer>(new StubBuffer());
    }
};

class DefaultIpcFactory : public mf::ProtobufIpcFactory
{
public:
    explicit DefaultIpcFactory(
        std::shared_ptr<ms::ApplicationSurfaceOrganiser> const& surface_organiser,
        std::shared_ptr<mg::Platform> const& graphics_platform) :
        surface_organiser(surface_organiser),
        graphics_platform(graphics_platform)
    {
    }

private:
    std::shared_ptr<ms::ApplicationSurfaceOrganiser> surface_organiser;
    std::shared_ptr<mg::Platform> const graphics_platform;

    virtual std::shared_ptr<mir::protobuf::DisplayServer> make_ipc_server()
    {
        return std::make_shared<mf::ApplicationProxy>(surface_organiser, graphics_platform);
    }
};

// This (with make_ipc_factory()) builds a large chunk of the "inner" system.
// We may want to move this out of "configuration" someday
struct Surfaces :
    public mc::BufferBundleManager,
    public ms::SurfaceStack,
    public ms::SurfaceController
{
    Surfaces(std::shared_ptr<mc::BufferAllocationStrategy> const& strategy) :
        mc::BufferBundleManager(strategy),
        ms::SurfaceStack(this),
        ms::SurfaceController(this) {}
};
}

mir::DefaultServerConfiguration::DefaultServerConfiguration(std::string const& socket_file) :
socket_file(socket_file)
{
}

std::shared_ptr<mg::Platform> mir::DefaultServerConfiguration::make_graphics_platform()
{
    if (!graphics_platform)
    {
        // TODO I doubt we need the extra level of indirection provided by
        // mg::create_platform() - we just need to move the implementation
        // of DefaultServerConfiguration::make_graphics_platform() to the
        // graphics libraries.
        graphics_platform = mg::create_platform();
    }

    return graphics_platform;
}

std::shared_ptr<mc::GraphicBufferAllocator> mir::DefaultServerConfiguration::make_graphic_buffer_allocator()
{
//    return make_graphics_platform()->create_buffer_allocator();
    return std::make_shared<StubGraphicBufferAllocator>();
}

std::shared_ptr<mc::BufferAllocationStrategy> mir::DefaultServerConfiguration::make_buffer_allocation_strategy()
{
    auto graphic_buffer_allocator = make_graphic_buffer_allocator();
    return std::make_shared<mc::DoubleBufferAllocationStrategy>(graphic_buffer_allocator);
}

std::shared_ptr<mg::Renderer> mir::DefaultServerConfiguration::make_renderer()
{
    return std::make_shared<StubRenderer>();
}

std::shared_ptr<mir::frontend::ProtobufIpcFactory>
mir::DefaultServerConfiguration::make_ipc_factory(
    std::shared_ptr<compositor::BufferAllocationStrategy> const& buffer_allocation_strategy)
{
    auto surface_organiser = std::make_shared<Surfaces>(
        buffer_allocation_strategy);
    return std::make_shared<DefaultIpcFactory>(surface_organiser, make_graphics_platform());
}

std::shared_ptr<mf::Communicator>
mir::DefaultServerConfiguration::make_communicator(
    std::shared_ptr<compositor::BufferAllocationStrategy> const& buffer_allocation_strategy)
{
    return std::make_shared<mf::ProtobufAsioCommunicator>(
        socket_file, make_ipc_factory(buffer_allocation_strategy));
}
