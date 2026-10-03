#include "helper_funcs.h"

#include "btrandom.h"
#include "entt/entity/fwd.hpp"
#include "game_system_logic/entity_container.h"
#include "game_system_logic/component/character_movement.h"
#include "service_finder/service_finder.h"
#include "txp_renderer_public.h"


bool BT::system::helper::fetch_wanted_afa_data(
    Entity_container const& entity_container,
    component::Character_mvt_animated_state const& char_mvt_anim_state,
    bool& out_can_move,
    bool& out_request_new_attack)
{
    auto& renderer{ service_finder::find_service<TXP::Renderer>() };
    auto animator_optional{ renderer.try_get_skeletal_animator(
        entity_container.find_entity(char_mvt_anim_state.affecting_animator_uuid)) };

    if (!animator_optional.has_value())
    {
        return false;
    }

    auto& animator{ animator_optional.value() };

    // Get animator AFA data.
    auto& afa_data{ animator.get_anim_frame_action_data_handle() };

    // Fill in data.
    using AFA_ctrl = TXP::anim_frame_action::Controllable_data_label;
    out_can_move           = afa_data.get_bool_data_handle(AFA_ctrl::CTRL_DATA_LABEL_can_move).get_val();
    out_request_new_attack = afa_data.get_reeve_data_handle(AFA_ctrl::CTRL_DATA_LABEL_request_new_attack).check_if_rising_edge_occurred();

    return true;
}

int32_t BT::system::helper::calc_random_afa_action_map_action_idx(
    Entity_container const& entity_container,
    component::Character_mvt_animated_state const& char_mvt_anim_state,
    std::string const& action_map_name,
    float_t const distance_to_target)
{

    auto& renderer{ service_finder::find_service<TXP::Renderer>() };
    auto animator_optional{ renderer.try_get_skeletal_animator(
        entity_container.find_entity(char_mvt_anim_state.affecting_animator_uuid)) };

    if (!animator_optional.has_value())
    {
        return -1;
    }

    auto const& animator{ animator_optional.value() };

    return animator.calc_action_map_weighted_action_idx(action_map_name,
                                                        distance_to_target,
                                                        random::fast_float_01_exclusive());
}
