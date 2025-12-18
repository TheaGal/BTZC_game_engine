#include "tick_sim_char_mvt_animator.h"

#include "animation_frame_action_tool/runtime_data.h"
#include "game_system_logic/component/animator_root_motion.h"
#include "game_system_logic/component/character_movement.h"
#include "game_system_logic/component/render_object_settings.h"
#include "game_system_logic/entity_container.h"
#include "renderer/renderer.h"
#include "service_finder/service_finder.h"

#include <cassert>


void BT::system::tick_sim_char_mvt_animator()
{
    auto& rend_obj_pool{ service_finder::find_service<Renderer>().get_render_object_pool() };
    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };

    {   // Character mvt animators.
        auto view{ reg.view<component::Character_mvt_animated_state>() };
        for (auto entity : view)
        {   // Get animator.
            auto& char_mvt_anim_state{ view.get<component::Character_mvt_animated_state>(entity) };

            auto affecting_rend_obj_ecs_entity{ entity_container.find_entity(
                char_mvt_anim_state.affecting_animator_uuid) };
            auto const affecting_rend_obj_ref{
                reg.try_get<component::Created_render_object_reference const>(
                    affecting_rend_obj_ecs_entity)
            };
            if (!affecting_rend_obj_ref)
                continue;  // Cancel bc no created render object.

            auto& affecting_rend_obj{ *rend_obj_pool
                                           .checkout_render_obj_by_key(
                                               { affecting_rend_obj_ref->render_obj_uuid_ref })
                                           .front() };

            auto animator{ affecting_rend_obj.get_model_animator() };
            if (!animator)
            {   // Cancel bc animator doesn't exist.
                rend_obj_pool.return_render_objs({ &affecting_rend_obj });
                continue;
            }

            // Set animator vars.
            #define SET_ANIMATOR_BOOL_VAR(_var)                                                     \
                animator->set_bool_variable(#_var, char_mvt_anim_state.write_to_animator_data._var);
            #define SET_ANIMATOR_FLOAT_VAR(_var)                                                    \
                animator->set_float_variable(#_var, char_mvt_anim_state.write_to_animator_data._var);
            #define SET_ANIMATOR_TRIGGER(_var)                                                      \
                if (char_mvt_anim_state.write_to_animator_data._var)                                \
                    animator->set_trigger_variable(#_var);                                          \
                char_mvt_anim_state.write_to_animator_data._var = false;

            SET_ANIMATOR_BOOL_VAR(is_moving)
            SET_ANIMATOR_BOOL_VAR(is_locked_on)
            SET_ANIMATOR_TRIGGER(on_suspicion)
            SET_ANIMATOR_BOOL_VAR(is_suspicious_approaching)
            SET_ANIMATOR_TRIGGER(on_unaware)
            SET_ANIMATOR_TRIGGER(on_aware)
            SET_ANIMATOR_FLOAT_VAR(mvt_facing_angle)
            SET_ANIMATOR_TRIGGER(on_turnaround)
            SET_ANIMATOR_BOOL_VAR(is_grounded)
            SET_ANIMATOR_TRIGGER(on_jump)
            SET_ANIMATOR_TRIGGER(on_attack)
            SET_ANIMATOR_TRIGGER(on_parry_hurt)
            SET_ANIMATOR_TRIGGER(on_guard_hurt)
            SET_ANIMATOR_TRIGGER(on_receive_hurt)
            SET_ANIMATOR_TRIGGER(on_receive_hurt_from_back)
            SET_ANIMATOR_BOOL_VAR(is_guarding)

            #undef SET_ANIMATOR_BOOL_VAR
            #undef SET_ANIMATOR_FLOAT_VAR
            #undef SET_ANIMATOR_TRIGGER

            // Update animator.
            animator->update(Model_animator::SIMULATION_PROFILE,
                             Physics_engine::k_simulation_delta_time);

            // Read animator root motion AFA data.
            if (animator->get_is_using_root_motion())
            {
                auto& anim_root_motion{ reg.get<component::Animator_root_motion>(
                    affecting_rend_obj_ecs_entity) };
                auto& anim_afa_data_handle{ animator->get_anim_frame_action_data_handle() };

                anim_root_motion.root_motion_multiplier =
                    anim_afa_data_handle
                        .get_float_data_handle(anim_frame_action::CTRL_DATA_LABEL_root_motion_multi)
                        .get_val();

                animator->get_anim_root_motion_delta_pos(Model_animator::SIMULATION_PROFILE,
                                                         anim_root_motion.delta_pos);

                anim_root_motion.turn_speed =
                    anim_afa_data_handle
                        .get_float_data_handle(anim_frame_action::CTRL_DATA_LABEL_turn_speed)
                        .get_val();
                anim_root_motion.can_do_turnaround_anim =
                    anim_afa_data_handle
                        .get_bool_data_handle(
                            anim_frame_action::CTRL_DATA_LABEL_can_do_turnaround_anim)
                        .get_val();
                anim_root_motion.mvt_input.enabled =
                    anim_afa_data_handle
                        .get_bool_data_handle(anim_frame_action::CTRL_DATA_LABEL_mvt_input_enabled)
                        .get_val();
                anim_root_motion.mvt_input.max_speed =
                    anim_afa_data_handle
                        .get_float_data_handle(anim_frame_action::CTRL_DATA_LABEL_mvt_input_max_speed)
                        .get_val();
                anim_root_motion.mvt_input.accel =
                    anim_afa_data_handle
                        .get_float_data_handle(anim_frame_action::CTRL_DATA_LABEL_mvt_input_accel)
                        .get_val();
                anim_root_motion.mvt_input.decel =
                    anim_afa_data_handle
                        .get_float_data_handle(anim_frame_action::CTRL_DATA_LABEL_mvt_input_decel)
                        .get_val();
            }

            // Finish.
            rend_obj_pool.return_render_objs({ &affecting_rend_obj });
        }
    }
}
