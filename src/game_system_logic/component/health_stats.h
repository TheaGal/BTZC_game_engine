#pragma once

#include "btjson.h"

#include <cstdint>

#define OLD_HITCAPSULE_ATK_PROCESS 0


namespace BT
{
namespace component
{

/// Data for health of an entity.
struct Health_stats_data
{
    int32_t max_health_pts{ 100 };
    int32_t health_pts{ max_health_pts };  // 0 causes death trigger.

    int32_t max_posture_pts{ 100 };
    int32_t posture_pts{ 0 };              // `max_posture_pts` causes posture break. Min is 0.
    float_t posture_pts_regen_rate{ 20 };  // N per sec to decrement `posture_pts`.

    bool is_invincible{ false };           // `true` prevents death trigger and decrement of `health_pts`.

#if OLD_HITCAPSULE_ATK_PROCESS
    double_t atk_receive_debounce_time{ 0.2 };  // Min time between attacks in seconds.
    double_t prev_atk_received_time{ std::numeric_limits<double_t>::lowest() };  // DO NOT INCLUDE IN SERIALIZATION.
#endif // OLD_HITCAPSULE_ATK_PROCESS

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Health_stats_data,
        max_health_pts,
        health_pts,
        max_posture_pts,
        posture_pts,
        posture_pts_regen_rate,
        is_invincible
#if OLD_HITCAPSULE_ATK_PROCESS
        ,
        atk_receive_debounce_time
#endif // OLD_HITCAPSULE_ATK_PROCESS
    );
};

}  // namespace component
}  // namespace BT
