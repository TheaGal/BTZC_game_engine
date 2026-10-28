#include "process_ui_interfaces.h"

#include "entt/entity/fwd.hpp"
#include "game_system_logic/component/follow_camera.h"
#include "game_system_logic/component/health_stats.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "game_system_logic/world/world_properties.h"
#include "service_finder/service_finder.h"
#include "txp_renderer/renderer.h"
#include "txp_renderer_public.h"
#include "ui_interface/hp_w_posture_ui.h"
#include "ui_interface/target_lockon_reticle_ui.h"

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
        ui_ifc::Hp_w_posture::Canvas_name canvas_name;

        if (health_stats_data.ui_name == "playchar")
        {
            canvas_name = ui.CANVAS_PLAYCHAR;
            found_playchar = true;
        }
        else if (!health_stats_data.ui_name.empty())
        {
            canvas_name = ui.CANVAS_BOSS_OPPO;
            found_boss_oppo = true;
            ui.set_boss_oppo_name(health_stats_data.ui_name);
        }
        else
        {
            throw std::runtime_error("whoopsie not implemente dyet!!");
        }

        ui.set_hp_bar(canvas_name, health_stats_data.health_pts, health_stats_data.max_health_pts);
        ui.set_posture_bar(canvas_name,
                           health_stats_data.posture_pts,
                           health_stats_data.max_posture_pts);
    }

    ui.set_visible(ui.CANVAS_PLAYCHAR, found_playchar);
    ui.set_visible(ui.CANVAS_BOSS_OPPO, found_boss_oppo);
}

void process_ui_target_lockon_reticle(Process_method const method,
                                      TXP::UI_state& ui_state,
                                      Entity_container const& entity_container,
                                      entt::registry& reg,
                                      float_t const delta_time)
{
    auto& ui{ service_finder::find_service<ui_ifc::Target_lockon_reticle_ui>() };

    bool found_locked_on_entity{ false };

    for (auto&& [entity, follow_cam_follow_ref] :
         reg.view<component::Follow_camera_follow_ref>().each())
    {
        if (!follow_cam_follow_ref.state.locked_on_entity.is_nil() &&
            entity_container.entity_exists(follow_cam_follow_ref.state.locked_on_entity))
        {
            entt::entity locked_on_entity{ entity_container.find_entity(
                follow_cam_follow_ref.state.locked_on_entity) };
            if (reg.all_of<component::Transform, component::Follow_camera_lockon_target>(
                    locked_on_entity))
            {
                auto& transform{ reg.get<component::Transform const>(locked_on_entity) };
                auto& cam_lockon_target{ reg.get<component::Follow_camera_lockon_target const>(
                    locked_on_entity) };

                vec3 target_locked_on_pos;
                btglm_rvec3_to_vec3(transform.position.raw, target_locked_on_pos);
                target_locked_on_pos[1] += cam_lockon_target.follow_offset_y;

                // Record locked on position.
                found_locked_on_entity = true;
                ui.set_reticle_pos_world(
                    service_finder::find_service<TXP::Renderer>().get_main_camera(),
                    target_locked_on_pos);
                break;
            }
        }
    }

    ui.set_visible(found_locked_on_entity);
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

    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };

    process_ui_hp_w_posture(method, ui_state, reg, delta_time);
    process_ui_target_lockon_reticle(method, ui_state, entity_container, reg, delta_time);
    process_ui_ninsatu(method, ui_state, delta_time);
    process_ui_you_died(method, ui_state, delta_time);
}
