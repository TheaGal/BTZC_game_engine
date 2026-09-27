#include "hitcapsule_attack_processing.h"

#include "btdatecheck.h"
#include "btglm.h"
#include "btlogger.h"
#include "entt/entity/fwd.hpp"
#include "entt/entity/registry.hpp"
#include "game_system_logic/component/character_movement.h"
#include "game_system_logic/component/combat_stats.h"
#include "game_system_logic/component/health_stats.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "service_finder/service_finder.h"
#include "txp_renderer_public.h"

#include <cassert>

#define OLD_HITCAPSULE_ATK_PROCESS 0


namespace
{

using namespace BT;

void process_attack_interaction(Entity_container& entity_container,
                                entt::registry& reg,
                                TXP::Renderer& renderer,
                                entt::entity offender_ecs_entity,
#if OLD_HITCAPSULE_ATK_PROCESS
                                component::Base_combat_stats_data const& offe_combat_stats,
                                component::Health_stats_data& offe_health_stats,
#endif // OLD_HITCAPSULE_ATK_PROCESS
                                entt::entity defender_ecs_entity,
#if OLD_HITCAPSULE_ATK_PROCESS
                                component::Health_stats_data& defe_health_stats,
#endif // OLD_HITCAPSULE_ATK_PROCESS
                                double_t const global_attack_timer,
                                UUID offender_uuid,
                                UUID defender_uuid)
{
#if OLD_HITCAPSULE_ATK_PROCESS
    // Update attack timer.
    bool attack_process_allowed{ false };
    if (defe_health_stats.prev_atk_received_time + defe_health_stats.atk_receive_debounce_time <=
        global_attack_timer)
    {   // Attack allowed!!
        attack_process_allowed = true;
        defe_health_stats.prev_atk_received_time = global_attack_timer;
    }

    // Skip this attack pair if attack is not allowed.
    if (!attack_process_allowed)
        return;

    // Attack process logic.
    struct Attack_hit_result
    {
        struct Result_data
        {
            int32_t delta_hit_pts{ 0 };
            int32_t delta_posture_pts{ 0 };
            bool can_enter_posture_break{ true };
        };
        Result_data defender;
        Result_data offender;
    } atk_res;

    atk_res.defender.delta_hit_pts     = -offe_combat_stats.dmg_pts;
    atk_res.defender.delta_posture_pts = offe_combat_stats.posture_dmg_pts;

    if (auto try_get_defe_combat_stats{
            reg.try_get<component::Base_combat_stats_data const>(defender_ecs_entity) };
        try_get_defe_combat_stats != nullptr)
    {   // Defend against offender's attack.
        auto& defe_combat_stats{ *try_get_defe_combat_stats };

        atk_res.defender.delta_hit_pts += defe_combat_stats.dmg_def_pts;
        atk_res.defender.delta_posture_pts -= defe_combat_stats.posture_dmg_def_pts;
    }
#endif // OLD_HITCAPSULE_ATK_PROCESS

    auto defender_animator{ renderer.try_get_skeletal_animator(defender_ecs_entity) };
    auto offender_animator{ renderer.try_get_skeletal_animator(offender_ecs_entity) };

    if (defender_animator.has_value() && offender_animator.has_value())
    {   // Get sending root motion multiplier from offender.
        float_t root_motion_multiplier;
#if OLD_HITCAPSULE_ATK_PROCESS
        bool can_cancel_attack_w_parry;
#endif // OLD_HITCAPSULE_ATK_PROCESS
        {
            auto& animator{ offender_animator.value() };

            auto& afa_data_handle{ animator.get_anim_frame_action_data_handle() };
            root_motion_multiplier =
                afa_data_handle
                    .get_float_data_handle(
                        TXP::anim_frame_action::CTRL_DATA_LABEL_attack_send_root_motion_multi)
                    .get_val();
#if OLD_HITCAPSULE_ATK_PROCESS
            can_cancel_attack_w_parry =
                afa_data_handle
                    .get_bool_data_handle(
                        TXP::anim_frame_action::CTRL_DATA_LABEL_can_cancel_attack_w_parry)
                    .get_val();
#endif // OLD_HITCAPSULE_ATK_PROCESS
        }

#if OLD_HITCAPSULE_ATK_PROCESS
        // Check for parry or guard in defender.
        bool is_parry_active;
        bool is_guard_active;
#endif // OLD_HITCAPSULE_ATK_PROCESS
        // Write root motion multiplier from offender to defender.
        {
            auto& animator{ defender_animator.value() };

            auto& afa_data_handle{ animator.get_anim_frame_action_data_handle() };
#if OLD_HITCAPSULE_ATK_PROCESS
            is_parry_active =
                afa_data_handle
                    .get_bool_data_handle(TXP::anim_frame_action::CTRL_DATA_LABEL_is_parry_active)
                    .get_val();
            is_guard_active =
                afa_data_handle
                    .get_bool_data_handle(TXP::anim_frame_action::CTRL_DATA_LABEL_is_guard_active)
                    .get_val();
#endif // OLD_HITCAPSULE_ATK_PROCESS

            // Write root motion multiplier from offender to AFA data.
            afa_data_handle
                .get_float_data_handle(TXP::anim_frame_action::CTRL_DATA_LABEL_root_motion_multi)
                .write_val(root_motion_multiplier);
        }

#if OLD_HITCAPSULE_ATK_PROCESS
        // Parry attack.
        if (is_parry_active)
        {
            atk_res.defender.delta_hit_pts = 0;
            atk_res.defender.delta_posture_pts *= 0.5;
            atk_res.defender.can_enter_posture_break = false;

            // Sendback posture pts to offender.
            atk_res.offender.delta_posture_pts = atk_res.defender.delta_posture_pts;
        }
        // Guard attack.
        else if (is_guard_active)
        {
            atk_res.defender.delta_hit_pts = 0;
        }
#endif // OLD_HITCAPSULE_ATK_PROCESS

        // Get parent of defender.
        auto defender_parent_ecs_entity{ entity_container.find_entity(
            reg.get<component::Transform_hierarchy>(defender_ecs_entity).parent_entity) };

        // Try to align defender to offender (facing towards or away).
        bool turn_to_face_away{ false };
        if (auto char_mvt_state{
                reg.try_get<component::Character_mvt_state>(defender_parent_ecs_entity) };
            char_mvt_state)
        {
            rvec3s delta_pos;
            btglm_rvec3_sub(reg.get<component::Transform>(offender_ecs_entity).position.raw,
                            reg.get<component::Transform>(defender_ecs_entity).position.raw,
                            delta_pos.raw);

            float_t target_facing_angle{ atan2f(delta_pos.x, delta_pos.z) };

            auto delta_angle_1{ target_facing_angle - char_mvt_state->get_facing_angle() };
            auto delta_angle_2{ delta_angle_1 + glm_rad(180.0f) };

            while (delta_angle_1 > glm_rad(180.0f)) delta_angle_1 -= glm_rad(360.0f);
            while (delta_angle_1 <= glm_rad(-180.0f)) delta_angle_1 += glm_rad(360.0f);
            while (delta_angle_2 > glm_rad(180.0f)) delta_angle_2 -= glm_rad(360.0f);
            while (delta_angle_2 <= glm_rad(-180.0f)) delta_angle_2 += glm_rad(360.0f);

            // Disabled for ensuring char always face towards where it got attacked.
            //   -Thea 2025/12/23
            if constexpr(false)
            {
                if (std::abs(delta_angle_2) < std::abs(delta_angle_1))
                {   // Delta angle 2 is more optimal.
                    target_facing_angle += glm_rad(180.0f);
                    while (target_facing_angle > glm_rad(180.0f)) target_facing_angle -= glm_rad(360.0f);
                    while (target_facing_angle <= glm_rad(-180.0f)) target_facing_angle += glm_rad(360.0f);

                    // This is facing away case.
                    turn_to_face_away = true;
                }
            }

            // Apply.
            char_mvt_state->set_facing_angle(target_facing_angle);
        }

        // Try to apply some kind of hurt anim.
        if (auto char_mvt_anim_state{ reg.try_get<component::Character_mvt_animated_state>(
                defender_parent_ecs_entity) };
            char_mvt_anim_state)
        {
#if OLD_HITCAPSULE_ATK_PROCESS
            assert(false);  // huh?
            // @ANIMATOR_REFACTOR if (turn_to_face_away)
            // @ANIMATOR_REFACTOR     // Parry/guard undoable when attacked from behind, so just do hurt-forward anim.
            // @ANIMATOR_REFACTOR     char_mvt_anim_state->write_to_animator_data.on_receive_hurt_from_back = true;
            // @ANIMATOR_REFACTOR else if (is_parry_active)
            // @ANIMATOR_REFACTOR     char_mvt_anim_state->write_to_animator_data.on_parry_hurt = true;
            // @ANIMATOR_REFACTOR else if (is_guard_active)
            // @ANIMATOR_REFACTOR     char_mvt_anim_state->write_to_animator_data.on_guard_hurt = true;
            // @ANIMATOR_REFACTOR else
            // @ANIMATOR_REFACTOR     char_mvt_anim_state->write_to_animator_data.on_receive_hurt = true;
#endif // OLD_HITCAPSULE_ATK_PROCESS

            using hurt_type_t = component::Character_mvt_animated_state::Input_mvt_state::Hurt_type;
            char_mvt_anim_state->input_mvt_state.on_hurt = hurt_type_t::HURT_TYPE_LIGHT_FROM_FRONT;
        }
    }

#if OLD_HITCAPSULE_ATK_PROCESS
    // Apply damage results.
    static auto const s_apply_dmg_results_fn =
        [](component::Health_stats_data& health_stats,
            Attack_hit_result::Result_data const& atk_res) {
            if (health_stats.is_invincible)
                return;

            health_stats.health_pts += atk_res.delta_hit_pts;
            health_stats.health_pts = std::max(0, health_stats.health_pts);

            health_stats.posture_pts += atk_res.delta_posture_pts;
            health_stats.posture_pts = std::min(health_stats.max_posture_pts +
                                                    (atk_res.can_enter_posture_break ? 0 : -1),
                                                health_stats.posture_pts);
        };

    s_apply_dmg_results_fn(offe_health_stats, atk_res.offender);
    s_apply_dmg_results_fn(defe_health_stats, atk_res.defender);
#endif // OLD_HITCAPSULE_ATK_PROCESS
}

}  // namespace


