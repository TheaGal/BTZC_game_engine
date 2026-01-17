#include "combat_stats.h"
#include "btlogger.h"

#include <exception>


void BT::component::Attack_queue::update_attack_timer(double_t atk_timer)
{
    state.current_atk_timer = atk_timer;
}

uint32_t BT::component::Attack_queue::get_attack_index(std::string const& atk_name) const
{
    uint32_t idx{ 0 };
    for (auto const& atk : list_of_attacks)
    {
        if (atk.attack_anim_state_name == atk_name)
        {
            return idx;
        }
        idx++;
    }

    throw std::exception("Could not find attack idx.");
}

void BT::component::Attack_queue::push_attack_to_queue(uint32_t atk_idx)
{
    state.atk_anim_idxs_queue.emplace_back(state.current_atk_timer, atk_idx);
    BT_TRACEF("Pushed attack idx: %i", atk_idx);
}

uint32_t BT::component::Attack_queue::pop_attack_from_queue()
{
    uint32_t atk_idx{ (uint32_t)-1 };

    while (!state.atk_anim_idxs_queue.empty() && atk_idx == (uint32_t)-1)
    {
        auto const& atk{ list_of_attacks[state.atk_anim_idxs_queue.front().second] };
        if (atk.queue_expire_time < 0.0f ||
            state.current_atk_timer - state.atk_anim_idxs_queue.front().first <
                atk.queue_expire_time)
        {   // Found elem to pop!
            atk_idx = state.atk_anim_idxs_queue.front().second;
        }

        // Delete front.
        state.atk_anim_idxs_queue.erase(state.atk_anim_idxs_queue.begin());
    }

    BT_TRACEF("Popped attack idx: %i", atk_idx);

    return atk_idx;
}
