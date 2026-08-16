#include "rail_line.h"

#include "btdatecheck.h"
#include "btglm.h"

#include <cassert>
#include <cmath>
#include <unordered_map>


namespace BT
{
namespace component
{

auto Rail_line::Build_code_info::calculate_transform(vec3 place_pos,
                                                     float_t place_angle,
                                                     float_t length) const
    -> Rail_position_transform
{
    Rail_position_transform trans = calc_transform_fn(length);

    mat4 rot;
    glm_euler_zyx(vec3{ 0, place_angle, 0 }, rot);
    glm_mat4_mulv3(rot, trans.position.raw, 0, trans.position.raw);

    glm_vec3_add(place_pos, trans.position.raw, trans.position.raw);

    trans.angle_y += place_angle;

    date_deadline(2026, 8, 19);  // @CHECK: confirm working

    return trans;
}

void Rail_line::Build_code_info::advance_place_transform(vec3& place_pos,
                                                         float_t& place_angle,
                                                         bool forward) const
{
    mat4 place_advance_rot = GLM_MAT4_IDENTITY_INIT;
    glm_rotate_y(place_advance_rot, place_angle, place_advance_rot);
    vec3 place_advance_delta_pos_cooked;
    glm_mat4_mulv3(place_advance_rot,
                   const_cast<float_t*>(place_advance_delta_pos.raw),
                   0,
                   place_advance_delta_pos_cooked);

    glm_vec3_add(place_pos, place_advance_delta_pos_cooked, place_pos);
    place_angle += place_advance_delta_angle;
}


namespace
{

struct Bezier_curve
{
    vec3s c0;
    vec3s c1;
    vec3s c2;
    vec3s c3;

    /// @NOTE: expensive calc.
    constexpr float_t calc_length() const
    {
        auto c0r{ const_cast<float_t*>(c0.raw) };
        auto c1r{ const_cast<float_t*>(c1.raw) };
        auto c2r{ const_cast<float_t*>(c2.raw) };
        auto c3r{ const_cast<float_t*>(c3.raw) };

        float_t rough_length = 0;
        rough_length += glm_vec3_distance(c0r, c1r);  // @TODO: make this constexpr so that this all is constexpr.
        rough_length += glm_vec3_distance(c1r, c2r);
        rough_length += glm_vec3_distance(c2r, c3r);

        uint32_t iterations = rough_length * 1000;
        if (iterations == 0)
            return 0.0f;

        float_t fine_length = 0;
        vec3 prev_pos;
        for (uint32_t i = 0; i <= iterations; i++)
        {
            float_t t{ static_cast<float_t>(i) / iterations };
            vec3s pos = calc_position_on_curve(t);

            if (i > 0)
            {
                fine_length += glm_vec3_distance(prev_pos, pos.raw);
            }

            glm_vec3_copy(pos.raw, prev_pos);
        }

        return fine_length;
    }

