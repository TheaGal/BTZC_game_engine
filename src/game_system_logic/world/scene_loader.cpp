#include "scene_loader.h"

#include "btlogger.h"
#include "btuuid.h"
#include "entt/entity/fwd.hpp"
#include "game_system_logic/component/component_registry.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "scene_serialization.h"
#include "service_finder/service_finder.h"
#include "timer/timer.h"

#include <stdexcept>
#include <string>
#include <vector>


BT::world::Scene_loader::Scene_loader()
{
    BT_SERVICE_FINDER_ADD_SERVICE(Scene_loader, this);
}

void BT::world::Scene_loader::load_scene_additive(std::string const& scene_name)
{
    m_load_scene_requests.emplace_back(scene_name);
}

void BT::world::Scene_loader::unload_all_scenes()
{
    m_unload_all_scenes_request = true;

    // Clear load requests since these haven't been processed yet.
    // (and would have been unloaded immediately if they did get processed)
    m_load_scene_requests.clear();
}

size_t BT::world::Scene_loader::get_num_loaded_scenes() const
{
    return m_loaded_scenes.size();
}

void BT::world::Scene_loader::save_all_entities_into_scene(std::string const& scene_name) const
{   // Ensure no multi-scene saving (yet!!! ... but, i still dont knoew how scenes and stuff will be used in the game istsef.  -Thea 2025/11/04
    assert(m_loaded_scenes.size() <= 1);

    // Build scene serialization. 
    Scene_serialization scene_serialized;

    auto all_ent_uuids{ service_finder::find_service<Entity_container>().get_all_entity_uuids() };
    scene_serialized.entities.reserve(all_ent_uuids.size());

    for (auto ent_uuid : all_ent_uuids)
        scene_serialized.entities.emplace_back(serialize_entity(ent_uuid));

    // Write to disk.
    serialize_scene_data_to_disk(scene_serialized, scene_name);
}


