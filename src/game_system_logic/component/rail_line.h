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
    std::string built_ctor_code;

    /// Cached length from building the rail line parts.
    float_t built_length;

    /// Flag to automatically create the render object when component is added.
    bool created_render_object{ false };

    /// List of entities created from the construction code.
    std::vector<entt::entity> created_entities;

    /// Build codes and information.
    struct Build_code_info
    {
        float_t length;
        std::function<vec3s(float_t)> calc_position_fn;

        // @TODO: add advance angle and advance position info to here too.
    };
    static std::unordered_map<char, Build_code_info> build_code_to_info_map;
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
