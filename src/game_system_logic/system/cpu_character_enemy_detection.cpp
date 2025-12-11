#include "cpu_character_enemy_detection.h"

#include "btglm.h"
#include "entt/entity/entity.hpp"
#include "game_system_logic/component/animator_root_motion.h"
#include "game_system_logic/component/character_movement.h"
#include "game_system_logic/component/cpu_enemy_awareness.h"
#include "game_system_logic/component/follow_camera.h"
#include "game_system_logic/component/physics_object_settings.h"
#include "game_system_logic/component/render_object_settings.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "game_system_logic/world/world_properties.h"
#include "renderer/renderer.h"
#include "service_finder/service_finder.h"
#include "uuid/uuid.h"

#include <cassert>


namespace
{

using namespace BT;

void fetch_eyesight_data(Render_object_pool& rend_obj_pool,
                         UUID rend_obj_uuid,
                         rvec3& out_eyesight_pos,
                         vec3& out_eyesight_forward)
{
    auto& rend_obj{ *rend_obj_pool.checkout_render_obj_by_key({ rend_obj_uuid }).front() };

    // Update whether capsules are enabled and keep capsules attached to connecting bone in
    // animator.
    auto& animator{ *rend_obj.get_model_animator() };

    animator.get_anim_frame_action_data_handle().assign_hitcapsule_enabled_flags();

    std::vector<mat4s> joint_matrices;
    animator.get_anim_floored_frame_pose(Model_animator::SIMULATION_PROFILE,
                                         animator.get_is_using_root_motion(),
                                         joint_matrices);

    animator.get_anim_frame_action_data_handle().update_hitcapsule_transforms(
        rend_obj.render_transform(),
        joint_matrices);

    rend_obj_pool.return_render_objs({ &rend_obj });
}

}  // namespace


void BT::system::cpu_character_enemy_detection()
{   // Exit early if simulation not running.
    if (!service_finder::find_service<world::World_properties_container>()
             .get_data_handle()
             .is_simulation_running)
        return;

    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };
    auto view{ reg.view<component::Transform const, component::CPU_enemy_awareness>() };
    auto& rend_obj_pool{ service_finder::find_service<Renderer>().get_render_object_pool() };

    // Process all CPU characters.
    for (auto&& [entity, transform, cpu_enemy_awareness] : view.each())
    {
        // entt::entity disp_repr_ecs_entity{ entt::null };

        // // Try to find facing direction of character.
        // versor facing_rotation = GLM_QUAT_IDENTITY_INIT;  // @TODO: I THINK THIS ISNT NEEDED
        // if (auto disp_repr{ reg.try_get<component::Display_repr_transform_ref const>(entity) };
        //     disp_repr != nullptr)
        // {
        //     disp_repr_ecs_entity = entity_container.find_entity(disp_repr->display_repr_uuid);
        //     if (auto disp_repr_trans{
        //             reg.try_get<component::Transform const>(disp_repr_ecs_entity) };
        //         disp_repr_trans != nullptr)
        //         glm_quat_copy(const_cast<float_t*>(disp_repr_trans->rotation.raw),
        //                         facing_rotation);
        // }

        // Calc eyesight.
        rvec3 eyesight_pos;
        vec3 eyesight_forward{ 0, 0, 1 };

        btglm_rvec3_copy(transform.position.raw, eyesight_pos);

        if (auto disp_repr{ reg.try_get<component::Display_repr_transform_ref const>(entity) };
            disp_repr != nullptr)
        {
            if (auto rend_obj_ref{ reg.try_get<component::Created_render_object_reference>(
                    entity_container.find_entity(disp_repr->display_repr_uuid)) };
                rend_obj_ref != nullptr)
                if (!rend_obj_ref->render_obj_uuid_ref.is_nil())
                    fetch_eyesight_data(rend_obj_pool,
                                        rend_obj_ref->render_obj_uuid_ref,
                                        eyesight_pos,
                                        eyesight_forward);
        }

        // Awareness state machine.
        switch (cpu_enemy_awareness.runtime_state.enemy_awareness)
        {
        case component::CPU_enemy_awareness::State::UNAWARE:
        case component::CPU_enemy_awareness::State::SUSPICIOUS:
        {   // If a new hint of suspicion is found whether to set `suspicion=1` or `suspicion+=1`.
            bool is_suspicion_additive{ cpu_enemy_awareness.runtime_state.enemy_awareness ==
                                        component::CPU_enemy_awareness::State::SUSPICIOUS };

            // Check in awareness zone.
            // @HERE

            // Check in suspicion sight zone.
            // @HERE

            // Check in suspicion sound zone.
            // @TODO: This needs to look up a `Sound_maker` component or something in order to have
            //        a position it can look at so search for.
            // @AMEND: Also, just use `eyesight_pos` as where the ears are!
            // @AMEND: It would be really good if each sound made could also have some kind of
            // "loudness radius" so that there's a collision detection with the sound made and the
            // `suspicion_sound_distance`.
            //   -Thea 2025/12/11

            // If suspicion/awareness is enough, enter AWARE state.
            if (/*suspicion >= some_number*/false)  // @HERE
                cpu_enemy_awareness.runtime_state.enemy_awareness =
                    component::CPU_enemy_awareness::State::AWARE;
            break;
        }

        case component::CPU_enemy_awareness::State::AWARE:
        {
            // Check for line-of-sight with CPU's enemy.
            // @HERE

            // If line-of-sight is lost for long enough, go to SUSPICIOUS state.
            if (/*last_line_of_sight_update >= some_time*/false)  // @HERE
                cpu_enemy_awareness.runtime_state.enemy_awareness =
                    component::CPU_enemy_awareness::State::SUSPICIOUS;
            break;
        }

        default: assert(false); break;
        }
    }



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
