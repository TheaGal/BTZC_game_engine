#include "rail_line_editor_update.h"

#include "btuuid.h"
#include "entt/entity/registry.hpp"
#include "game_system_logic/component/rail_line.h"
#include "game_system_logic/entity_container.h"
#include "service_finder/service_finder.h"
#include "txp_renderer_public.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>


void BT::system::rail_line_editor_update()
{
    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };

    auto create_rail_piece_fn = [&entity_container,
                                 &reg](component::Rail_line& rail_line,
                                       component::Rail_line::Build_code build_code,
                                       std::string const& sub_mesh_name,
                                       vec3 mesh_origin_pos,
                                       vec3& place_pos,
                                       float_t& place_angle) {
        auto const& build_template{component::Rail_line::s_build_code_to_info_map.at(build_code)};

        rail_line.built_length += build_template.length;
        rail_line.built_ctor_code.emplace_back(build_code);

        // @NOTE: using registry::create() directly here guarantees the created rail objects are not
        //        seen by the object registry. Also, it bypasses having to create a UUID, though
        //        that means that nothing can directly interact with the rail line pieces, which is
        //        intended.  -Thea 2026/08/11
        auto new_ent = entity_container.create_entity(UUID_helper::generate_uuid());
        rail_line.created_entities.emplace_back(new_ent);

        auto& rend_obj_cfg = reg.emplace_or_replace<TXP::component::Render_object_config>(new_ent);
        rend_obj_cfg.model_name = "rails";
        rend_obj_cfg.sub_mesh_name = sub_mesh_name;

        glm_translate(rend_obj_cfg.transform.raw, place_pos);
        glm_rotate_y(rend_obj_cfg.transform.raw, place_angle, rend_obj_cfg.transform.raw);

        glm_vec3_negate(mesh_origin_pos);
        glm_translate(rend_obj_cfg.transform.raw, mesh_origin_pos);

        build_template.advance_place_transform(place_pos, place_angle, true);
    };

    // Write transforms.
    auto view{ reg.view<component::Rail_line>() };

    for (auto&& [ent, rail_line] : view.each())
    {
        if (rail_line.prev_construction_code == rail_line.construction_code)
            continue;

        rail_line.prev_construction_code = rail_line.construction_code;

        rail_line.built_length = 0;
        rail_line.built_ctor_code.clear();

        // Clear all entities.
        for (auto ent : rail_line.created_entities)
        {
            entity_container.destroy_entity(entity_container.find_entity_uuid(ent));
        }
        rail_line.created_entities.clear();

        // Build construction code.
        enum Mode : int32_t
        {
            FLAT = 0,
            INCLINE,
            DECLINE,
        } ctor_mode{ 0 };

        enum Tilt : int32_t
        {
            NO_TILT = 0,
            LEFT_TILT,
            RIGHT_TILT,
        };
        Tilt ctor_tilt{ 0 };
        Tilt prev_ctor_tilt{ ctor_tilt };

        vec3 current_pos = GLM_VEC3_ZERO_INIT;
        float_t current_angle{ 0 };

        using Build_code = component::Rail_line::Build_code;

        auto process_tilt_connection_fn =
            [&create_rail_piece_fn,
             &rail_line,
             &prev_ctor_tilt,
             &ctor_tilt,
             &current_pos,
             &current_angle]() {
                // Add connecting tilt pieces.
                switch (prev_ctor_tilt)
                {
                case NO_TILT:
                    // Do nothing.
                    break;

                case LEFT_TILT:
                    create_rail_piece_fn(rail_line,
                                         Build_code::BC_STRAIGHT_ROLL_LEFT_RETURN,
                                         "StraightRailRoll.LR",
                                         vec3{ -14, 0, 0 },
                                         current_pos,
                                         current_angle);
                    break;

                case RIGHT_TILT:
                    create_rail_piece_fn(rail_line,
                                         Build_code::BC_STRAIGHT_ROLL_RIGHT_RETURN,
                                         "StraightRailRoll.RR",
                                         vec3{ -16, 0, 0 },
                                         current_pos,
                                         current_angle);
                    break;

                default:
                    throw std::runtime_error("huh?");
                    break;
                }

                switch (ctor_tilt)
                {
                case NO_TILT:
                    // Do nothing.
                    break;

                case LEFT_TILT:
                    create_rail_piece_fn(rail_line,
                                         Build_code::BC_STRAIGHT_ROLL_LEFT,
                                         "StraightRailRoll.L",
                                         vec3{ -18, 0, 0 },
                                         current_pos,
                                         current_angle);
                    break;

                case RIGHT_TILT:
                    create_rail_piece_fn(rail_line,
                                         Build_code::BC_STRAIGHT_ROLL_RIGHT,
                                         "StraightRailRoll.R",
                                         vec3{ -20, 0, 0 },
                                         current_pos,
                                         current_angle);
                    break;

                default:
                    throw std::runtime_error("huh?");
                    break;
                }
            };

        for (size_t code_char_i = 0; code_char_i < rail_line.construction_code.size();
             code_char_i++)
        {
            auto code_char{ rail_line.construction_code[code_char_i] };

            std::string curve_code;
            vec3s curve_origin = GLM_VEC3_ZERO_INIT;
            Build_code curve_build_code;

            switch (code_char)
            {
            case 's':
                // Add straight.
                ctor_tilt = NO_TILT;

                switch (ctor_mode)
                {
                case FLAT:
                {
                    process_tilt_connection_fn();

                    if (prev_ctor_tilt == NO_TILT)
                        create_rail_piece_fn(rail_line,
                                             Build_code::BC_STRAIGHT,
                                             "StraightRail",
                                             vec3{ -22, 0, 0 },
                                             current_pos,
                                             current_angle);
                    break;
                }
                case INCLINE:
                {
                    create_rail_piece_fn(rail_line,
                                         Build_code::BC_STRAIGHT_UPHILL,
                                         "SlopedRail.U",
                                         vec3{ 18, 0, 0 },
                                         current_pos,
                                         current_angle);
                    break;
                }
                case DECLINE:
                {
                    create_rail_piece_fn(rail_line,
                                         Build_code::BC_STRAIGHT_DOWNHILL,
                                         "SlopedRail.D",
                                         vec3{ 12, 0, 0 },
                                         current_pos,
                                         current_angle);
                    break;
                }
                default:
                    throw std::runtime_error("huh?");
                    break;
                }
                break;

            case 'q':
                if (curve_code.empty())
                {
                    curve_code = "L.001";

                    curve_origin.x = 38.8178;

                    curve_build_code = Build_code::BC_CURVE_LEFT_1;

                    ctor_tilt = LEFT_TILT;
                }
            case 'w':
                if (curve_code.empty())
                {
                    curve_code = "L.002";

                    curve_origin.x = 42.8178;

                    curve_build_code = Build_code::BC_CURVE_LEFT_2;

                    ctor_tilt = LEFT_TILT;
                }
            case 'e':
                if (curve_code.empty())
                {
                    curve_code = "L.003";

                    curve_origin.x = 46.8178;

                    curve_build_code = Build_code::BC_CURVE_LEFT_3;

                    ctor_tilt = LEFT_TILT;
                }
            case 'r':
                if (curve_code.empty())
                {
                    curve_code = "L.004";

                    curve_origin.x = 50.8178;

                    curve_build_code = Build_code::BC_CURVE_LEFT_4;

                    ctor_tilt = LEFT_TILT;
                }
            case 'p':
                if (curve_code.empty())
                {
                    curve_code = "R.001";

                    curve_origin.x = -38.8178;

                    curve_build_code = Build_code::BC_CURVE_RIGHT_1;

                    ctor_tilt = RIGHT_TILT;
                }
            case 'o':
                if (curve_code.empty())
                {
                    curve_code = "R.002";

                    curve_origin.x = -42.8178;

                    curve_build_code = Build_code::BC_CURVE_RIGHT_2;

                    ctor_tilt = RIGHT_TILT;
                }
            case 'i':
                if (curve_code.empty())
                {
                    curve_code = "R.003";

                    curve_origin.x = -46.8178;

                    curve_build_code = Build_code::BC_CURVE_RIGHT_3;

                    ctor_tilt = RIGHT_TILT;
                }
            case 'u':
                if (curve_code.empty())
                {
                    curve_code = "R.004";

                    curve_origin.x = -50.8178;

                    curve_build_code = Build_code::BC_CURVE_RIGHT_4;

                    ctor_tilt = RIGHT_TILT;
                }

                // Make sure that the curve is built in flat mode.
                if (ctor_mode != FLAT)
                    throw std::runtime_error("huh?");

                if (prev_ctor_tilt != ctor_tilt)
                {
                    process_tilt_connection_fn();

                    if (ctor_tilt == NO_TILT)
                        throw std::runtime_error("huh?");
                }

                // Add curve.
                create_rail_piece_fn(rail_line,
                                     curve_build_code,
                                     "CurveRail." + curve_code,
                                     curve_origin.raw,
                                     current_pos,
                                     current_angle);
                break;

            case '(':
                if (ctor_mode != FLAT)
                    throw std::runtime_error("huh?");

                ctor_tilt = NO_TILT;

                process_tilt_connection_fn();

                // Add incline start.
                create_rail_piece_fn(rail_line,
                                     Build_code::BC_SLOPECHANGE_UP,
                                     "SlopechangeRail.U",
                                     vec3{ 16, 0, 0 },
                                     current_pos,
                                     current_angle);
                ctor_mode = INCLINE;
                break;

            case ')':
                if (ctor_mode != INCLINE)
                    throw std::runtime_error("huh?");

                // Add incline end.
                create_rail_piece_fn(rail_line,
                                     Build_code::BC_SLOPECHANGE_UP_RETURN,
                                     "SlopechangeRail.UR",
                                     vec3{ 20, 0, 0 },
                                     current_pos,
                                     current_angle);
                ctor_mode = FLAT;
                ctor_tilt = NO_TILT;
                break;

            case '[':
                if (ctor_mode != FLAT)
                    throw std::runtime_error("huh?");

                ctor_tilt = NO_TILT;

                process_tilt_connection_fn();

                // Add decline start.
                create_rail_piece_fn(rail_line,
                                     Build_code::BC_SLOPECHANGE_DOWN,
                                     "SlopechangeRail.D",
                                     vec3{ 10, 0, 0 },
                                     current_pos,
                                     current_angle);
                ctor_mode = DECLINE;
                break;

            case ']':
                if (ctor_mode != DECLINE)
                    throw std::runtime_error("huh?");

                // Add decline end.
                create_rail_piece_fn(rail_line,
                                     Build_code::BC_SLOPECHANGE_DOWN_RETURN,
                                     "SlopechangeRail.DR",
                                     vec3{ 14, 0, 0 },
                                     current_pos,
                                     current_angle);
                ctor_mode = FLAT;
                ctor_tilt = NO_TILT;
                break;

            default:
                throw std::runtime_error("This code char not implemented.");
                break;
            }

            prev_ctor_tilt = ctor_tilt;
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
