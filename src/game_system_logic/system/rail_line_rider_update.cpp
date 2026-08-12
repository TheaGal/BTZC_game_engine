#include "rail_line_rider_update.h"

#include "game_system_logic/component/physics_object_settings.h"
#include "game_system_logic/component/rail_line.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "service_finder/service_finder.h"


void BT::system::rail_line_rider_update()
{
    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };

    auto view{ reg.view<component::Rail_line_rider>() };

    for (auto&& [ent, line_rider] : view->each())
    {
        auto const& rail_line{ reg.get<component::Rail_line>(
            entity_container.find_entity(line_rider.riding_line_uuid)) };

        // Modulo the distance.
        double_t pos_mod{ line_rider.line_position };
        while (pos_mod >= rail_line.built_length)
        {
            pos_mod -= rail_line.built_length;
        }

        // There needs to be some easy way to go thru the lookup. Like a linked list type of structure?? Idk. It doesn't have to be O(n) lookup but it has to be bidirectionally traversable.

        if (false)
        {
            // Update transform.
            component::submit_transform_change_helper(reg, s_state.selected_entity, pos, rot, sca);
            component::try_set_physics_object_transform_helper(reg,
                                                               s_state.selected_entity,
                                                               pos,
                                                               rot);
        }
    }
}
