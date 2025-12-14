#include "cpu_character_enemy_detection.h"

#include "btglm.h"
#include "btlogger.h"
#include "cglm/affine.h"
#include "cglm/mat4.h"
#include "cglm/quat.h"
#include "cglm/util.h"
#include "cglm/vec3.h"
#include "entt/entity/entity.hpp"
#include "game_system_logic/component/character_movement.h"
#include "game_system_logic/component/cpu_enemy_awareness.h"
#include "game_system_logic/component/render_object_settings.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "game_system_logic/world/world_properties.h"
#include "physics_engine/physics_engine.h"  // For `k_simulation_delta_time`.
#include "renderer/debug_render_job.h"
#include "renderer/renderer.h"
#include "service_finder/service_finder.h"
#include "uuid/uuid.h"

#include <cassert>
#include <cmath>


namespace
{

using namespace BT;

void fetch_eyesight_data(Render_object_pool& rend_obj_pool,
                         UUID rend_obj_uuid,
                         component::CPU_enemy_awareness& in_out_awareness,
                         vec3 eyes_origin,
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
    vec3 eyesight_pos;
    glm_mat4_mulv3(eyes_bone_trans, eyes_origin, 1, eyesight_pos);

    out_eyesight_pos[0] = eyesight_pos[0];  // @TODO: Conform to `write_render_transforms.cpp`
    out_eyesight_pos[1] = eyesight_pos[1];
    out_eyesight_pos[2] = eyesight_pos[2];

    glm_mat4_mulv3(eyes_bone_trans, out_eyesight_forward, 0, out_eyesight_forward);
}

void draw_detection_cone(component::CPU_enemy_awareness::Sight_detection_zone const& sdz,
                         vec3 eyesight_pos,
                         vec3 eyesight_forward,
                         vec4 line_color_immediate,
                         vec4 line_color_buildup)
{
    auto half_fov{ sdz.fov * 0.5f };
    auto sin_half_fov{ std::sinf(half_fov) };
    auto cos_half_fov{ std::cosf(half_fov) };
    versor view_rotation;
    glm_quat_from_vecs(vec3{ 0, 0, 1 }, eyesight_forward, view_rotation);

    // Detection cone pre-transformation.
    std::vector<vec3s> detection_cone_pts{
        { 0, 0, 0 }, { -sin_half_fov, 0, cos_half_fov },
        { 0, 0, 0 }, {  sin_half_fov, 0, cos_half_fov },
        { 0, 0, 0 }, { 0, -sin_half_fov, cos_half_fov },
        { 0, 0, 0 }, { 0,  sin_half_fov, cos_half_fov },
        { 0, 0, 0 }, { 0, 0, 1 },
    };

    // Transform and submit lines for debug draw.
    assert(detection_cone_pts.size() % 2 == 0);
    for (size_t i = 1; i < detection_cone_pts.size(); i += 2)
    {
        assert(sdz.distance_immediate <= sdz.distance_buildup);

        std::array<vec3s, 3> pts;
        for (size_t j = 0; j < pts.size(); j++)
        {
            auto const& pt_read{ detection_cone_pts[j == 0 ? i - 1 : i] };
            auto& pt_write{ pts[j] };
            glm_vec3_scale(const_cast<float_t*>(pt_read.raw),
                           j == 2 ? sdz.distance_buildup : sdz.distance_immediate,
                           pt_write.raw);
            glm_quat_rotatev(view_rotation, pt_write.raw, pt_write.raw);
            glm_vec3_add(pt_write.raw, eyesight_pos, pt_write.raw);
        }

        for (size_t l = 0; l < 2; l++)
        {   // Draw debug line.
            Debug_line dbg_line;
            glm_vec3_copy(pts[l + 0].raw, dbg_line.pos1);
            glm_vec3_copy(pts[l + 1].raw, dbg_line.pos2);
            glm_vec4_copy(l == 0 ? line_color_immediate : line_color_buildup, dbg_line.color1);
            glm_vec4_copy(l == 0 ? line_color_immediate : line_color_buildup, dbg_line.color2);

            get_main_debug_line_pool().emplace_debug_line(std::move(dbg_line), 0.03f);
        }
    }
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
    auto view_cpus{ reg.view<component::Transform const, component::CPU_enemy_awareness>() };
    auto view_det_chars{ reg.view<component::Transform const, component::Detectable_character>() };
    auto& rend_obj_pool{ service_finder::find_service<Renderer>().get_render_object_pool() };

    // Process all CPU characters.
    for (auto&& [entity, transform, cpu_enemy_awareness] : view_cpus.each())
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
                                            cpu_enemy_awareness.eyes_origin.raw,
                                            eyesight_pos,
                                            eyesight_forward);
        }

        // Awareness state machine.
        switch (cpu_enemy_awareness.runtime_state.enemy_awareness)
        {
        case component::CPU_enemy_awareness::State::UNAWARE:
        case component::CPU_enemy_awareness::State::SUSPICIOUS:
        {
            auto const enemy_awa_copy{ cpu_enemy_awareness.runtime_state.enemy_awareness };

            // Draw debug view.
            vec3 eyesight_pos_f{ static_cast<float_t>(eyesight_pos[0]),  // @TODO: Conform to `write_render_transforms.cpp`
                                 static_cast<float_t>(eyesight_pos[1]),
                                 static_cast<float_t>(eyesight_pos[2]), };

            // Draw debug aware sight zone.
            draw_detection_cone(cpu_enemy_awareness.aware_sdz,
                                eyesight_pos_f,
                                eyesight_forward,
                                vec4{ 0.550, 0.0275, 0.193 },
                                vec4{ 0.790, 0.493, 0.0474 });

            // Draw debug suspicion sight zone.
            draw_detection_cone(cpu_enemy_awareness.suspicion_sdz,
                                eyesight_pos_f,
                                eyesight_forward,
                                vec4{ 0.550, 0.541, 0.0275 },
                                vec4{ 0.555, 0.790, 0.0474 });

            // Draw debug suspicion sound zone.
            get_main_debug_line_pool().emplace_debug_line_based_capsule(
                eyesight_pos_f,
                eyesight_pos_f,
                cpu_enemy_awareness.suspicion_sound_distance,
                vec4{ 0.297, 0.0275, 0.550 },
                0.03f);

            // Check suspicion zone and awareness zone for entities.
            auto suspicion_sight_cos{ std::cosf(cpu_enemy_awareness.suspicion_sdz.fov * 0.5f) };
            auto aware_sight_cos{ std::cosf(cpu_enemy_awareness.aware_sdz.fov * 0.5f) };
            for (auto&& [det_entity, det_trans, det_char] : view_det_chars.each())
            {
                if (det_entity == entity)
                    continue;  // Skip when comparing self to self.

                if ((det_char.type & cpu_enemy_awareness.my_enemy_bitmask) == 0)
                    continue;  // Skip when not the type of detectable character that is an enemy.

                vec3 delta_pos;
                float_t eye_forward_dot_delta_pos_n;
                float_t delta_pos_dist2;
                {
                    rvec3 delta_pos_r;
                    btglm_rvec3_sub(det_trans.position.raw, eyesight_pos, delta_pos_r);

                    // @TODO: Conform to `write_render_transforms.cpp`
                    // Now add transform offset too.
                    delta_pos[0] = delta_pos_r[0] + det_char.transform_offset.x;
                    delta_pos[1] = delta_pos_r[1] + det_char.transform_offset.y;
                    delta_pos[2] = delta_pos_r[2] + det_char.transform_offset.z;

                    vec3 dpn;
                    glm_vec3_normalize_to(delta_pos, dpn);
                    eye_forward_dot_delta_pos_n = glm_vec3_dot(eyesight_forward, dpn);
                    delta_pos_dist2 = glm_vec3_norm2(delta_pos);

                    constexpr bool k_draw_line_of_sight_line{ false };
                    if constexpr(k_draw_line_of_sight_line)
                    {
                        Debug_line dbg_line{
                            { eyesight_pos_f[0], eyesight_pos_f[1], eyesight_pos_f[2] },
                            { eyesight_pos_f[0] + delta_pos[0],
                              eyesight_pos_f[1] + delta_pos[1],
                              eyesight_pos_f[2] + delta_pos[2] },
                            { 1, 1, 1 },
                            { 1, 1, 1 }
                        };
                        get_main_debug_line_pool().emplace_debug_line(std::move(dbg_line), 0.03f);
                    }
                }

                // @DEBUG: Draw debug line when within detection zone.
                bool draw_detection_zone_debug_line{ false };
                vec4 detection_zone_debug_line_color;

                // Detection zone stats.
                bool inside_aware_sdz_buildup_zone{ false };
                bool inside_suspicion_sdz_buildup_zone{ false };

                // Check in awareness zone.  @COPYPASTA
                if (eye_forward_dot_delta_pos_n > aware_sight_cos)
                {
                    auto& sdz{ cpu_enemy_awareness.aware_sdz };
                    assert(sdz.distance_immediate <= sdz.distance_buildup);

                    if (delta_pos_dist2 < sdz.distance_buildup2())
                    {   // Inside detection zone.
                        bool is_immediate_det_zone{
                            delta_pos_dist2 < sdz.distance_immediate2()
                        };

                        // @DEBUG: Draw detection line.
                        draw_detection_zone_debug_line = true;
                        glm_vec4_copy(is_immediate_det_zone ? vec4{ 0.990, 0.0198, 0.359 }
                                                            : vec4{ 1.00, 0.608, 0.0200 },
                                      detection_zone_debug_line_color);

                        // Add detection zone buildup.
                        sdz.current_buildup =
                            (is_immediate_det_zone
                                 ? sdz.buildup_threshold + 0.1f
                                 : sdz.current_buildup + Physics_engine::k_simulation_delta_time);
                        inside_aware_sdz_buildup_zone = true;
                    }
                }

                // Check in suspicion sight zone.  @COPYPASTA
                if (eye_forward_dot_delta_pos_n > suspicion_sight_cos)
                {
                    auto& sdz{ cpu_enemy_awareness.suspicion_sdz };
                    assert(sdz.distance_immediate <= sdz.distance_buildup);

                    if (delta_pos_dist2 < sdz.distance_buildup2())
                    {   // Inside detection zone.
                        bool is_immediate_det_zone{
                            delta_pos_dist2 < sdz.distance_immediate2()
                        };

                        // @DEBUG: Draw detection line.
                        draw_detection_zone_debug_line = true;
                        glm_vec4_copy(is_immediate_det_zone ? vec4{ 0.958, 0.990, 0.0198 }
                                                            : vec4{ 0.771, 1.00, 0.0200 },
                                      detection_zone_debug_line_color);

                        // Add detection zone buildup.
                        sdz.current_buildup =
                            (is_immediate_det_zone
                                 ? sdz.buildup_threshold + 0.1f
                                 : sdz.current_buildup + Physics_engine::k_simulation_delta_time);
                        inside_suspicion_sdz_buildup_zone = true;
                    }
                }

                // Calm down awareness level when leaving detection zones.
                bool in_line_of_sight{ true };  // @TODO: @FIXME: IMPLEMENT THIS!!!
                if (!inside_suspicion_sdz_buildup_zone || !in_line_of_sight)
                {
                    auto& sdz{ cpu_enemy_awareness.suspicion_sdz };
                    sdz.current_buildup =
                        glm_max(0,
                                sdz.current_buildup - Physics_engine::k_simulation_delta_time);

                    if (!inside_aware_sdz_buildup_zone || !in_line_of_sight)
                    {
                        auto& sdz{ cpu_enemy_awareness.aware_sdz };
                        sdz.current_buildup =
                            glm_max(0,
                                    sdz.current_buildup - Physics_engine::k_simulation_delta_time);

                        // Increase out-of-detection timer to step down awareness states.
                        cpu_enemy_awareness.runtime_state.out_of_detection_timer +=
                            Physics_engine::k_simulation_delta_time;
                    }
                }

                // Reset out-of-detection timer if within zone(s) and line-of-sight is found
                if ((inside_suspicion_sdz_buildup_zone || inside_aware_sdz_buildup_zone) &&
                    in_line_of_sight)
                {
                    cpu_enemy_awareness.runtime_state.out_of_detection_timer = 0;
                }

                // @DEBUG: Detection line drawing.
                if (draw_detection_zone_debug_line)
                {
                    // @TODO: Conform to `write_render_transforms.cpp`
                    Debug_line dbg_line{
                        { eyesight_pos_f[0],
                          eyesight_pos_f[1],
                          eyesight_pos_f[2] },
                        { static_cast<float_t>(det_trans.position.x) + det_char.transform_offset.x,
                          static_cast<float_t>(det_trans.position.y) + det_char.transform_offset.y,
                          static_cast<float_t>(det_trans.position.z) + det_char.transform_offset.z }
                    };
                    glm_vec4_copy(detection_zone_debug_line_color, dbg_line.color1);
                    glm_vec4_copy(detection_zone_debug_line_color, dbg_line.color2);

                    get_main_debug_line_pool().emplace_debug_line(std::move(dbg_line), 0.03f);
                }

                // Suspicion buildup can only happen while in UNAWARE state.
                bool is_unaware_state{ cpu_enemy_awareness.runtime_state.enemy_awareness ==
                                       component::CPU_enemy_awareness::State::UNAWARE };

                // Gain awareness thru awareness buildup.
                if (auto& sdz{ cpu_enemy_awareness.aware_sdz };
                    sdz.current_buildup >= sdz.buildup_threshold)
                {
                    cpu_enemy_awareness.runtime_state.enemy_awareness =
                        component::CPU_enemy_awareness::State::AWARE;
                }

                if (auto& sdz{ cpu_enemy_awareness.suspicion_sdz };
                    is_unaware_state && sdz.current_buildup >= sdz.buildup_threshold)
                {
                    cpu_enemy_awareness.runtime_state.enemy_awareness =
                        component::CPU_enemy_awareness::State::SUSPICIOUS;
                }

                // Lose awareness thru out-of-detection time.
                if (cpu_enemy_awareness.runtime_state.enemy_awareness ==
                    component::CPU_enemy_awareness::State::SUSPICIOUS)
                {
                    if (cpu_enemy_awareness.runtime_state.out_of_detection_timer >=
                        cpu_enemy_awareness.lose_suspicion_time)
                    {
                        cpu_enemy_awareness.runtime_state.enemy_awareness =
                            component::CPU_enemy_awareness::State::UNAWARE;
                    }
                }
                else if (cpu_enemy_awareness.runtime_state.enemy_awareness ==
                         component::CPU_enemy_awareness::State::AWARE)
                {
                    if (cpu_enemy_awareness.runtime_state.out_of_detection_timer >=
                        cpu_enemy_awareness.lose_aware_time)
                    {
                        cpu_enemy_awareness.runtime_state.enemy_awareness =
                            component::CPU_enemy_awareness::State::SUSPICIOUS;
                    }
                }

                // Break out of searching entities and clear state if state has changed.
                if (enemy_awa_copy != cpu_enemy_awareness.runtime_state.enemy_awareness)
                {
                    cpu_enemy_awareness.aware_sdz.current_buildup = 0;
                    cpu_enemy_awareness.suspicion_sdz.current_buildup = 0;
                    cpu_enemy_awareness.runtime_state.out_of_detection_timer = 0;
                    break;
                }
            }

            // Check in suspicion sound zone.
            // @TODO: This needs to look up a `Sound_maker` component or something in order to have
            //        a position it can look at so search for.
            // @AMEND: Also, just use `eyesight_pos` as where the ears are!
            // @AMEND: It would be really good if each sound made could also have some kind of
            // "loudness radius" so that there's a collision detection with the sound made and the
            // `suspicion_sound_distance`.
            //   -Thea 2025/12/11
            if (false)
            {
                cpu_enemy_awareness.runtime_state.enemy_awareness =
                    component::CPU_enemy_awareness::State::SUSPICIOUS;
            }
            break;
        }

        case component::CPU_enemy_awareness::State::AWARE:
        {   // Draw debug view.
            assert(false);;  // @TODO IMPLEMENT!

            // Check for line-of-sight with CPU's enemy.
            // @HERE

            // If line-of-sight is lost for long enough, go to SUSPICIOUS state.
            if (/*last_line_of_sight_update >= some_time*/false)  // @HERE
            {
                BT_TRACE("Entered SUSPICIOUS state.");
                cpu_enemy_awareness.runtime_state.enemy_awareness =
                    component::CPU_enemy_awareness::State::SUSPICIOUS;
            }
            break;
        }

        default: assert(false); break;
        }

        // @DEBUG: Print stats.
        static std::vector<std::string> const k_awareness_strs{
            "UNAWARE",
            "SUSPICIOUS",
            "AWARE",
        };
        BT_TRACEF("state=%s\taware_sdz=%.2f\tsus_sdz=%.2f\toodt=%.2f/%.2f",
                  k_awareness_strs[cpu_enemy_awareness.runtime_state.enemy_awareness].c_str(),
                  cpu_enemy_awareness.aware_sdz.current_buildup /
                      cpu_enemy_awareness.aware_sdz.buildup_threshold,
                  cpu_enemy_awareness.suspicion_sdz.current_buildup /
                      cpu_enemy_awareness.suspicion_sdz.buildup_threshold,
                  cpu_enemy_awareness.runtime_state.out_of_detection_timer,
                  cpu_enemy_awareness.runtime_state.enemy_awareness ==
                          component::CPU_enemy_awareness::State::UNAWARE
                      ? 0.0f
                      : (cpu_enemy_awareness.runtime_state.enemy_awareness ==
                                 component::CPU_enemy_awareness::State::SUSPICIOUS
                             ? cpu_enemy_awareness.lose_suspicion_time
                             : cpu_enemy_awareness.lose_aware_time));
    }
}
