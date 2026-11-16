#include "hitcapsule_attack_processing.h"

#include "btdatecheck.h"
#include "btglm.h"
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
#include <stdexcept>
#include <tracy/Tracy.hpp>


namespace
{

using namespace BT;

void process_attack_interaction(Entity_container& entity_container,
                                entt::registry& reg,
                                TXP::Renderer& renderer,
                                entt::entity offender_ecs_entity,
                                TXP::Hurtcapsule_info const& hurt_info,
                                entt::entity defender_ecs_entity,
                                component::Health_stats_data& defe_health_stats,
                                double_t const global_attack_timer,
                                UUID offender_uuid,
                                UUID defender_uuid)
{
    // Record that attack has landed to the offender.
    if (auto* char_mvt_anim_state =
            reg.try_get<component::Character_mvt_animated_state>(offender_ecs_entity);
        char_mvt_anim_state != nullptr)
    {
        char_mvt_anim_state->input_mvt_state.on_landed_attack = true;
    }

    // Record sent attack into the defender.
    using Attack_stats = component::Health_stats_data::Runtime_state::Attack_stats;
    defe_health_stats.state.last_received_attack = {
        .offender_entity = offender_ecs_entity,
        .physical_type = Attack_stats::Atk_type(hurt_info.attack_physical_type),
        .direction_type = Attack_stats::Atk_dir(hurt_info.attack_direction_type),
        .damage = hurt_info.attack_damage,
        .is_honed = hurt_info.attack_is_honed,
        .has_fire = false,  // @UNUSED?
    };
    date_deadline(2027, 1, 1);  // @TODO: make the Atk_type and Atk_dir not copypasta anymore!!

    // Get sending root motion multiplier from offender.
    auto defender_animator{ renderer.try_get_skeletal_animator(defender_ecs_entity) };
    auto offender_animator{ renderer.try_get_skeletal_animator(offender_ecs_entity) };

    if (defender_animator.has_value() && offender_animator.has_value())
    {
        // Write root motion multiplier from offender to defender.
        {
            date_deadline(2026, 12, 1);  // @TODO: delete all this fuckass dead code

            auto& animator{ defender_animator.value() };

            auto& afa_data_handle{ animator.get_anim_frame_action_data_handle() };

            // Write root motion multiplier from offender to defender AFA data.
            afa_data_handle
                .get_float_data_handle(
                    TXP::anim_frame_action::CTRL_DATA_LABEL_receive_staged_root_motion_multi)
                .write_val(hurt_info.attack_send_root_motion_multi);
        }

        // Try to align defender to offender (facing towards or away).
        bool turn_to_face_away{ false };
        if (auto char_mvt_state{ reg.try_get<component::Character_mvt_state>(defender_ecs_entity) };
            char_mvt_state)
        {
            rvec3s delta_pos;
            btglm_rvec3_sub(reg.get<component::Transform>(offender_ecs_entity).position.raw,
                            reg.get<component::Transform>(defender_ecs_entity).position.raw,
                            delta_pos.raw);

            float_t target_facing_angle{ atan2f(delta_pos.x, delta_pos.z) };

            // Disabled for ensuring char always face towards where it got attacked.
            //   -Thea 2025/12/23
            if constexpr (false)
            {
                auto delta_angle_1{ target_facing_angle - char_mvt_state->get_facing_angle() };
                auto delta_angle_2{ delta_angle_1 + glm_rad(180.0f) };

                btglm_angle_normalize(delta_angle_1, glm_rad(-180.0f), glm_rad(180.0f));
                btglm_angle_normalize(delta_angle_2, glm_rad(-180.0f), glm_rad(180.0f));

                if (std::abs(delta_angle_2) < std::abs(delta_angle_1))
                {   // Delta angle 2 is more optimal.
                    target_facing_angle += glm_rad(180.0f);
                    btglm_angle_normalize(target_facing_angle, glm_rad(-180.0f), glm_rad(180.0f));

                    // This is facing away case.
                    turn_to_face_away = true;
                }
            }

            // Apply.
            char_mvt_state->set_facing_angle(target_facing_angle);
        }

        // Try to apply some kind of hurt anim.
        if (auto char_mvt_anim_state{
                reg.try_get<component::Character_mvt_animated_state>(defender_ecs_entity) };
            char_mvt_anim_state)
        {
            using hurt_type_t = component::Character_mvt_animated_state::Input_mvt_state::Hurt_type;
            uint32_t on_hurt_flags{ hurt_type_t::HURT_TYPE_REGULAR };
            if (hurt_info.attack_is_unparryable)
                on_hurt_flags |= hurt_type_t::HURT_TYPE_UNPARRYABLE;
            if (hurt_info.attack_is_unguardable)
                on_hurt_flags |= hurt_type_t::HURT_TYPE_UNGUARDABLE;
            if (hurt_info.attack_is_mikiriable)
                on_hurt_flags |= hurt_type_t::HURT_TYPE_MIKIRIABLE;

            char_mvt_anim_state->input_mvt_state.on_hurt = static_cast<hurt_type_t>(on_hurt_flags);
        }
    }
}

}  // namespace


void BT::system::hitcapsule_attack_processing(float_t delta_time)
{
    ZoneScoped;

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

    for (auto&& [offender_uuid, defender_uuid, hurt_info] :
         s_overlap_pairs_buffer[s_cur_ovrl_pir_buf % k_ovrl_pir_buf_size].give_rece_hurt_pairs)
    {
        if (!entity_container.entity_exists(offender_uuid) ||
            !entity_container.entity_exists(defender_uuid))
        {   // Skip evaluation since pair is invalid now (1 or 2 entity(s) destroyed).
            continue;
        }

        // Get offender stats.
        auto offender_ecs_entity{ entity_container.find_entity(offender_uuid) };

        if (!reg.all_of<component::Base_combat_stats_data const, component::Health_stats_data>(
                offender_ecs_entity))
            throw std::runtime_error(
                "this should never happen that an offe hitcapsule set doesnt have both combat "
                "stats and health stats");

        auto const& offe_combat_stats{ offender_view.get<component::Base_combat_stats_data const>(
            offender_ecs_entity) };
        auto& offe_health_stats{ offender_view.get<component::Health_stats_data>(
            offender_ecs_entity) };

        // Get defender stats.
        auto defender_ecs_entity{ entity_container.find_entity(defender_uuid) };

        if (!reg.any_of<component::Health_stats_data>(defender_ecs_entity))
            throw std::runtime_error(
                "this should never happen that a defe hitcapsule set doesnt have health stats");

        auto& defe_health_stats{ defender_view.get<component::Health_stats_data>(
            defender_ecs_entity) };

        // Process.
        process_attack_interaction(entity_container,
                                   reg,
                                   renderer,
                                   offender_ecs_entity,
                                   hurt_info,
                                   defender_ecs_entity,
                                   defe_health_stats,
                                   s_global_attack_timer,
                                   offender_uuid,
                                   defender_uuid);
    }

    // Update attack timer.
    s_global_attack_timer += delta_time;
}
