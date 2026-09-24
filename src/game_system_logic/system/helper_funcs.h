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
                           entt::registry& reg,
                           component::Character_mvt_animated_state const& char_mvt_anim_state,
                           bool& out_can_move,
                           bool& out_request_new_attack);

}  // namespace helper
}  // namespace system
}  // namespace BT
