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

class You_died
{
public:
    You_died(TXP::UI_state& ui_state);

    void play_screen(std::function<void()>&& on_resurrect_callback_fn,
                     std::function<void()>&& on_death_callback_fn);
    void stop_screen();

    inline bool is_playing() const
    {
        return m_is_playing;
    }

    void update(float_t const delta_time);

private:
    TXP::UI_state& m_ui_state;

    bool m_is_playing{ false };
    std::function<void()> m_on_resurrect_callback_fn;
    std::function<void()> m_on_death_callback_fn;
};

} // namespace ui_ifc
} // namespace BT
