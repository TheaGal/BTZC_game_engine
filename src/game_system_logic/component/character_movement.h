#pragma once

#include "btglm.h"
#include "btjson.h"
#include "btuuid.h"
#include "entt/entity/entity.hpp"

#include <cmath>
#include <cstdint>


namespace BT
{
namespace component
{

/// Tag to mark the player character. `player_idx` marks which player this player character is.
struct Player_character
{
    size_t player_idx{ 0 };

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Player_character,
        player_idx
    );
};

/// World space input for character controller movement.
/// NPC AI would output to this struct. Player input would output to this struct.
struct Character_world_space_input
{
    vec3s ws_flat_clamped_input{ 0, 0, 0 };
    vec3s delta_to_position_of_interest{ 0, 0, 0 };

    bool jump_pressed{ false };
    bool prev_jump_pressed{ false };
    bool crouch_pressed{ false };
    bool prev_crouch_pressed{ false };

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Character_world_space_input,
                                                ws_flat_clamped_input);
};

/// Holds movement state for characters. Also contains settings for movement parameters.
/// @NOTE: Only settings are serialized.
struct Character_mvt_state
{
    struct Grounded_state
    {
        bool allow_grounded_sliding{ false };
        float_t facing_angle{ 0.0f };
    } grounded_state;

    struct Airborne_state
    {
        float_t input_facing_angle{ 0.0f };
    } airborne_state;

    struct Settings
    {
        float_t jump_speed{ 30.0f };

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Settings,
                                                    jump_speed);
    } settings;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Character_mvt_state, settings);

    vec3 prev_velocity = GLM_VEC3_ZERO_INIT;

    /// Helper func that sets facing angle of this component.
    void set_facing_angle(float_t angle_radians);

    /// Helper func that gets facing angle of this component.
    float_t get_facing_angle() const;
};

/// Communicates state of the animated model from the `input_controlled_character_movement` system.
struct Character_mvt_animated_state
{
    /// For tracking input data from character.
    struct Input_mvt_state
    {   // Mode.
        enum Mode : int32_t
        {
            MODE_INVALID = -1,
            MODE_PLAYER_CHAR,
            MODE_CPU_CHAR
        } mode{ -1 };  // @NOTE: this is a seting

        // Movement inputs.
        bool is_moving{ false };
        bool on_jump{ false };
        bool is_grounded{ false };

        // Combat reactions.
        bool on_landed_attack{ false };

        enum Hurt_type : uint32_t
        {
            HURT_TYPE_NONE = 0b0,

            HURT_TYPE_REGULAR     = 0b0000'0001,
            HURT_TYPE_UNPARRYABLE = 0b0000'0010,
            HURT_TYPE_UNGUARDABLE = 0b0000'0100,
            HURT_TYPE_MIKIRIABLE  = 0b0000'1000,
        } on_hurt{ HURT_TYPE_NONE };

        bool on_health_depleted{ false };
        bool on_posture_broken{ false };

        bool is_alive{ true };
        bool choose_to_resurrect{ false };
        bool choose_to_die{ false };
        float_t resurrect_make_choice_timer{ 0 };

        // Deathblow syncing.
        bool on_give_deathblow{ false };
        bool align_self_as_deathblow_giver{ false };
        bool on_finish_deathblow_give{ false };

        bool on_receive_deathblow{ false };
        bool align_self_as_deathblow_receiver{ false };
        bool on_finish_deathblow_receive{ false };

        entt::entity deathblow_other_entity{ entt::null };  // persistent

        // Combat inputs (player char).
        bool on_attack_press{ false };
        bool is_attack_released{ false };
        bool on_guard_press{ false };
        bool is_guard_released{ false };

        // @THOUGHT: maybe the CPU can have some kind of anxiety or fatigue when posture bar goes up
        //           or a long attack?  -Thea 2026/10/06

        // Combat inputs (cpu char).
        bool on_exec_movement{ false };

        enum CPU_attack_type : int32_t
        {
            CPU_ATK_TYPE_NONE = 0,
            CPU_ATK_TYPE_REQUESTED,
            CPU_ATK_TYPE_BY_CHANCE,
        } on_exec_attack_combo{ 0 };

        float_t distance_to_target{ 0 };  // this is persistent
        float_t aggression_radius{ 10 };  // @NOTE: this is a setting  @THEA: is this the best place for this?

        void reset_state(bool reset_persist_vals)
        {
            // capture persistent vals here.
            auto cap0 = deathblow_other_entity;
            auto cap1 = distance_to_target;

            *this = {
                .mode = mode,
                .aggression_radius = aggression_radius,
            };

            if (!reset_persist_vals)
            {
                // rewrite in persistent vals here.
                deathblow_other_entity = cap0;
                distance_to_target = cap1;
            }
        }

        inline bool is_player_char_mode() const
        {
            return (mode == MODE_PLAYER_CHAR);
        }
    } input_mvt_state;

    struct Write_to_animator_data
    {
        float_t mvt_facing_angle{ 0 };

        // @ANIMATOR_REFACTOR vv
        // // vv @NOTE: below the old information to write. vv
        // bool is_moving{ false };
        // bool is_locked_on{ false };  // Unused. Had a former use but made every movement blendtrees.
        // bool on_suspicion{ false };
        // bool is_suspicious_approaching{ false };
        // bool on_unaware{ false };
        // bool on_aware{ false };
        // float_t mvt_facing_angle{ 0 };
        // bool on_turnaround{ false };
        // bool is_grounded{ false };
        // bool on_jump{ false };
        // bool on_attack{ false };
        // bool on_cancel_parried{ false };
        // bool on_parry_hurt{ false };
        // bool on_guard_hurt{ false };
        // bool on_receive_hurt{ false };
        // bool on_receive_hurt_from_back{ false };
        // bool on_guard{ false };
        // bool is_guarding{ false };
    } write_to_animator_data;

    struct State
    {
        bool prev_attack_pressed{ false };
        bool prev_guard_pressed{ false };
    } state;

    char _;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Character_mvt_animated_state, _);
};

}  // namespace component
}  // namespace BT
