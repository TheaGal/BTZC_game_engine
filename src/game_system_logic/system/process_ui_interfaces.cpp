#include "process_ui_interfaces.h"

#include "btdatecheck.h"
#include "btuuid.h"
#include "game_system_logic/component/character_movement.h"
#include "game_system_logic/component/physics_object_settings.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "game_system_logic/world/world_properties.h"
#include "physics_engine/physics_engine.h"
#include "service_finder/service_finder.h"
#include "txp_renderer_public.h"

#include <cassert>


namespace
{

using namespace BT;

void process_ui_ninsatu(bool const is_sim_running,
                        TXP::UI_state& ui_state,
                        float_t const delta_time)
{}

void process_ui_you_died(bool const is_sim_running,
                         TXP::UI_state& ui_state,
                         float_t const delta_time)
{}

} // namespace


void BT::system::process_ui_interfaces(TXP::UI_state& ui_state, float_t const delta_time)
{   // Exit early if simulation not running.
    bool const is_sim_running{ service_finder::find_service<world::World_properties_container>()
                                   .get_data_handle()
                                   .is_simulation_running };

    process_ui_ninsatu(is_sim_running, ui_state, delta_time);
    process_ui_you_died(is_sim_running, ui_state, delta_time);
}
