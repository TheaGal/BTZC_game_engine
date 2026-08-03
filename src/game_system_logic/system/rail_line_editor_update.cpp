#include "rail_line_editor_update.h"

#include "entt/entity/registry.hpp"
#include "game_system_logic/component/rail_line.h"
#include "game_system_logic/entity_container.h"
#include "service_finder/service_finder.h"
#include "txp_renderer_public.h"


void BT::system::rail_line_editor_update()
{
    auto& reg{ service_finder::find_service<Entity_container>().get_ecs_registry() };

    // Write transforms.
    auto view{ reg.view<component::Rail_line>() };
    // auto view{
    //     reg.view<component::Rail_line const, TXP::component::Render_object_config>()
    // };
    for (auto&& [ent, rail_line] : view.each())
    {
        if (rail_line.built_ctor_code == rail_line.construction_code)
            continue;

        // Build construction code.
        
        // @TODO
        assert(false);

        rail_line.built_ctor_code = rail_line.construction_code;

        // Enforce rail-line render obj.
        if (!rail_line.created_render_object)
        {
            rail_line.created_render_object = true;

            auto& rend_obj_cfg = reg.emplace_or_replace<TXP::component::Render_object_config>(ent);
            rend_obj_cfg = {
                .render_layer = TXP::RENDER_LAYER_LEVEL_EDITOR,
                .model_name = "rail_line_editor_gizmo",
            };
        }
    }
}
