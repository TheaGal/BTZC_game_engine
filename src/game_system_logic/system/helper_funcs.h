#pragma once

#include "entt/entity/fwd.hpp"
#include "game_system_logic/entity_container.h"
#include "game_system_logic/component/character_movement.h"


namespace BT
{
namespace system
{
namespace helper
{

/// Fetches certain AFA data from animator. Return true if animator is found.
/// @param[in,out] out_request_new_attack data for whether CPU should queue up a new attack.
bool fetch_wanted_afa_data(Entity_container const& entity_container,
                           component::Character_mvt_animated_state const& char_mvt_anim_state,
                           bool& out_can_move,
                           bool& out_request_new_attack);

/// Calculates an AFA action map action index randomly.
int32_t calc_random_afa_action_map_action_idx(
    Entity_container const& entity_container,
    component::Character_mvt_animated_state const& char_mvt_anim_state,
    std::string const& action_map_name,
    float_t const distance_to_target);

}  // namespace helper
}  // namespace system
}  // namespace BT
