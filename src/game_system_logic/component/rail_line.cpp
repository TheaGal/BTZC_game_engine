#include "rail_line.h"

#include "btglm.h"
#include "btjson.h"
#include "btuuid.h"
#include "entt/entity/fwd.hpp"

#include <cassert>
#include <cmath>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>


namespace BT
{
namespace component
{

auto Rail_line::Build_code_info::calculate_transform(vec3 place_pos,
                                                     float_t place_angle,
                                                     float_t length) -> Rail_position_transform
{
    assert(false);
}

void Rail_line::Build_code_info::advance_place_transform(vec3& place_pos,
                                                         float_t& place_angle,
                                                         bool forward)
{
    assert(false);
}

std::unordered_map<Rail_line::Build_code, Rail_line::Build_code_info>
Rail_line::s_build_code_to_info_map{
    { BC_STRAIGHT, {} },  // @TODO: figure out how to get the info into here.
    { BC_STRAIGHT_UPHILL, {} },
    { BC_STRAIGHT_DOWNHILL, {} },
    { BC_STRAIGHT_ROLL_LEFT, {} },
    { BC_STRAIGHT_ROLL_LEFT_RETURN, {} },
    { BC_STRAIGHT_ROLL_RIGHT, {} },
    { BC_STRAIGHT_ROLL_RIGHT_RETURN, {} },
    { BC_CURVE_LEFT_1, {} },
    { BC_CURVE_LEFT_2, {} },
    { BC_CURVE_LEFT_3, {} },
    { BC_CURVE_LEFT_4, {} },
    { BC_CURVE_RIGHT_1, {} },
    { BC_CURVE_RIGHT_2, {} },
    { BC_CURVE_RIGHT_3, {} },
    { BC_CURVE_RIGHT_4, {} },
    { BC_SLOPECHANGE_UP, {} },
    { BC_SLOPECHANGE_UP_RETURN, {} },
    { BC_SLOPECHANGE_DOWN, {} },
    { BC_SLOPECHANGE_DOWN_RETURN, {} },
};

}  // namespace component
}  // namespace BT
