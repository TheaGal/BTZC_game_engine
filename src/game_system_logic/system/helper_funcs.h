#pragma once

#include "entt/entity/fwd.hpp"


namespace BT
{
namespace system
{
namespace helper
{

/// Fetches certain AFA data from animator. Return true if animator is found.
/// @param[in,out] out_request_new_attack data for whether CPU should queue up a new attack.
bool fetch_wanted_afa_data(entt::entity const ecs_entity,
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
