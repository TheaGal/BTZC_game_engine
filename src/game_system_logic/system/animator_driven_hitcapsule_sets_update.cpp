#include "animator_driven_hitcapsule_sets_update.h"

#include "entt/entity/fwd.hpp"
#include "entt/entity/registry.hpp"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "service_finder/service_finder.h"
#include "txp_renderer_public.h"


void BT::system::animator_driven_hitcapsule_sets_update()
{
    auto& reg{ service_finder::find_service<Entity_container>().get_ecs_registry() };
    auto view{ reg.view<TXP::component::Animator_driven_hitcapsule_set const,
                        component::Transform const>() };
    auto& renderer{ service_finder::find_service<TXP::Renderer>() };

    // Work with tagged entities.
    for (auto ent : view)
    {
        // Update whether capsules are enabled and keep capsules attached to connecting bone in
        // animator.
        auto animator{ renderer.try_get_skeletal_animator(ent).value() };

        animator.get_anim_frame_action_data_handle().assign_hitcapsule_enabled_flags();

        std::vector<mat4s> joint_matrices;
        animator.get_simulation_profile_frame_pose(joint_matrices);

        mat4 entity_transform;
        view.get<component::Transform const>(ent).calc_mat4_transform(entity_transform);

        animator.get_anim_frame_action_data_handle().update_hitcapsule_transforms(entity_transform,
                                                                                  joint_matrices);
    }
}
