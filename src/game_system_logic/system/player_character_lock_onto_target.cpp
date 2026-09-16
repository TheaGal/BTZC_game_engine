#include "player_character_lock_onto_target.h"

#include "btglm.h"
#include "btlogger.h"
#include "btuuid.h"
#include "game_system_logic/component/follow_camera.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "game_system_logic/world/world_properties.h"
#include "service_finder/service_finder.h"
#include "txp_renderer_public.h"

#include <cassert>
#include <cmath>


void BT::system::player_character_lock_onto_target()
{   // Exit early if simulation not running.
    if (!service_finder::find_service<world::World_properties_container>()
             .get_data_handle()
             .is_simulation_running)
        return;

    // @THEA: hmmmm?? commenting this out doesn't make it work.
    // // Exit early if not in right camera view mode.
    auto& camera{ service_finder::find_service<TXP::Renderer>().get_main_camera() };
    // if (!camera.is_follow_orbit())
    //     return;
    
    // ECS parts.
    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };

    // Get locked on entity.
    bool found_player_character{ false };
    component::Follow_camera_follow_ref::State* follow_state{ nullptr };
    {
        size_t count{ 0 };
        for (auto&& [ecs_entity, follow_cam_follow_ref, transform] :
            reg.view<component::Follow_camera_follow_ref, component::Transform const>().each())
        {
            found_player_character = true;
            follow_state = &follow_cam_follow_ref.state;
            count++;
        }
        assert(count <= 1);  // Enforce only one iteration (or exit system if no found pointer).
    }

    // Exit early if no player character.
    if (!found_player_character)
        return;

    // Input checks.
    bool on_lockon_press;
    {
        static bool s_prev_lockon_pressed{ false };
        auto& input_handler{ service_finder::find_service<TXP::Input::Input_handler>() };
        bool lockon_pressed{ input_handler.get_mouse_button_state(BT_MOUSE_BUTTON_MIDDLE).pressed ||
                             input_handler.get_keyboard_key_state(BT_KEY_RIGHT_ALT).pressed };
        on_lockon_press = (!s_prev_lockon_pressed && lockon_pressed);

        s_prev_lockon_pressed = lockon_pressed;
    }

    // Remove other character reference if (1) clicked lock off or (2) reference is broken.
    if (!follow_state->locked_on_entity.is_nil())
    {
        if (on_lockon_press ||
            !entity_container.entity_exists(follow_state->locked_on_entity))
        {   // Remove character reference.
            follow_state->locked_on_entity = UUID();
        }
    }
    // Look for other character if clicking lockon.
    else if (on_lockon_press)
    {
        UUID best_entity;
        float_t best_dot_prod{ std::sinf(glm_rad(45.0f)) };

        // Get facing dir of camera.
        vec3 cam_position;
        vec3 cam_facing_dir;
        {
            camera.get_position(cam_position);
            camera.get_view_direction(cam_facing_dir);

            assert(glm_eq(glm_vec3_norm2(cam_facing_dir), 1.0f));
        }

        for (auto&& [ecs_entity, transform, cam_lockon_target] :
             reg.view<component::Transform const, component::Follow_camera_lockon_target const>(
                    entt::exclude<component::Follow_camera_follow_ref>)
                 .each())
        {   // Loop thru to find best entity to focus on.
            // @TODO: Conform to `write_render_transforms.cpp`
            vec3 trans_pos{ static_cast<float_t>(transform.position.x),
                            static_cast<float_t>(transform.position.y) +
                                cam_lockon_target.follow_offset_y,
                            static_cast<float_t>(transform.position.z) };

            vec3 facing_to_trans;
            glm_vec3_sub(trans_pos, cam_position, facing_to_trans);
            glm_vec3_normalize(facing_to_trans);

            auto facing_dot_prod{ glm_vec3_dot(facing_to_trans, cam_facing_dir) };
            if (facing_dot_prod > best_dot_prod)
            {   // Found new best facing dir!
                best_entity   = entity_container.find_entity_uuid(ecs_entity);
                best_dot_prod = facing_dot_prod;
            }
        }

        if (!best_entity.is_nil())
        {   // Apply best entity.
            follow_state->locked_on_entity = best_entity;
        }
    }

    // Exit early if no locked on entity.
    if (follow_state->locked_on_entity.is_nil())
        return;

    ////////////////////////////////////////////////////////////////////////////////////////////////

    // Move camera direction to following transform.
    vec3 follow_pos;
    camera.get_follow_orbit_follow_pos(follow_pos);

    constexpr float_t k_camera_circle_radius{ 2 };  // @HARDCODE: cam offset position as circle1 radius.

    vec3 ideal_orbit_cam_pos_as_flat;
    float_t ideal_orbit_cam_angle_tilt;
    {
        auto locked_on_ecs_entity{ entity_container.find_entity(follow_state->locked_on_entity) };
        auto& transform{ reg.get<component::Transform const>(locked_on_ecs_entity) };
        auto& cam_lockon_target{ reg.get<component::Follow_camera_lockon_target const>(
            locked_on_ecs_entity) };

        // @TODO: Conform to `write_render_transforms.cpp`
        vec3 target_locked_on_pos{
            static_cast<float_t>(transform.position.x),
            static_cast<float_t>(transform.position.y) + cam_lockon_target.follow_offset_y,
            static_cast<float_t>(transform.position.z),
        };

        // Calc center of inscribing circle for desired angle.
        vec2s inscribe_circ_center;
        float_t inscribe_circ_radius;
        {
            // @REF: "targeting_cam_angle_idea2.png"
            float_t d{ glm_vec3_distance(follow_pos, target_locked_on_pos) };

            float_t const min_d{ k_camera_circle_radius * 0.365f };  // @HARDCODE: value pulled from: https://www.desmos.com/calculator/y05tgmsplz

            if (d < min_d)
            {
                // To small to make an intersection for camera positioning; shove the locked on pos
                // a bit further away.
                d = min_d;

                vec3 delta;
                glm_vec3_sub(target_locked_on_pos, follow_pos, delta);
                glm_vec3_scale_as(delta, min_d, delta);

                glm_vec3_add(follow_pos, delta, target_locked_on_pos);
            }

            inscribe_circ_center.x = (d * 0.5f);
            inscribe_circ_center.y = inscribe_circ_center.x / tanf(glm_rad(20.0f));  // @HARDCODE: wanted angle difference is 20deg.

            inscribe_circ_radius = glm_vec2_norm(inscribe_circ_center.raw);
        }

        // Calc intersection point of camera circle and inscribe circle.
        vec2s circ_intersection;
        {
            float_t d{ inscribe_circ_radius };  // origin of circle1 is on circle2, so `d` is also radius.

            static auto const k_calc_pos_circle_intersection =
                [](float_t circle1_r, float_t circle2_r, float_t d) -> vec2s {
                // @REF: https://mathworld.wolfram.com/Circle-CircleIntersection.html
                float_t x{ ((d * d) - (circle2_r * circle2_r) + (circle1_r * circle1_r)) / (2 * d) };
                float_t y{ sqrtf((circle1_r * circle1_r) - (x * x)) };

                assert(!std::isnan(x));
                assert(!std::isnan(y));

                return { .x = x, .y = y };
            };


            circ_intersection =
                k_calc_pos_circle_intersection(k_camera_circle_radius, inscribe_circ_radius, d);
        }

        // Transform intersection point into inscribe circle space.
        vec2s inscribe_circ_intersection;
        {
            vec2 basis_x;
            glm_vec2_normalize_to(inscribe_circ_center.raw, basis_x);

            vec2 basis_y{ -basis_x[1], basis_x[0] };

            glm_vec2_scale(basis_x, circ_intersection.x, inscribe_circ_intersection.raw);
            glm_vec2_muladds(basis_y, circ_intersection.y, inscribe_circ_intersection.raw);
        }

        // Transform inscribe-circle-space intersection point into world space.
        {
            vec3 basis_x;
            glm_vec3_sub(target_locked_on_pos, follow_pos, basis_x);
            basis_x[1] = 0;  // flatten basis_x since the ideal orbit cam should be as flattened.  @NOTE: this algorithm requires the y delta to be 0, so, there is a separate angle for calculating the y delta difference to manually affect the orbit angles.  -Thea 2026/09/15
            glm_vec3_normalize(basis_x);
            assert(basis_x[0] != 0 || basis_x[1] != 0 || basis_x[2] != 0);

            vec3 basis_y{ 0, 1, 0 };  // since basis_x is flattened.

            glm_vec3_scale(basis_x, inscribe_circ_intersection.x, ideal_orbit_cam_pos_as_flat);
            glm_vec3_muladds(basis_y, inscribe_circ_intersection.y, ideal_orbit_cam_pos_as_flat);

            // Find signed angle tilt of flattened delta (follow_pos to target_locked_on_pos).
            vec3 real_delta;
            glm_vec3_sub(target_locked_on_pos, follow_pos, real_delta);

            ideal_orbit_cam_angle_tilt = std::atan2f(glm_vec3_dot(real_delta, basis_x),
                                                     glm_vec3_dot(real_delta, basis_y)) -
                                         glm_rad(90);

            while (ideal_orbit_cam_angle_tilt < glm_rad(-180))
                ideal_orbit_cam_angle_tilt += glm_rad(360);
            while (ideal_orbit_cam_angle_tilt >= glm_rad(180))
                ideal_orbit_cam_angle_tilt -= glm_rad(360);
        }
    }

    vec3 delta_pos;
    glm_vec3_negate_to(ideal_orbit_cam_pos_as_flat, delta_pos);

    // Ref: https://assetsio.gnwcdn.com/sekiro-owl-father.jpg?width=1600&height=900&fit=crop&quality=100&format=png&enable=upscale&auto=webp
    vec2 new_orbits;
    new_orbits[0] = std::atan2f(delta_pos[0], delta_pos[2]);
    new_orbits[1] = -std::atan2f(delta_pos[1], glm_vec2_norm(vec2{ delta_pos[0], delta_pos[2] })) +
                    ideal_orbit_cam_angle_tilt;
    camera.set_follow_orbit_orbits(new_orbits);

    // Save locked on facing angle.
    follow_state->locked_on_facing_angle = new_orbits[0];
    assert(!std::isnan(follow_state->locked_on_facing_angle));
}
