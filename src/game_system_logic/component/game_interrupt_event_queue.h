#pragma once

#include "btjson.h"

#include <array>
#include <cstddef>
#include <cstdint>


namespace BT
{
namespace component
{

/// Queue for interrupt events.
struct Game_interrupt_event_queue
{
    uint32_t _;
    uint32_t queue_write_idx{ 0 };
    uint32_t queue_read_idx{ 0 };

    struct Interrupt_event
    {
        enum Event_type : uint32_t
        {
            EVE_TYPE_INVALID = 0,
            EVE_TYPE_OPEN_PAUSE_MENU,
            EVE_TYPE_NINSATSU_SCREEN,
            EVE_TYPE_YOU_DIED_SCREEN,
        } event_type{ 0 };

        int32_t argv0;
    };

    static constexpr size_t k_queue_size{ 32 };
    static_assert((k_queue_size & (k_queue_size - 1)) == 0, "k_queue_size must be pow2!");
    std::array<Interrupt_event, k_queue_size> interrupt_event_queue;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Game_interrupt_event_queue, _);

    // Helper funcs.
    static Game_interrupt_event_queue* instance();

    void push_event(Interrupt_event&& event);
    void pop_event();
    bool peek_event_has(Interrupt_event::Event_type const event_type) const;
};

}  // namespace component
}  // namespace BT
