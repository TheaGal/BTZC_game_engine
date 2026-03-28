#include "player_character_world_space_input.h"

#include "Jolt/Jolt.h"
#include "Jolt/Math/Vec3.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "animation_frame_action_tool/runtime_data.h"
#include "btdatecheck.h"
#include "btglm.h"
#include "game_system_logic/component/character_movement.h"
#include "game_system_logic/component/physics_object_settings.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "game_system_logic/system/helper_funcs.h"
#include "physics_engine/physics_engine.h"
#include "physics_engine/physics_object.h"
#include "physics_engine/raycast_helper.h"
#include "service_finder/service_finder.h"
#include "txp_renderer_public.h"

#include <cassert>


namespace
{

using namespace BT;

/// Takes `input_vec` user input and transforms it into a world space input vector where forward is
/// the direction the camera is facing.
void transform_input_to_camera_pov_input(TXP::Camera& main_camera,
                                         vec2 const input_vec,
                                         vec3s& out_ws_input_vec)
{
    if (!main_camera.is_follow_orbit())
    {   // Exit since camera isn't accepting input.
        glm_vec3_zero(out_ws_input_vec.raw);
        return;
    }

    // Calc forward and right axis vectors.
    vec3 cam_forward;
    main_camera.get_view_direction(cam_forward);
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
    auto& main_camera = service_finder::find_service<TXP::Renderer>().get_main_camera();

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
        auto const& input_handler{ service_finder::find_service<TXP::Input::Input_handler>() };

        // @TODO: make better input vv below vv that can handle directional move.
        BT::date_deadline(2026, 4, 24);
        vec2 move_input{ 0, 0 };
        if (input_handler.get_keyboard_key_state(BT_KEY_W).pressed)
            move_input[1] += 1;
        if (input_handler.get_keyboard_key_state(BT_KEY_A).pressed)
            move_input[0] -= 1;
        if (input_handler.get_keyboard_key_state(BT_KEY_S).pressed)
            move_input[1] -= 1;
        if (input_handler.get_keyboard_key_state(BT_KEY_D).pressed)
            move_input[0] += 1;

        if (!can_move)
            glm_vec2_zero(move_input);

        transform_input_to_camera_pov_input(main_camera,
                                            move_input,
                                            char_ws_input.ws_flat_clamped_input);

        // Update input state.
        char_ws_input.prev_jump_pressed   = char_ws_input.jump_pressed;
        char_ws_input.jump_pressed        = input_handler.get_keyboard_key_state(BT_KEY_SPACE).pressed;
        char_ws_input.prev_crouch_pressed = char_ws_input.crouch_pressed;
        char_ws_input.crouch_pressed      = input_handler.get_keyboard_key_state(BT_KEY_LEFT_CONTROL).pressed;

        // On attack trigger.
        bool attack_pressed{ input_handler.get_mouse_button_state(BT_MOUSE_BUTTON_LEFT).pressed };
        // @ANIMATOR_REFACTOR if (camera.is_follow_orbit() &&
        // @ANIMATOR_REFACTOR     can_attack_exit &&
        // @ANIMATOR_REFACTOR     !char_mvt_anim_state->state.prev_attack_pressed &&
        // @ANIMATOR_REFACTOR     attack_pressed)
        // @ANIMATOR_REFACTOR     char_mvt_anim_state->write_to_animator_data.on_attack = true;
        char_mvt_anim_state->state.prev_attack_pressed = attack_pressed;

        // On guard trigger and is-guarding bool.
        bool on_guard;
        bool is_guarding;
        {
            bool guard_pressed =
                input_handler.get_mouse_button_state(BT_MOUSE_BUTTON_RIGHT).pressed;
            is_guarding = (main_camera.is_follow_orbit() && can_guard_exit && guard_pressed);
            on_guard = (is_guarding && !char_mvt_anim_state->state.prev_guard_pressed);

            char_mvt_anim_state->state.prev_guard_pressed = guard_pressed;
        }
        // @ANIMATOR_REFACTOR char_mvt_anim_state->write_to_animator_data.on_guard = on_guard;
        // @ANIMATOR_REFACTOR char_mvt_anim_state->write_to_animator_data.is_guarding = is_guarding;

        // End of first iteration.
        is_first = false;
    }
}
