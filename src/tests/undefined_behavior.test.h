#pragma once

#include "btglm.h"
#include "entt/entity/registry.hpp"
#include "test_types.h"
#include "txp_renderer_public.h"

#include <cstddef>
#include <cstdint>
#include <cstring>


namespace BT
{
namespace tests
{

Test_result test_ub_problematic_struct()
{
    // @THEA: This test didn't work for isolating the problem for me.

    // @NOTE: this is just a copy of the problematic type.
    struct Render_object_config
    {
        #warning struct uses cglm
        enum Render_layer : uint16_t
        {
            RENDER_LAYER_ALL          = 0b1111'1111'1111'1111,
            RENDER_LAYER_NONE         = 0b0000'0000'0000'0000,

            RENDER_LAYER_DEFAULT      = 0b0000'0000'0000'0001,
            RENDER_LAYER_INVISIBLE    = 0b0000'0000'0000'0010,
            RENDER_LAYER_LEVEL_EDITOR = 0b0000'0000'0000'0100,
        } render_layer{ 0 };

        std::string model_name;

        mat4s transform = mat4s{ GLM_MAT4_IDENTITY_INIT };

        // ^^ Required ^^ / vv Optional vv

        std::string sub_mesh_name;
        bool sub_mesh_zero_origin_position;  // @NOTE: setting this to true will crash the program since implementation is aborted.  -Thea 2026/08/04

        std::string material_palette;
        bool is_deformed{ false };

        // ^^ Optional ^^ / vv Set up by Renderer vv

        struct Renderer_owned_data
        {
            using pool_key_t = std::uint32_t;
            pool_key_t pool_key{ (pool_key_t)-1 };
        } renderer_owned_data;
    };

    entt::registry ecs_registry;

    for (size_t i = 0; i < 10000; i++)
    {
        auto new_ent{ ecs_registry.create() };
        auto const& new_roc{ ecs_registry.emplace_or_replace<Render_object_config>(new_ent) };

        static auto const check_mat4_fn = [](void (*check_fn)(float_t, float_t), mat4s a, mat4s b) {
            check_fn(a.m00, b.m00);
            check_fn(a.m01, b.m01);
            check_fn(a.m02, b.m02);
            check_fn(a.m03, b.m03);
            check_fn(a.m10, b.m10);
            check_fn(a.m11, b.m11);
            check_fn(a.m12, b.m12);
            check_fn(a.m13, b.m13);
            check_fn(a.m20, b.m20);
            check_fn(a.m21, b.m21);
            check_fn(a.m22, b.m22);
            check_fn(a.m23, b.m23);
            check_fn(a.m30, b.m30);
            check_fn(a.m31, b.m31);
            check_fn(a.m32, b.m32);
            check_fn(a.m33, b.m33);
        };

        mat4s identity_mat4;
        glm_mat4_identity(identity_mat4.raw);

        check_mat4_fn(check_eq, identity_mat4, new_roc.transform);
    }

    return TEST_RESULT_PASSED;
}

Test_result test_ub_struct_elem_align()
{
    // @THEA: This test didn't work for isolating the problem for me.

    using T = TXP::component::Render_object_config;

    T default_obj{};
    alignas(alignof(T)) std::array<uint8_t, sizeof(T) + alignof(T) - 1> data_field{};

    for (size_t i = 0; i < alignof(T); i++)
    {
        auto& ref_to_data{ *reinterpret_cast<T*>(&data_field[i]) };
        ref_to_data = {};

        check_eq(ref_to_data.render_layer, default_obj.render_layer);
        check_eq(ref_to_data.model_name, default_obj.model_name);
        check_eq(ref_to_data.transform.m00, default_obj.transform.m00);
        check_eq(ref_to_data.transform.m01, default_obj.transform.m01);
        check_eq(ref_to_data.transform.m02, default_obj.transform.m02);
        check_eq(ref_to_data.transform.m03, default_obj.transform.m03);
        check_eq(ref_to_data.transform.m10, default_obj.transform.m10);
        check_eq(ref_to_data.transform.m11, default_obj.transform.m11);
        check_eq(ref_to_data.transform.m12, default_obj.transform.m12);
        check_eq(ref_to_data.transform.m13, default_obj.transform.m13);
        check_eq(ref_to_data.transform.m20, default_obj.transform.m20);
        check_eq(ref_to_data.transform.m21, default_obj.transform.m21);
        check_eq(ref_to_data.transform.m22, default_obj.transform.m22);
        check_eq(ref_to_data.transform.m23, default_obj.transform.m23);
        check_eq(ref_to_data.transform.m30, default_obj.transform.m30);
        check_eq(ref_to_data.transform.m31, default_obj.transform.m31);
        check_eq(ref_to_data.transform.m32, default_obj.transform.m32);
        check_eq(ref_to_data.transform.m33, default_obj.transform.m33);
        check_eq(ref_to_data.sub_mesh_name, default_obj.sub_mesh_name);
        check_eq(ref_to_data.sub_mesh_zero_origin_position,
                 default_obj.sub_mesh_zero_origin_position);
        check_eq(ref_to_data.material_palette, default_obj.material_palette);
        check_eq(ref_to_data.is_deformed, default_obj.is_deformed);
        check_eq(ref_to_data.renderer_owned_data.pool_key,
                 default_obj.renderer_owned_data.pool_key);
    }

    return TEST_RESULT_PASSED;
}

} // namespace tests
} // namespace BT