    constexpr vec3s calc_position_on_curve(float_t t) const
    {
        // Ref: https://acegikmo.medium.com/the-ever-so-lovely-b%C3%A9zier-curve-eb27514da3bf
        float_t t2 = t * t;
        float_t t3 = t2 * t;

        // var point =
        //     c0 * (    -t3 + 3 * t2 - 3 * t + 1) +
        //     c1 * ( 3 * t3 - 6 * t2 + 3 * t    ) +
        //     c2 * (-3 * t3 + 3 * t2            ) +
        //     c3 * (     t3                     );
        vec3s point = GLM_VEC3_ZERO_INIT;
        // clang-format off
        glm_vec3_muladds(const_cast<float_t*>(c0.raw),     -t3 + 3 * t2 - 3 * t + 1, point.raw);
        glm_vec3_muladds(const_cast<float_t*>(c1.raw),  3 * t3 - 6 * t2 + 3 * t    , point.raw);
        glm_vec3_muladds(const_cast<float_t*>(c2.raw), -3 * t3 + 3 * t2            , point.raw);
        glm_vec3_muladds(const_cast<float_t*>(c3.raw),      t3                     , point.raw);
        // clang-format on

        return point;
    }
};

vec3s calc_place_advance_delta_pos_of_curve(float_t radius, float_t curve_angle, bool flip_x)
{
    return vec3s{
        .x = (cosf(curve_angle) - cosf(0)) * radius * (flip_x ? -1 : 1),
        .y = 0,
        .z = -(sinf(curve_angle) - sinf(0)) * radius,
    };
}

} // namespace

constexpr float_t curve_radius_1{ 38.8178 };
constexpr float_t curve_radius_2{ 42.8178 };
constexpr float_t curve_radius_3{ 46.8178 };
constexpr float_t curve_radius_4{ 50.8178 };

constexpr Bezier_curve k_bz_slopechange_up{ .c0 = { 0, 0, 0 },
                                            .c1 = { 0, 0, 9.8995 },
                                            .c2 = { 0, 0.004963, 10.0496 },
                                            .c3 = { 0, 1, 20 } };
constexpr Bezier_curve k_bz_slopechange_up_return{ .c0 = { 0, 0, 0 },
                                                   .c1 = { 0, 0.985037, 9.85037 },
                                                   .c2 = { 0, 1, 10 },
                                                   .c3 = { 0, 1, 20 } };
constexpr Bezier_curve k_bz_slopechange_down{ .c0 = { 0, 0, 0 },
                                              .c1 = { 0, 0, 9.8995 },
                                              .c2 = { 0, -0.004963, 10.0496 },
                                              .c3 = { 0, -1, 20 } };
constexpr Bezier_curve k_bz_slopechange_down_return{ .c0 = { 0, 0, 0 },
                                                     .c1 = { 0, -0.985037, 9.85037 },
                                                     .c2 = { 0, -1, 10 },
                                                     .c3 = { 0, -1, 20 } };

using Build_code_info_map = std::unordered_map<Rail_line::Build_code, Rail_line::Build_code_info>;
using Rail_position_transform = Rail_line::Build_code_info::Rail_position_transform;

Build_code_info_map Rail_line::s_build_code_to_info_map{  // @COPYPASTA: there is so much here!!
    {
        BC_STRAIGHT,
        {
            .length = 10,
            .calc_transform_fn =
                [](float_t length) {
                    vec3s pos{ 0, 0, -1 };
                    glm_vec3_scale(pos.raw, length, pos.raw);
                    return Rail_position_transform{ .position = pos };
                },
            .place_advance_delta_pos = { 0, 0, -10 },
            .place_advance_delta_angle = glm_rad(0),
        },
    },
    {
        BC_STRAIGHT_UPHILL,
        {
            .length = glm_vec3_norm(vec3{ 0, 1, 10 }),
            .calc_transform_fn =
                [](float_t length) {
                    static float_t const k_length_normalizer{ 1.0f /
                                                              glm_vec3_norm(vec3{ 0, 1, 10 }) };

                    vec3s pos;
                    glm_vec3_lerp(vec3{}, vec3{ 0, 1, -10 }, length * k_length_normalizer, pos.raw);

                    static float_t const k_uphill_angle{ atan2f(1, -10) };
                    return Rail_position_transform{ .position = pos, .angle_x = k_uphill_angle };
                },
            .place_advance_delta_pos = { 0, 1, -10 },
            .place_advance_delta_angle = glm_rad(0),
        },
    },
    {
        BC_STRAIGHT_DOWNHILL,
        {
            .length = glm_vec3_norm(vec3{ 0, -1, 10 }),
            .calc_transform_fn =
                [](float_t length) {
                    static float_t const k_length_normalizer{ 1.0f /
                                                              glm_vec3_norm(vec3{ 0, -1, 10 }) };

                    vec3s pos;
                    glm_vec3_lerp(vec3{},
                                  vec3{ 0, -1, -10 },
                                  length * k_length_normalizer,
                                  pos.raw);

                    static float_t const k_downhill_angle{ atan2f(-1, -10) };
                    return Rail_position_transform{ .position = pos, .angle_x = k_downhill_angle };
                },
            .place_advance_delta_pos = { 0, -1, -10 },
            .place_advance_delta_angle = glm_rad(0),
        },
    },
    {
        BC_STRAIGHT_ROLL_LEFT,
        {
            .length = 10,
            .calc_transform_fn =
                [](float_t length) {
                    vec3s pos{ 0, 0, -1 };
                    glm_vec3_scale(pos.raw, length, pos.raw);
                    return Rail_position_transform{
                        .position = pos,
                        .angle_z = glm_lerp(glm_rad(0), glm_rad(10), length / 10),
                    };
                },
            .place_advance_delta_pos = { 0, 0, -10 },
            .place_advance_delta_angle = glm_rad(0),
        },
    },
    {
        BC_STRAIGHT_ROLL_LEFT_RETURN,
        {
            .length = 10,
            .calc_transform_fn =
                [](float_t length) {
                    vec3s pos{ 0, 0, -1 };
                    glm_vec3_scale(pos.raw, length, pos.raw);
                    return Rail_position_transform{
                        .position = pos,
                        .angle_z = glm_lerp(glm_rad(10), glm_rad(0), length / 10),
                    };
                },
            .place_advance_delta_pos = { 0, 0, -10 },
            .place_advance_delta_angle = glm_rad(0),
        },
    },
    {
        BC_STRAIGHT_ROLL_RIGHT,
        {
            .length = 10,
            .calc_transform_fn =
                [](float_t length) {
                    vec3s pos{ 0, 0, -1 };
                    glm_vec3_scale(pos.raw, length, pos.raw);
                    return Rail_position_transform{
                        .position = pos,
                        .angle_z = glm_lerp(glm_rad(0), glm_rad(-10), length / 10),
                    };
                },
            .place_advance_delta_pos = { 0, 0, -10 },
            .place_advance_delta_angle = glm_rad(0),
        },
    },
    {
        BC_STRAIGHT_ROLL_RIGHT_RETURN,
        {
            .length = 10,
            .calc_transform_fn =
                [](float_t length) {
                    vec3s pos{ 0, 0, -1 };
                    glm_vec3_scale(pos.raw, length, pos.raw);
                    return Rail_position_transform{
                        .position = pos,
                        .angle_z = glm_lerp(glm_rad(-10), glm_rad(0), length / 10),
                    };
                },
            .place_advance_delta_pos = { 0, 0, -10 },
            .place_advance_delta_angle = glm_rad(0),
        },
    },
    {
        BC_CURVE_LEFT_1,
        {
            .length = curve_radius_1 * glm_rad(15),
            .calc_transform_fn =
                [](float_t length) {
                    constexpr float_t k_len{ curve_radius_1 * glm_rad(15) };

                    float_t t = length / k_len;
                    float_t const advance_angle{ glm_lerp(glm_rad(0), glm_rad(15), t) };
                    return Rail_position_transform{
                        .position = calc_place_advance_delta_pos_of_curve(curve_radius_1,
                                                                          advance_angle,
                                                                          false),
                        .angle_y = advance_angle,
                        .angle_z = glm_rad(10),
                    };
                },
            .place_advance_delta_pos =
                calc_place_advance_delta_pos_of_curve(curve_radius_1, glm_rad(15), false),
            .place_advance_delta_angle = glm_rad(15),
        },
    },
    {
        BC_CURVE_LEFT_2,
        {
            .length = curve_radius_2 * glm_rad(15),
            .calc_transform_fn =
                [](float_t length) {
                    constexpr float_t k_len{ curve_radius_2 * glm_rad(15) };

                    float_t t = length / k_len;
                    float_t const advance_angle{ glm_lerp(glm_rad(0), glm_rad(15), t) };
                    return Rail_position_transform{
                        .position = calc_place_advance_delta_pos_of_curve(curve_radius_2,
                                                                          advance_angle,
                                                                          false),
                        .angle_y = advance_angle,
                        .angle_z = glm_rad(10),
                    };
                },
            .place_advance_delta_pos =
                calc_place_advance_delta_pos_of_curve(curve_radius_2, glm_rad(15), false),
            .place_advance_delta_angle = glm_rad(15),
        },
    },
    {
        BC_CURVE_LEFT_3,
        {
            .length = curve_radius_3 * glm_rad(15),
            .calc_transform_fn =
                [](float_t length) {
                    constexpr float_t k_len{ curve_radius_3 * glm_rad(15) };

                    float_t t = length / k_len;
                    float_t const advance_angle{ glm_lerp(glm_rad(0), glm_rad(15), t) };
                    return Rail_position_transform{
                        .position = calc_place_advance_delta_pos_of_curve(curve_radius_3,
                                                                          advance_angle,
                                                                          false),
                        .angle_y = advance_angle,
                        .angle_z = glm_rad(10),
                    };
                },
            .place_advance_delta_pos =
                calc_place_advance_delta_pos_of_curve(curve_radius_3, glm_rad(15), false),
            .place_advance_delta_angle = glm_rad(15),
        },
    },
    {
        BC_CURVE_LEFT_4,
        {
            .length = curve_radius_4 * glm_rad(15),
            .calc_transform_fn =
                [](float_t length) {
                    constexpr float_t k_len{ curve_radius_4 * glm_rad(15) };

                    float_t t = length / k_len;
                    float_t const advance_angle{ glm_lerp(glm_rad(0), glm_rad(15), t) };
                    return Rail_position_transform{
                        .position = calc_place_advance_delta_pos_of_curve(curve_radius_4,
                                                                          advance_angle,
                                                                          false),
                        .angle_y = advance_angle,
                        .angle_z = glm_rad(10),
                    };
                },
            .place_advance_delta_pos =
                calc_place_advance_delta_pos_of_curve(curve_radius_4, glm_rad(15), false),
            .place_advance_delta_angle = glm_rad(15),
        },
    },
    {
        BC_CURVE_RIGHT_1,
        {
            .length = curve_radius_1 * glm_rad(15),
            .calc_transform_fn =
                [](float_t length) {
                    constexpr float_t k_len{ curve_radius_1 * glm_rad(15) };

                    float_t t = length / k_len;
                    float_t const advance_angle{ glm_lerp(glm_rad(0), glm_rad(15), t) };
                    return Rail_position_transform{
                        .position = calc_place_advance_delta_pos_of_curve(curve_radius_1,
                                                                          advance_angle,
                                                                          true),
                        .angle_y = -advance_angle,
                        .angle_z = glm_rad(-10),
                    };
                },
            .place_advance_delta_pos =
                calc_place_advance_delta_pos_of_curve(curve_radius_1, glm_rad(15), true),
            .place_advance_delta_angle = glm_rad(-15),
        },
    },
    {
        BC_CURVE_RIGHT_2,
        {
            .length = curve_radius_2 * glm_rad(15),
            .calc_transform_fn =
                [](float_t length) {
                    constexpr float_t k_len{ curve_radius_2 * glm_rad(15) };

                    float_t t = length / k_len;
                    float_t const advance_angle{ glm_lerp(glm_rad(0), glm_rad(15), t) };
                    return Rail_position_transform{
                        .position = calc_place_advance_delta_pos_of_curve(curve_radius_2,
                                                                          advance_angle,
                                                                          true),
                        .angle_y = -advance_angle,
                        .angle_z = glm_rad(-10),
                    };
                },
            .place_advance_delta_pos =
                calc_place_advance_delta_pos_of_curve(curve_radius_2, glm_rad(15), true),
            .place_advance_delta_angle = glm_rad(-15),
        },
    },
    {
        BC_CURVE_RIGHT_3,
        {
            .length = curve_radius_3 * glm_rad(15),
            .calc_transform_fn =
                [](float_t length) {
                    constexpr float_t k_len{ curve_radius_3 * glm_rad(15) };

                    float_t t = length / k_len;
                    float_t const advance_angle{ glm_lerp(glm_rad(0), glm_rad(15), t) };
                    return Rail_position_transform{
                        .position = calc_place_advance_delta_pos_of_curve(curve_radius_3,
                                                                          advance_angle,
                                                                          true),
                        .angle_y = -advance_angle,
                        .angle_z = glm_rad(-10),
                    };
                },
            .place_advance_delta_pos =
                calc_place_advance_delta_pos_of_curve(curve_radius_3, glm_rad(15), true),
            .place_advance_delta_angle = glm_rad(-15),
        },
    },
    {
        BC_CURVE_RIGHT_4,
        {
            .length = curve_radius_4 * glm_rad(15),
            .calc_transform_fn =
                [](float_t length) {  // @COPYPASTA
                    constexpr float_t k_len{ curve_radius_4 * glm_rad(15) };

                    float_t t = length / k_len;
                    float_t const advance_angle{ glm_lerp(glm_rad(0), glm_rad(15), t) };
                    return Rail_position_transform{
                        .position = calc_place_advance_delta_pos_of_curve(curve_radius_4,
                                                                          advance_angle,
                                                                          true),
                        .angle_y = -advance_angle,
                        .angle_z = glm_rad(-10),
                    };
                },
            .place_advance_delta_pos =
                calc_place_advance_delta_pos_of_curve(curve_radius_4, glm_rad(15), true),
            .place_advance_delta_angle = glm_rad(-15),
        },
    },
    {
        BC_SLOPECHANGE_UP,
        {
            .length = k_bz_slopechange_up.calc_length(),
            .calc_transform_fn =
                [](float_t length) {
                    static float_t const k_len{ k_bz_slopechange_up.calc_length() };

                    float_t t = length / k_len;

                    vec3s pos0 = k_bz_slopechange_up.calc_position_on_curve(t);
                    vec3s pos1 = k_bz_slopechange_up.calc_position_on_curve(t + 0.001f);
                    
                    float_t ang = atan2f(pos1.z - pos0.z, pos1.y - pos0.y);

                    return Rail_position_transform{
                        .position = pos0,
                        .angle_x = ang,
                    };
                },
            .place_advance_delta_pos = { 0, 1, -20 },
            .place_advance_delta_angle = glm_rad(0),
        },
    },
    {
        BC_SLOPECHANGE_UP_RETURN,
        {
            .length = k_bz_slopechange_up_return.calc_length(),
            .calc_transform_fn =
                [](float_t length) {
                    static float_t const k_len{ k_bz_slopechange_up_return.calc_length() };

                    float_t t = length / k_len;

                    vec3s pos0 = k_bz_slopechange_up_return.calc_position_on_curve(t);
                    vec3s pos1 = k_bz_slopechange_up_return.calc_position_on_curve(t + 0.001f);
                    
                    float_t ang = atan2f(pos1.z - pos0.z, pos1.y - pos0.y);

                    return Rail_position_transform{
                        .position = pos0,
                        .angle_x = ang,
                    };
                },
            .place_advance_delta_pos = { 0, 1, -20 },
            .place_advance_delta_angle = glm_rad(0),
        },
    },
    {
        BC_SLOPECHANGE_DOWN,
        {
            .length = k_bz_slopechange_down.calc_length(),
            .calc_transform_fn =
                [](float_t length) {
                    static float_t const k_len{ k_bz_slopechange_down.calc_length() };

                    float_t t = length / k_len;

                    vec3s pos0 = k_bz_slopechange_down.calc_position_on_curve(t);
                    vec3s pos1 = k_bz_slopechange_down.calc_position_on_curve(t + 0.001f);

                    float_t ang = atan2f(pos1.z - pos0.z, pos1.y - pos0.y);

                    return Rail_position_transform{
                        .position = pos0,
                        .angle_x = ang,
                    };
                },
            .place_advance_delta_pos = { 0, -1, -20 },
            .place_advance_delta_angle = glm_rad(0),
        },
    },
    {
        BC_SLOPECHANGE_DOWN_RETURN,
        {
            .length = k_bz_slopechange_down_return.calc_length(),
            .calc_transform_fn =
                [](float_t length) {  // @COPYPASTA
                    static float_t const k_len{ k_bz_slopechange_down_return.calc_length() };

                    float_t t = length / k_len;

                    vec3s pos0 = k_bz_slopechange_down_return.calc_position_on_curve(t);
                    vec3s pos1 = k_bz_slopechange_down_return.calc_position_on_curve(t + 0.001f);

                    float_t ang = atan2f(pos1.z - pos0.z, pos1.y - pos0.y);

                    return Rail_position_transform{
                        .position = pos0,
                        .angle_x = ang,
                    };
                },
            .place_advance_delta_pos = { 0, -1, -20 },
            .place_advance_delta_angle = glm_rad(0),
        },
    },
};

}  // namespace component
}  // namespace BT
