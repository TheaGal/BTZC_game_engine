#include "cpu_character_world_space_input.h"

#include "btglm.h"
#include "game_system_logic/component/character_movement.h"
#include "game_system_logic/component/combat_stats.h"
#include "game_system_logic/component/cpu_enemy_awareness.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "game_system_logic/system/helper_funcs.h"
#include "service_finder/service_finder.h"


void BT::system::cpu_character_world_space_input()
{
    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };
    auto view{ reg.view<component::Transform const,
                        component::CPU_enemy_awareness const,
                        component::Character_world_space_input,
                        component::Character_mvt_animated_state>() };

    for (auto&& [entity,
                 transform,
                 cpu_enemy_awareness,
                 char_ws_input,
                 char_mvt_anim_state] : view.each())
    {   // Get AFA data.
        bool can_move{ false };
        bool can_guard_exit{ false };
        bool can_attack_exit{ false };
        helper::fetch_wanted_afa_data(entity_container,
                                      reg,
                                      char_mvt_anim_state,
                                      can_move,
                                      can_guard_exit,
                                      can_attack_exit);

        // World-space movement input.
        bool enter_state{ cpu_enemy_awareness.runtime_state.prev_enemy_awareness !=
                          cpu_enemy_awareness.runtime_state.enemy_awareness };
        switch (cpu_enemy_awareness.runtime_state.enemy_awareness)
        {
        case component::CPU_enemy_awareness::State::UNAWARE:
            if (enter_state)
            {   // Trigger new state entered.
                // @ANIMATOR_REFACTOR char_mvt_anim_state.write_to_animator_data.on_unaware = true;
            }

            // Stand still.
            glm_vec3_zero(char_ws_input.ws_flat_clamped_input.raw);
            break;

        case component::CPU_enemy_awareness::State::SUSPICIOUS:
        {
            if (enter_state)
            {   // Trigger new state entered.
                // @ANIMATOR_REFACTOR char_mvt_anim_state.write_to_animator_data.on_suspicion = true;

                // Stand still (for just the enter state tick so that animator has a tick to update
                // the animator state to a different animation than the idle anim which will do an
                // immediate turn speed which we want to avoid).
                glm_vec3_zero(char_ws_input.ws_flat_clamped_input.raw);
            }
            else
            {   // Calc desired direction.
                rvec3 desired_direction{ 0, 0, 0 };
                btglm_rvec3_sub(cpu_enemy_awareness.runtime_state.position_of_interest,
                                transform.position.raw,
                                desired_direction);

                // @TODO: Conform to `write_render_transforms.cpp`
                char_ws_input.ws_flat_clamped_input.raw[0] = desired_direction[0];
                char_ws_input.ws_flat_clamped_input.raw[1] = 0;  // desired_direction[1];
                char_ws_input.ws_flat_clamped_input.raw[2] = desired_direction[2];

                constexpr float_t k_close_enough_dist{ 0.1f };
                constexpr float_t k_close_enough_dist2{ k_close_enough_dist * k_close_enough_dist };
                // @ANIMATOR_REFACTOR char_mvt_anim_state.write_to_animator_data.is_suspicious_approaching =
                // @ANIMATOR_REFACTOR     (glm_vec3_norm2(char_ws_input.ws_flat_clamped_input.raw) >
                // @ANIMATOR_REFACTOR      k_close_enough_dist2);

                // Stand still (put this at the end so that other vars can take advantage of the
                // desired movement vector).
                if (!can_move)
                    glm_vec3_zero(char_ws_input.ws_flat_clamped_input.raw);
            }
            break;
        }

        case component::CPU_enemy_awareness::State::AWARE:
            if (enter_state)
            {   // Trigger new state entered.
                // @ANIMATOR_REFACTOR char_mvt_anim_state.write_to_animator_data.on_aware = true;
            }
            else
            {   // @TEMP: @DEBUG: Keep attack anim up!
                // char_mvt_anim_state.write_to_animator_data.on_attack = true;
                
                // Calc desired direction. (@COPYPASTA, also @TEMP bc this just assumes the attack anim.)
                rvec3 desired_direction{ 0, 0, 0 };
                btglm_rvec3_sub(cpu_enemy_awareness.runtime_state.position_of_interest,
                                transform.position.raw,
                                desired_direction);

                // @TODO: Conform to `write_render_transforms.cpp`
                char_ws_input.ws_flat_clamped_input.raw[0] = desired_direction[0];
                char_ws_input.ws_flat_clamped_input.raw[1] = 0;  // desired_direction[1];
                char_ws_input.ws_flat_clamped_input.raw[2] = desired_direction[2];

                // Reads broadcasts that other enemy is attacking.
                if (auto detect_char{ reg.try_get<component::Detectable_character>(entity) };
                    detect_char != nullptr)
                {
                    size_t num_accepted_msgs{ 0 };

                    if (auto char_mvt_st{ reg.try_get<component::Character_mvt_state>(entity) };  // @NOTE: I don't really like how this is getting accessed before `system::input_controlled_character_movement()` is run.
                        char_mvt_st != nullptr)
                    {
                        for (auto const& msg : detect_char->state.broadcasted_enemy_atk_msgs)
                        {
                            float_t flat_distance2{ glm_vec2_norm2(  // @NOTE: Ignore Y axis.
                                vec2{ msg.other_to_this_delta_pos[0],
                                    msg.other_to_this_delta_pos[2] }) };

                            // Get similarity of facing angles.
                            auto ang_diff{ std::abs(msg.other_facing_angle - char_mvt_st->get_facing_angle()) };
                            while (ang_diff > glm_rad(180.0f)) ang_diff -= glm_rad(360.0f);
                            while (ang_diff <= glm_rad(-180.0f)) ang_diff += glm_rad(360.0f);

                            constexpr float_t k_max_flat_distance{ 7.5f };
                            constexpr float_t k_min_ang_diff{ glm_rad(45.0f) };
                            if (flat_distance2 < k_max_flat_distance * k_max_flat_distance &&
                                ang_diff > k_min_ang_diff)
                            {   // Accept this msg and attempt to parry attack.
                                // @ANIMATOR_REFACTOR char_mvt_anim_state.write_to_animator_data.on_guard = true;

                                // // @DEBUG: Just print out what's up.
                                // BT_TRACEF("Accept msg: flat_dist:%.3f \tang_diff(deg):%.3f",
                                //           std::sqrtf(flat_distance2),
                                //           glm_deg(ang_diff));

                                num_accepted_msgs++;
                            }
                        }

                        if (auto attack_queue{ reg.try_get<component::Attack_queue>(entity) };
                            attack_queue != nullptr)
                        {
                            for (auto const& msg : detect_char->state.broadcasted_enemy_heal_msgs)
                            {
                                float_t flat_distance2{ glm_vec2_norm2(  // @NOTE: Ignore Y axis.
                                    vec2{ msg.other_to_this_delta_pos[0],
                                          msg.other_to_this_delta_pos[2] }) };

                                constexpr float_t k_max_flat_distance{ 50.0f };  // Very far for far reaching pinch attacks.
                                if (flat_distance2 < k_max_flat_distance * k_max_flat_distance)
                                {   // Accept this msg and attempt to pinch in distance and attack.
                                    attack_queue->push_attack_to_queue(0);  // @HARDCODE: @TODO: @NOCHECKIN

                                    num_accepted_msgs++;
                                }
                            }
                        }
                    }

                    // Clear msgs.
                    if (!detect_char->state.broadcasted_enemy_atk_msgs.empty())
                    {
                        BT_TRACEF("Used %llu/%llu broadcasted atk msgs.",
                                  num_accepted_msgs,
                                  detect_char->state.broadcasted_enemy_atk_msgs.size());
                        detect_char->state.broadcasted_enemy_atk_msgs.clear();
                    }
                }
            }
            break;

        default: assert(false); break;
        }
    }
}
