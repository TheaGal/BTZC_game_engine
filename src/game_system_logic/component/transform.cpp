#include "transform.h"

#include "btlogger.h"
#include "btuuid.h"
#include "entt/entity/fwd.hpp"
#include "entt/entity/registry.hpp"
#include "game_system_logic/entity_container.h"

#include <algorithm>


void BT::component::form_parent_child_relationship_helper(Entity_container& entity_container,
                                                          UUID parent,
                                                          UUID child)
{
    auto& reg{ entity_container.get_ecs_registry() };

    auto parent_ent{ entity_container.find_entity(parent) };
    auto child_ent{ entity_container.find_entity(child) };

    auto* parent_trans_hier{ reg.try_get<Transform_hierarchy>(parent_ent) };
    auto* child_trans_hier{ reg.try_get<Transform_hierarchy>(child_ent) };

    // Check for existing relationships.
    if (child_trans_hier)
    {
        if (child_trans_hier->parent_entity == parent)
        {
            // Assume relationship is fine and made already!!
            BT_WARNF(
                "Detected parent %s and child %s relationship already formed (note: quick check so "
                "parent may not have child listed as a child however).",
                UUID_helper::to_pretty_repr(parent).c_str(),
                UUID_helper::to_pretty_repr(child).c_str());
            return;
        }
        else if (!child_trans_hier->parent_entity.is_nil())
        {
            // Sever the connection with the other parent before continuing!
            auto old_parent_ent{ entity_container.find_entity(child_trans_hier->parent_entity) };
            auto& old_parent_trans_hier{ reg.get<Transform_hierarchy>(old_parent_ent) };
            auto& opth_children_entities{ old_parent_trans_hier.children_entities };

            opth_children_entities.erase(
                std::find(opth_children_entities.begin(), opth_children_entities.end(), child));
        }
    }

    // Create transform hierarchy components if non-existent.
    if (!parent_trans_hier)
    {
        parent_trans_hier = &reg.emplace<Transform_hierarchy>(parent_ent);
    }
    if (!child_trans_hier)
    {
        child_trans_hier = &reg.emplace<Transform_hierarchy>(child_ent);
    }

    // Make connection.
    parent_trans_hier->children_entities.emplace_back(child);
    child_trans_hier->parent_entity = parent;
}

void BT::component::sever_parent_child_relationship_helper(Entity_container& entity_container,
                                                           UUID parent,
                                                           UUID child)
{
    auto& reg{ entity_container.get_ecs_registry() };

    // Remove child from children.
    auto parent_ent{ entity_container.find_entity(parent) };
    auto& parent_trans_hier{ reg.get<Transform_hierarchy>(parent_ent) };
    auto& parent_children_entities{ parent_trans_hier.children_entities };

    parent_children_entities.erase(
        std::find(parent_children_entities.begin(), parent_children_entities.end(), child));

    // Remove parent from child.
    auto child_ent{ entity_container.find_entity(child) };
    auto& child_trans_hier{ reg.get<Transform_hierarchy>(child_ent) };
    child_trans_hier.parent_entity = UUID();
}

void BT::component::submit_transform_change_helper(entt::registry& reg,
                                                   entt::entity entity,
                                                   rvec3s pos,
                                                   versors rot,
                                                   vec3s sca)
{   // Write new transform as a change request.
    auto& trans_changed{ reg.get_or_emplace<component::Transform_changed>(entity) };
    trans_changed.next_transform.position = pos;
    trans_changed.next_transform.rotation = rot;
    trans_changed.next_transform.scale    = sca;
}

void BT::component::submit_transform_change_no_scale_helper(entt::registry& reg,
                                                            entt::entity entity,
                                                            rvec3s pos,
                                                            versors rot)
{   // Grab scale from somewhere.
    vec3s scale;
    if (reg.any_of<component::Transform_changed>(entity))
    {   // Get scale from transform request.
        scale = reg.get<component::Transform_changed>(entity).next_transform.scale;
    }
    else
    {   // Get scale from current transform.
        auto poss_transform{ reg.try_get<component::Transform>(entity) };
        if (!poss_transform)
        {   // If there is no transform to read scale from then no no!
            assert(false);
            return;
        }

        scale = poss_transform->scale;
    }

    // Pass off to other function.
    submit_transform_change_helper(reg, entity, pos, rot, scale);
}

void BT::component::submit_transform_change_only_rotation_helper(entt::registry& reg,
                                                                 entt::entity entity,
                                                                 versors rot)
{   // Grab position and scale from somewhere.
    rvec3s position;
    vec3s scale;
    if (reg.any_of<component::Transform_changed>(entity))
    {   // Get from transform request.
        auto& nxt_trans{ reg.get<component::Transform_changed>(entity).next_transform };
        position = nxt_trans.position;
        scale = nxt_trans.scale;
    }
    else
    {   // Get from current transform.
        auto poss_transform{ reg.try_get<component::Transform>(entity) };
        if (!poss_transform)
        {   // If there is no transform to read pos/scale from then no no!
            assert(false);
            return;
        }

        position = poss_transform->position;
        scale = poss_transform->scale;
    }

    // Pass off to other function.
    submit_transform_change_helper(reg, entity, position, rot, scale);
}
