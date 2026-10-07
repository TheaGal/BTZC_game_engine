#include "cpu_character_world_space_input.h"

#include "btglm.h"
#include "btrandom.h"
#include "game_system_logic/component/character_movement.h"
#include "game_system_logic/component/cpu_enemy_awareness.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "game_system_logic/system/helper_funcs.h"
#include "service_finder/service_finder.h"
#include "txp_renderer/debug/debug_printable_info.h"

#include <cstdint>


namespace
{
using namespace BT;

/// Convert rvec3 to vec3.
void cust_btglm_rvec3_to_vec3(rvec3 const src, vec3 dest)
{
    dest[0] = src[0];
    dest[1] = src[1];
    dest[2] = src[2];
}

/// Convert rvec3 to vec3 with custom y value.
void cust_btglm_rvec3_to_vec3_xz(rvec3 const src, float_t const y, vec3 dest)
{
    dest[0] = src[0];
    dest[1] = y;
    dest[2] = src[2];
}

/// Reads and processes broadcasts of actions the enemy is doing.
void process_broadcasted_enemy_msgs(component::Detectable_character* detect_char,
                                    component::Character_mvt_state const* char_mvt_state,
                                    component::Character_mvt_animated_state& char_mvt_anim_state)
{
    if (detect_char == nullptr)
        return;


    float_t char_facing_angle;
    {
        // @NOTE: I don't really like how this is getting accessed before
        //        `system::input_controlled_character_movement()` is run.
        char_facing_angle = (char_mvt_state == nullptr ? 0 : char_mvt_state->get_facing_angle());
    }

    // Attack messages.
    size_t num_atk_accepted_msgs{ 0 };
    for (auto const& msg : detect_char->state.broadcasted_enemy_atk_msgs)
    {
        float_t flat_distance2{ glm_vec2_norm2(  // @NOTE: Ignore Y axis.
            vec2{ msg.other_to_this_delta_pos[0], msg.other_to_this_delta_pos[2] }) };

        // Get similarity of facing angles.
        auto ang_diff{ std::abs(msg.other_facing_angle - char_facing_angle) };
        while (ang_diff > glm_rad(180.0f))
            ang_diff -= glm_rad(360.0f);
        while (ang_diff <= glm_rad(-180.0f))
            ang_diff += glm_rad(360.0f);

        constexpr float_t k_max_flat_distance{ 7.5f };
        constexpr float_t k_min_ang_diff{ glm_rad(45.0f) };

        if (flat_distance2 < k_max_flat_distance * k_max_flat_distance && ang_diff > k_min_ang_diff)
        {  // Accept msg and input to parry attack.
            char_mvt_anim_state.input_mvt_state.on_guard_press = true;

            num_atk_accepted_msgs++;
        }
    }
    if (!detect_char->state.broadcasted_enemy_atk_msgs.empty())
    {
        BT_TRACEF("Used %zu/%zu broadcasted atk msgs.",
                  num_atk_accepted_msgs,
                  detect_char->state.broadcasted_enemy_atk_msgs.size());
        detect_char->state.broadcasted_enemy_atk_msgs.clear();
    }

    // Heal messages.
    size_t num_heal_accepted_msgs{ 0 };
    for (auto const& msg : detect_char->state.broadcasted_enemy_heal_msgs)
    {
        float_t flat_distance2{ glm_vec2_norm2(  // @NOTE: Ignore Y axis.
            vec2{ msg.other_to_this_delta_pos[0], msg.other_to_this_delta_pos[2] }) };

        /// Too far for distance-closing pinch attacks.
        constexpr float_t k_very_far_distance{ 50.0f };

        /// Everything closer is close combat and the opposite is range combat
        /// distance.
        constexpr float_t k_range_combat_distance{ 25.0f };

        if (flat_distance2 < k_very_far_distance * k_very_far_distance)
        {   // Accept msg and input to pinch in distance and attack.
            // char_mvt_anim_state.input_mvt_state.on_exec_attack_combo_idx = 123;  // @HARDCODE: idk maybe use some kind of setting? (set the setting to -1 for do nothing when this happens?)
            assert(false);  // @TODO: do ^^ above ^^

            num_heal_accepted_msgs++;
        }
    }
    if (!detect_char->state.broadcasted_enemy_heal_msgs.empty())
    {
        BT_TRACEF("Used %zu/%zu broadcasted heal msgs.",
                  num_heal_accepted_msgs,
                  detect_char->state.broadcasted_enemy_heal_msgs.size());
        detect_char->state.broadcasted_enemy_heal_msgs.clear();
    }
}

} // namespace


