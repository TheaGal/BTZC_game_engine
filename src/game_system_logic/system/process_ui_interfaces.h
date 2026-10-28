#pragma once

#include <cmath>


namespace TXP
{
class UI_state;  // forward decl
} // namespace TXP


namespace BT
{
namespace system
{

/// Process all UI interfaces.
void process_ui_interfaces(TXP::UI_state& ui_state, float_t const delta_time);

}  // namespace system
}  // namespace BT
