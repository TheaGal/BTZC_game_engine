#include "process_receive_attack_events.h"

#include "game_system_logic/component/character_movement.h"
#include "game_system_logic/component/health_stats.h"
#include "game_system_logic/system/helper_funcs.h"
#include "service_finder/service_finder.h"
#include "txp_renderer/ui/ui_state.h"


void BT::system::process_receive_attack_events()
{
    auto& reg{ service_finder::find_service<Entity_container>().get_ecs_registry() };
    auto view{ reg.view<component::Health_stats_data>() };

    for (auto&& [entity, health_stats_data] : view.each())
    {
        bool receive_atk_as_hurt, receive_atk_as_guard, receive_atk_as_parry;

        bool const afa_check_success =
            helper::check_for_receive_atk_afa_events(entity,
                                                     receive_atk_as_hurt,
                                                     receive_atk_as_guard,
                                                     receive_atk_as_parry);
        if (!afa_check_success)
            continue;

        // Check if nothing happened.
        if (!receive_atk_as_hurt && !receive_atk_as_guard && !receive_atk_as_parry)
            continue;

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
        float_t const real_posture{ proc_atk.damage * rece_meth_posture_mult *
                                    atk_type_posture_mult };

        // Apply damage.
        health_stats_data.health_pts -= real_damage;
        health_stats_data.posture_pts += real_posture;

        // Cap damage if player char and parrying.
        bool const is_playchar_health_stats{ health_stats_data.ui_name == "playchar" };
        if (receive_atk_as_parry && is_playchar_health_stats)
        {
            health_stats_data.posture_pts =
                std::min(health_stats_data.posture_pts, health_stats_data.max_posture_pts - 1);
        }

        // Update UI.
        std::string canvas_name = "regular_oppo_hp_w_posture.btui";
        if (is_playchar_health_stats)
            canvas_name = "playchar_hp_w_posture.btui";
        else if (!health_stats_data.ui_name.empty())
            canvas_name = "boss_oppo_hp_w_posture.btui";

        auto& ui_canvas{ service_finder::find_service<TXP::UI_state>().canvas(canvas_name) };
        ui_canvas.elem("hp_back").set_width(health_stats_data.max_health_pts + 4);
        ui_canvas.elem("hp_inside").set_width(health_stats_data.health_pts);
        ui_canvas.elem("posture_back").set_width(health_stats_data.max_posture_pts + 4);
        ui_canvas.elem("posture_inside").set_width(health_stats_data.posture_pts);
    }
}
