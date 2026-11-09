#pragma once

#include "btjson.h"

#include <cmath>
#include <limits>


namespace BT
{
namespace component
{

/// Tag for a perilous signal.
struct Perilous_signal_receiver
{
    float_t time_since_last_perilous_msg{ std::numeric_limits<float_t>::max() };

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Perilous_signal_receiver,
        time_since_last_perilous_msg
    );
};

}  // namespace component
}  // namespace BT
