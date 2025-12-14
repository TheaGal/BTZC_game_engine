#include "cpu_character_world_space_input.h"
#include "game_system_logic/component/character_movement.h"
#include "game_system_logic/component/cpu_enemy_awareness.h"
#include "game_system_logic/entity_container.h"
#include "game_system_logic/system/helper_funcs.h"
#include "service_finder/service_finder.h"


void BT::system::cpu_character_world_space_input()
{
    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };
    auto view{ reg.view<component::CPU_enemy_awareness const,
                        component::Character_world_space_input,
                        component::Character_mvt_animated_state>() };

    for (auto&& [entity,
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

        // // Get input for player character, transformed into camera view direction.
        // auto const& input_state{ service_finder::find_service<Input_handler>().get_input_state() };

        vec2 move_input{ input_state.move.x.val, input_state.move.y.val };
        if (!can_move)
            glm_vec2_zero(move_input);

        // Update input state.
        char_ws_input.prev_jump_pressed   = char_ws_input.jump_pressed;
        char_ws_input.jump_pressed        = input_state.jump.val;
        char_ws_input.prev_crouch_pressed = char_ws_input.crouch_pressed;
        char_ws_input.crouch_pressed      = input_state.crouch.val;

        // On attack trigger.
        bool attack_pressed{ input_state.attack.val };
        if (camera.is_follow_orbit() &&
            can_attack_exit &&
            !char_mvt_anim_state->state.prev_attack_pressed &&
            attack_pressed)
            char_mvt_anim_state->write_to_animator_data.on_attack = true;
        char_mvt_anim_state->state.prev_attack_pressed = attack_pressed;

        char_mvt_anim_state->write_to_animator_data.is_guarding = (camera.is_follow_orbit() &&
                                                                   can_guard_exit &&
                                                                   input_state.guard.val);
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
