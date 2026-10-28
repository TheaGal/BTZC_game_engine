#include "process_ui_interfaces.h"

#include "entt/entity/fwd.hpp"
#include "game_system_logic/component/health_stats.h"
#include "game_system_logic/entity_container.h"
#include "game_system_logic/world/world_properties.h"
#include "service_finder/service_finder.h"
#include "ui_interface/hp_w_posture_ui.h"
#include "txp_renderer_public.h"

#include <cassert>
#include <stdexcept>


namespace
{

using namespace BT;

enum Process_method  // is this needed?
{
    PROC_START_UI,
    PROC_END_UI,
    PROC_TICK_UI,
};

void process_ui_hp_w_posture(Process_method const method,
                             TXP::UI_state& ui_state,
                             entt::registry& reg,
                             float_t const delta_time)
{
    auto& ui{ service_finder::find_service<ui_ifc::Hp_w_posture>() };

    // switch (method)
    // {
    // case PROC_START_UI:
    //     for (std::string fname : { "playchar_hp_w_posture.btui", "boss_oppo_hp_w_posture.btui" })
    //     {
    //         ui_state.load_persistent_canvas(fname);
    //     }
    //     break;

    // case PROC_END_UI:
    //     for (std::string fname : { "playchar_hp_w_posture.btui", "boss_oppo_hp_w_posture.btui" })
    //     {
    //         ui_state.unload_persistent_canvas(fname);
    //     }
    //     break;

    // case PROC_TICK_UI:
    //     break;
    // }




    bool found_playchar{ false };
    bool found_boss_oppo{ false };

    for (auto&& [entity, health_stats_data] : reg.view<component::Health_stats_data>().each())
    {
        if (health_stats_data.ui_name == "playchar")
        {
            found_playchar = true;
        }
        else if (!health_stats_data.ui_name.empty())
        {
            found_boss_oppo = true;
            ui.set_boss_oppo_name(health_stats_data.ui_name);
        }
        else
        {
            throw std::runtime_error("whoopsie not implemente dyet!!");
        }
    }

    ui.set_visible(ui.CANVAS_PLAYCHAR, found_playchar);
    ui.set_visible(ui.CANVAS_BOSS_OPPO, found_boss_oppo);
}

void process_ui_ninsatu(Process_method const method,
                        TXP::UI_state& ui_state,
                        float_t const delta_time)
{}

void process_ui_you_died(Process_method const method,
                         TXP::UI_state& ui_state,
                         float_t const delta_time)
{}

} // namespace


void BT::system::process_ui_interfaces(TXP::UI_state& ui_state, float_t const delta_time)
{
    Process_method method;
    {
        static bool s_prev_sim_running{ false };
        bool const is_sim_running{ service_finder::find_service<world::World_properties_container>()
                                       .get_data_handle()
                                       .is_simulation_running };

        if (!s_prev_sim_running && is_sim_running)
            method = PROC_START_UI;
        else if (s_prev_sim_running && !is_sim_running)
            method = PROC_END_UI;
        else
            method = PROC_TICK_UI;

        s_prev_sim_running = is_sim_running;
    }

    auto& reg{ service_finder::find_service<Entity_container>().get_ecs_registry() };

    process_ui_hp_w_posture(method, ui_state, reg, delta_time);
    process_ui_ninsatu(method, ui_state, delta_time);
    process_ui_you_died(method, ui_state, delta_time);
}
