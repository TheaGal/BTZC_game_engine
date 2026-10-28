#pragma once

#include "btglm.h"


namespace TXP
{
class Camera;  // forward decl
class UI_state;  // forward decl
} // namespace TXP


namespace BT
{
namespace ui_ifc
{

class Target_lockon_reticle_ui
{
public:
    Target_lockon_reticle_ui(TXP::UI_state& ui_state);

    void set_visible(bool const flag);
    void set_reticle_pos_world(TXP::Camera const& camera, vec3 world_pos);

private:
    TXP::UI_state& m_ui_state;

    bool m_prev_visible{ false };
};

} // namespace ui_ifc
} // namespace BT
