#include "write_render_transforms.h"

#include "entt/entity/registry.hpp"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "service_finder/service_finder.h"
#include "txp_renderer_public.h"


void BT::system::write_render_transforms()
{
    auto& reg{ service_finder::find_service<Entity_container>().get_ecs_registry() };

    // Write transforms.
    auto view{
        reg.view<component::Transform const, TXP::component::Render_object_config>()
    };
    for (auto&& [_, transform, rend_obj_ref] : view.each())
    {
        // @TODO: Include interpolation instead of just straight copying.

        // Calculate TRS into mat4 transform.
        auto* rend_trans{ rend_obj_ref.transform.raw };
        glm_translate_make(rend_trans,
                           vec3{ static_cast<float_t>(transform.position.x),
                                 static_cast<float_t>(transform.position.y),
                                 static_cast<float_t>(transform.position.z) });
        glm_quat_rotate(rend_trans, const_cast<float_t*>(transform.rotation.raw), rend_trans);
        glm_scale(rend_trans, const_cast<float_t*>(transform.scale.raw));
    }
}
