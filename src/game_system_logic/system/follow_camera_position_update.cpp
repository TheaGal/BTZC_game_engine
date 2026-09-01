#include "follow_camera_position_update.h"

#include "btdatecheck.h"
#include "entt/entity/registry.hpp"
#include "game_system_logic/component/follow_camera.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "service_finder/service_finder.h"
#include "txp_renderer_public.h"


void BT::system::follow_camera_position_update()
{
    auto& reg{ service_finder::find_service<Entity_container>().get_ecs_registry() };
    auto& renderer{ service_finder::find_service<TXP::Renderer>() };

    // Use follow ref transform.
    auto view{ reg.view<component::Transform const, component::Follow_camera_follow_ref const>() };

    bool first{ true };
    for (auto&& [_, transform, follow_ref] : view.each())
    {
        assert(first);
        first = false;

        // Calc follow position.
        auto follow_pos = transform.position;
        follow_pos.y += follow_ref.follow_offset_y;

        date_deadline(2026, 10, 24);  // @TODO: in case if there's a world-streaming or chunking system, figure out more better way of going from real to float.
        renderer.get_main_camera().set_follow_orbit_follow_pos(
            vec3{ static_cast<float_t>(follow_pos.x),
                  static_cast<float_t>(follow_pos.y),
                  static_cast<float_t>(follow_pos.z) });
    }
}
