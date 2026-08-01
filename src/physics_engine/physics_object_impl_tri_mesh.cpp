#include "physics_object_impl_tri_mesh.h"

#include "Jolt/Jolt.h"
#include "Jolt/Geometry/IndexedTriangle.h"
#include "Jolt/Geometry/Triangle.h"
#include "Jolt/Math/Float3.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Body/BodyInterface.h"
#include "Jolt/Physics/Body/MotionType.h"
#include "Jolt/Physics/Collision/Shape/MeshShape.h"
#include "Jolt/Physics/EActivation.h"
#include "btdatecheck.h"
#include "btglm.h"
#include "btlogger.h"
#include "physics_engine.h"
#include "physics_engine_impl_layers.h"
#include "service_finder/service_finder.h"
#include "txp_renderer/debug/debug_render_job.h"
#include "txp_renderer_public.h"

#include <cassert>


BT::Phys_obj_impl_tri_mesh::Phys_obj_impl_tri_mesh(std::string const& model_name,
                                                   JPH::EMotionType motion_type,
                                                   Physics_transform&& init_transform)
    : m_phys_body_ifc{ *reinterpret_cast<JPH::BodyInterface*>(service_finder::find_service<Physics_engine>().get_physics_body_ifc()) }
    , m_can_move{ motion_type == JPH::EMotionType::Kinematic }
{
    if (motion_type == JPH::EMotionType::Dynamic)
    {
        logger::printe(logger::ERROR, "Dynamic motion type not allowed.");
        assert(false);
        return;
    }

    auto basic_model =
        BT::service_finder::find_service<TXP::Renderer>().get_model_basic_data(model_name);

    // @NOTE: I think there might be some extra stuff Jolt is doing in the behind
    //   that makes the triangle list better than using the inefficient-for-physics
    //   indexed triangle list from the obj file (or placebo hehe), so I'm gonna
    //   stick to the triangle list below for now.  -Thea 2025/05/29
#define INDEXED_TRIANGLE_LIST 0
#if INDEXED_TRIANGLE_LIST
    JPH::VertexList vertex_list;
    vertex_list.reserve(basic_model.vertices.size());
    for (auto& vertex : basic_model.vertices)
    {
        vertex_list.emplace_back(vertex.position[0], vertex.position[1], vertex.position[2]);
    }

    assert(basic_model.indices.size() % 3 == 0);
    JPH::IndexedTriangleList indexed_tris_list;
    indexed_tris_list.reserve(basic_model.indices.size() / 3);
    for (size_t i = 0 ; i < basic_model.indices.size(); i += 3)
    {
        indexed_tris_list.emplace_back(basic_model.indices[i + 0],
                                       basic_model.indices[i + 1],
                                       basic_model.indices[i + 2],
                                       0);
    }
    JPH::MeshShapeSettings mesh_settings(vertex_list, indexed_tris_list);
#else
    JPH::TriangleList tri_list;
    for (size_t i = 0; i < basic_model.indices.size(); i += 3)
    {
        JPH::Float3 p0{ basic_model.vertices[basic_model.indices[i + 0]].position[0],
                        basic_model.vertices[basic_model.indices[i + 0]].position[1],
                        basic_model.vertices[basic_model.indices[i + 0]].position[2] };
        JPH::Float3 p1{ basic_model.vertices[basic_model.indices[i + 1]].position[0],
                        basic_model.vertices[basic_model.indices[i + 1]].position[1],
                        basic_model.vertices[basic_model.indices[i + 1]].position[2] };
        JPH::Float3 p2{ basic_model.vertices[basic_model.indices[i + 2]].position[0],
                        basic_model.vertices[basic_model.indices[i + 2]].position[1],
                        basic_model.vertices[basic_model.indices[i + 2]].position[2] };
        tri_list.emplace_back(p0, p1, p2);
    }
    JPH::MeshShapeSettings mesh_settings(tri_list);
#endif  // INDEXED_TRIANGLE_LIST

    mesh_settings.SetEmbedded();
    JPH::BodyCreationSettings mesh_body_settings(&mesh_settings,
                                                 init_transform.position,
                                                 init_transform.rotation,
                                                 motion_type,
                                                 (m_can_move ? Layers::MOVING : Layers::NON_MOVING));
    m_body_id = m_phys_body_ifc.CreateAndAddBody(mesh_body_settings, JPH::EActivation::DontActivate);

    // Create debug render job.
    m_debug_mesh_id = TXP::debug::emplace_debug_model(model_name, TXP::debug::PHYSICS_WIREFRAME);
}

BT::Phys_obj_impl_tri_mesh::~Phys_obj_impl_tri_mesh()
{
    m_phys_body_ifc.RemoveBody(m_body_id);
    TXP::debug::remove_debug_model(m_debug_mesh_id);
}

void BT::Phys_obj_impl_tri_mesh::move_kinematic(Physics_transform&& new_transform)
{
    if (!m_can_move)
    {
        logger::printe(logger::ERROR, "Object marked as unmovable.");

        constexpr bool k_error_on_try_move{ false };
        if constexpr (k_error_on_try_move)
        {
            assert(false);
        }

        return;
    }
    m_phys_body_ifc.MoveKinematic(m_body_id,
                                  new_transform.position,
                                  new_transform.rotation,
                                  Physics_engine::k_simulation_delta_time);
}

BT::Physics_transform BT::Phys_obj_impl_tri_mesh::read_transform()
{
    return { m_phys_body_ifc.GetCenterOfMassPosition(m_body_id),
             m_phys_body_ifc.GetRotation(m_body_id) };
}

void BT::Phys_obj_impl_tri_mesh::update_debug_mesh()
{
    auto current_trans{ read_transform() };

    // @TODO: When camera or renderer changes, this needs to change.
    //        Ensure matching with `write_render_transforms.cpp`.
    mat4 graphic_trans;
    glm_translate_make(graphic_trans, vec3{ static_cast<float_t>(current_trans.position.GetX()),
                                            static_cast<float_t>(current_trans.position.GetY()),
                                            static_cast<float_t>(current_trans.position.GetZ()) });
    glm_quat_rotate(graphic_trans, versor{ current_trans.rotation.GetX(),
                                           current_trans.rotation.GetY(),
                                           current_trans.rotation.GetZ(),
                                           current_trans.rotation.GetW() }, graphic_trans);
    BT::date_deadline(2026, 8, 10);
    // glm_mat4_copy(graphic_trans,
    //               get_main_debug_mesh_pool()
    //                   .get_debug_mesh_volatile_handle(m_debug_mesh_id).transform);
}
