#include "rail_line_editor_update.h"

#include "entt/entity/registry.hpp"
#include "game_system_logic/component/rail_line.h"
#include "game_system_logic/entity_container.h"
#include "service_finder/service_finder.h"
#include "txp_renderer_public.h"

#include <stdexcept>


void BT::system::rail_line_editor_update()
{
    auto& reg{ service_finder::find_service<Entity_container>().get_ecs_registry() };

    auto create_rail_piece_fn = [&reg](std::string const& model_name,
                                       std::string const& sub_mesh_name) {
        auto new_ent = reg.create();
        auto& rend_obj_cfg = reg.emplace_or_replace<TXP::component::Render_object_config>(new_ent);
        rend_obj_cfg.model_name = model_name;
        rend_obj_cfg.sub_mesh_name = sub_mesh_name;
        rend_obj_cfg.sub_mesh_zero_origin_position = true;
    };

    // Write transforms.
    auto view{ reg.view<component::Rail_line>() };

    for (auto&& [ent, rail_line] : view.each())
    {
        if (rail_line.built_ctor_code == rail_line.construction_code)
            continue;

        rail_line.built_ctor_code = rail_line.construction_code;

        // Clear all entities.
        for (auto ent : rail_line.created_entities)
        {
            reg.destroy(ent);
        }
        rail_line.created_entities.clear();

        // Build construction code.
        enum Mode : int32_t
        {
            FLAT = 0,
            INCLINE,
            DECLINE,
        } ctor_mode{ 0 };

        for (auto code_char : rail_line.construction_code)
        {
            std::string curve_code;

            switch (code_char)
            {
            case 's':
                // Add straight.
                switch (ctor_mode)
                {
                case FLAT:
                {
                    create_rail_piece_fn("rails", "StraightRail");
                    break;
                }
                case INCLINE:
                {
                    create_rail_piece_fn("rails", "SlopedRail.U");
                    break;
                }
                case DECLINE:
                {
                    create_rail_piece_fn("rails", "SlopedRail.D");
                    break;
                }
                default:
                    throw std::runtime_error("huh?");
                    break;
                }
                break;

            case 'q':
                if (curve_code.empty())
                    curve_code = "L.001";
            case 'w':
                if (curve_code.empty())
                    curve_code = "L.002";
            case 'e':
                if (curve_code.empty())
                    curve_code = "L.003";
            case 'r':
                if (curve_code.empty())
                    curve_code = "L.004";
            case 'p':
                if (curve_code.empty())
                    curve_code = "R.001";
            case 'o':
                if (curve_code.empty())
                    curve_code = "R.002";
            case 'i':
                if (curve_code.empty())
                    curve_code = "R.003";
            case 'u':
                if (curve_code.empty())
                    curve_code = "R.004";

                // Make sure that the curve is built in flat mode.
                if (ctor_mode != FLAT)
                    throw std::runtime_error("huh?");

                // Add curve.
                create_rail_piece_fn("rails", "CurveRail." + curve_code);
                break;

            case '(':
                if (ctor_mode != FLAT)
                    throw std::runtime_error("huh?");

                // Add incline start.
                create_rail_piece_fn("rails", "SlopechangeRail.U");
                ctor_mode = INCLINE;
                break;

            case ')':
                if (ctor_mode != INCLINE)
                    throw std::runtime_error("huh?");

                // Add incline end.
                create_rail_piece_fn("rails", "SlopechangeRail.UR");
                ctor_mode = FLAT;
                break;

            case '[':
                if (ctor_mode != FLAT)
                    throw std::runtime_error("huh?");

                // Add decline start.
                create_rail_piece_fn("rails", "SlopechangeRail.D");
                ctor_mode = DECLINE;
                break;

            case ']':
                if (ctor_mode != DECLINE)
                    throw std::runtime_error("huh?");

                // Add decline end.
                create_rail_piece_fn("rails", "SlopechangeRail.DR");
                ctor_mode = FLAT;
                break;

            default:
                throw std::runtime_error("This code char not implemented.");
                break;
            }
        }

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
