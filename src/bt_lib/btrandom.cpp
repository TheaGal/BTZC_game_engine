#include "btrandom.h"
#include "btdatecheck.h"

#include <cmath>
#include <random>


float_t BT::random::fast_float_01_exclusive()
{
    date_deadline(2027, 1, 1);  // @TODO: move this to a TAS-friendly rng system!! (for only gameplay tho)
    static thread_local std::mt19937 generator;
    std::uniform_real_distribution<float_t> distribution(0, 1);
    return distribution(generator);
}
