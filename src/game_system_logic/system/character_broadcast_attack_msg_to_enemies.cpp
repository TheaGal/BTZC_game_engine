#include "character_broadcast_attack_msg_to_enemies.h"

#include "animation_frame_action_tool/runtime_data.h"
#include "btglm.h"
#include "game_system_logic/component/animator_root_motion.h"
#include "game_system_logic/component/character_movement.h"
#include "game_system_logic/component/combat_stats.h"
#include "game_system_logic/component/cpu_enemy_awareness.h"
#include "game_system_logic/component/render_object_settings.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "physics_engine/physics_engine.h"
#include "renderer/renderer.h"
#include "service_finder/service_finder.h"

#include <cassert>


namespace
{

using namespace BT;

void iter_and_asdfasdfasdf()
{

}

}  // namespace


void BT::system::character_broadcast_attack_msg_to_enemies()
{
    auto& rend_obj_pool{ service_finder::find_service<Renderer>().get_render_object_pool() };
    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };

    auto view{ reg.view<component::Transform const,
                        component::Display_repr_transform_ref const,
                        component::Character_mvt_state const,
                        component::Detectable_character const>() };

    auto view_sub{ reg.view<component::Created_render_object_reference>() };

    auto view2{ reg.view<component::Transform const,
                         component::CPU_enemy_awareness const,  // These 2 are on the same layer (NTD).
                         component::Detectable_character>() };  // These 2 are on the same layer (NTD).

    for (auto&& [ecs_entity, transform, disp_repr_ref, char_mvt_st, detect_char] : view.each())
    {   // Check if event to do broadcasts exists.
        bool do_atk_broadcast{ false };
        bool do_heal_broadcast{ false };

        if (view_sub.contains(entity_container.find_entity(disp_repr_ref.display_repr_uuid)))
        {
            auto& rend_obj_ref{ view_sub.get<component::Created_render_object_reference>(
                entity_container.find_entity(disp_repr_ref.display_repr_uuid)) };

            auto& rend_obj{ *rend_obj_pool
                                 .checkout_render_obj_by_key({ rend_obj_ref.render_obj_uuid_ref })
                                 .front() };

            auto animator{ rend_obj.get_model_animator() };
            if (!animator)
            {   // Cancel bc animator doesn't exist.
                rend_obj_pool.return_render_objs({ &rend_obj });
                continue;
            }

            // Get AFA broadcast message.
            do_atk_broadcast =
                animator->get_anim_frame_action_data_handle()
                    .get_reeve_data_handle(
                        anim_frame_action::CTRL_DATA_LABEL_broadcast_attack_to_enemies)
                    .check_if_rising_edge_occurred();

            do_heal_broadcast =
                animator->get_anim_frame_action_data_handle()
                    .get_reeve_data_handle(
                        anim_frame_action::CTRL_DATA_LABEL_broadcast_healing_to_enemies)
                    .check_if_rising_edge_occurred();

            rend_obj_pool.return_render_objs({ &rend_obj });
        }

        // Exit early if no broadcast event.
        if (!do_atk_broadcast && !do_heal_broadcast)
        {
            continue;
        }

        // Broadcast event.
        for (auto&& [ecs_entity2, transform2, cpu_enemy_awareness2, detect_char2] : view2.each())
        {
            if ((cpu_enemy_awareness2.my_enemy_bitmask & detect_char.type) == 0)
            {   // Does not consider `detect_char` as an enemy. Ignore broadcast and skip.
                continue;
            }

            if (do_atk_broadcast)
            {   // Send broadcast.
                vec3s delta_pos;
                delta_pos.x = (transform.position.x + detect_char.transform_offset.x) - (transform2.position.x + detect_char2.transform_offset.x);
                delta_pos.y = (transform.position.y + detect_char.transform_offset.y) - (transform2.position.y + detect_char2.transform_offset.y);
                delta_pos.z = (transform.position.z + detect_char.transform_offset.z) - (transform2.position.z + detect_char2.transform_offset.z);

                component::Detectable_character::Runtime_state::Enemy_atk_msg new_msg;
                glm_vec3_copy(delta_pos.raw, new_msg.other_to_this_delta_pos);

                new_msg.other_facing_angle = char_mvt_st.get_facing_angle();

                detect_char2.state.broadcasted_enemy_atk_msgs.emplace_back(std::move(new_msg));
                BT_TRACE("Broadcasted atk msg.");
            }

            if (do_heal_broadcast)
            {   // Send broadcast.
                vec3s delta_pos;
                delta_pos.x = (transform.position.x + detect_char.transform_offset.x) - (transform2.position.x + detect_char2.transform_offset.x);
                delta_pos.y = (transform.position.y + detect_char.transform_offset.y) - (transform2.position.y + detect_char2.transform_offset.y);
                delta_pos.z = (transform.position.z + detect_char.transform_offset.z) - (transform2.position.z + detect_char2.transform_offset.z);

                component::Detectable_character::Runtime_state::Enemy_heal_msg new_msg;
                glm_vec3_copy(delta_pos.raw, new_msg.other_to_this_delta_pos);

                detect_char2.state.broadcasted_enemy_heal_msgs.emplace_back(std::move(new_msg));
                BT_TRACE("Broadcasted heal msg.");
            }
        }
    }
}