void BT::system::cpu_character_world_space_input(float_t const delta_time)
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
    {   // Reset mvt inputs.
        auto& mvt_mode{ char_mvt_anim_state.input_mvt_state.mode };
        using mvt_mode_t = component::Character_mvt_animated_state::Input_mvt_state::Mode;
        if (mvt_mode == mvt_mode_t::MODE_INVALID)
            mvt_mode = mvt_mode_t::MODE_CPU_CHAR;

        // Get AFA data.
        bool _;
        bool do_attack_by_request{ false };
        bool afa_data_success = helper::fetch_wanted_afa_data(entity_container,
                                                              char_mvt_anim_state,
                                                              _,
                                                              do_attack_by_request);
        if (!afa_data_success)
            continue;

        constexpr uint32_t k_do_action_movement{ 0 };
        constexpr uint32_t k_do_action_atk_by_request{ 1 };
        constexpr uint32_t k_do_action_atk_by_chance{ 2 };
        uint32_t action_to_submit{ do_attack_by_request ? k_do_action_atk_by_request
                                                        : k_do_action_movement };

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

                cust_btglm_rvec3_to_vec3_xz(desired_direction,
                                            0,
                                            char_ws_input.ws_flat_clamped_input.raw);
                cust_btglm_rvec3_to_vec3(desired_direction,
                                         char_ws_input.delta_to_position_of_interest.raw);

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
            {
                // Calc desired direction. (@COPYPASTA, also @TEMP bc this just assumes the attack anim.)
                rvec3 desired_direction{ 0, 0, 0 };
                btglm_rvec3_sub(cpu_enemy_awareness.runtime_state.position_of_interest,
                                transform.position.raw,
                                desired_direction);

                cust_btglm_rvec3_to_vec3_xz(desired_direction,
                                            0,
                                            char_ws_input.ws_flat_clamped_input.raw);
                cust_btglm_rvec3_to_vec3(desired_direction,
                                         char_ws_input.delta_to_position_of_interest.raw);

                // Reads broadcasts of actions the enemy is doing.
                process_broadcasted_enemy_msgs(
                    reg.try_get<component::Detectable_character>(entity),
                    reg.try_get<component::Character_mvt_state>(entity),
                    char_mvt_anim_state);

                // Check if should do new attack.
                if (action_to_submit == k_do_action_movement &&
                    random::fast_float_01_exclusive() < 0.3f)
                    action_to_submit = k_do_action_atk_by_chance;

                // Input new movement.
                float_t const flat_distance_to_target{ glm_vec2_norm(
                    vec2{ static_cast<float_t>(desired_direction[0]),
                          static_cast<float_t>(desired_direction[2]) }) };
                TXP::debug::emplace_data_point("CPU-dist-to-target", flat_distance_to_target);

                using Atk_t =
                        component::Character_mvt_animated_state::Input_mvt_state::CPU_attack_type;

                char_mvt_anim_state.input_mvt_state.distance_to_target = flat_distance_to_target;

                switch (action_to_submit)
                {
                case k_do_action_movement:
                    char_mvt_anim_state.input_mvt_state.on_exec_movement = true;
                    break;

                case k_do_action_atk_by_request:
                    char_mvt_anim_state.input_mvt_state.on_exec_attack_combo =
                        Atk_t::CPU_ATK_TYPE_REQUESTED;
                    break;

                case k_do_action_atk_by_chance:
                    char_mvt_anim_state.input_mvt_state.on_exec_attack_combo =
                        Atk_t::CPU_ATK_TYPE_BY_CHANCE;
                    break;
                }
            }
            break;

        default: assert(false); break;
        }
    }
}
