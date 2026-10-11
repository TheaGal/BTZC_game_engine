#include "test_runner.h"

#include "btglm.h"
#include "btglm_structs.test.h"
#include "btlogger.h"
#include "test_types.h"
#include "undefined_behavior.test.h"


bool BT::tests::run_all_tests()
{
    Test_run_result_stats trrs;

    BT_INFO("-= RUNNING ALL TESTS =-=-=-=-=-=-=-=-=-=-=-=-");

    #define RUN_TEST(__test_func) trrs.run_test(__test_func, #__test_func)

    RUN_TEST(test_btglm_vec2s_copies_and_moves);
    RUN_TEST(test_btglm_vec3s_copies_and_moves);
    RUN_TEST(test_btglm_vec4s_copies_and_moves);
    RUN_TEST(test_btglm_mat3s_copies_and_moves);
    RUN_TEST(test_btglm_mat4s_copies_and_moves);
    RUN_TEST(test_btglm_vec2s_in_vector);
    RUN_TEST(test_btglm_vec3s_in_vector);
    RUN_TEST(test_btglm_vec4s_in_vector);
    RUN_TEST(test_btglm_mat3s_in_vector);
    RUN_TEST(test_btglm_mat4s_in_vector);
    RUN_TEST(test_ub_problematic_struct);

    bool const grand_pass{ trrs.aggregate_results_by_type[TEST_RESULT_FAILED] == 0 };

    BT_INFOF(
        "\n"
        "RESULTS\n"
        "  passed:       %u\n"
        "  failed:       %u\n"
        "  ignore:       %u\n"
        "  run time:     %.3fms\n"
        "  GRAND RESULT: %s\n"
        "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n",
        trrs.aggregate_results_by_type[TEST_RESULT_PASSED],
        trrs.aggregate_results_by_type[TEST_RESULT_FAILED],
        trrs.aggregate_results_by_type[TEST_RESULT_IGNORE],
        trrs.aggregate_time * 1000,
        grand_pass ? "PASS" : "FAIL");

    return grand_pass;
}
