#include "follow_camera_position_update.h"

#include "btdatecheck.h"
#include "btglm.h"
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

        auto& cam{ renderer.get_main_camera() };
        cam.set_follow_orbit_cam_offset_distance(follow_ref.orbit_cam_offset_distance);  // @CHECK: needed????

        vec3 follow_pos_f;
        btglm_rvec3_to_vec3(follow_pos.raw, follow_pos_f);
        cam.set_follow_orbit_follow_pos(follow_pos_f);
    }
}
