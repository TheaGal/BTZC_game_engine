#pragma once

#include <cmath>
#include <functional>


namespace TXP
{
class UI_state;  // forward decl
} // namespace TXP


namespace BT
{
namespace ui_ifc
{

class Ninsatsu
{
public:
    Ninsatsu(TXP::UI_state& ui_state);

    void play_screen(std::function<void()>&& on_finish_callback_fn);
    bool is_playing() const;

    void update(float_t const delta_time);

private:
    TXP::UI_state& m_ui_state;

    float_t m_play_timer{ -1 };
    std::function<void()> m_on_finish_callback_fn;
};

} // namespace ui_ifc
} // namespace BT
