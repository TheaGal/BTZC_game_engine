#pragma once

#include "btglm.h"
#include "test_types.h"


namespace BT
{
namespace tests
{

Test_result test_btglm_vec2s_copies_and_moves()
{
    vec2s koko = { 1, 2 };

    vec2s jojo = koko;

    auto const check_vec2_fn = [](void (*check_fn)(float_t, float_t), vec2s a, vec2s b) {
        check_fn(a.x, b.x);
        check_fn(a.y, b.y);
    };

    check_vec2_fn(check_eq,jojo, koko);

    vec2s lolo = std::move(jojo);

    check_vec2_fn(check_eq, lolo, koko);
    // check_vec2_fn(check_neq, lolo, jojo);

    return TEST_RESULT_PASSED;
}

Test_result test_btglm_vec3s_copies_and_moves()
{
    vec3s koko = { 1, 2, 3 };

    vec3s jojo = koko;

    auto const check_vec3_fn = [](void (*check_fn)(float_t, float_t), vec3s a, vec3s b) {
        check_fn(a.x, b.x);
        check_fn(a.y, b.y);
        check_fn(a.z, b.z);
    };

    check_vec3_fn(check_eq, jojo, koko);

    vec3s lolo = std::move(jojo);

    check_vec3_fn(check_eq, lolo, koko);
    // check_vec3_fn(check_neq, lolo, koko);

    return TEST_RESULT_PASSED;
}

Test_result test_btglm_vec4s_copies_and_moves()
{
    vec4s koko = { 1, 2, 3, 4 };

    vec4s jojo = koko;

    auto const check_vec4_fn = [](void (*check_fn)(float_t, float_t), vec4s a, vec4s b) {
        check_fn(a.x, b.x);
        check_fn(a.y, b.y);
        check_fn(a.z, b.z);
        check_fn(a.w, b.w);
    };

    check_vec4_fn(check_eq, jojo, koko);

    vec4s lolo = std::move(jojo);

    check_vec4_fn(check_eq, lolo, koko);
    // check_vec4_fn(check_neq, lolo, koko);

    return TEST_RESULT_PASSED;
}

Test_result test_btglm_mat3s_copies_and_moves()
{
    mat3 _{ { 1, 2, 3 }, { 4, 5, 6 }, { 7, 8, 9 } };

    mat3s koko;
    glm_mat3_copy(_, koko.raw);

    mat3s jojo = koko;

    auto const check_mat3_fn = [](void (*check_fn)(float_t, float_t), mat3s a, mat3s b) {
        check_fn(a.m00, b.m00);
        check_fn(a.m01, b.m01);
        check_fn(a.m02, b.m02);
        check_fn(a.m10, b.m10);
        check_fn(a.m11, b.m11);
        check_fn(a.m12, b.m12);
        check_fn(a.m20, b.m20);
        check_fn(a.m21, b.m21);
        check_fn(a.m22, b.m22);
    };

    check_mat3_fn(check_eq, jojo, koko);

    mat3s lolo = std::move(jojo);

    check_mat3_fn(check_eq, lolo, koko);
    // check_mat3_fn(check_neq, lolo, koko);

    return TEST_RESULT_PASSED;
}

Test_result test_btglm_mat4s_copies_and_moves()
{
    mat4 _{ { 1, 2, 3, 4 }, { 5, 6, 7, 8 }, { 9, 10, 11, 12 }, { 13, 14, 15, 16 } };

    mat4s koko;
    glm_mat4_copy(_, koko.raw);

    mat4s jojo = koko;

    auto const check_mat4_fn = [](void (*check_fn)(float_t, float_t), mat4s a, mat4s b) {
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

    check_mat4_fn(check_eq, jojo, koko);

    mat4s lolo = std::move(jojo);

    check_mat4_fn(check_eq, lolo, koko);
    // check_mat4_fn(check_neq, lolo, koko);

    return TEST_RESULT_PASSED;
}

Test_result test_btglm_vec2s_in_vector()
{
    vec2s pilot_vec2 = { 1, 2 };

    std::vector<vec2s> things_copied;
    for (uint32_t i = 0; i < 100; i++)
        things_copied.emplace_back(pilot_vec2);
    things_copied.shrink_to_fit();

    std::vector<vec2s> things_moved;
    for (uint32_t i = 0; i < 100; i++)
    {
        vec2s new_vec2;
        glm_vec2_copy(pilot_vec2.raw, new_vec2.raw);
        things_moved.emplace_back(std::move(new_vec2));
    }
    things_moved.shrink_to_fit();

    return TEST_RESULT_PASSED;
}

Test_result test_btglm_vec3s_in_vector()
{
    vec3s pilot_vec3 = { 1, 2, 3 };

    std::vector<vec3s> things_copied;
    for (uint32_t i = 0; i < 100; i++)
        things_copied.emplace_back(pilot_vec3);
    things_copied.shrink_to_fit();

    std::vector<vec3s> things_moved;
    for (uint32_t i = 0; i < 100; i++)
    {
        vec3s new_vec3;
        glm_vec3_copy(pilot_vec3.raw, new_vec3.raw);
        things_moved.emplace_back(std::move(new_vec3));
    }
    things_moved.shrink_to_fit();

    return TEST_RESULT_PASSED;
}

Test_result test_btglm_vec4s_in_vector()
{
    vec4s pilot_vec4 = { 1, 2, 3, 4 };

    std::vector<vec4s> things_copied;
    for (uint32_t i = 0; i < 100; i++)
        things_copied.emplace_back(pilot_vec4);
    things_copied.shrink_to_fit();

    std::vector<vec4s> things_moved;
    for (uint32_t i = 0; i < 100; i++)
    {
        vec4s new_vec4;
        glm_vec4_copy(pilot_vec4.raw, new_vec4.raw);
        things_moved.emplace_back(std::move(new_vec4));
    }
    things_moved.shrink_to_fit();

    return TEST_RESULT_PASSED;
}

Test_result test_btglm_mat3s_in_vector()
{
    mat3 _{ { 1, 2, 3 }, { 4, 5, 6 }, { 7, 8, 9 } };
    mat3s pilot_mat3;
    glm_mat3_copy(_, pilot_mat3.raw);

    std::vector<mat3s> things_copied;
    for (uint32_t i = 0; i < 100; i++)
        things_copied.emplace_back(pilot_mat3);
    things_copied.shrink_to_fit();

    std::vector<mat3s> things_moved;
    for (uint32_t i = 0; i < 100; i++)
    {
        mat3s new_mat3;
        glm_mat3_copy(pilot_mat3.raw, new_mat3.raw);
        things_moved.emplace_back(std::move(new_mat3));
    }
    things_moved.shrink_to_fit();

    return TEST_RESULT_PASSED;
}

Test_result test_btglm_mat4s_in_vector()
{
    mat4 _{ { 1, 2, 3, 4 }, { 5, 6, 7, 8 }, { 9, 10, 11, 12 }, { 13, 14, 15, 16 } };
    mat4s pilot_mat4;
    glm_mat4_copy(_, pilot_mat4.raw);

    std::vector<mat4s> things_copied;
    for (uint32_t i = 0; i < 100; i++)
        things_copied.emplace_back(pilot_mat4);
    things_copied.shrink_to_fit();

    std::vector<mat4s> things_moved;
    for (uint32_t i = 0; i < 100; i++)
    {
        mat4s new_mat4;
        glm_mat4_copy(pilot_mat4.raw, new_mat4.raw);
        things_moved.emplace_back(std::move(new_mat4));
    }
    things_moved.shrink_to_fit();

    return TEST_RESULT_PASSED;
}

} // namespace tests
} // namespace BT