/*
 * Copyright © 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "test_window_manager_tools.h"

using namespace miral;
using namespace testing;
namespace mt = mir::test;

namespace
{
X const display_left{0};
Y const display_top{0};
Width const display_width{1280};
Height const display_height{720};

Rectangle const display_area{{display_left,  display_top},
                             {display_width, display_height}};

struct DepthLayer : TestWindowManagerTools, WithParamInterface<MirDepthLayer>
{
    void SetUp() override
    {
        basic_window_manager.add_display_for_testing(display_area);
        basic_window_manager.add_session(session);
    }

    auto create_window(mir::scene::SurfaceCreationParameters creation_parameters) -> Window
    {
        Window result;

        EXPECT_CALL(*window_manager_policy, advise_new_window(_))
            .WillOnce(
                Invoke(
                    [&result](WindowInfo const& window_info)
                        { result = window_info.window(); }));

        basic_window_manager.add_surface(session, creation_parameters, &create_surface);
        basic_window_manager.select_active_window(result);

        // Clear the expectations used to capture parent & child
        Mock::VerifyAndClearExpectations(window_manager_policy);

        return result;
    }
};
}

TEST_P(DepthLayer, creation_depth_layer_is_applied)
{
    MirDepthLayer layer = GetParam();

    Window window;
    {
        mir::scene::SurfaceCreationParameters params;
        params.depth_layer = layer;
        window = create_window(params);
    }
    auto const& info = basic_window_manager.info_for(window);
    std::shared_ptr<mir::scene::Surface> surface = window;
    ASSERT_THAT(surface, NotNull());

    EXPECT_THAT(info.depth_layer(), Eq(layer));
    EXPECT_THAT(surface->depth_layer(), Eq(layer));
}

TEST_P(DepthLayer, modify_surface_updates_depth_layer)
{
    MirDepthLayer layer = GetParam();

    Window window;
    {
        mir::scene::SurfaceCreationParameters params;
        window = create_window(params);
    }
    auto const& info = basic_window_manager.info_for(window);
    std::shared_ptr<mir::scene::Surface> surface = window;
    ASSERT_THAT(surface, NotNull());

    EXPECT_THAT(surface->depth_layer(), Eq(mir_depth_layer_application));

    mir::shell::SurfaceSpecification modifications;
    modifications.depth_layer = layer;
    basic_window_manager.modify_surface(session, surface, modifications);

    EXPECT_THAT(info.depth_layer(), Eq(layer));
    EXPECT_THAT(surface->depth_layer(), Eq(layer));
}

TEST_P(DepthLayer, modify_window_updates_depth_layer)
{
    MirDepthLayer layer = GetParam();

    Window window;
    {
        mir::scene::SurfaceCreationParameters params;
        params.type = mir_window_type_normal;
        window = create_window(params);
    }
    auto& info = basic_window_manager.info_for(window);
    std::shared_ptr<mir::scene::Surface> surface = window;
    ASSERT_THAT(surface, NotNull());

    EXPECT_THAT(surface->depth_layer(), Eq(mir_depth_layer_application));

    WindowSpecification mods;
    mods.depth_layer() = layer;
    basic_window_manager.modify_window(info, mods);

    EXPECT_THAT(info.depth_layer(), Eq(layer));
    EXPECT_THAT(surface->depth_layer(), Eq(layer));
}

INSTANTIATE_TEST_CASE_P(DepthLayer, DepthLayer, ::testing::Values(
    mir_depth_layer_background,
    mir_depth_layer_below,
    mir_depth_layer_application,
    mir_depth_layer_always_on_top,
    mir_depth_layer_above,
    mir_depth_layer_overlay
));
