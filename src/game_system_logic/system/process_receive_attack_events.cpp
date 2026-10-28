#include "process_receive_attack_events.h"

#include "entt/entity/fwd.hpp"
#include "game_system_logic/component/character_movement.h"
#include "game_system_logic/component/health_stats.h"
#include "game_system_logic/system/helper_funcs.h"
#include "game_system_logic/world/world_properties.h"
#include "service_finder/service_finder.h"
#include "txp_renderer/ui/ui_state.h"

#include <stdexcept>


namespace
{
using namespace BT;

inline bool is_playchar_health_stats(component::Health_stats_data const& health_stats_data)
{
    return (health_stats_data.ui_name == "playchar");
}

inline component::Character_mvt_animated_state& find_char_mvt_anim_state(entt::registry& reg,
                                                                         entt::entity entity)
{
    UUID entity_uuid{ service_finder::find_service<Entity_container>().find_entity_uuid(entity) };

    for (auto&& [_, char_mvt_anim_state] :
         reg.view<component::Character_mvt_animated_state>().each())
    {
        if (char_mvt_anim_state.affecting_animator_uuid == entity_uuid)
        {
            return char_mvt_anim_state;
        }
    }

    throw std::runtime_error("Whoopsie! could not find one heheh");
}

/// Process receiving attack.
void process_receive_attack(entt::registry& reg,
                            entt::entity entity,
                            component::Health_stats_data& health_stats_data)
{
    bool receive_atk_as_hurt, receive_atk_as_guard, receive_atk_as_parry;

    bool const afa_check_success =
        system::helper::check_for_receive_atk_afa_events(entity,
                                                         receive_atk_as_hurt,
                                                         receive_atk_as_guard,
                                                         receive_atk_as_parry);
    if (!afa_check_success)
        return;

    // Check for health depletion.
    if (health_stats_data.health_pts < 0)
    {
        find_char_mvt_anim_state(reg, entity).input_mvt_state.on_health_depleted = true;

        health_stats_data.health_pts = 0;  // to ensure event doesn't come in again

        // Force posture break too!
        health_stats_data.posture_pts = health_stats_data.max_posture_pts;
    }

    // Check for posture break.
    if (health_stats_data.posture_pts >= health_stats_data.max_posture_pts)
    {
        find_char_mvt_anim_state(reg, entity).input_mvt_state.on_posture_broken = true;

        health_stats_data.posture_pts = health_stats_data.max_posture_pts - 1;  // to ensure event doesn't come in again
        assert(health_stats_data.posture_pts >= 0);
    }

    // Check if nothing happened.
    if (!receive_atk_as_hurt && !receive_atk_as_guard && !receive_atk_as_parry)
        return;

    // Calc damage multiplier depending on receive method.
    auto const& proc_atk{ health_stats_data.state.last_received_attack };

    float_t rece_meth_dmg_mult;
    float_t rece_meth_posture_mult;

    if (receive_atk_as_parry || (receive_atk_as_guard && !proc_atk.is_honed))
    {
        rece_meth_dmg_mult = 0;
        rece_meth_posture_mult = 0.5;
    }
    else if (receive_atk_as_guard && proc_atk.is_honed)
    {
        rece_meth_dmg_mult = 0.5;
        rece_meth_posture_mult = 0.75;
    }
    else
    {
        rece_meth_dmg_mult = 1;
        rece_meth_posture_mult = 1;
    }

    // Calc damage multiplier depending on attack type.
    float_t atk_type_dmg_mult;
    float_t atk_type_posture_mult;

    using Atk_type = component::Health_stats_data::Runtime_state::Attack_stats::Atk_type;
    switch (proc_atk.type)
    {
    case Atk_type::CRUSH:
        atk_type_dmg_mult = 1;
        atk_type_posture_mult = 1;
        if (!receive_atk_as_hurt)
        {
            BT_WARN("TODO: implement playing sfx for attack type.");
        }
        break;

    case Atk_type::SLICE:
        atk_type_dmg_mult = 1;
        atk_type_posture_mult = 1;
        if (!receive_atk_as_hurt)
        {
            BT_WARN("TODO: implement playing sfx for attack type.");
        }
        break;

    case Atk_type::SPEAR:
        atk_type_dmg_mult = 1;
        atk_type_posture_mult = 1;
        if (!receive_atk_as_hurt)
        {
            BT_WARN("TODO: implement playing sfx for attack type.");
        }
        break;
    }

    // Calc damage.
    float_t const real_damage{ proc_atk.damage * rece_meth_dmg_mult * atk_type_dmg_mult };
    float_t const real_posture{ proc_atk.damage * rece_meth_posture_mult * atk_type_posture_mult };

    // Apply damage.
    if (!health_stats_data.is_invincible)
        health_stats_data.health_pts -= real_damage;
    health_stats_data.posture_pts += real_posture;

    // Cap posture if player char and parrying.
    if (receive_atk_as_parry && is_playchar_health_stats(health_stats_data))
    {
        health_stats_data.posture_pts =
            std::min(health_stats_data.posture_pts, health_stats_data.max_posture_pts - 1);
    }

    // Return posture damage to other entity.
    if (receive_atk_as_parry)
    {
        if (auto* offe_hp_stats_data =
                reg.try_get<component::Health_stats_data>(proc_atk.offender_entity);
            offe_hp_stats_data != nullptr)
        {
            offe_hp_stats_data->posture_pts +=
                (offe_hp_stats_data->parry_return_posture_multi * real_posture);

            // Cap posture if player.
            if (is_playchar_health_stats(*offe_hp_stats_data))
            {
                offe_hp_stats_data->posture_pts = std::min(offe_hp_stats_data->posture_pts,
                                                           offe_hp_stats_data->max_posture_pts - 1);
            }
        }
    }
}

} // namespace


void BT::system::process_receive_attack_events()
{   // Exit early if simulation not running.
    if (!service_finder::find_service<world::World_properties_container>()
             .get_data_handle()
             .is_simulation_running)
        return;

    auto& reg{ service_finder::find_service<Entity_container>().get_ecs_registry() };
    auto view{ reg.view<component::Health_stats_data>() };

    for (auto&& [entity, health_stats_data] : view.each())
    {
        process_receive_attack(reg, entity, health_stats_data);
    }
}
