#include "convert_atk_to_deathblow.h"

#include "btglm.h"
#include "game_system_logic/component/character_movement.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "service_finder/service_finder.h"
#include "txp_renderer_public.h"

#include <cassert>

#define REAL_DEATHBLOW_CHECKS 0


void BT::system::convert_atk_input_to_deathblow()
{
    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };
    auto view{ reg.view<component::Transform const, component::Character_mvt_animated_state>() };

    auto& renderer{ service_finder::find_service<TXP::Renderer>() };

    for (auto&& [entity, transform, char_mvt_anim_state] : view.each())
    {
        // Find character that has attack input pressed.
        if (!char_mvt_anim_state.input_mvt_state.on_attack_press)
            continue;

        // Look for deathblow opportunities.
        for (auto const& [other_entity, other_transform, other_char_mvt_anim_state] : view.each())
        {
            if (entity == other_entity)
                continue;  // don't compare to self

            // Is the opponent in a deathblow receiving state?
            auto const other_char_animator_ecs_entity{ entity_container.find_entity(
                other_char_mvt_anim_state.affecting_animator_uuid) };
            
            auto opt_animator = renderer.try_get_skeletal_animator(other_char_animator_ecs_entity);
            if (!opt_animator.has_value())
                continue;

            bool const can_receive_deathblow{
                opt_animator.value()
                    .get_anim_frame_action_data_handle()
                    .get_bool_data_handle(
                        TXP::anim_frame_action::CTRL_DATA_LABEL_can_receive_deathblow)
                    .get_val()
            };
        #if REAL_DEATHBLOW_CHECKS
            if (!can_receive_deathblow)
                continue;
        #endif // REAL_DEATHBLOW_CHECKS

            // Is the positioning of the character and other character optimal for the deathblow?
            constexpr float_t k_db_activate_cylinder_radius{ 2.5 };
            constexpr float_t k_db_activate_cylinder_height{ 1 };
            constexpr float_t k_db_activate_facing_dir_cos{ glm_rad(45 * 0.5) };

            vec3 delta_pos;
            {
                rvec3 delta_pos_r;
                btglm_rvec3_sub(other_transform.position.raw, transform.position.raw, delta_pos_r);
                btglm_rvec3_to_vec3(delta_pos_r, delta_pos);
            }

        #if REAL_DEATHBLOW_CHECKS
            if (std::abs(delta_pos[1]) > k_db_activate_cylinder_height)
                continue;
        #endif // REAL_DEATHBLOW_CHECKS

            vec2 flat_delta_pos{
                delta_pos[0],
                delta_pos[2],
            };

        #if REAL_DEATHBLOW_CHECKS
            if (glm_vec2_norm2(flat_delta_pos) >
                k_db_activate_cylinder_radius * k_db_activate_cylinder_radius)
                continue;
        #endif // REAL_DEATHBLOW_CHECKS

            vec2 flat_facing_dir_n;
            {
                mat3 transform_rot;
                glm_quat_mat3(const_cast<float_t*>(transform.rotation.raw), transform_rot);

                vec3 facing_dir;
                glm_vec3_rotate_m3(transform_rot,
                                   const_cast<float_t*>(transform.rotation.raw),
                                   facing_dir);

                flat_facing_dir_n[0] = facing_dir[0];
                flat_facing_dir_n[1] = facing_dir[2];
                glm_vec2_normalize(flat_facing_dir_n);
            }

            vec2 flat_delta_pos_n;
            glm_vec2_copy(flat_delta_pos, flat_delta_pos_n);
            glm_vec2_normalize(flat_delta_pos_n);

        #if REAL_DEATHBLOW_CHECKS
            if (glm_vec2_dot(flat_facing_dir_n, flat_delta_pos_n) > k_db_activate_facing_dir_cos)
                continue;
        #endif // REAL_DEATHBLOW_CHECKS

            // This is a winner!!!! Convert the attack to a deathblow.
            auto& my_input_mvt_state{ char_mvt_anim_state.input_mvt_state };
            my_input_mvt_state.on_attack_press = false;
            my_input_mvt_state.on_give_deathblow = true;
            my_input_mvt_state.deathblow_other_entity = other_entity;

            auto& other_input_mvt_state{ other_char_mvt_anim_state.input_mvt_state };
            other_input_mvt_state.on_receive_deathblow = true;
            other_input_mvt_state.deathblow_other_entity = entity;
            break;
        }
    }
}
