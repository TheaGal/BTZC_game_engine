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

/// Status of CPU awareness and list of its enemies.
struct CPU_enemy_awareness
{
    bool is_player_char_an_enemy{ false };

    std::string eyes_bone{ "" };  // Bone from display repr model to base "eyes" off of.

    float_t aware_sight_fov{ glm_rad(45.0f) };  // @NOTE: In radians.
    float_t aware_sight_distance{ 10.0f };

    float_t suspicion_sight_fov{ glm_rad(80.0f) };  // @NOTE: In radians.
    float_t suspicion_sight_distance{ 30.0f };

    /// @NOTE: Can hear in all directions. However, cannot gain awareness from sound alone.
    float_t suspicion_sound_distance{ 30.0f };

    /// Serialization.
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        CPU_enemy_awareness,
        is_player_char_an_enemy,
        eyes_bone,
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

}  // namespace component
}  // namespace BT