namespace
{

using namespace BT;
using Scene_entity_list_t = world::Scene_loader::Scene_entity_list_t;

void internal_unload_scene(Entity_container& entity_container,
                           std::string const& scene_name,
                           Scene_entity_list_t const& scene_entity_list)
{   // Destroy all entities associated with scene.
    for (auto entity : scene_entity_list)
    {
        entity_container.destroy_entity(entity);
    }

    BT_TRACEF("Unloaded scene \"%s\"", scene_name.c_str());
}

Scene_entity_list_t internal_load_scene(Entity_container& entity_container,
                                        std::string const& scene_name)
{   // Deserialize scene into creation list.
    auto scene_data{ world::deserialize_scene_data_from_disk(scene_name) };

    // Create entities with components.
    Scene_entity_list_t created_entities;

    for (auto& entity : scene_data.entities)
    {   // Assert that the provided UUID is valid.
        assert(!entity.entity_uuid.is_nil());

        auto ecs_entity = entity_container.create_entity(entity.entity_uuid);

        for (auto& component : entity.components)
        {   // Construct component inside entity.
            component::construct_component(ecs_entity, component.type_name, component.members_j);
        }

        // Add entity to creation list.
        created_entities.emplace_back(entity.entity_uuid);
    }

    BT_TRACEF("Loaded scene \"%s\"", scene_name.c_str());

    return created_entities;
}

void add_uuid_to_list_recursive(Entity_container const& entity_container,
                                entt::registry const& reg,
                                UUID id,
                                std::vector<UUID>& out_uuid_list)
{
    out_uuid_list.emplace_back(id);

    auto const& trans_hier =
        reg.get<component::Transform_hierarchy>(entity_container.find_entity(id));
    for (UUID child_id : trans_hier.children_entities)
    {
        add_uuid_to_list_recursive(entity_container, reg, child_id, out_uuid_list);
    }
}

void destroy_dangling_child_entities(Entity_container& entity_container)
{
    Timer timer;
    timer.start_timer();

    auto& reg{ entity_container.get_ecs_registry() };
    auto view{ reg.view<component::Transform_hierarchy>() };

    std::vector<UUID> delete_uuids;

    for (auto&& [ent, trans_hier] : view->each())
    {
        if (!trans_hier.parent_entity.is_nil() &&
            !entity_container.entity_exists(trans_hier.parent_entity))
        {
            // This one's dangling. Mark this and children to delete.
            add_uuid_to_list_recursive(entity_container,
                                       reg,
                                       entity_container.find_entity_uuid(ent),
                                       delete_uuids);
        }
    }

    // Delete marked entities.
    for (UUID id : delete_uuids)
    {
        entity_container.destroy_entity(id);
    }

    BT_WARNF("%s() took %.3f ms.", __func__, timer.calc_delta_time() * 1000);
}

void check_loaded_scenes_integrity(Entity_container& entity_container)
{
    Timer timer;
    timer.start_timer();

    // Ensure transform hierarchies have correct alignment.
    auto const& reg{ entity_container.get_ecs_registry() };
    auto view{ reg.view<component::Transform_hierarchy const>() };

    for (auto&& [ent, trans_hier] : view->each())
    {
        UUID ent_uuid{ entity_container.find_entity_uuid(ent) };

        if (!trans_hier.parent_entity.is_nil())
        {
            // Parent must list me as child.
            auto parent_ent{ entity_container.find_entity(trans_hier.parent_entity) };
            auto& parent_trans_hier{ view->get(parent_ent) };

            bool found_self_in_parents_child_list{
                std::find(parent_trans_hier.children_entities.begin(),
                          parent_trans_hier.children_entities.end(),
                          ent_uuid) != parent_trans_hier.children_entities.end()
            };
            if (!found_self_in_parents_child_list)
            {
                BT_ERRORF(
                    "During %s(), %s declares parent as %s, however, the parent does not list the "
                    "child in list of children.",
                    __func__,
                    UUID_helper::to_pretty_repr(ent_uuid).c_str(),
                    UUID_helper::to_pretty_repr(trans_hier.parent_entity).c_str());

                throw std::runtime_error("Should have found self in parent's child list.");
            }
        }

        for (UUID child_uuid : trans_hier.children_entities)
        {
            // Children must list me as parent.
            auto child_ent{ entity_container.find_entity(child_uuid) };
            auto& child_trans_hier{ view->get(child_ent) };

            bool found_self_in_childs_parent{ child_trans_hier.parent_entity == ent_uuid };
            if (!found_self_in_childs_parent)
            {
                BT_ERRORF(
                    "During %s(), %s lists %s as a child, however, the child does not declare the "
                    "parent as its parent. Instead, it declares: %s",
                    __func__,
                    UUID_helper::to_pretty_repr(ent_uuid).c_str(),
                    UUID_helper::to_pretty_repr(child_uuid).c_str(),
                    UUID_helper::to_pretty_repr(child_trans_hier.parent_entity).c_str());

                throw std::runtime_error("Should have found self in child's parent UUID.");
            }
        }
    }

    BT_WARNF("%s() took %.3f ms.", __func__, timer.calc_delta_time() * 1000);
}

}  // namespace


void BT::world::Scene_loader::process_scene_loading_requests()
{
    auto& entity_container{ service_finder::find_service<Entity_container>() };

    // Process unload request first.
    if (m_unload_all_scenes_request)
    {
        for (auto& loaded_scene : m_loaded_scenes)
        {
            internal_unload_scene(entity_container, loaded_scene.first, loaded_scene.second);
        }
        m_loaded_scenes.clear();

        destroy_dangling_child_entities(entity_container);

        m_unload_all_scenes_request = false;
    }

    // Process load requests.
    bool check_integrity_after{ !m_load_scene_requests.empty() };

    for (auto& scene_name : m_load_scene_requests)
    {
        auto created_entity_list{ internal_load_scene(entity_container, scene_name) };
        m_loaded_scenes.emplace(scene_name, std::move(created_entity_list));
    }
    m_load_scene_requests.clear();

    if (check_integrity_after)
        check_loaded_scenes_integrity(entity_container);

    // @TEMP: @THEA: Just makinig sure that only one scene can be loaded in at a time.
    assert(m_loaded_scenes.size() == 0 || m_loaded_scenes.size() == 1);
}
