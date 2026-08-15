#include "rail_line_rider_update.h"

#include "btglm.h"
#include "game_system_logic/component/physics_object_settings.h"
#include "game_system_logic/component/rail_line.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "service_finder/service_finder.h"


void BT::system::rail_line_rider_update()
{
    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };

    auto view{ reg.view<component::Rail_line_rider>() };

    for (auto&& [ent, line_rider] : view->each())
    {
        auto const& rail_line{ reg.get<component::Rail_line>(
            entity_container.find_entity(line_rider.riding_line_uuid)) };

        // Modulo the distance.
        double_t pos_mod{ line_rider.line_position };
        while (pos_mod >= rail_line.built_length)
        {
            pos_mod -= rail_line.built_length;
        }

        // Find transform at position in build line.
        uint32_t built_part_idx{ 0 };
        float_t leftover_length = pos_mod;

        vec3 place_pos = GLM_VEC3_ZERO_INIT;
        float_t place_angle = 0;

        auto* build_code{ &component::Rail_line::s_build_code_to_info_map.at(
            rail_line.built_ctor_code.at(built_part_idx)) };
        while (leftover_length >= build_code->length)
        {
            // Move to try next build code.
            build_code->advance_place_transform(place_pos, place_angle, true);

            leftover_length -= build_code->length;

            built_part_idx++;
            build_code = &component::Rail_line::s_build_code_to_info_map.at(
                rail_line.built_ctor_code.at(built_part_idx));
        }

        auto transform{ build_code->calculate_transform(place_pos, place_angle, leftover_length) };

        // Build real transform.
        rvec3s transform_rpos{
            .x = transform.position.x,
            .y = transform.position.y,
            .z = transform.position.z,
        };

        versors transform_rot;
        glm_euler_zyx_quat(vec3{ transform.angle_x, transform.angle_y, transform.angle_z },
                           transform_rot.raw);

        // Update transform.
        component::submit_transform_change_no_scale_helper(reg,
                                                           ent,
                                                           transform_rpos,
                                                           transform_rot);
        component::try_set_physics_object_transform_helper(reg,
                                                           ent,
                                                           transform_rpos,
                                                           transform_rot);
    }
}
