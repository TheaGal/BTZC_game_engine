#include "process_finish_deathblow.h"

#include "btdatecheck.h"
#include "btuuid.h"
#include "game_system_logic/component/character_movement.h"
#include "game_system_logic/component/game_interrupt_event_queue.h"
#include "game_system_logic/component/health_stats.h"
#include "game_system_logic/component/physics_object_settings.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "game_system_logic/world/world_properties.h"
#include "physics_engine/physics_engine.h"
#include "service_finder/service_finder.h"

#include <cassert>


void BT::system::process_finish_deathblow()
{   // Exit early if simulation not running.
    if (!service_finder::find_service<world::World_properties_container>()
             .get_data_handle()
             .is_simulation_running)
        return;

    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };
    auto view{ reg.view<component::Transform const, component::Character_mvt_animated_state>() };

    for (auto&& [entity, transform, char_mvt_anim_state] : view.each())
    {
        // Finishing step for deathblow giver.
        if (char_mvt_anim_state.input_mvt_state.on_finish_deathblow_give)
        {
            // Reactivate inner body for physics char con.
            if (auto* poss_ref = reg.try_get<component::Created_physics_object_reference>(entity);
                poss_ref != nullptr)
            {
                auto& phys_engine{ service_finder::find_service<Physics_engine>() };

                auto& phys_obj{ *phys_engine.checkout_physics_object(
                    poss_ref->physics_obj_uuid_ref) };

                phys_obj.get_impl()->set_cc_use_inner_body(true);

                phys_engine.return_physics_object(&phys_obj);
            }

            BT_TRACEF("Finished deathblow give (entity: %u)", entity);
        }
        // Finishing step for deathblow receiver.
        else if (char_mvt_anim_state.input_mvt_state.on_finish_deathblow_receive)
        {
            // Remove physics object.
            reg.remove<component::Physics_object_settings>(entity);
            reg.remove<component::Physics_obj_type_char_con_settings>(entity);
            reg.remove<component::Created_physics_object_reference>(entity);

            // Remove health component to ensure this isn't alive anymore.
            auto const inner_entity{ entity_container.find_entity(
                char_mvt_anim_state.affecting_animator_uuid) };

            auto const& health_stats_data{ reg.get<component::Health_stats_data>(inner_entity) };
            bool const is_boss_oppo{ !health_stats_data.ui_name.empty() &&
                                     health_stats_data.ui_name != "playchar" };

            reg.remove<component::Health_stats_data>(inner_entity);

            // Add ninsatsu interrupt.
            if (is_boss_oppo)
            {
                component::Game_interrupt_event_queue::comp_inst()->push_event(
                    { .event_type = component::Game_interrupt_event_queue::Interrupt_event::
                          EVE_TYPE_NINSATSU_SCREEN });
            }

            // @TODO: change the model into a ragdoll.
            date_deadline(2026, 12, 1);

            BT_TRACEF("Finished deathblow receive (entity: %u)", entity);
        }
    }
}
