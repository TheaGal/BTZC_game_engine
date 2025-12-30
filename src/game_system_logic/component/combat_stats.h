#pragma once

#include "btjson.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>


namespace BT
{
namespace component
{

/// Combat stats prior to any modifiers.
struct Base_combat_stats_data
{
    int32_t dmg_pts{ 0 };
    int32_t dmg_def_pts{ 0 };

    int32_t posture_dmg_pts{ 0 };
    int32_t posture_dmg_def_pts{ 0 };

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Base_combat_stats_data,
        dmg_pts,
        dmg_def_pts,
        posture_dmg_pts,
        posture_dmg_def_pts
    );
};

/// List of attacks queued up to be used for CPU and PC.
struct Attack_queue
{
    std::vector<std::string> attack_anim_state_names;
    float_t queue_item_expire_time{ -1 };  // -1 means never expires.

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Attack_queue,
        attack_anim_state_names,
        queue_item_expire_time
    );

    /// Runtime State.
    struct Runtime_state
    {
        std::vector<std::pair<double_t, uint32_t>> atk_anim_idxs_queue;
    } state;

    /// Looks up attack index. Throws if not found.
    uint32_t get_attack_index(std::string const& atk_name) const;

    /// Pushes an attack to the back of the queue.
    /// `atk_timer` is for queue item expiration.
    void push_attack_to_queue(double_t atk_timer, uint32_t atk_idx);

    /// Pops an attack from the front of the queue.
    /// `atk_timer` is compared to the pushed time and will expire items as checked.
    /// If no attacks are in the queue, return -1.
    uint32_t pop_attack_from_queue(double_t atk_timer);
};

}  // namespace component
}  // namespace BT
