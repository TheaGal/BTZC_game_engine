#include "cpu_character_enemy_detection.h"

#include "btglm.h"
#include "cglm/affine.h"
#include "cglm/mat4.h"
#include "entt/entity/entity.hpp"
#include "game_system_logic/component/character_movement.h"
#include "game_system_logic/component/cpu_enemy_awareness.h"
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
                         component::CPU_enemy_awareness& in_out_awareness,
                         rvec3& out_eyesight_pos,
                         vec3& out_eyesight_forward)
{
    auto& rend_obj{ *rend_obj_pool.checkout_render_obj_by_key({ rend_obj_uuid }).front() };

    if (rend_obj.get_model_animator() == nullptr)
    {   // Exit since no model animator attached.
        rend_obj_pool.return_render_objs({ &rend_obj });
        return;
    }

    // Update joint matrix of eyes bone.
    auto& animator{ *rend_obj.get_model_animator() };

    std::vector<mat4s> joint_matrices;
    animator.get_anim_floored_frame_pose(Model_animator::SIMULATION_PROFILE,
                                         animator.get_is_using_root_motion(),
                                         joint_matrices);

    if (in_out_awareness.runtime_state.eyes_bone_idx == (uint32_t)-1)
    {   // Get the bone idx for eyes-bone.
        in_out_awareness.runtime_state.eyes_bone_idx =
            animator.get_model_skin().joint_name_to_idx.at(in_out_awareness.eyes_bone);
    }
    assert(in_out_awareness.runtime_state.eyes_bone_idx != (uint32_t)-1);

    mat4 eyes_bone_trans;
    glm_mat4_mul(rend_obj.render_transform(),
                 joint_matrices[in_out_awareness.runtime_state.eyes_bone_idx].raw,
                 eyes_bone_trans);

    rend_obj_pool.return_render_objs({ &rend_obj });

    // Write eyesight data.
    vec3 translate;
    glm_vec3(eyes_bone_trans[3], translate);

    out_eyesight_pos[0] = translate[0];  // @TODO: Conform to `write_render_transforms.cpp`
    out_eyesight_pos[1] = translate[1];
    out_eyesight_pos[2] = translate[2];

    glm_mat4_mulv3(eyes_bone_trans, out_eyesight_forward, 0, out_eyesight_forward);
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

        if (!cpu_enemy_awareness.eyes_bone.empty())
        {
            if (auto disp_repr{ reg.try_get<component::Display_repr_transform_ref const>(entity) };
                disp_repr != nullptr)
                if (auto rend_obj_ref{ reg.try_get<component::Created_render_object_reference>(
                        entity_container.find_entity(disp_repr->display_repr_uuid)) };
                    rend_obj_ref != nullptr)
                    if (!rend_obj_ref->render_obj_uuid_ref.is_nil())
                        fetch_eyesight_data(rend_obj_pool,
                                            rend_obj_ref->render_obj_uuid_ref,
                                            cpu_enemy_awareness,
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
