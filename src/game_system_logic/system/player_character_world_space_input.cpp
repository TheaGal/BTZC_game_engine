#include "player_character_world_space_input.h"

#include "Jolt/Jolt.h"
#include "Jolt/Math/Vec3.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "animation_frame_action_tool/runtime_data.h"
#include "btglm.h"
#include "game_system_logic/component/character_movement.h"
#include "game_system_logic/component/physics_object_settings.h"
#include "game_system_logic/component/render_object_settings.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "game_system_logic/system/helper_funcs.h"
#include "input_handler/input_handler.h"
#include "physics_engine/physics_engine.h"
#include "physics_engine/physics_object.h"
#include "physics_engine/raycast_helper.h"
#include "renderer/camera.h"
#include "renderer/renderer.h"
#include "service_finder/service_finder.h"

#include <cassert>


namespace
{

using namespace BT;

/// Takes `input_vec` user input and transforms it into a world space input vector where forward is
/// the direction the camera is facing.
void transform_input_to_camera_pov_input(Camera& camera, vec2 const input_vec, vec3s& out_ws_input_vec)
{
    if (!camera.is_follow_orbit())
    {   // Exit since camera isn't accepting input.
        glm_vec3_zero(out_ws_input_vec.raw);
        return;
    }

    // Calc forward and right axis vectors.
    vec3 cam_forward;
    camera.get_view_direction(cam_forward);
    cam_forward[1] = 0;
    glm_vec3_normalize(cam_forward);

    vec3 cam_right;
    glm_vec3_cross(cam_forward, vec3{ 0, 1, 0 }, cam_right);
    cam_right[1] = 0;
    glm_vec3_normalize(cam_right);

    // Transform `input_vec` into axis vectors.
    glm_vec3_zero(out_ws_input_vec.raw);
    glm_vec3_muladds(cam_right, input_vec[0], out_ws_input_vec.raw);
    glm_vec3_muladds(cam_forward, input_vec[1], out_ws_input_vec.raw);
    out_ws_input_vec.y = 0;

    // Clamp magnitude to <=1.0
    if (glm_vec3_norm2(out_ws_input_vec.raw) > 1.0f * 1.0f)
        glm_vec3_normalize(out_ws_input_vec.raw);
}

}  // namespace


void BT::system::player_character_world_space_input()
{
    auto& camera{ *service_finder::find_service<Renderer>().get_camera_obj() };

    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };
    auto view{ reg.view<component::Player_character const,
                        component::Character_world_space_input>() };

    // @TEMP: @UNSURE: Only support one player character and fail if not the first.
    bool is_first{ true };

    for (auto entity : view)
    {
        assert(is_first);

        // Get AFA data.
        auto char_mvt_anim_state{ reg.try_get<component::Character_mvt_animated_state>(
            entity) };

        bool can_move{ false };
        bool can_guard_exit{ false };
        bool can_attack_exit{ false };

        if (char_mvt_anim_state)
            helper::fetch_wanted_afa_data(entity_container,
                                          reg,
                                          *char_mvt_anim_state,
                                          can_move,
                                          can_guard_exit,
                                          can_attack_exit);

        // Get writing handle for world-space input.
        auto& char_ws_input{ view.get<component::Character_world_space_input>(entity) };

        // Get input for player character, transformed into camera view direction.
        auto const& input_state{ service_finder::find_service<Input_handler>().get_input_state() };

        vec2 move_input{ input_state.move.x.val, input_state.move.y.val };
        if (!can_move)
            glm_vec2_zero(move_input);

        transform_input_to_camera_pov_input(camera,
                                            move_input,
                                            char_ws_input.ws_flat_clamped_input);

        // Update input state.
        char_ws_input.prev_jump_pressed   = char_ws_input.jump_pressed;
        char_ws_input.jump_pressed        = input_state.jump.val;
        char_ws_input.prev_crouch_pressed = char_ws_input.crouch_pressed;
        char_ws_input.crouch_pressed      = input_state.crouch.val;

        // On attack trigger.
        bool attack_pressed{ input_state.attack.val };
        if (camera.is_follow_orbit() &&
            can_attack_exit &&
            !char_mvt_anim_state->state.prev_attack_pressed &&
            attack_pressed)
            char_mvt_anim_state->write_to_animator_data.on_attack = true;
        char_mvt_anim_state->state.prev_attack_pressed = attack_pressed;

        char_mvt_anim_state->write_to_animator_data.is_guarding = (camera.is_follow_orbit() &&
                                                                   can_guard_exit &&
                                                                   input_state.guard.val);

        // End of first iteration.
        is_first = false;
    }
}
