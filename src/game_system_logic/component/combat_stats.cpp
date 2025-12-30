#include "combat_stats.h"
#include "btlogger.h"

#include <exception>


uint32_t BT::component::Attack_queue::get_attack_index(std::string const& atk_name) const
{
    uint32_t idx{ 0 };
    for (auto const& name : attack_anim_state_names)
    {
        if (name == atk_name)
        {
            return idx;
        }
        idx++;
    }

    throw new std::exception("Could not find attack idx.");
}

void BT::component::Attack_queue::push_attack_to_queue(double_t atk_timer, uint32_t atk_idx)
{
    state.atk_anim_idxs_queue.emplace_back(atk_timer, atk_idx);
    BT_TRACEF("Pushed attack idx: %i", atk_idx);
}

uint32_t BT::component::Attack_queue::pop_attack_from_queue(double_t atk_timer)
{
    uint32_t atk_idx{ (uint32_t)-1 };

    while (!state.atk_anim_idxs_queue.empty() && atk_idx == (uint32_t)-1)
    {
        if (queue_item_expire_time < 0.0f ||
            atk_timer - state.atk_anim_idxs_queue.front().first < queue_item_expire_time)
        {   // Found elem to pop!
            atk_idx = state.atk_anim_idxs_queue.front().second;
        }

        // Delete front.
        state.atk_anim_idxs_queue.erase(state.atk_anim_idxs_queue.begin());
    }

    BT_TRACEF("Popped attack idx: %i", atk_idx);

    return atk_idx;
}
