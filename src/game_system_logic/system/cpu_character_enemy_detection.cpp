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
    {   // Calc eyesight.
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
}
