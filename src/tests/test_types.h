#pragma once

#include "btlogger.h"
#include "timer/timer.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <sstream>
#include <stdexcept>


namespace BT
{
namespace tests
{

void check_eq(auto a, auto b)
{
    if (a != b)
    {
        std::ostringstream ss;
        ss << a << " and " << b << " are supposed to be equal.";
        throw std::runtime_error(ss.str());
    }
}

void check_neq(auto a, auto b)
{
    if (a == b)
    {
        std::ostringstream ss;
        ss << a << " and " << b << " are supposed to be not equal.";
        throw std::runtime_error(ss.str());
    }
}

enum Test_result : uint32_t
{
    TEST_RESULT_PASSED = 0,
    TEST_RESULT_FAILED,
    TEST_RESULT_IGNORE,

    NUM_TEST_RESULT_TYPES
};

constexpr char const* k_test_result_strs[]{
    "PASSED",
    "FAILED",
    "IGNORE",
};

struct Test_run_result_stats
{
    std::array<uint32_t, NUM_TEST_RESULT_TYPES> aggregate_results_by_type{};
    float_t aggregate_time{ 0 };

    using Test_fn_t = Test_result(*)();
    void run_test(Test_fn_t const test, char const* const test_name)
    {
        Timer run_timer;
        run_timer.start_timer();

        Test_result result;
        try
        {
            result = test();
        }
        catch (std::exception& ex)
        {
            result = TEST_RESULT_FAILED;
            BT_ERRORF("    ERROR: %s", ex.what());
        }
        catch (...)
        {
            result = TEST_RESULT_FAILED;
            assert(false);
        }

        aggregate_results_by_type[result]++;

        float_t const run_time{ run_timer.calc_delta_time() };
        aggregate_time += run_time;

        BT_INFOF("  [%s] :: %s (%.3fms)", k_test_result_strs[result], test_name, run_time * 1000);
    }
};

} // namespace tests
} // namespace BT
