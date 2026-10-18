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
                           bool& out_request_new_attack,
                           bool& out_request_new_attack_early);

bool check_for_receive_atk_afa_events(
    entt::entity ecs_entity,
    bool& out_receive_atk_as_hurt,
    bool& out_receive_atk_as_guard,
    bool& out_receive_atk_as_parry);

}  // namespace helper
}  // namespace system
}  // namespace BT
