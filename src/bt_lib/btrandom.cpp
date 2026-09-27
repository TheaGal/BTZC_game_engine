#include "btrandom.h"

#include <cmath>
#include <random>


float_t BT::random::fast_float_01_exclusive()
{
    static thread_local std::mt19937 generator;
    std::uniform_real_distribution<float_t> distribution(0, 1);
    return distribution(generator);
}
