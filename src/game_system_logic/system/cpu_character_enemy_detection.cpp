#include "cpu_character_enemy_detection.h"

#include "btglm.h"
#include "game_system_logic/component/animator_root_motion.h"
#include "game_system_logic/component/character_movement.h"
#include "game_system_logic/component/cpu_enemy_awareness.h"
#include "game_system_logic/component/follow_camera.h"
#include "game_system_logic/component/physics_object_settings.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "game_system_logic/world/world_properties.h"
#include "service_finder/service_finder.h"

#include <cassert>


void BT::system::cpu_character_enemy_detection()
{   // Exit early if simulation not running.
    if (!service_finder::find_service<world::World_properties_container>()
             .get_data_handle()
             .is_simulation_running)
        return;

    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };
    auto view{ reg.view<component::Transform const, component::CPU_enemy_awareness>() };

    // Process all CPU characters.
    for (auto&& [entity, transform, cpu_enemy_awareness] : view.each())
    {
        // Try to find facing direction of character.
        versor facing_rotation = GLM_QUAT_IDENTITY_INIT;
        {
            if (auto disp_repr{ reg.try_get<component::Display_repr_transform_ref const>(entity) };
                disp_repr != nullptr)
                if (auto disp_repr_trans{ reg.try_get<component::Transform const>(
                        entity_container.find_entity(disp_repr->display_repr_uuid)) };
                    disp_repr_trans != nullptr)
                    glm_quat_copy(const_cast<float_t*>(disp_repr_trans->rotation.raw),
                                  facing_rotation);
        }

        // Calc eyesight.
        rvec3 eyesight_pos;
        vec3 eyesight_forward;  // @TODO START HERE!!! And make the thing that queries the model for the bone of the head.




#if 0
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
