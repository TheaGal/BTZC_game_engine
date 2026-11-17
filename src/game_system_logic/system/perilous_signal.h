#pragma once

#include <cmath>


namespace BT
{
namespace system
{

/// Signals the perilous message to all.
void signal_perilous();

/// Processes perilous signals and animation.
void update_perilous_signal(float_t const delta_time);

}  // namespace system
}  // namespace BT
