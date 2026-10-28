#include "target_lockon_reticle_ui.h"

#include "btdatecheck.h"
#include "service_finder/service_finder.h"
#include "txp_renderer_public.h"


namespace BT
{
namespace ui_ifc
{

constexpr char const k_target_lockon_reticle_fname[]{ "target_lockon_reticle.btui" };


Target_lockon_reticle_ui::Target_lockon_reticle_ui(TXP::UI_state& ui_state)
    : m_ui_state{ ui_state }
{
    BT_SERVICE_FINDER_ADD_SERVICE(Target_lockon_reticle_ui, this);

    date_deadline(2026, 11, 1);  // is it necessary to preregister still?
    ui_state.canvas(k_target_lockon_reticle_fname);
}

void Target_lockon_reticle_ui::set_visible(bool const flag)
{
    if (m_prev_visible == flag)
        return;

    if (flag)
        m_ui_state.load_persistent_canvas(k_target_lockon_reticle_fname);
    else
        m_ui_state.unload_persistent_canvas(k_target_lockon_reticle_fname);

    m_prev_visible = flag;
}

void Target_lockon_reticle_ui::set_reticle_pos_world(TXP::Camera const& camera, vec3 world_pos)
{
    auto& ui_canvas{ m_ui_state.canvas(k_target_lockon_reticle_fname) };
    if (!ui_canvas.is_loaded())
        return;

    vec2 ui_position;
    bool const is_visible =
        TXP::convert_world_position_to_ui_position(camera, world_pos, ui_position);

    auto& reticle_ui_elem{ ui_canvas.elem("lockon_reticle") };

    reticle_ui_elem.set_opacity(is_visible ? 1 : 0);
    if (is_visible)
    {
        reticle_ui_elem.set_position(ui_position[0], ui_position[1]);
    }
}

} // namespace ui_ifc
} // namespace BT
