#include "update_camera_billboards.h"

#include "btglm.h"
#include "entt/entity/fwd.hpp"
#include "game_system_logic/component/billboard_to_camera.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "game_system_logic/world/world_properties.h"
#include "service_finder/service_finder.h"
#include "txp_renderer_public.h"

#include <cassert>
#include <tracy/Tracy.hpp>


void BT::system::update_camera_billboards()
{
    ZoneScoped;

    if (!service_finder::find_service<world::World_properties_container>()
             .get_data_handle()
             .is_simulation_running)
        return;

    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };

    auto view{ reg.view<component::Transform, component::Billboard_to_camera>() };

    bool first{ true };
    versors cam_view_inv_rot;

    for (auto const& [entity, _, billboard_to_camera] : view.each())
    {
        if (first)
        {
            // Calc rotation to billboard to camera.
            mat4 cam_view;
            glm_mat4_copy(const_cast<vec4*>(service_finder::find_service<TXP::Renderer>()
                                                .get_main_camera()
                                                .get_calculated_camera_matrices()
                                                .front()
                                                .view),
                          cam_view);
            glm_mat4_inv(cam_view, cam_view);

            glm_mat4_quat(cam_view, cam_view_inv_rot.raw);
        }

        // @THEA: this would make this have to wait a cycle before it updates... hmmm which is not ideal. idk, maybe i'll have to rethink how write-render transforms happens or smth!!! idk..

        // Calc billboard position offset.
        if (!billboard_to_camera.copy_position_entity.is_nil())
        {
            rvec3s pos =
                reg.get<component::Transform>(
                       entity_container.find_entity(billboard_to_camera.copy_position_entity))
                    .position;

            rvec3 offset;
            btglm_vec3_to_rvec3(billboard_to_camera.copy_position_offset.raw, offset);

            btglm_rvec3_add(pos.raw, offset, pos.raw);

            component::submit_transform_change_only_translation_helper(reg, entity, pos);
        }

        // Calc billboard rotation.
        mat4 extra_rot;
        glm_euler_zyx(billboard_to_camera.rotation_euler.raw, extra_rot);
        versor extra_rot_q;
        glm_mat4_quat(extra_rot, extra_rot_q);

        versors full_rot;
        glm_quat_mul(cam_view_inv_rot.raw, extra_rot_q, full_rot.raw);

        component::submit_transform_change_only_rotation_helper(reg, entity, full_rot);
    }
}
