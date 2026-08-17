#include "rail_line_rider_update.h"

#include "btglm.h"
#include "entt/entity/fwd.hpp"
#include "game_system_logic/component/physics_object_settings.h"
#include "game_system_logic/component/rail_line.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "service_finder/service_finder.h"
#include "txp_renderer_public.h"

#include <cstdlib>


namespace
{

using namespace BT;

using Rail_position_transform = component::Rail_line::Build_code_info::Rail_position_transform;

void build_bogie_entities(entt::registry& reg,
                          entt::entity ent,
                          component::Rail_line_rider& line_rider)
{
    // Destroy prev created entities.
    for (auto sub_ent : line_rider.created_entities)
    {
        if (sub_ent == ent)
            continue;

        reg.destroy(sub_ent);
    }
    line_rider.created_entities.clear();

    // Create new entities.
    for (uint32_t i = 0; i < line_rider.bogie_positions.size(); i++)
    {
        auto& bogie_pos{ line_rider.bogie_positions[i] };

        // Use current entity as first entity if first bogie and 0 relative position.
        // If not, create a new entity.
        line_rider.created_entities.emplace_back(i == 0 && bogie_pos == 0 ? ent : reg.create());

        // Add model.
        auto& rend_obj_cfg = reg.emplace_or_replace<TXP::component::Render_object_config>(
            line_rider.created_entities.back());
        rend_obj_cfg.model_name = "unit_box";
    }

    // Recreate bogie springs.
    line_rider.bogie_springs.clear();
    line_rider.bogie_springs.reserve(line_rider.bogie_positions.size());

    for (auto& bogie_pos : line_rider.bogie_positions)
    {
        line_rider.bogie_springs.emplace_back(bogie_pos, 0.0f);
    }
}

Rail_position_transform calc_transform_on_rail_line(component::Rail_line const& rail_line,
                                                    float_t line_position)
{
    // Make sure the line position is already modulus-ized.
    assert(line_position < rail_line.built_length);

    // Find transform at position in build line.
    uint32_t built_part_idx{ 0 };
    float_t leftover_length = line_position;

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

    return build_code->calculate_transform(place_pos, place_angle, leftover_length);
}

using Bogie_spring_memory = component::Rail_line_rider::Bogie_spring_memory;

std::pair<Rail_position_transform, float_t> find_transform_approx(
    component::Rail_line const& rail_line,
    Rail_position_transform const& prev_transform,
    Bogie_spring_memory& bogie_spring,
    float_t signed_distance,
    float_t prev_line_pos)
{
    assert(signed_distance <= 0);
    float_t distance{ abs(signed_distance) };

    // Bounce against signed distance bound.
    if (bogie_spring.line_position - prev_line_pos > 0)
    {
        bogie_spring.line_position = prev_line_pos;
        bogie_spring.velocity *= -0.5f;
    }

    // Adjust spring velocity based off distance error.
    bogie_spring.line_position += bogie_spring.velocity;

    Rail_position_transform found_trans =
        calc_transform_on_rail_line(rail_line, bogie_spring.line_position);

    auto found_distance = glm_vec3_distance(found_trans.position.raw,
                                            const_cast<float_t*>(prev_transform.position.raw));

    bogie_spring.velocity = found_distance - distance;

    return { found_trans, bogie_spring.line_position };
}

} // namespace


void BT::system::rail_line_rider_update()
{
    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };

    auto view{ reg.view<component::Rail_line_rider>() };

    for (auto&& [ent, line_rider] : view->each())
    {
        // Ensure there are objects for each bogie to update.
        if (line_rider.bogie_positions.size() != line_rider.created_entities.size())
        {
            build_bogie_entities(reg, ent, line_rider);
        }

        // Ensure rail line is built before attempting to ride it.
        auto const& rail_line{ reg.get<component::Rail_line>(
            entity_container.find_entity(line_rider.riding_line_uuid)) };

        if (rail_line.built_length < 1e-6f)
        {
            BT_WARNF("Nothing is built wtf?!?!? \t%s()", __func__);
            continue;
        }

        // Modulo the distance.
        double_t pos_mod{ line_rider.line_position };
        while (pos_mod >= rail_line.built_length)
        {
            pos_mod -= rail_line.built_length;
        }

        // Find transform at position in build line.
        auto transform{ calc_transform_on_rail_line(rail_line, pos_mod) };

        // Build real transform.
        static auto const k_apply_transform_to_entity =
            [](entt::registry& reg, entt::entity ent, Rail_position_transform const& transform) {
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
            };

        k_apply_transform_to_entity(reg, ent, transform);

        // Update transforms of bogies.
        Rail_position_transform prev_transform = transform;
        float_t prev_line_position = pos_mod;

        for (size_t i = 0; i < line_rider.bogie_positions.size(); i++)
        {
            auto bogie_offset{ line_rider.bogie_positions[i] };
            auto prev_bogie_offset{ i == 0 ? 0.0f : line_rider.bogie_positions[i - 1] };

            if (i == 0 && bogie_offset == 0)
                continue;

            std::tie(prev_transform, prev_line_position) =
                find_transform_approx(rail_line,
                                      prev_transform,
                                      line_rider.bogie_springs[i],
                                      bogie_offset - prev_bogie_offset,
                                      prev_line_position);

            // Apply transform to render object.
            static auto const k_apply_transform_to_render_obj = [](entt::registry& reg,
                                                                   entt::entity ent,
                                                                   Rail_position_transform const&
                                                                       transform) {
                auto& rend_obj_cfg = reg.get<TXP::component::Render_object_config>(ent);

                glm_translate_make(rend_obj_cfg.transform.raw,
                                   const_cast<float_t*>(transform.position.raw));

                versor transform_rot;
                glm_euler_zyx_quat(vec3{ transform.angle_x, transform.angle_y, transform.angle_z },
                                   transform_rot);
                glm_quat_rotate(rend_obj_cfg.transform.raw,
                                transform_rot,
                                rend_obj_cfg.transform.raw);
            };

            k_apply_transform_to_render_obj(reg, line_rider.created_entities[i], prev_transform);
        }
    }
}
