#include "helper_funcs.h"

#include "animation_frame_action_tool/runtime_data.h"
#include "entt/entity/fwd.hpp"
#include "game_system_logic/entity_container.h"
#include "game_system_logic/component/character_movement.h"
#include "service_finder/service_finder.h"


void BT::system::helper::fetch_wanted_afa_data(
    Entity_container const& entity_container,
    entt::registry& reg,
    component::Character_mvt_animated_state const& char_mvt_anim_state,
    bool& out_can_move,
    bool& out_can_guard_exit,
    bool& out_can_attack_exit)
{   // @NOTE: BRUH I HATE HOW DIFFICULT IT IS TO ACCESS THE ANIMATOR DATA IT'S SO
    //        FREAKIN STUPID WHY DID I DESIGN THE SYSTEM LIKE THIS PLEEEEEAAAAASE CHANGE
    //        IT AT SOME POINT WTF!!!!!!  -Thea 2025/11/24
    auto rend_obj_ref{ reg.try_get<component::Created_render_object_reference>(
        entity_container.find_entity(char_mvt_anim_state.affecting_animator_uuid)) };

    if (!rend_obj_ref)
        return;  // Exit since rend_obj_ref not found.

    // Get animator AFA data.
    auto& rend_obj_pool{ service_finder::find_service<Renderer>().get_render_object_pool() };
    auto& rend_obj{
        *rend_obj_pool.checkout_render_obj_by_key({ rend_obj_ref->render_obj_uuid_ref }).front()
    };

    if (auto animator{ rend_obj.get_model_animator() })
    {
        auto& afa_data{ animator->get_anim_frame_action_data_handle() };

        // Fill in data.
        out_can_move        = afa_data.get_bool_data_handle(anim_frame_action::CTRL_DATA_LABEL_can_move).get_val();
        out_can_guard_exit  = afa_data.get_bool_data_handle(anim_frame_action::CTRL_DATA_LABEL_can_guard_exit).get_val();
        out_can_attack_exit = afa_data.get_bool_data_handle(anim_frame_action::CTRL_DATA_LABEL_can_attack_exit).get_val();
    }

    rend_obj_pool.return_render_objs({ &rend_obj });
}
