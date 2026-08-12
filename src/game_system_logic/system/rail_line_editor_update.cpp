#include "rail_line_editor_update.h"

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
    auto& reg{ service_finder::find_service<Entity_container>().get_ecs_registry() };

    auto create_rail_piece_fn = [&reg](std::vector<entt::entity>& created_entities,
                                       std::string const& model_name,
                                       std::string const& sub_mesh_name,
                                       vec3 mesh_origin_pos,
                                       vec3& place_pos,
                                       float_t& place_angle,
                                       vec3 place_advance_delta_pos,
                                       float place_advance_delta_angle) {
        // @NOTE: using registry::create() directly here guarantees the created rail objects are not
        //        seen by the object registry. Also, it bypasses having to create a UUID, though
        //        that means that nothing can directly interact with the rail line pieces, which is
        //        intended.  -Thea 2026/08/11
        auto new_ent = reg.create();
        created_entities.emplace_back(new_ent);

        auto& rend_obj_cfg = reg.emplace_or_replace<TXP::component::Render_object_config>(new_ent);
        rend_obj_cfg.model_name = model_name;
        rend_obj_cfg.sub_mesh_name = sub_mesh_name;

        glm_translate(rend_obj_cfg.transform.raw, place_pos);
        glm_rotate_y(rend_obj_cfg.transform.raw, place_angle, rend_obj_cfg.transform.raw);

        glm_vec3_negate(mesh_origin_pos);
        glm_translate(rend_obj_cfg.transform.raw, mesh_origin_pos);

        // Advance placement.
        mat4 place_advance_rot = GLM_MAT4_IDENTITY_INIT;
        glm_rotate_y(place_advance_rot, place_angle, place_advance_rot);
        glm_mat4_mulv3(place_advance_rot, place_advance_delta_pos, 0, place_advance_delta_pos);

        glm_vec3_add(place_pos, place_advance_delta_pos, place_pos);
        place_angle += place_advance_delta_angle;
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
                    create_rail_piece_fn(rail_line.created_entities,
                                         "rails",
                                         "StraightRailRoll.LR",
                                         vec3{ -14, 0, 0 },
                                         current_pos,
                                         current_angle,
                                         vec3{ 0, 0, -10 },
                                         0);
                    break;

                case RIGHT_TILT:
                    create_rail_piece_fn(rail_line.created_entities,
                                         "rails",
                                         "StraightRailRoll.RR",
                                         vec3{ -16, 0, 0 },
                                         current_pos,
                                         current_angle,
                                         vec3{ 0, 0, -10 },
                                         0);
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
                    create_rail_piece_fn(rail_line.created_entities,
                                         "rails",
                                         "StraightRailRoll.L",
                                         vec3{ -18, 0, 0 },
                                         current_pos,
                                         current_angle,
                                         vec3{ 0, 0, -10 },
                                         0);
                    break;

                case RIGHT_TILT:
                    create_rail_piece_fn(rail_line.created_entities,
                                         "rails",
                                         "StraightRailRoll.R",
                                         vec3{ -20, 0, 0 },
                                         current_pos,
                                         current_angle,
                                         vec3{ 0, 0, -10 },
                                         0);
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
            vec3s curve_adv_delta = GLM_VEC3_ZERO_INIT;

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
                        create_rail_piece_fn(rail_line.created_entities,
                                             "rails",
                                             "StraightRail",
                                             vec3{ -22, 0, 0 },
                                             current_pos,
                                             current_angle,
                                             vec3{ 0, 0, -10 },
                                             0);
                    break;
                }
                case INCLINE:
                {
                    create_rail_piece_fn(rail_line.created_entities,
                                         "rails",
                                         "SlopedRail.U",
                                         vec3{ 18, 0, 0 },
                                         current_pos,
                                         current_angle,
                                         vec3{ 0, 1, -10 },
                                         0);
                    break;
                }
                case DECLINE:
                {
                    create_rail_piece_fn(rail_line.created_entities,
                                         "rails",
                                         "SlopedRail.D",
                                         vec3{ 12, 0, 0 },
                                         current_pos,
                                         current_angle,
                                         vec3{ 0, -1, -10 },
                                         0);
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

                    curve_adv_delta.x = -1.3187;  // @HARDCODE: could prob use cos(15) and sin(15) mult by curve_origin.x for this (if you touch again).  -Thea 2026/08/05
                    curve_adv_delta.z = -10.035;

                    ctor_tilt = LEFT_TILT;
                }
            case 'w':
                if (curve_code.empty())
                {
                    curve_code = "L.002";

                    curve_origin.x = 42.8178;

                    curve_adv_delta.x = -1.455;
                    curve_adv_delta.z = -11.069;

                    ctor_tilt = LEFT_TILT;
                }
            case 'e':
                if (curve_code.empty())
                {
                    curve_code = "L.003";

                    curve_origin.x = 46.8178;

                    curve_adv_delta.x = -1.591;
                    curve_adv_delta.z = -12.103;

                    ctor_tilt = LEFT_TILT;
                }
            case 'r':
                if (curve_code.empty())
                {
                    curve_code = "L.004";

                    curve_origin.x = 50.8178;

                    curve_adv_delta.x = -1.726;
                    curve_adv_delta.z = -13.137;

                    ctor_tilt = LEFT_TILT;
                }
            case 'p':
                if (curve_code.empty())
                {
                    curve_code = "R.001";

                    curve_origin.x = -38.8178;

                    curve_adv_delta.x = 1.3187;
                    curve_adv_delta.z = -10.035;

                    ctor_tilt = RIGHT_TILT;
                }
            case 'o':
                if (curve_code.empty())
                {
                    curve_code = "R.002";

                    curve_origin.x = -42.8178;

                    curve_adv_delta.x = 1.455;
                    curve_adv_delta.z = -11.069;

                    ctor_tilt = RIGHT_TILT;
                }
            case 'i':
                if (curve_code.empty())
                {
                    curve_code = "R.003";

                    curve_origin.x = -46.8178;

                    curve_adv_delta.x = 1.591;
                    curve_adv_delta.z = -12.103;

                    ctor_tilt = RIGHT_TILT;
                }
            case 'u':
                if (curve_code.empty())
                {
                    curve_code = "R.004";

                    curve_origin.x = -50.8178;

                    curve_adv_delta.x = 1.726;
                    curve_adv_delta.z = -13.137;

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
                create_rail_piece_fn(rail_line.created_entities,
                                     "rails",
                                     "CurveRail." + curve_code,
                                     curve_origin.raw,
                                     current_pos,
                                     current_angle,
                                     curve_adv_delta.raw,
                                     glm_rad(curve_code[0] == 'L' ? 15 : -15));
                break;

            case '(':
                if (ctor_mode != FLAT)
                    throw std::runtime_error("huh?");

                ctor_tilt = NO_TILT;

                process_tilt_connection_fn();

                // Add incline start.
                create_rail_piece_fn(rail_line.created_entities,
                                     "rails",
                                     "SlopechangeRail.U",
                                     vec3{ 16, 0, 0 },
                                     current_pos,
                                     current_angle,
                                     vec3{ 0, 1, -20 },
                                     0);
                ctor_mode = INCLINE;
                break;

            case ')':
                if (ctor_mode != INCLINE)
                    throw std::runtime_error("huh?");

                // Add incline end.
                create_rail_piece_fn(rail_line.created_entities,
                                     "rails",
                                     "SlopechangeRail.UR",
                                     vec3{ 20, 0, 0 },
                                     current_pos,
                                     current_angle,
                                     vec3{ 0, 1, -20 },
                                     0);
                ctor_mode = FLAT;
                ctor_tilt = NO_TILT;
                break;

            case '[':
                if (ctor_mode != FLAT)
                    throw std::runtime_error("huh?");

                ctor_tilt = NO_TILT;

                process_tilt_connection_fn();

                // Add decline start.
                create_rail_piece_fn(rail_line.created_entities,
                                     "rails",
                                     "SlopechangeRail.D",
                                     vec3{ 10, 0, 0 },
                                     current_pos,
                                     current_angle,
                                     vec3{ 0, -1, -20 },
                                     0);
                ctor_mode = DECLINE;
                break;

            case ']':
                if (ctor_mode != DECLINE)
                    throw std::runtime_error("huh?");

                // Add decline end.
                create_rail_piece_fn(rail_line.created_entities,
                                     "rails",
                                     "SlopechangeRail.DR",
                                     vec3{ 14, 0, 0 },
                                     current_pos,
                                     current_angle,
                                     vec3{ 0, -1, -20 },
                                     0);
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
