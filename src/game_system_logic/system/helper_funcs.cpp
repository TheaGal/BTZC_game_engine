#include "helper_funcs.h"

#include "entt/entity/fwd.hpp"
#include "game_system_logic/entity_container.h"
#include "game_system_logic/component/character_movement.h"
#include "service_finder/service_finder.h"
#include "txp_renderer_public.h"


void BT::system::helper::fetch_wanted_afa_data(
    Entity_container const& entity_container,
    entt::registry& reg,
    component::Character_mvt_animated_state const& char_mvt_anim_state,
    bool& out_can_move,
    bool& out_can_guard_exit,
    bool& out_can_attack_exit)
{
    auto& renderer{ service_finder::find_service<TXP::Renderer>() };
    auto animator_optional{ renderer.try_get_skeletal_animator(
        entity_container.find_entity(char_mvt_anim_state.affecting_animator_uuid)) };

    if (!animator_optional.has_value())
        return;  // Exit since animator not found.

    auto& animator{ animator_optional.value() };

    // Get animator AFA data.
    auto& afa_data{ animator.get_anim_frame_action_data_handle() };

    // Fill in data.
    using AFA_ctrl = TXP::anim_frame_action::Controllable_data_label;
    out_can_move        = afa_data.get_bool_data_handle(AFA_ctrl::CTRL_DATA_LABEL_can_move).get_val();
    out_can_guard_exit  = afa_data.get_bool_data_handle(AFA_ctrl::CTRL_DATA_LABEL_can_guard_exit).get_val();
    out_can_attack_exit = afa_data.get_bool_data_handle(AFA_ctrl::CTRL_DATA_LABEL_can_attack_exit).get_val();
}
