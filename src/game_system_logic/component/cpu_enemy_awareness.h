#pragma once

#include "btglm.h"
#include "btjson.h"
#include "uuid/uuid.h"

#include <cmath>
#include <cstdint>
#include <limits>


namespace BT
{
namespace component
{

/// Categories for all types of detectable entities.
namespace detectable_character_type
{
    constexpr uint8_t PLAYER   = 0b00000001;
    constexpr uint8_t GOOD_GUY = 0b00000010;
    constexpr uint8_t BAD_GUY  = 0b00000100;
    constexpr uint8_t NEUTRAL  = 0b00001000;
}

/// Status of CPU awareness and list of its enemies.
struct CPU_enemy_awareness
{
    uint8_t my_enemy_bitmask{ 0 };  // Use `detectable_character_type`.

    std::string eyes_bone{ "" };  // Bone from display repr model to base "eyes" off of.
    vec3s eyes_origin{ 0, 1, 0 };  // Origin point of eyes position.

    float_t aware_sight_fov{ glm_rad(90.0f) };  // @NOTE: In radians.
    float_t aware_sight_distance{ 10.0f };

    float_t suspicion_sight_fov{ glm_rad(160.0f) };  // @NOTE: In radians.
    float_t suspicion_sight_distance{ 30.0f };

    /// @NOTE: Can hear in all directions. However, cannot gain awareness from sound alone.
    float_t suspicion_sound_distance{ 15.0f };

    /// Serialization.
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        CPU_enemy_awareness,
        my_enemy_bitmask,
        eyes_bone,
        eyes_origin,
        aware_sight_fov,
        aware_sight_distance,
        suspicion_sight_fov,
        suspicion_sight_distance,
        suspicion_sound_distance
    );

    /// DO NOT SERIALIZE.
    struct State
    {
        enum Awareness
        {
            UNAWARE,
            SUSPICIOUS,
            AWARE,

            NUM_AWARENESS_STATES
        } enemy_awareness{ UNAWARE };
        UUID focused_enemy;

        uint32_t eyes_bone_idx{ (uint32_t)-1 };
    } runtime_state;
};

/// Marks an entity as a possible enemy for the CPU.
struct Detectable_character
{
    uint8_t type{ detectable_character_type::NEUTRAL };  // Use `detectable_character_type`.
    vec3s transform_offset{ 0, 1, 0 };

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Detectable_character,
        type,
        transform_offset
    );
};

}  // namespace component
}  // namespace BT
