#pragma once

#include <cstdint>
#include <string>


namespace TXP
{
class UI_state;  // forward decl
} // namespace TXP


namespace BT
{
namespace ui_ifc
{

class Hp_w_posture
{
public:
    Hp_w_posture(TXP::UI_state& ui_state);

    enum Canvas_name
    {
        CANVAS_PLAYCHAR,
        CANVAS_BOSS_OPPO,
    };

    void set_visible(Canvas_name const canvas_name, bool const flag);

    void set_hp_bar(Canvas_name const canvas_name,
                    int32_t const health_pts,
                    int32_t const max_health_pts);
    void set_posture_bar(Canvas_name const canvas_name,
                         int32_t const posture_pts,
                         int32_t const max_posture_pts);

    void set_boss_oppo_name(std::string const& name);

private:
    TXP::UI_state& m_ui_state;

    bool m_prev_playchar_visible{ false };
    bool m_prev_boss_oppo_visible{ false };
};

} // namespace ui_ifc
} // namespace BT
