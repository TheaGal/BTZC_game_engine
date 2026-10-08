////////////////////////////////////////////////////////////////////////////////////////////////////
/// @copyright 2025 Thea Bennett
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief ECS system for locking onto another character. It faces the player character towards the
///        other char, and faces the camera towards the other char.
///        If the other char is missing, then it nullifies the uuid for the other char.
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cmath>


namespace BT
{
namespace system
{

/// Processes input and following entity for locking the camera onto an opponent/other character.
void player_character_lock_onto_target(float_t const delta_time);

}  // namespace system
}  // namespace BT
