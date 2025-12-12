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
        is_player_char_an_enemy,
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

}  // namespace component
}  // namespace BT
