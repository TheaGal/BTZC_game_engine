#pragma once

#include "btjson.h"
#include "entt/entity/fwd.hpp"

#include <cstdint>
#include <limits>
#include <string>


namespace BT
{
namespace component
{

/// Data for health of an entity.
struct Health_stats_data
{
    int32_t max_health_pts{ 100 };
    int32_t health_pts{ std::numeric_limits<int32_t>::max() };     // <0 causes death trigger.

    int32_t max_posture_pts{ 100 };
    int32_t posture_pts{ 0 };                 // `max_posture_pts` causes posture break. Min is 0.
    float_t posture_pts_regen_rate{ 20 };     // N per sec to decrement `posture_pts`.

    float_t parry_return_posture_multi{ 1 };  // amount of posture that comes back if defender does a perfect parry

    int32_t max_num_deathblows{ 0 };
    int32_t num_deathblows{ std::numeric_limits<int32_t>::max() };

    bool is_invincible{ false };              // `true` prevents death trigger and decrement of `health_pts`.

    std::string ui_name;  // empty string just makes a ui health/posture bar above the characters
                          // head. "playchar" makes the bottom screen ui for the player character,
                          // and anything else creates an enemy boss ui with the ui name.

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Health_stats_data,
        max_health_pts,
        max_posture_pts,
        posture_pts_regen_rate,
        max_num_deathblows,
        is_invincible,
        ui_name
    );

    // Runtime state.
    struct Runtime_state
    {
        struct Attack_stats
        {
            entt::entity offender_entity;

            enum Atk_type
            {
                CRUSH,
                SLICE,
                SPEAR,
            };

            Atk_type type;
            int32_t damage;

            bool is_honed;  // can be blocked, but causes chip damage unless parried
            bool has_fire;  // builds up the fire status meter
        };
        Attack_stats last_received_attack;
    } state;
};

}  // namespace component
}  // namespace BT
