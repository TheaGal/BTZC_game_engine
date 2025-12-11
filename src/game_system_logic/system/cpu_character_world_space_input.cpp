#include "cpu_character_world_space_input.h"

#if 0
#include "Jolt/Jolt.h"
#include "Jolt/Math/MathTypes.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "Jolt/Math/Vec3.h"
#include "btglm.h"
#include "game_system_logic/component/animator_root_motion.h"
#include "game_system_logic/component/character_movement.h"
#include "game_system_logic/component/follow_camera.h"
#include "game_system_logic/component/physics_object_settings.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "physics_engine/physics_engine.h"
#include "physics_engine/physics_object.h"
#include "physics_engine/raycast_helper.h"
#include "renderer/model_animator.h"  // For `Model_joint_animation::k_frames_per_second`
#include "service_finder/service_finder.h"

#include <cassert>
#endif  // 0


void BT::system::cpu_character_world_space_input()
{
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
