// @NOTE: this moreso belongs as a game tool, so figure out if you want the game tools to be a part
//        of the game engine itself or not!  -Thea 2026/08/01
#pragma once

#include "btjson.h"

#include <string>


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

    /// Flag to automatically create the render object when component is added.
    bool created_render_object{ false };
};

}  // namespace component
}  // namespace BT
