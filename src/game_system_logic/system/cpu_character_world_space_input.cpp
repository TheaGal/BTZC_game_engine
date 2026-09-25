#include "cpu_character_world_space_input.h"

#include "btdatecheck.h"
#include "btglm.h"
#include "btrandom.h"
#include "game_system_logic/component/character_movement.h"
#include "game_system_logic/component/cpu_enemy_awareness.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "game_system_logic/system/helper_funcs.h"
#include "service_finder/service_finder.h"


void BT::system::cpu_character_world_space_input(float_t const delta_time)
{
    // @REFACTOR: this needs to get broken up into smaller funcs.
    date_deadline(2026, 10, 3);

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
    {   // Reset mvt inputs.
        auto& mvt_mode{ char_mvt_anim_state.input_mvt_state.mode };
        using mvt_mode_t = component::Character_mvt_animated_state::Input_mvt_state::Mode;
        if (mvt_mode == mvt_mode_t::MODE_INVALID)
            mvt_mode = mvt_mode_t::MODE_CPU_CHAR;

        // Get AFA data.
        bool _;
        bool request_new_attack{ false };
        bool afa_data_success = helper::fetch_wanted_afa_data(entity_container,
                                                              reg,
                                                              char_mvt_anim_state,
                                                              _,
                                                              request_new_attack);
        if (!afa_data_success)
            continue;

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
            glm_vec3_zero(char_ws_input.delta_to_position_of_interest.raw);
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
                glm_vec3_zero(char_ws_input.delta_to_position_of_interest.raw);
            }
            else
            {   // Calc desired direction.
                rvec3 desired_direction{ 0, 0, 0 };
                btglm_rvec3_sub(cpu_enemy_awareness.runtime_state.position_of_interest,
                                transform.position.raw,
                                desired_direction);

                char_ws_input.ws_flat_clamped_input.raw[0] = desired_direction[0];
                char_ws_input.ws_flat_clamped_input.raw[1] = 0;  // desired_direction[1];
                char_ws_input.ws_flat_clamped_input.raw[2] = desired_direction[2];

                char_ws_input.delta_to_position_of_interest.raw[0] = desired_direction[0];
                char_ws_input.delta_to_position_of_interest.raw[1] = desired_direction[1];
                char_ws_input.delta_to_position_of_interest.raw[2] = desired_direction[2];

                constexpr float_t k_close_enough_dist{ 0.1f };
                constexpr float_t k_close_enough_dist2{ k_close_enough_dist * k_close_enough_dist };
                // @ANIMATOR_REFACTOR char_mvt_anim_state.write_to_animator_data.is_suspicious_approaching =
                // @ANIMATOR_REFACTOR     (glm_vec3_norm2(char_ws_input.ws_flat_clamped_input.raw) >
                // @ANIMATOR_REFACTOR      k_close_enough_dist2);
            }
            break;
        }

        case component::CPU_enemy_awareness::State::AWARE:
            if (enter_state)
            {   // Trigger new state entered.
                char_mvt_anim_state.input_mvt_state.reset_state(true);
            }
            else
            {   // @TEMP: @DEBUG: Keep attack anim up!
                // char_mvt_anim_state.write_to_animator_data.on_attack = true;
                
                // Calc desired direction. (@COPYPASTA, also @TEMP bc this just assumes the attack anim.)
                rvec3 desired_direction{ 0, 0, 0 };
                btglm_rvec3_sub(cpu_enemy_awareness.runtime_state.position_of_interest,
                                transform.position.raw,
                                desired_direction);

                char_ws_input.ws_flat_clamped_input.raw[0] = desired_direction[0];
                char_ws_input.ws_flat_clamped_input.raw[1] = 0;  // desired_direction[1];
                char_ws_input.ws_flat_clamped_input.raw[2] = desired_direction[2];

                char_ws_input.delta_to_position_of_interest.raw[0] = desired_direction[0];
                char_ws_input.delta_to_position_of_interest.raw[1] = desired_direction[1];
                char_ws_input.delta_to_position_of_interest.raw[2] = desired_direction[2];

                // Reads broadcasts that other enemy is attacking.
                if (auto* detect_char{ reg.try_get<component::Detectable_character>(entity) };
                    detect_char != nullptr)
                {
                    size_t num_accepted_msgs{ 0 };

                    if (auto* char_mvt_st{ reg.try_get<component::Character_mvt_state>(entity) };  // @NOTE: I don't really like how this is getting accessed before `system::input_controlled_character_movement()` is run.
                        char_mvt_st != nullptr)
                    {
                        for (auto const& msg : detect_char->state.broadcasted_enemy_atk_msgs)
                        {
                            float_t flat_distance2{ glm_vec2_norm2(  // @NOTE: Ignore Y axis.
                                vec2{ msg.other_to_this_delta_pos[0],
                                      msg.other_to_this_delta_pos[2] }) };

                            // Get similarity of facing angles.
                            date_deadline(2026, 9, 30);  // @TODO: remove try-get block for the character-mvt-state just for this one get_facing_angle(). (just have the try-get happen once right in here)
                            auto ang_diff{ std::abs(msg.other_facing_angle - char_mvt_st->get_facing_angle()) };
                            while (ang_diff > glm_rad(180.0f)) ang_diff -= glm_rad(360.0f);
                            while (ang_diff <= glm_rad(-180.0f)) ang_diff += glm_rad(360.0f);

                            constexpr float_t k_max_flat_distance{ 7.5f };
                            constexpr float_t k_min_ang_diff{ glm_rad(45.0f) };

                            if (flat_distance2 < k_max_flat_distance * k_max_flat_distance &&
                                ang_diff > k_min_ang_diff)
                            {   // Accept msg and input to parry attack.
                                char_mvt_anim_state.input_mvt_state.on_guard_press = true;

                                num_accepted_msgs++;
                            }
                        }

                        for (auto const& msg : detect_char->state.broadcasted_enemy_heal_msgs)
                        {
                            float_t flat_distance2{ glm_vec2_norm2(  // @NOTE: Ignore Y axis.
                                vec2{ msg.other_to_this_delta_pos[0],
                                      msg.other_to_this_delta_pos[2] }) };

                            /// Too far for distance-closing pinch attacks.
                            constexpr float_t k_very_far_distance{ 50.0f };

                            /// Everything closer is close combat and the opposite is range combat
                            /// distance.
                            constexpr float_t k_range_combat_distance{ 25.0f };

                            if (flat_distance2 < k_very_far_distance * k_very_far_distance)
                            {   // Accept msg and input to pinch in distance and attack.
                                char_mvt_anim_state.input_mvt_state.on_exec_attack_combo_idx = 123;  // @HARDCODE: idk maybe use some kind of setting? (set the setting to -1 for do nothing when this happens?)

                                num_accepted_msgs++;
                            }
                        }
                    }

                    // Clear received msgs.
                    if (!detect_char->state.broadcasted_enemy_atk_msgs.empty())
                    {
                        BT_TRACEF("Used %zu/%zu broadcasted atk msgs.",
                                  num_accepted_msgs,
                                  detect_char->state.broadcasted_enemy_atk_msgs.size());
                        detect_char->state.broadcasted_enemy_atk_msgs.clear();
                    }

                    if (!detect_char->state.broadcasted_enemy_heal_msgs.empty())
                    {
                        BT_TRACEF("Used %zu/%zu broadcasted heal msgs.",
                                  num_accepted_msgs,
                                  detect_char->state.broadcasted_enemy_heal_msgs.size());
                        detect_char->state.broadcasted_enemy_heal_msgs.clear();
                    }
                }

                // Check if should request new attack.
                {
                    float_t& combat_tempo_timer{
                        char_mvt_anim_state.input_mvt_state.cpu_char_combat_tempo_timer
                    };
                    float_t const resting_combat_tempo{
                        char_mvt_anim_state.input_mvt_state.cpu_char_resting_combat_tempo
                    };

                    if (combat_tempo_timer >= resting_combat_tempo)
                    {
                        combat_tempo_timer = 0;

                        if (!request_new_attack)
                        {
                            float_t rand_01{ random::fast_float_01_exclusive() };
                            request_new_attack = (rand_01 < 0.3f);
                        }
                    }
                    else
                    {
                        combat_tempo_timer += delta_time;
                    }
                }

                // Input new movement.
                if (request_new_attack)
                {
                    char_mvt_anim_state.input_mvt_state.on_exec_attack_combo_idx = 0;  // @HARDCODE
                }
                else
                {
                    char_mvt_anim_state.input_mvt_state.on_exec_movement_idx = 0;  // @HARDCODE
                }
            }
            break;

        default: assert(false); break;
        }
    }
}
