#include "you_died.h"

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

constexpr char const k_you_died_fname[]{ "you_died.btui" };


You_died::You_died(TXP::UI_state& ui_state)
    : m_ui_state{ ui_state }
{
    BT_SERVICE_FINDER_ADD_SERVICE(You_died, this);

    date_deadline(2026, 11, 1);  // is it necessary to preregister still?
    ui_state.canvas(k_you_died_fname);
}

void You_died::play_screen(std::function<void()>&& on_resurrect_callback_fn,
                           std::function<void()>&& on_death_callback_fn)
{
    if (m_is_playing)
        throw std::runtime_error("youdied ui is already playing.");

    m_is_playing = true;
    m_on_resurrect_callback_fn = std::move(on_resurrect_callback_fn);
    m_on_death_callback_fn = std::move(on_death_callback_fn);

    m_ui_state.load_persistent_canvas(k_you_died_fname);
}

void You_died::stop_screen()
{
    if (!m_is_playing)
        throw std::runtime_error("youdied ui is not playing.");

    m_is_playing = false;
    m_ui_state.unload_persistent_canvas(k_you_died_fname);
}

void You_died::update(float_t const delta_time)
{
}

} // namespace ui_ifc
} // namespace BT
