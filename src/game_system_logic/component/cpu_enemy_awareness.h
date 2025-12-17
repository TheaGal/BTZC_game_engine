#pragma once

#include "btglm.h"
#include "btjson.h"
#include "uuid/uuid.h"

#include <cmath>
#include <cstdint>


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

    struct Sight_detection_zone
    {
        float_t fov;  // @NOTE: In radians.
        float_t distance_immediate;  // Distance where the detection happens immediately.
        float_t distance_buildup;  // Distance where the detection builds up.
        float_t buildup_threshold;  // Threshold for detection to trigger (buildup is 1.0 per second).

        inline float_t distance_immediate2() const
        {
            return (distance_immediate * distance_immediate);
        }

        inline float_t distance_buildup2() const
        {
            return (distance_buildup * distance_buildup);
        }

        /// Serialize.
        NLOHMANN_DEFINE_TYPE_INTRUSIVE(
            Sight_detection_zone,
            fov,
            distance_immediate,
            distance_buildup,
            buildup_threshold
        );

        // State.
        float_t current_buildup{ 0 };
    };

    Sight_detection_zone aware_sdz{ .fov = glm_rad(90.0f),
                                    .distance_immediate = 10.0f,
                                    .distance_buildup = 25.0f,
                                    .buildup_threshold = 8.0f };
    float_t lose_aware_time{ 8.0f };

    Sight_detection_zone suspicion_sdz{ .fov = glm_rad(160.0f),
                                        .distance_immediate = 15.0f,
                                        .distance_buildup = 30.0f,
                                        .buildup_threshold = 4.0f };
    float_t lose_suspicion_time{ 16.0f };

    /// @NOTE: Can hear in all directions. However, cannot gain awareness from sound alone.
    float_t suspicion_sound_distance{ 15.0f };

    /// Serialization.
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        CPU_enemy_awareness,
        my_enemy_bitmask,
        eyes_bone,
        eyes_origin,
        aware_sdz,
        lose_aware_time,
        suspicion_sdz,
        lose_suspicion_time,
        suspicion_sound_distance
    );

    /// DO NOT SERIALIZE.
    struct State
    {
        enum Awareness : uint32_t
        {
            UNAWARE,
            SUSPICIOUS,
            AWARE,
            NUM_AWARENESS_STATES
        };
        Awareness enemy_awareness{ UNAWARE };
        Awareness prev_enemy_awareness{ enemy_awareness };
        float_t out_of_detection_timer{ 0 };
        rvec3 position_of_interest;  // Last detected position of suspicion or aware-enemy.
        UUID aware_enemy;

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
