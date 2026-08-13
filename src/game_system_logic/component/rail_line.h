// @NOTE: this moreso belongs as a game tool, so figure out if you want the game tools to be a part
//        of the game engine itself or not!  -Thea 2026/08/01
#pragma once

#include "btglm.h"
#include "btjson.h"
#include "btuuid.h"
#include "entt/entity/fwd.hpp"

#include <cmath>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>


namespace BT
{
namespace component
{

/// Rail line component for creating a path and rail line curve.
struct Rail_line
{
    std::string construction_code;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Rail_line,
        construction_code
    );

    /// This is used by the systems to make sure that the construction is up to date.
    std::string prev_construction_code;

    /// Cached length from building the rail line parts.
    float_t built_length;

    /// Flag to automatically create the render object when component is added.
    bool created_render_object{ false };

    /// List of entities created from the construction code.
    std::vector<entt::entity> created_entities;

    /// Build codes.
    enum Build_code : int32_t
    {
        BC_STRAIGHT = 0,
        BC_STRAIGHT_UPHILL,
        BC_STRAIGHT_DOWNHILL,
        BC_STRAIGHT_ROLL_LEFT,
        BC_STRAIGHT_ROLL_LEFT_RETURN,
        BC_STRAIGHT_ROLL_RIGHT,
        BC_STRAIGHT_ROLL_RIGHT_RETURN,
        BC_CURVE_LEFT_1,
        BC_CURVE_LEFT_2,
        BC_CURVE_LEFT_3,
        BC_CURVE_LEFT_4,
        BC_CURVE_RIGHT_1,
        BC_CURVE_RIGHT_2,
        BC_CURVE_RIGHT_3,
        BC_CURVE_RIGHT_4,
        BC_SLOPECHANGE_UP,
        BC_SLOPECHANGE_UP_RETURN,
        BC_SLOPECHANGE_DOWN,
        BC_SLOPECHANGE_DOWN_RETURN,

        NUM_BUILD_CODES
    };

    /// All codes to construct the track.
    std::vector<Build_code> built_ctor_code;

    /// Build code information.
    struct Build_code_info
    {
        float_t length;

        struct Rail_position_transform
        {
            vec3s position;
            float_t angle_y;
            float_t angle_x;
        };
        std::function<Rail_position_transform(float_t)> calc_transform_fn;

        Rail_position_transform calculate_transform(vec3 place_pos,
                                                    float_t place_angle,
                                                    float_t length);

        vec3s place_advance_delta_pos;
        float_t place_advance_delta_angle;

        void advance_place_transform(vec3& place_pos, float_t& place_angle, bool forward);
    };
    static std::unordered_map<Build_code, Build_code_info> s_build_code_to_info_map;
};

/// Rail line riding component.
struct Rail_line_rider
{
    UUID riding_line_uuid;

    double_t line_position;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(
        Rail_line_rider,
        riding_line_uuid,
        line_position
    );
};

}  // namespace component
}  // namespace BT
