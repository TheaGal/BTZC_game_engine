#pragma once

#include <cmath>


namespace BT
{
namespace system
{

/// Depending on the enemy awareness status and the CPU behavior, submits inputs to move in a
/// certain way.
void cpu_character_world_space_input(float_t const delta_time);

}  // namespace system
}  // namespace BT
