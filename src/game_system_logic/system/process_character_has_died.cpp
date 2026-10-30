#include "process_character_has_died.h"

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
#include "txp_renderer_public.h"

#include <cassert>
#include <cmath>

using Interrupt_event = BT::component::Game_interrupt_event_queue::Interrupt_event;


void BT::system::process_character_has_died(float_t const delta_time)
{   // Exit early if simulation not running.
    if (!service_finder::find_service<world::World_properties_container>()
             .get_data_handle()
             .is_simulation_running)
        return;

    auto* poss_gieq{ component::Game_interrupt_event_queue::instance() };
    if (poss_gieq == nullptr)
        return;

    auto& gieq{ *poss_gieq };

    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };
    auto view{ reg.view<component::Transform const, component::Character_mvt_animated_state>() };

    for (auto&& [entity, transform, char_mvt_anim_state] : view.each())
    {
        if (!char_mvt_anim_state.input_mvt_state.is_alive)
        {
            auto const inner_entity{ entity_container.find_entity(
                char_mvt_anim_state.affecting_animator_uuid) };

            auto* poss_health_stats_data{ reg.try_get<component::Health_stats_data>(inner_entity) };

            if (poss_health_stats_data == nullptr)
                continue;

            auto& health_stats_data{ *poss_health_stats_data };

            // Have resurrect screen for playchar death.
            if (health_stats_data.ui_name == "playchar")
            {
                if (!gieq.peek_event_has(Interrupt_event::EVE_TYPE_YOU_DIED_SCREEN))
                {
                    gieq.push_event({ .event_type = Interrupt_event::EVE_TYPE_YOU_DIED_SCREEN });

                    char_mvt_anim_state.input_mvt_state.choose_to_resurrect = false;
                    char_mvt_anim_state.input_mvt_state.choose_to_die = false;
                    char_mvt_anim_state.input_mvt_state.resurrect_make_choice_timer = 0;
                }

                auto const& input_handler{
                    service_finder::find_service<TXP::Input::Input_handler>()
                };

                bool const death_input_pressed{
                    input_handler.get_mouse_button_state(BT_MOUSE_BUTTON_RIGHT).pressed
                };
                bool const resurrect_input_pressed{
                    input_handler.get_mouse_button_state(BT_MOUSE_BUTTON_LEFT).pressed
                };

                constexpr float_t k_make_choice_time{ 8 };

                if (char_mvt_anim_state.input_mvt_state.resurrect_make_choice_timer >=
                        k_make_choice_time ||
                    death_input_pressed)
                {
                    // Do real die.
                    char_mvt_anim_state.input_mvt_state.choose_to_die = true;
                }
                else if (resurrect_input_pressed)
                {
                    // Do resurrect.
                    char_mvt_anim_state.input_mvt_state.choose_to_resurrect = true;

                    date_deadline(2026, 12, 1);
                    health_stats_data.health_pts = (health_stats_data.max_health_pts / 2);  // @TODO: replace this with a health stats driver like `.do_heal(.max_health() * 0.5);`
                    health_stats_data.posture_pts = 0;  // @TODO: ditto above `.zero_out_posture()`
                }

                char_mvt_anim_state.input_mvt_state.resurrect_make_choice_timer += delta_time;
            }
            // Just do regular die for NPCs.
            else
            {
                // Remove physics object.
                reg.remove<component::Physics_object_settings>(entity);
                reg.remove<component::Physics_obj_type_char_con_settings>(entity);
                reg.remove<component::Created_physics_object_reference>(entity);

                // Remove health component to ensure this isn't alive anymore.

                bool const is_boss_oppo{ !health_stats_data.ui_name.empty() &&
                                         health_stats_data.ui_name != "playchar" };

                reg.remove<component::Health_stats_data>(inner_entity);

                // Add ninsatsu interrupt.
                if (is_boss_oppo)
                {
                    gieq.push_event({ .event_type = Interrupt_event::EVE_TYPE_NINSATSU_SCREEN });
                }

                // @TODO: change the model into a ragdoll.
                date_deadline(2026, 12, 1);

                BT_TRACEF("Processed entity dying (entity: %u)", entity);
            }
        }
    }
}
