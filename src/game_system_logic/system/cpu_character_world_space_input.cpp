#include "cpu_character_world_space_input.h"

#include "btglm.h"
#include "game_system_logic/component/character_movement.h"
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
                char_mvt_anim_state.write_to_animator_data.on_unaware = true;
            }

            // Stand still.
            glm_vec3_zero(char_ws_input.ws_flat_clamped_input.raw);
            break;

        case component::CPU_enemy_awareness::State::SUSPICIOUS:
        {
            if (enter_state)
            {   // Trigger new state entered.
                char_mvt_anim_state.write_to_animator_data.on_suspicion = true;

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
                char_mvt_anim_state.write_to_animator_data.is_suspicious_approaching =
                    (glm_vec3_norm2(char_ws_input.ws_flat_clamped_input.raw) >
                     k_close_enough_dist2);

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
                char_mvt_anim_state.write_to_animator_data.on_aware = true;
            }
            else
            {   // @TEMP: @DEBUG: Keep attack anim up!
                char_mvt_anim_state.write_to_animator_data.on_attack = true;
                
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
                    if (auto char_mvt_st{ reg.try_get<component::Character_mvt_state>(entity) };  // @NOTE: I don't really like how this is getting accessed before `system::input_controlled_character_movement()` is run.
                        char_mvt_st != nullptr)
                    {
                        for (auto const& msg : detect_char->state.broadcasted_enemy_atk_msgs)
                        {
                            float_t flat_distance{ glm_vec3_norm(
                                vec3{ msg.other_to_this_delta_pos[0],
                                    0,  // Zero out Y.
                                    msg.other_to_this_delta_pos[2] }) };

                            // Get similarity of .
                            char_mvt_st->get_facing_angle();  // @TODO.
                            msg.other_facing_angle;  // @TODO: Get the angle difference and use these two facing angles for comparison.
                        }
                    }
                }
            }
            break;

        default: assert(false); break;
        }






        // // // Get input for player character, transformed into camera view direction.
        // // auto const& input_state{ service_finder::find_service<Input_handler>().get_input_state() };

        // vec2 move_input{ input_state.move.x.val, input_state.move.y.val };
        // if (!can_move)
        //     glm_vec2_zero(move_input);

        // // Update input state.
        // char_ws_input.prev_jump_pressed   = char_ws_input.jump_pressed;
        // char_ws_input.jump_pressed        = input_state.jump.val;
        // char_ws_input.prev_crouch_pressed = char_ws_input.crouch_pressed;
        // char_ws_input.crouch_pressed      = input_state.crouch.val;

        // // On attack trigger.
        // bool attack_pressed{ input_state.attack.val };
        // if (camera.is_follow_orbit() &&
        //     can_attack_exit &&
        //     !char_mvt_anim_state->state.prev_attack_pressed &&
        //     attack_pressed)
        //     char_mvt_anim_state->write_to_animator_data.on_attack = true;
        // char_mvt_anim_state->state.prev_attack_pressed = attack_pressed;

        // char_mvt_anim_state->write_to_animator_data.is_guarding = (camera.is_follow_orbit() &&
        //                                                            can_guard_exit &&
        //                                                            input_state.guard.val);
    }





#if 0
    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };
    auto view{ reg.view<component::CPU_enemy_awareness const,
                        component::Character_world_space_input const,
                        component::Character_mvt_state,
                        component::Character_mvt_animated_state,
                        component::Created_physics_object_reference const>() };
    auto& phys_engine{ service_finder::find_service<Physics_engine>() };

    // Process all character movements.
    for (auto&& [entity,
                 cpu_enemy_awareness,
                 char_ws_input,
                 char_mvt_state,
                 char_mvt_anim_state,
                 cre_phys_obj_ref] : view.each())
    {
        auto anim_root_motion{ reg.try_get<component::Animator_root_motion const>(
            entity_container.find_entity(char_mvt_anim_state.affecting_animator_uuid)) };
        if (anim_root_motion == nullptr)
        {
            BT_ERRORF(
                "`component::Animator_root_motion` not found at UUID=%s!! Skipping entity.",
                UUID_helper::to_pretty_repr(char_mvt_anim_state.affecting_animator_uuid).c_str());
            assert(false);
            continue;
        }

        // Process input into character movement logic.
        auto phys_obj_uuid{
            view.get<component::Created_physics_object_reference const>(entity).physics_obj_uuid_ref
        };
        auto& phys_obj{ *phys_engine.checkout_physics_object(phys_obj_uuid) };

        auto mvt_logic_result = character_controller_movement_logic(char_ws_input,
                                                                    char_mvt_state,
                                                                    char_mvt_anim_state,
                                                                    anim_root_motion,
                                                                    // follow_cam_state,
                                                                    phys_obj);

        // Apply movement logic outputs to physics object character controller inputs.
        auto const& physics_gravity{
            reinterpret_cast<JPH::PhysicsSystem*>(
                service_finder::find_service<Physics_engine>().get_physics_system_ptr())
                ->GetGravity()
        };
        apply_velocity_to_char_con(char_mvt_state.grounded_state,
                                   phys_obj,
                                   mvt_logic_result.is_grounded,
                                   mvt_logic_result.up_rotation,
                                   physics_gravity,
                                   mvt_logic_result.new_velocity);

        phys_engine.return_physics_object(&phys_obj);

        // Try writing a new facing direction.
        if (poss_display_repr_ref != nullptr)
        {   // Calculate rotation.
            versors rot;
            glm_quat(rot.raw, mvt_logic_result.display_facing_angle, 0.0f, 1.0f, 0.0f);

            // Write to display repr entity transform.
            auto display_repr_ecs_ent{ entity_container.find_entity(
                poss_display_repr_ref->display_repr_uuid) };
            component::submit_transform_change_only_rotation_helper(reg, display_repr_ecs_ent, rot);
        }
    }
