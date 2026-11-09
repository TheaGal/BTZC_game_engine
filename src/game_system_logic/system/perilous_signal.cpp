#include "perilous_signal.h"

#include "entt/entity/fwd.hpp"
#include "game_system_logic/component/perilous_signal_receiver.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "game_system_logic/world/world_properties.h"
#include "service_finder/service_finder.h"
#include "txp_renderer_public.h"

#include <cassert>
#include <cmath>


void BT::system::signal_perilous()
{
    if (!service_finder::find_service<world::World_properties_container>()
             .get_data_handle()
             .is_simulation_running)
        return;

    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };

    auto view{ reg.view<component::Perilous_signal_receiver>() };

    for (auto&& [entity, peril_sign_rece] : view.each())
    {
        peril_sign_rece.time_since_last_perilous_msg = 0;
    }
}

void BT::system::update_perilous_signal(float_t const delta_time)
{
    if (!service_finder::find_service<world::World_properties_container>()
             .get_data_handle()
             .is_simulation_running)
        return;

    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };

    auto view{
        reg.view<component::Perilous_signal_receiver, TXP::component::Render_object_config>()
    };

    for (auto const& [entity, peril_sign_rece, rend_obj_cfg] : view.each())
    {
        constexpr float_t k_show_peril_sign_time{ 1 };
        if (peril_sign_rece.time_since_last_perilous_msg > k_show_peril_sign_time)
            continue;

        peril_sign_rece.time_since_last_perilous_msg += delta_time;
        rend_obj_cfg.is_visible =
            (peril_sign_rece.time_since_last_perilous_msg <= k_show_peril_sign_time);
    }
}