void BT::system::hitcapsule_attack_processing(float_t delta_time)
{
    static double_t s_global_attack_timer{ 0 };

    // Overlap pairs buffer for lagging the attacks by `k_lagging_ticks` ticks.
    constexpr size_t k_lagging_ticks{ 2 };
    constexpr size_t k_ovrl_pir_buf_size{ k_lagging_ticks + 1 };
    static std::array<TXP::Overlap_result_set, k_ovrl_pir_buf_size> s_overlap_pairs_buffer;
    static size_t s_cur_ovrl_pir_buf{ 0 };

    // Update hitcapsule overlaps and write result to writing buffer position.
    s_overlap_pairs_buffer[s_cur_ovrl_pir_buf % k_ovrl_pir_buf_size] =
        service_finder::find_service<TXP::Hitcapsule_group_overlap_solver>().update_overlaps();

    // Tick to processing buffer position (`k_lagging_ticks` behind, but using wraparound).
    s_cur_ovrl_pir_buf++;

    // Process all attacks.
    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };
    auto& renderer{ service_finder::find_service<TXP::Renderer>() };

    auto offender_view{
        reg.view<component::Base_combat_stats_data const, component::Health_stats_data>()
    };
    auto defender_view{ reg.view<component::Health_stats_data>() };  // `Base_combat_stats_data` is optional.

    for (auto&& [offender_uuid, defender_uuid] :
         s_overlap_pairs_buffer[s_cur_ovrl_pir_buf % k_ovrl_pir_buf_size].give_rece_hurt_pairs)
    {
        if (!entity_container.entity_exists(offender_uuid) ||
            !entity_container.entity_exists(defender_uuid))
        {   // Skip evaluation since pair is invalid now (1 or 2 entity(s) destroyed).
            continue;
        }

        // Get offender stats.
        auto offender_ecs_entity{ entity_container.find_entity(offender_uuid) };
        auto const& offe_combat_stats{ offender_view.get<component::Base_combat_stats_data const>(
            offender_ecs_entity) };
        auto& offe_health_stats{ offender_view.get<component::Health_stats_data>(
            offender_ecs_entity) };

        // Get defender stats.
        auto defender_ecs_entity{ entity_container.find_entity(defender_uuid) };
        auto& defe_health_stats{ offender_view.get<component::Health_stats_data>(
            defender_ecs_entity) };

        // Process.
        process_attack_interaction(entity_container,
                                   reg,
                                   renderer,
                                   offender_ecs_entity,
#if OLD_HITCAPSULE_ATK_PROCESS
                                   offe_combat_stats,
                                   offe_health_stats,
#endif // OLD_HITCAPSULE_ATK_PROCESS
                                   defender_ecs_entity,
#if OLD_HITCAPSULE_ATK_PROCESS
                                   defe_health_stats,
#endif // OLD_HITCAPSULE_ATK_PROCESS
                                   s_global_attack_timer,
                                   offender_uuid,
                                   defender_uuid);
    }

    // Update attack timer.
    s_global_attack_timer += delta_time;
}
