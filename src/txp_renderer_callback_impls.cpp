// clang-format off
#include "txp_renderer_public.h"
// clang-format on

#include "btrandom.h"

#include <cmath>


float_t TXP::callback::calc_random_value_01_exclusive_callback()
{
    return BT::random::fast_float_01_exclusive();
}
