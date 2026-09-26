#include "tick_sim_char_mvt_animator.h"

#include "game_system_logic/component/character_movement.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "physics_engine/physics_engine.h"  // for `k_simulation_delta_time`
#include "service_finder/service_finder.h"
#include "txp_renderer_public.h"

#include <cassert>
#include <stdexcept>


void BT::system::tick_sim_char_mvt_animator()
{
    auto& renderer{ service_finder::find_service<TXP::Renderer>() };
    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };

    {   // Character mvt animators.
        auto view{ reg.view<component::Character_mvt_animated_state>() };
        for (auto entity : view)
        {   // Get animator.
            auto& char_mvt_anim_state{ view.get<component::Character_mvt_animated_state>(entity) };

            auto affecting_rend_obj_ecs_entity{ entity_container.find_entity(
                char_mvt_anim_state.affecting_animator_uuid) };
            auto animator_optional{ renderer.try_get_skeletal_animator(
                affecting_rend_obj_ecs_entity) };

            if (!animator_optional.has_value())
                continue;  // Cancel bc animator doesn't exist.

            auto& animator{ animator_optional.value() };

            // Write animator vars.
            #define SET_ANIMATOR_BOOL_VAR(_var)                                                     \
                animator.set_bool_variable(#_var, char_mvt_anim_state.write_to_animator_data._var);
            #define SET_ANIMATOR_FLOAT_VAR(_var)                                                    \
                animator.set_float_variable(#_var, char_mvt_anim_state.write_to_animator_data._var);
            #define SET_ANIMATOR_TRIGGER(_var)                                                      \
                if (char_mvt_anim_state.write_to_animator_data._var)                                \
                    animator.set_trigger_variable(#_var);                                           \
                char_mvt_anim_state.write_to_animator_data._var = false;
            // //--------------------------------------------------------------------------------------
            // @ANIMATOR_REFACTOR: the vv below vv is removed for this refactor.
            // @THEA: @TEMP: @REFACTOR: for refactor into watch_jump_queue() ctrl cmd.
            // SET_ANIMATOR_BOOL_VAR(is_moving)
            // SET_ANIMATOR_BOOL_VAR(is_locked_on)
            // SET_ANIMATOR_TRIGGER(on_suspicion)
            // SET_ANIMATOR_BOOL_VAR(is_suspicious_approaching)
            // SET_ANIMATOR_TRIGGER(on_unaware)
            // SET_ANIMATOR_TRIGGER(on_aware)
            SET_ANIMATOR_FLOAT_VAR(mvt_facing_angle)  // <- Except this one!!!!!
            // SET_ANIMATOR_TRIGGER(on_turnaround)
            // SET_ANIMATOR_BOOL_VAR(is_grounded)
            // SET_ANIMATOR_TRIGGER(on_jump)
            // SET_ANIMATOR_TRIGGER(on_attack)
            // SET_ANIMATOR_TRIGGER(on_cancel_parried)
            // SET_ANIMATOR_TRIGGER(on_parry_hurt)
            // SET_ANIMATOR_TRIGGER(on_guard_hurt)
            // SET_ANIMATOR_TRIGGER(on_receive_hurt)
            // SET_ANIMATOR_TRIGGER(on_receive_hurt_from_back)
            // SET_ANIMATOR_TRIGGER(on_guard)
            // SET_ANIMATOR_BOOL_VAR(is_guarding)
            // //--------------------------------------------------------------------------------------
            #undef SET_ANIMATOR_BOOL_VAR
            #undef SET_ANIMATOR_FLOAT_VAR
            #undef SET_ANIMATOR_TRIGGER

            // @THOUGHT: how the new event system should be working (from thea_notes.md).
            //
            //   - ok so there's an issue. the `character_movement.h` anim states sucks ass. there
            //     needs to be a way to know what state sets to create if an event (joystick tilted,
            //     jump btn pressed, )
            //   - so then, maybe the ~~jump queue~~ event queue list needs some kind of input event
            //     to watch for (or just generic event, since CPUs don't listen for input events),
            //     and if it hears that event, then switches to another state set instead of
            //     emplacing one.
            //       - but then how do state sets work for something like a random set?
            //       - there should be the option to transition to a random set of state sets. for
            //         something like the player character, it could be transitioning to "st_jump"
            //         or "st_jump_mirrored" or something randomly. weights could be applied here
            //         too to affect the randomness.
            //       - and then for a CPU, it could be the list of available attacks to do.

            // Send animator events.
            auto const& mvt_state{ char_mvt_anim_state.input_mvt_state };

            using mvt_state_mode_t = component::Character_mvt_animated_state::Input_mvt_state::Mode;
            using hurt_type_t = component::Character_mvt_animated_state::Input_mvt_state::Hurt_type;

            switch (mvt_state.mode)
            {
            case mvt_state_mode_t::MODE_PLAYER_CHAR:
                if (mvt_state.is_moving)
                    animator.emplace_event("evq_is_moving", 0.0f, 0);
                else
                    animator.emplace_event("evq_is_idle", 0.0f, 0);

                if (mvt_state.on_jump)
                    animator.emplace_event("evq_on_jump", 0.5f, 0);

                if (mvt_state.is_grounded)
                    animator.emplace_event("evq_is_grounded", 0.0f, 0);
                else
                    animator.emplace_event("evq_is_midair", 0.0f, 0);

                if (mvt_state.on_hurt > hurt_type_t::HURT_TYPE_NONE)
                    animator.emplace_event("evq_on_hurt", 0.0f, mvt_state.on_hurt);

                if (mvt_state.on_attack_press)
                    animator.emplace_event("evq_on_attack_press", 0.5f, 0);
                if (mvt_state.is_attack_released)
                    animator.emplace_event("evq_is_attack_released", 0.0f, 0);
                if (mvt_state.on_guard_press)
                    animator.emplace_event("evq_on_guard_press", 0.5f, 0);
                if (mvt_state.is_guard_released)
                    animator.emplace_event("evq_is_guard_released", 0.0f, 0);
                break;

            case mvt_state_mode_t::MODE_CPU_CHAR:
                if (mvt_state.on_hurt > hurt_type_t::HURT_TYPE_NONE)
                    animator.emplace_event("evq_on_hurt", 0.0f, mvt_state.on_hurt);

                if (mvt_state.on_guard_press)
                    animator.emplace_event("evq_on_guard_press", 0.0f, 0);

                if (mvt_state.on_exec_movement_idx >= 0)
                    animator.emplace_event("evq_exec_mvt_idx",
                                           0.0f,
                                           mvt_state.on_exec_movement_idx);
                if (mvt_state.on_exec_attack_combo_idx >= 0)
                    animator.emplace_event("evq_exec_atk_combo_idx",
                                           mvt_state.cpu_char_resting_combat_tempo,
                                           mvt_state.on_exec_attack_combo_idx);
                break;

            default:
                throw std::runtime_error("Cannot have invalid input mvt state mode.");
            }

            // Reset inputs.
            char_mvt_anim_state.input_mvt_state.reset_state(false);


            // Give animator transform information for update.
            mat4 entity_transform;
            reg.get<component::Transform const>(affecting_rend_obj_ecs_entity)
                .calc_mat4_transform(entity_transform);

            animator.cache_simulation_transform(entity_transform);


            // Update animator.
            animator.update(TXP::SIMULATION_TIMER_PROFILE,
                            Physics_engine::k_simulation_delta_time);


            // Read animator root motion AFA data.
            if (animator.get_is_using_root_motion())
            {
                auto& anim_root_motion{ reg.get<TXP::component::Animator_root_motion>(
                    affecting_rend_obj_ecs_entity) };
                auto& anim_afa_data_handle{ animator.get_anim_frame_action_data_handle() };

                using AFA_ctrl = TXP::anim_frame_action::Controllable_data_label;

                anim_root_motion.root_motion_multiplier =
                    anim_afa_data_handle
                        .get_float_data_handle(AFA_ctrl::CTRL_DATA_LABEL_root_motion_multi)
                        .get_val();

                animator.get_anim_root_motion_delta_pos(TXP::SIMULATION_TIMER_PROFILE,
                                                        anim_root_motion.delta_pos);


                anim_root_motion.turn_speed =
                    anim_afa_data_handle
                        .get_float_data_handle(AFA_ctrl::CTRL_DATA_LABEL_turn_speed)
                        .get_val();
                anim_root_motion.can_do_turnaround_anim =
                    anim_afa_data_handle
                        .get_bool_data_handle(AFA_ctrl::CTRL_DATA_LABEL_can_do_turnaround_anim)
                        .get_val();

                anim_root_motion.jump_up =
                    anim_afa_data_handle.get_reeve_data_handle(AFA_ctrl::CTRL_DATA_LABEL_jump_up)
                        .check_if_rising_edge_occurred();
                anim_root_motion.inherit_prev_velocity =
                    anim_afa_data_handle
                        .get_reeve_data_handle(
                            AFA_ctrl::CTRL_DATA_LABEL_inherit_prev_velocity)
                        .check_if_rising_edge_occurred();

                anim_root_motion.calc_pos_of_interest_root_motion_multi =
                    anim_afa_data_handle
                        .get_reeve_data_handle(
                            AFA_ctrl::CTRL_DATA_LABEL_calc_pos_of_interest_root_motion_multi)
                        .check_if_rising_edge_occurred();
                anim_root_motion.use_pos_of_interest_root_motion_multi =
                    anim_afa_data_handle
                        .get_bool_data_handle(
                            AFA_ctrl::CTRL_DATA_LABEL_use_pos_of_interest_root_motion_multi)
                        .get_val();

                anim_root_motion.mvt_input.enabled =
                    anim_afa_data_handle
                        .get_bool_data_handle(AFA_ctrl::CTRL_DATA_LABEL_mvt_input_enabled)
                        .get_val();
                anim_root_motion.mvt_input.max_speed =
                    anim_afa_data_handle
                        .get_float_data_handle(AFA_ctrl::CTRL_DATA_LABEL_mvt_input_max_speed)
                        .get_val();
                anim_root_motion.mvt_input.accel =
                    anim_afa_data_handle
                        .get_float_data_handle(AFA_ctrl::CTRL_DATA_LABEL_mvt_input_accel)
                        .get_val();
                anim_root_motion.mvt_input.decel =
                    anim_afa_data_handle
                        .get_float_data_handle(AFA_ctrl::CTRL_DATA_LABEL_mvt_input_decel)
                        .get_val();
            }
        }
    }
}
