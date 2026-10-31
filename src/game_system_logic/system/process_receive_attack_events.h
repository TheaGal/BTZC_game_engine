#pragma once

#include <cmath>


namespace BT
{
namespace system
{

/// Processes received attacks if an event has come thru.
void process_receive_attack_events(float_t const delta_time);

}  // namespace system
}  // namespace BT
