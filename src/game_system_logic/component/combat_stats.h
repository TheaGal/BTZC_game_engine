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
    struct Attack_data
    {
        std::string attack_anim_state_name;
        float_t queue_expire_time{ -1 };  // -1 means never expires.

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
            Attack_data,
            attack_anim_state_name,
            queue_expire_time
        );
    };

    std::vector<Attack_data> list_of_attacks;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Attack_queue,
        list_of_attacks
    );

    /// Runtime State.
    struct Runtime_state
    {
        /// Queue of attack animation indices.
        std::vector<std::pair<double_t, uint32_t>> atk_anim_idxs_queue;

        /// Holds copy of attack timer so that it doesn't have to be inserted for every queue
        /// mutation.
        double_t current_atk_timer;
    } state;

    /// Updates the attack timer.
    void update_attack_timer(double_t atk_timer);

    /// Looks up attack index. Throws if not found.
    uint32_t get_attack_index(std::string const& atk_name) const;

    /// Pushes an attack to the back of the queue.
    void push_attack_to_queue(uint32_t atk_idx);

    /// Pops an attack from the front of the queue.
    /// `current_atk_timer` is compared to the pushed time and will expire items as checked.
    /// If no attacks are in the queue, return -1.
    uint32_t pop_attack_from_queue();
};

}  // namespace component
}  // namespace BT
