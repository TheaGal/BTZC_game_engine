#include "update_light_transforms.h"

#include "btdatecheck.h"
#include "btglm.h"
#include "entt/entity/registry.hpp"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "service_finder/service_finder.h"
#include "txp_renderer_public.h"

#include <tracy/Tracy.hpp>


void BT::system::update_light_transforms()
{
    ZoneScoped;

    auto& reg{ service_finder::find_service<Entity_container>().get_ecs_registry() };

    // Write light transforms.
    for (auto&& [_, transform, light] :
         reg.view<component::Transform const, TXP::component::Directional_light>().each())
    {
        // @TODO: Include interpolation instead of just straight copying.
        date_deadline(2027, 1, 1);

        mat3 rot_mat3;
        glm_quat_mat3(const_cast<float_t*>(transform.rotation.raw), rot_mat3);

        glm_mat3_mulv(rot_mat3, vec3{ 0, 0, 1 }, light.direction.raw);
    }

    for (auto&& [_, transform, light] :
         reg.view<component::Transform const, TXP::component::Point_light>().each())
    {
        // @TODO: Include interpolation instead of just straight copying.
        date_deadline(2027, 1, 1);

        btglm_rvec3_to_vec3(transform.position.raw, light.origin.raw);
    }
}
