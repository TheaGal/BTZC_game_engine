#include "game_interrupt_event_queue.h"
#include "game_system_logic/entity_container.h"
#include "service_finder/service_finder.h"

#include <array>
#include <cstddef>
#include <stdexcept>


namespace BT
{
namespace component
{

/*static*/ Game_interrupt_event_queue* Game_interrupt_event_queue::instance()
{
    auto view{ service_finder::find_service<Entity_container>()
                   .get_ecs_registry()
                   .view<component::Game_interrupt_event_queue>() };
    if (view.empty())
        return nullptr;

    auto&& [_, gieq]{ *view.each().begin() };
    return &gieq;
}

void Game_interrupt_event_queue::push_event(Interrupt_event&& event)
{
    // Check whether there is space.
    if (((queue_write_idx + 1) & (k_queue_size - 1)) == (queue_read_idx & (k_queue_size - 1)))
        std::runtime_error("Trying to push event when queue is already full.");

    // Push new event.
    interrupt_event_queue[queue_write_idx] = std::move(event);
    queue_write_idx++;
}

void Game_interrupt_event_queue::pop_event()
{
    // Check whether there are events.
    if ((queue_write_idx & (k_queue_size - 1)) == (queue_read_idx & (k_queue_size - 1)))
        std::runtime_error("Trying to pop event when queue is empty.");

    // Pop oldest event.
    interrupt_event_queue[queue_read_idx] = {};
    queue_read_idx++;
}

bool Game_interrupt_event_queue::peek_event_has(Interrupt_event::Event_type const event_type) const
{
    // Check whether there are events.
    if ((queue_write_idx & (k_queue_size - 1)) == (queue_read_idx & (k_queue_size - 1)))
        return false;

    return (interrupt_event_queue[queue_read_idx].event_type == event_type);
}

}  // namespace component
}  // namespace BT
