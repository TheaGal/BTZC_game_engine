#include "hp_w_posture_ui.h"

#include "btdatecheck.h"
#include "service_finder/service_finder.h"
#include "txp_renderer_public.h"

#include <stdexcept>


namespace BT
{
namespace ui_ifc
{

constexpr char const k_playchar_hp_w_posture_fname[]{ "playchar_hp_w_posture.btui" };
constexpr char const k_boss_oppo_hp_w_posture_fname[]{ "boss_oppo_hp_w_posture.btui" };
constexpr char const k_reg_oppo_hp_w_posture_fname[]{ "reg_oppo_hp_w_posture.btui" };

char const* canvas_name_to_str(Hp_w_posture::Canvas_name const canvas_name)
{
    switch (canvas_name)
    {
    case Hp_w_posture::CANVAS_PLAYCHAR:
        return k_playchar_hp_w_posture_fname;

    case Hp_w_posture::CANVAS_BOSS_OPPO:
        return k_boss_oppo_hp_w_posture_fname;

    default:
        throw std::runtime_error("canvas name does not exist");
    }
}


Hp_w_posture::Hp_w_posture(TXP::UI_state& ui_state)
    : m_ui_state{ ui_state }
{
    BT_SERVICE_FINDER_ADD_SERVICE(Hp_w_posture, this);

    date_deadline(2026, 11, 1);  // is it necessary to preregister still?
    ui_state.canvas(k_playchar_hp_w_posture_fname);
    ui_state.canvas(k_boss_oppo_hp_w_posture_fname);
    ui_state.canvas(k_reg_oppo_hp_w_posture_fname);
}

void Hp_w_posture::set_visible(Canvas_name const canvas_name, bool const flag)
{
    bool* prev_visible{ nullptr };

    switch (canvas_name)
    {
    case CANVAS_PLAYCHAR:
        prev_visible = &m_prev_playchar_visible;
        break;

    case CANVAS_BOSS_OPPO:
        prev_visible = &m_prev_boss_oppo_visible;
        break;
    }

    if (*prev_visible == flag)
        return;

    char const* ui_canvas_fname{ canvas_name_to_str(canvas_name) };

    if (flag)
        m_ui_state.load_persistent_canvas(ui_canvas_fname);
    else
        m_ui_state.unload_persistent_canvas(ui_canvas_fname);

    *prev_visible = flag;
}

void Hp_w_posture::set_hp_bar(Canvas_name const canvas_name,
                              int32_t const health_pts,
                              int32_t const max_health_pts)
{
    auto& ui_canvas{ m_ui_state.canvas(canvas_name_to_str(canvas_name)) };
    ui_canvas.elem("hp_back").set_width(max_health_pts + 4);
    ui_canvas.elem("hp_inside").set_width(health_pts);
}

void Hp_w_posture::set_posture_bar(Canvas_name const canvas_name,
                                   int32_t const posture_pts,
                                   int32_t const max_posture_pts)
{
    auto& ui_canvas{ m_ui_state.canvas(canvas_name_to_str(canvas_name)) };
    ui_canvas.elem("posture_back").set_width(max_posture_pts + 4);
    ui_canvas.elem("posture_inside").set_width(posture_pts);
}

void Hp_w_posture::set_boss_oppo_name(std::string const& name)
{
    auto& ui_canvas{ m_ui_state.canvas(k_boss_oppo_hp_w_posture_fname) };
    if (!ui_canvas.is_loaded())
        return;

    ui_canvas.elem("boss_name_label").set_text(name);
}

} // namespace ui_ifc
} // namespace BT
