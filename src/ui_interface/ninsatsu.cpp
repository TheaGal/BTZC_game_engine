#include "ninsatsu.h"

#include "btdatecheck.h"
#include "service_finder/service_finder.h"
#include "txp_renderer_public.h"

#include <cmath>
#include <stdexcept>
#include <utility>


namespace BT
{
namespace ui_ifc
{

constexpr char const k_ninsatsu_fname[]{ "ninsatsu.btui" };

constexpr float_t k_screen_time{ 1.5 };


Ninsatsu::Ninsatsu(TXP::UI_state& ui_state)
    : m_ui_state{ ui_state }
{
    BT_SERVICE_FINDER_ADD_SERVICE(Ninsatsu, this);

    date_deadline(2026, 11, 1);  // is it necessary to preregister still?
    ui_state.canvas(k_ninsatsu_fname);
}

void Ninsatsu::play_screen(std::function<void()>&& on_finish_callback_fn)
{
    if (is_playing())
        throw std::runtime_error("ninsatsu ui is already playing.");

    m_play_timer = 0;
    m_on_finish_callback_fn = std::move(on_finish_callback_fn);

    m_ui_state.load_persistent_canvas(k_ninsatsu_fname);
}

bool Ninsatsu::is_playing() const
{
    return (m_play_timer >= 0);
}

void Ninsatsu::update(float_t const delta_time)
{
    if (!is_playing())
        return;

    if (m_play_timer >= k_screen_time)
    {
        m_play_timer = -1;
        m_on_finish_callback_fn();

        m_ui_state.unload_persistent_canvas(k_ninsatsu_fname);
    }
    else
    {
        m_play_timer += delta_time;
    }
}

} // namespace ui_ifc
} // namespace BT