#endif  // 0









#if 0
    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };
    auto view{ reg.view<component::Character_world_space_input const,
                        component::Character_mvt_state,
                        component::Created_physics_object_reference const>() };

    // Process all character movements.
    for (auto entity : view)
    {   // Get input and character movement state.
        auto const& char_ws_input{ view.get<component::Character_world_space_input const>(entity) };
        auto& char_mvt_state{ view.get<component::Character_mvt_state>(entity) };

        // Get char anim state.
        auto char_mvt_anim_state{ reg.try_get<component::Character_mvt_animated_state>(entity) };

        // Process input into character movement logic.
        auto& phys_engine{ service_finder::find_service<Physics_engine>() };
        auto phys_obj_uuid{
            view.get<component::Created_physics_object_reference const>(entity).physics_obj_uuid_ref
        };
        auto& phys_obj{ *phys_engine.checkout_physics_object(phys_obj_uuid) };

        auto anim_root_motion{ char_mvt_anim_state
                                   ? reg.try_get<component::Animator_root_motion const>(
                                         entity_container.find_entity(
                                             char_mvt_anim_state->affecting_animator_uuid))
                                   : nullptr };

        component::Follow_camera_follow_ref::State* follow_cam_state{ nullptr };
        auto poss_display_repr_ref{ reg.try_get<component::Display_repr_transform_ref>(entity) };
        if (poss_display_repr_ref != nullptr)
        {
            auto display_repr_ecs_ent{ entity_container.find_entity(
                poss_display_repr_ref->display_repr_uuid) };

            auto follow_cam_follow_ref{ reg.try_get<component::Follow_camera_follow_ref>(
                display_repr_ecs_ent) };

            if (follow_cam_follow_ref != nullptr)
                follow_cam_state = &follow_cam_follow_ref->state;
        }

        auto mvt_logic_result = character_controller_movement_logic(char_ws_input,
                                                                    char_mvt_state,
                                                                    char_mvt_anim_state,
                                                                    anim_root_motion,
                                                                    follow_cam_state,
                                                                    phys_obj);

        // Apply movement logic outputs to physics object character controller inputs.
        auto const& physics_gravity{
            reinterpret_cast<JPH::PhysicsSystem*>(
                service_finder::find_service<Physics_engine>().get_physics_system_ptr())
                ->GetGravity()
        };
        apply_velocity_to_char_con(char_mvt_state.grounded_state,
                                   phys_obj,
                                   mvt_logic_result.is_grounded,
                                   mvt_logic_result.up_rotation,
                                   physics_gravity,
                                   mvt_logic_result.new_velocity);

        phys_engine.return_physics_object(&phys_obj);

        // Try writing a new facing direction.
        if (poss_display_repr_ref != nullptr)
        {   // Calculate rotation.
            versors rot;
            glm_quat(rot.raw, mvt_logic_result.display_facing_angle, 0.0f, 1.0f, 0.0f);

            // Write to display repr entity transform.
            auto display_repr_ecs_ent{ entity_container.find_entity(
                poss_display_repr_ref->display_repr_uuid) };
            component::submit_transform_change_only_rotation_helper(reg, display_repr_ecs_ent, rot);
        }
    }
#endif  // 0
}
