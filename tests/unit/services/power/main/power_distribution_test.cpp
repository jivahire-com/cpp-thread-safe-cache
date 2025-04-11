//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_distribution_test.cpp
 * Tests for power distribution APIs
 */

/*------------- Includes -----------------*/
#include "power_test.h" // for POWER_TEST

#include <cstddef> // for NULL

extern "C" {
// clang-format off
#include <kng_soc_constants.h> // for NUM_AP_CORES_PER_DIE, NUM_CPU_TILES
// clang-format on
#include "corebits.h"   // for corebits_t
#include "power_test.h" // for POWER_TEST, UNUSED

#include <CMockaWrapper.h>  // for expect_any_always, CmockaWrapperTest
#include <assert.h>         // for assert
#include <cstddef>          // for NULL, size_t
#include <fpfw_status.h>    // for FPFW_STATUS_NULL_POINTER, FPFW_STATUS...
#include <mock_bug_check.h> // for __wrap_crash_dump_bug_check
#include <power_distribution_i.h>
#include <power_i.h>           // for power_fuses_get_dts_coeff, power_fuse...
#include <power_log.h>         // for power_log_init
#include <power_loops_i.h>     // for power_fuses_get_dts_coeff, power_fuse...
#include <power_runconfig.h>   // for power_fuse_data_t, dts_coeff_t, power...
#include <power_runconfig_i.h> // for power_fuses_read, power_fuses_get_cur...
#include <scf_power.h>         // for CSTATE enums
#include <stdint.h>            // for int8_t, uint64_t, uint8_t, uintptr_t
#include <string.h>            // for memcpy

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-- Declarations (Statics and globals) --*/

power_runconfig_t s_runconfig;
power_ctrl_loop_detail_t s_ctrl_loop_detail;

#define MAX_PERF                       0
#define MAX_STEPS2                     2
#define ARBITRARY_LIMIT5               5
#define TEST_NOMINAL_PERF              20
#define ARBITRARY_LIMIT5_BELOW_NOMINAL (TEST_NOMINAL_PERF + 5)
#define DEFAULT_CORE                   26 // just an arbitrary core number

/*-------- Function Prototypes -----------*/

/*------------- Functions ----------------*/
//
// Mocks
//
uint64_t __wrap_power_timer_get_counter(void)
{
    //! just a dummy value to satisfy the test
    //! this is not used in the test, but the function is called in the code
    return 0x123456789ABCDEFULL;
}

// end of extern C after mock functions ...
} // extern "C"

static bool OneTimeInitFlag = false;

static int distribution_setup(void** state)
{
    UNUSED(state);

    // specific for distribution tests
    memset(&s_ctrl_loop_detail, 0, sizeof(s_ctrl_loop_detail));
    for (unsigned int core = 0; core < NUM_AP_CORES_PER_DIE; ++core)
    {
        s_ctrl_loop_detail.cores.core[core].current_cstate = CSTATE_0;
        s_ctrl_loop_detail.cores.core[core].desired_pstate = MAX_PERF;
        s_ctrl_loop_detail.cores.core[core].current_plimit = MAX_PERF;
    }
    s_runconfig.knobs.allowed_plimit_minimum = MAX_PERF;
    s_runconfig.knobs.allowed_plimit_maximum = MAX_PLIMIT;
    s_runconfig.knobs.allow_plimit_below_nominal = true;
    s_runconfig.knobs.c3_cores_limit_to_nominal = false;
    s_runconfig.knobs.c2_cores_limit_to_nominal = false;
    s_runconfig.knobs.intervals_to_lower_plimit = 0;
    s_runconfig.knobs.max_plimit_step_size_down = MAX_PLIMIT;
    s_runconfig.knobs.max_plimit_step_size_up = MAX_PLIMIT;
    s_runconfig.knobs.force_pstate = NUM_PSTATES;

    s_runconfig.derived.vfts[0].min_plimit = 0;
    s_runconfig.derived.pnominal = TEST_NOMINAL_PERF;

    //! Sufficient to initialize power log once for all tests
    if (!OneTimeInitFlag)
    {
        expect_function_call(__wrap_crash_dump_register_address32);
        power_log_init();
        OneTimeInitFlag = true;
    }

    return 0;
}

static int distribution_teardown(void** state)
{
    UNUSED(state);
    // any specific teardown

    return 0;
}

void get_minimum_plimit_assert_expectations()
{
    expect_value_count(__wrap_FpFwAssert, expression, true, 3);
}

POWER_TEST(distribution_get_minimum_plimit__allowed_plimit_minimum, distribution_setup, distribution_teardown)
{
    get_minimum_plimit_assert_expectations();

    s_runconfig.knobs.allowed_plimit_minimum = ARBITRARY_LIMIT5; // just choosing an arbitrary limit > 0
    uint8_t minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
    assert_int_equal(minp, ARBITRARY_LIMIT5);
}

POWER_TEST(distribution_get_minimum_plimit__vft_plimit_minimum, distribution_setup, distribution_teardown)
{
    get_minimum_plimit_assert_expectations();

    s_runconfig.knobs.allowed_plimit_minimum = ARBITRARY_LIMIT5; // just choosing an arbitrary limit > 0
    s_runconfig.derived.vfts[0].min_plimit = TEST_NOMINAL_PERF;  // VF curve only supports up to nominal
    uint8_t minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
    assert_int_equal(minp, TEST_NOMINAL_PERF);
}

POWER_TEST(distribution_get_minimum_plimit__c3_limit_to_nominal, distribution_setup, distribution_teardown)
{
    s_runconfig.knobs.c3_cores_limit_to_nominal = true;

    get_minimum_plimit_assert_expectations();
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].current_cstate = CSTATE_2;
    uint8_t minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
    assert_int_equal(minp, MAX_PERF);

    get_minimum_plimit_assert_expectations();
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].current_cstate = CSTATE_3;
    minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
    assert_int_equal(minp, TEST_NOMINAL_PERF);

    get_minimum_plimit_assert_expectations();
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].current_cstate = CSTATE_4;
    minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
    assert_int_equal(minp, MAX_PERF);
}

POWER_TEST(distribution_get_minimum_plimit__c2_limit_to_nominal, distribution_setup, distribution_teardown)
{
    s_runconfig.knobs.c2_cores_limit_to_nominal = true;

    get_minimum_plimit_assert_expectations();
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].current_cstate = CSTATE_2;
    uint8_t minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
    assert_int_equal(minp, TEST_NOMINAL_PERF);

    get_minimum_plimit_assert_expectations();
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].current_cstate = CSTATE_3;
    minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
    assert_int_equal(minp, MAX_PERF);

    get_minimum_plimit_assert_expectations();
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].current_cstate = CSTATE_4;
    minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
    assert_int_equal(minp, MAX_PERF);
}

POWER_TEST(distribution_get_minimum_plimit__c4_limit_to_nominal, distribution_setup, distribution_teardown)
{
    s_runconfig.knobs.c4_cores_limit_to_nominal = true;

    get_minimum_plimit_assert_expectations();
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].current_cstate = CSTATE_2;
    uint8_t minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
    assert_int_equal(minp, MAX_PERF);

    get_minimum_plimit_assert_expectations();
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].current_cstate = CSTATE_3;
    minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
    assert_int_equal(minp, MAX_PERF);

    get_minimum_plimit_assert_expectations();
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].current_cstate = CSTATE_4;
    minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
    assert_int_equal(minp, TEST_NOMINAL_PERF);
}

POWER_TEST(distribution_get_minimum_plimit__os_desired, distribution_setup, distribution_teardown)
{
    get_minimum_plimit_assert_expectations();

    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].desired_pstate = ARBITRARY_LIMIT5_BELOW_NOMINAL;
    uint8_t minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
    assert_int_equal(minp, ARBITRARY_LIMIT5_BELOW_NOMINAL);
}
POWER_TEST(distribution_get_minimum_plimit__os_desired_limit_to_nominal, distribution_setup, distribution_teardown)
{
    get_minimum_plimit_assert_expectations();

    // confirm that if knob set to disallow plimit request below nominal that OS
    // desired below nominal returns nominal
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].desired_pstate = ARBITRARY_LIMIT5_BELOW_NOMINAL;
    s_runconfig.knobs.allow_plimit_below_nominal = false;
    uint8_t minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
    assert_int_equal(minp, TEST_NOMINAL_PERF);
}

POWER_TEST(distribution_get_minimum_plimit__max_steps_up, distribution_setup, distribution_teardown)
{
    get_minimum_plimit_assert_expectations();

    // setup sets desired to max_perf, 0
    // set current to 5
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].current_plimit = ARBITRARY_LIMIT5;
    // set max_steps up to limit the change
    s_runconfig.knobs.max_plimit_step_size_up = MAX_STEPS2;
    uint8_t minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
    assert_int_equal(minp, ARBITRARY_LIMIT5 - MAX_STEPS2);
}

POWER_TEST(distribution_get_minimum_plimit__max_steps_up_at_boundary, distribution_setup, distribution_teardown)
{
    get_minimum_plimit_assert_expectations();

    // setup sets desired to max_perf, 0
    // set current to 5
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].current_plimit = MAX_PERF + MAX_STEPS2 - 1;
    // set max_steps up to limit the change
    s_runconfig.knobs.max_plimit_step_size_up = MAX_STEPS2;
    uint8_t minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
    assert_int_equal(minp, MAX_PERF);
}

POWER_TEST(distribution_get_minimum_plimit__max_steps_down, distribution_setup, distribution_teardown)
{
    get_minimum_plimit_assert_expectations();

    // set desired to MAX (lowest perf)
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].desired_pstate = MAX_PLIMIT;
    // set current to 5
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].current_plimit = ARBITRARY_LIMIT5;
    // set max_steps down to limit the change
    s_runconfig.knobs.max_plimit_step_size_down = MAX_STEPS2;
    uint8_t minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
    assert_int_equal(minp, ARBITRARY_LIMIT5 + MAX_STEPS2);
}

POWER_TEST(distribution_get_minimum_plimit__max_steps_down_at_boundary, distribution_setup, distribution_teardown)
{
    get_minimum_plimit_assert_expectations();

    // set desired to MAX (lowest perf)
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].desired_pstate = MAX_PLIMIT;
    // set current to less than number of max steps away
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].current_plimit = MAX_PLIMIT - MAX_STEPS2 + 1;
    // set max_steps up to limit the change
    s_runconfig.knobs.max_plimit_step_size_down = MAX_STEPS2;
    uint8_t minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
    assert_int_equal(minp, MAX_PLIMIT);
}

POWER_TEST(distribution_get_minimum_plimit__intervals_to_lower_not_met, distribution_setup, distribution_teardown)
{
    get_minimum_plimit_assert_expectations();

    // set desired to MAX (lowest perf)
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].desired_pstate = MAX_PLIMIT;
    // set current to highes perf
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].current_plimit = MAX_PERF;
    s_runconfig.knobs.intervals_to_lower_plimit = 2;
    uint8_t minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
    assert_int_equal(minp, MAX_PERF);
}

POWER_TEST(distribution_get_minimum_plimit__intervals_to_lower_met, distribution_setup, distribution_teardown)
{
    // set current to highest perf
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].current_plimit = MAX_PERF;
    s_runconfig.knobs.intervals_to_lower_plimit = 2;
    uint8_t minp;
    // set desired to MAX (lowest perf)
    // - lower request to engage intervals to lower
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].desired_pstate = MAX_PLIMIT;
    for (int i = 0; i < s_runconfig.knobs.intervals_to_lower_plimit; ++i)
    {
        get_minimum_plimit_assert_expectations();
        minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
        assert_int_equal(minp, MAX_PERF);
    }
    get_minimum_plimit_assert_expectations();
    minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
    assert_int_equal(minp, MAX_PLIMIT);
}

POWER_TEST(distribution_get_minimum_plimit__intervals_to_lower_with_reset, distribution_setup, distribution_teardown)
{
    // set current to highes perf
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].current_plimit = MAX_PERF;
    s_runconfig.knobs.intervals_to_lower_plimit = 2;
    uint8_t minp;
    // set desired to MAX (lowest perf)
    // - lower request to engage intervals to lower
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].desired_pstate = MAX_PLIMIT;
    for (int i = 0; i < s_runconfig.knobs.intervals_to_lower_plimit; ++i)
    {
        get_minimum_plimit_assert_expectations();
        minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
        assert_int_equal(minp, MAX_PERF);
    }

    // reset count by pushing pstate request to higher perf
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].desired_pstate = MAX_PERF;
    get_minimum_plimit_assert_expectations();
    minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
    assert_int_equal(minp, MAX_PERF);

    // lower again
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].desired_pstate = MAX_PLIMIT;
    for (int i = 0; i < s_runconfig.knobs.intervals_to_lower_plimit; ++i)
    {
        get_minimum_plimit_assert_expectations();
        minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
        assert_int_equal(minp, MAX_PERF);
    }
    get_minimum_plimit_assert_expectations();
    minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
    assert_int_equal(minp, MAX_PLIMIT);
}

POWER_TEST(distribution_get_minimum_plimit__intervals_to_lower_with_increase_at_last, distribution_setup, distribution_teardown)
{
    s_runconfig.knobs.intervals_to_lower_plimit = 2;
    uint8_t minp;
    // set desired to MAX (lowest perf)
    // - lower request to engage intervals to lower
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].desired_pstate = MAX_PLIMIT;
    for (int i = 0; i < s_runconfig.knobs.intervals_to_lower_plimit; ++i)
    {
        get_minimum_plimit_assert_expectations();
        minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
        assert_int_equal(minp, MAX_PERF);
    }

    // raise perf at last interval
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].desired_pstate = MAX_PLIMIT - 1;
    get_minimum_plimit_assert_expectations();
    minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
    assert_int_equal(minp, MAX_PLIMIT - 1);
}

POWER_TEST(distribution_get_minimum_plimit__intervals_to_lower_with_decrease_at_last, distribution_setup, distribution_teardown)
{
    s_runconfig.knobs.intervals_to_lower_plimit = 2;
    uint8_t minp;
    // set desired to MAX-1 (next to lowest perf)
    // - lower request to engage intervals to lower
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].desired_pstate = MAX_PLIMIT - 1;
    for (int i = 0; i < s_runconfig.knobs.intervals_to_lower_plimit; ++i)
    {
        get_minimum_plimit_assert_expectations();
        minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
        assert_int_equal(minp, MAX_PERF);
    }

    // lower perf at last interval
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].desired_pstate = MAX_PLIMIT;
    for (int i = 0; i < s_runconfig.knobs.intervals_to_lower_plimit; ++i)
    {
        get_minimum_plimit_assert_expectations();
        minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
        assert_int_equal(minp, MAX_PLIMIT - 1);
    }
    // after interval, the new lower perf should kick in
    get_minimum_plimit_assert_expectations();
    minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
    assert_int_equal(minp, MAX_PLIMIT);
}

POWER_TEST(distribution_get_minimum_plimit__intervals_to_lower_with_increase_after_first, distribution_setup, distribution_teardown)
{
    s_runconfig.knobs.intervals_to_lower_plimit = 2;
    uint8_t minp;
    // set desired to MAX (lowest perf)
    // - lower request to engage intervals to lower
    s_ctrl_loop_detail.cores.core[DEFAULT_CORE].desired_pstate = MAX_PLIMIT;
    for (int i = 0; i < s_runconfig.knobs.intervals_to_lower_plimit; ++i)
    {
        get_minimum_plimit_assert_expectations();
        minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
        // raise perf after first interval
        s_ctrl_loop_detail.cores.core[DEFAULT_CORE].desired_pstate = MAX_PLIMIT - 1;
        assert_int_equal(minp, MAX_PERF);
    }

    get_minimum_plimit_assert_expectations();
    minp = power_distribution_get_minimum_plimit(&s_runconfig, &s_ctrl_loop_detail.cores, DEFAULT_CORE);
    assert_int_equal(minp, MAX_PLIMIT - 1);
}

#define INVALID_PERF (MAX_PERF - 1)
// nothing particularly special, but 4 cores each in a different set of 32
#define NUM_D_CORES 4
#define FORCED_CORE 2
static const int core_numbers[NUM_D_CORES] = {12, 32, 37, 67};

static int distribution_dist_setup(void** state)
{
    distribution_setup(state);
    // anything specific to distribute here
    // clear any throttle priority detail
    memset(s_ctrl_loop_detail.throttle_priority, 0, sizeof(s_ctrl_loop_detail.throttle_priority));
    memset(s_ctrl_loop_detail.boost_priority, 0, sizeof(s_ctrl_loop_detail.boost_priority));
    // valid cores is an input
    corebits_clear(&s_runconfig.fuses.valid_cores);
    // plimits pending is an output
    corebits_clear(&s_ctrl_loop_detail.plimits_pending);

    s_ctrl_loop_detail.curr_resources = PLIMIT_TO_RESOURCES(TEST_NOMINAL_PERF) * NUM_D_CORES;
    for (int i = 0; i < NUM_D_CORES; ++i)
    {
        s_ctrl_loop_detail.cores.core[core_numbers[i]].min_plimit = MAX_PERF;
        // setup invalid values to ensure no matches
        s_ctrl_loop_detail.cores.core[core_numbers[i]].current_plimit = INVALID_PERF;
        // selected plimit is an output
        s_ctrl_loop_detail.cores.core[core_numbers[i]].selected_plimit = INVALID_PERF;
    }
    // clear selection counts
    for (int pri = 0; pri < VM_THROT_COUNT; ++pri)
    {
        for (int pstate = 0; pstate < NUM_PSTATES; ++pstate)
        {
            s_ctrl_loop_detail.plimit.selections[pri].acc[pstate] = 0;
        }
    }
    // clear priority counts
    memset(&s_ctrl_loop_detail.local.pri_counts, 0, sizeof(s_ctrl_loop_detail.local.pri_counts));
    return 0;
}

static int distribution_dist_teardown(void** state)
{
    // any specific teardown

    // default teardown
    distribution_teardown(state);
    return 0;
}

void distribute_assert_expectations()
{
    expect_value_count(__wrap_FpFwAssert, expression, true, 2);
}

static void complete_plimit_valid_data()
{
    const unsigned pnominal = s_runconfig.derived.pnominal;
    for (int i = 0; i < NUM_D_CORES; ++i)
    {
        const uint8_t min_plimit = s_ctrl_loop_detail.cores.core[core_numbers[i]].min_plimit;
        const uint8_t boost_pri = s_ctrl_loop_detail.cores.core[core_numbers[i]].current_boost_priority;

        if ((pnominal > 0) && (min_plimit < pnominal))
        {
            s_ctrl_loop_detail.local.pri_counts.required_for_boost[min_plimit][boost_pri]++;
        }
    }

    // complete core count data in the boost range
    for (unsigned int plimit_idx = 1; plimit_idx < pnominal; ++plimit_idx)
    {
        for (int boost_pri_index = 0; boost_pri_index < VM_BOOST_COUNT; ++boost_pri_index)
        {
            // if a plimit is valid in a higher perf, we'll automatically indicate
            // it is valid in a lower perf
            s_ctrl_loop_detail.local.pri_counts.required_for_boost[plimit_idx][boost_pri_index] +=
                s_ctrl_loop_detail.local.pri_counts.required_for_boost[plimit_idx - 1][boost_pri_index];
        }
    }
}

static void create_nominal_count_data(unsigned core, unsigned nominal, unsigned throt_pri, unsigned boost_pri)
{
    unsigned pnominal = s_runconfig.derived.pnominal;
    s_ctrl_loop_detail.cores.core[core].current_boost_priority = boost_pri;
    s_ctrl_loop_detail.cores.core[core].current_throt_priority = throt_pri;

    // nominal has to be pnominal or higher
    nominal = (nominal > pnominal) ? nominal : pnominal;

    s_ctrl_loop_detail.cores.core[core].current_base_pstate = nominal;

    s_ctrl_loop_detail.local.pri_counts.throt_pri_count[throt_pri]++;
    s_ctrl_loop_detail.local.pri_counts.required_nominal_for_throttle[nominal][throt_pri]++;
    s_ctrl_loop_detail.local.pri_counts.required_for_boost[nominal][boost_pri]++;
}

static void setup_single_priority_alt(unsigned core, unsigned nominal)
{
    for (unsigned idx = 0; idx < NUM_D_CORES; ++idx)
    {
        corebits_set_bit(&s_runconfig.fuses.valid_cores, core_numbers[idx]);
        corebits_set_bit(&s_ctrl_loop_detail.throttle_priority[0], core_numbers[idx]);
        corebits_set_bit(&s_ctrl_loop_detail.boost_priority[0], core_numbers[idx]);
        if (core == idx)
        {
            create_nominal_count_data(core_numbers[idx], nominal, 0, 0);
        }
        else
        {
            create_nominal_count_data(core_numbers[idx], TEST_NOMINAL_PERF, 0, 0);
        }
    }
    complete_plimit_valid_data();
}

static void setup_single_priority()
{
    setup_single_priority_alt(NUM_D_CORES, 0);
}

static void setup_multi_priority()
{
    for (int i = 0; i < NUM_D_CORES; ++i)
    {
        corebits_set_bit(&s_runconfig.fuses.valid_cores, core_numbers[i]);
        // one core in each priority level
        corebits_set_bit(&s_ctrl_loop_detail.throttle_priority[i], core_numbers[i]);
        corebits_set_bit(&s_ctrl_loop_detail.boost_priority[i], core_numbers[i]);
        create_nominal_count_data(core_numbers[i], TEST_NOMINAL_PERF, i, i);
    }
    complete_plimit_valid_data();
}

POWER_TEST(distribution_distribute_resources__resources_for_nominal, distribution_dist_setup, distribution_dist_teardown)
{

    // use the default setup to ensure all cores get nominal
    setup_single_priority();
    distribute_assert_expectations();
    power_distribution_distribute_resources(&s_runconfig, &s_ctrl_loop_detail);
    for (int i = 0; i < NUM_D_CORES; ++i)
    {
        assert_int_equal(s_ctrl_loop_detail.cores.core[core_numbers[i]].selected_plimit, TEST_NOMINAL_PERF);
        assert_true(corebits_is_bit_set(&s_ctrl_loop_detail.plimits_pending, core_numbers[i]));
    }
    assert_int_equal(s_ctrl_loop_detail.plimit.selections[0].acc[TEST_NOMINAL_PERF], 1);
    assert_false(s_ctrl_loop_detail.throttling);
}

POWER_TEST(distribution_distribute_resources__resources_for_nominal__lower_base, distribution_dist_setup, distribution_dist_teardown)
{
#define TEST_UPDATED_BASE_PERF (TEST_NOMINAL_PERF + 2)

    // use the default setup to enable one core with a lower base perf than nominal
    setup_single_priority_alt(NUM_D_CORES - 1, TEST_UPDATED_BASE_PERF);
    distribute_assert_expectations();
    uint32_t resource_difference =
        (PLIMIT_TO_RESOURCES(TEST_NOMINAL_PERF) - PLIMIT_TO_RESOURCES(TEST_UPDATED_BASE_PERF));
    s_ctrl_loop_detail.curr_resources -= resource_difference;

    power_distribution_distribute_resources(&s_runconfig, &s_ctrl_loop_detail);
    // expect the core with the lower base perf to get the lower plimit
    for (int i = 0; i < NUM_D_CORES; ++i)
    {
        if (i == (NUM_D_CORES - 1))
        {
            assert_int_equal(s_ctrl_loop_detail.cores.core[core_numbers[i]].selected_plimit, TEST_UPDATED_BASE_PERF);
        }
        else
        {
            assert_int_equal(s_ctrl_loop_detail.cores.core[core_numbers[i]].selected_plimit, TEST_NOMINAL_PERF);
        }
        assert_true(corebits_is_bit_set(&s_ctrl_loop_detail.plimits_pending, core_numbers[i]));
    }
    assert_int_equal(s_ctrl_loop_detail.plimit.selections[0].acc[TEST_NOMINAL_PERF], 1);
    // throttling should still be false
    assert_false(s_ctrl_loop_detail.throttling);
}

POWER_TEST(distribution_distribute_warmstart_resources, distribution_dist_setup, distribution_dist_teardown)
{

    // use the warmstart distribution
    distribute_assert_expectations();
    power_distribution_distribute_warmstart_resources(&s_runconfig, &s_ctrl_loop_detail);
    // verify selected plimit value and that plimits are marked pending
    for (int core_idx = 0; core_idx < NUM_AP_CORES_PER_DIE; ++core_idx)
    {
        assert_int_equal(s_ctrl_loop_detail.cores.core[core_idx].selected_plimit, MAX_PLIMIT - 1);
        assert_true(corebits_is_bit_set(&s_ctrl_loop_detail.plimits_pending, core_idx));
    }
}

POWER_TEST(distribution_distribute_resources__resources_for_nominal__pstate_forced, distribution_dist_setup, distribution_dist_teardown)
{

    // use the default setup to ensure all cores get nominal
    setup_single_priority();
    // set forced knob
    s_runconfig.knobs.force_pstate = TEST_NOMINAL_PERF;
    // execute function being tested
    distribute_assert_expectations();
    power_distribution_distribute_resources(&s_runconfig, &s_ctrl_loop_detail);
    for (int i = 0; i < NUM_D_CORES; ++i)
    {
        // plimit should not have been touched on forced core
        uint8_t expected_plimit = INVALID_PERF;
        bool expect_bit_set = false;
        assert_int_equal(s_ctrl_loop_detail.cores.core[core_numbers[i]].selected_plimit, expected_plimit);
        assert_int_equal(corebits_is_bit_set(&s_ctrl_loop_detail.plimits_pending, core_numbers[i]), expect_bit_set);
    }
    assert_false(s_ctrl_loop_detail.throttling);
}

POWER_TEST(distribution_distribute_resources__one_too_few_resources_for_nominal, distribution_dist_setup, distribution_dist_teardown)
{

    // use the default setup to ensure all cores get nominal, but subtract one
    // count from resource
    setup_single_priority();
    s_ctrl_loop_detail.curr_resources -= 1;
    distribute_assert_expectations();
    power_distribution_distribute_resources(&s_runconfig, &s_ctrl_loop_detail);
    // at the same priority, no cores will get nominal
    for (int i = 0; i < NUM_D_CORES; ++i)
    {
        assert_int_equal(s_ctrl_loop_detail.cores.core[core_numbers[i]].selected_plimit, TEST_NOMINAL_PERF + 1);
        assert_true(corebits_is_bit_set(&s_ctrl_loop_detail.plimits_pending, core_numbers[i]));
    }
    assert_int_equal(s_ctrl_loop_detail.plimit.selections[0].acc[TEST_NOMINAL_PERF + 1], 1); // one selection for nominal + 1
    assert_int_equal(s_ctrl_loop_detail.plimit.selections[0].acc[TEST_NOMINAL_PERF], 0); // no selections for nominal
    assert_true(s_ctrl_loop_detail.throttling);
}

POWER_TEST(distribution_distribute_resources__one_too_few_resources_for_nominal_throttling_pri, distribution_dist_setup, distribution_dist_teardown)
{

    // use the default setup to ensure all cores get nominal, but subtract one
    // count from resource
    setup_multi_priority();
    s_ctrl_loop_detail.curr_resources -= 1;
    distribute_assert_expectations();
    power_distribution_distribute_resources(&s_runconfig, &s_ctrl_loop_detail);
    // at the same priority, no cores will get nominal
    for (int i = 0; i < NUM_D_CORES; ++i)
    {
        uint8_t expected_limit;
        if (i == (NUM_D_CORES - 1))
        {
            expected_limit = TEST_NOMINAL_PERF + 1;
        }
        else
        {
            expected_limit = TEST_NOMINAL_PERF;
        }
        assert_int_equal(s_ctrl_loop_detail.cores.core[core_numbers[i]].selected_plimit, expected_limit);
        assert_true(corebits_is_bit_set(&s_ctrl_loop_detail.plimits_pending, core_numbers[i]));
    }
    assert_true(s_ctrl_loop_detail.throttling);
}

POWER_TEST(distribution_distribute_resources__min_perf_for_one_resources_for_nominal_throttling_pri,
           distribution_dist_setup,
           distribution_dist_teardown)
{

    static_assert(NUM_D_CORES >= 2, "Unsafe use of NUM_D_CORES - 2 in this function");
    // use the default setup to ensure all cores get nominal, but subtract one +
    // resources to achieve nominal for one core from resource--this should
    // leave one priority level at minimum perf, and next priority at one less
    // than nominal
    setup_multi_priority();
    s_ctrl_loop_detail.curr_resources -= (PLIMIT_TO_RESOURCES(TEST_NOMINAL_PERF) + 1);
    distribute_assert_expectations();
    power_distribution_distribute_resources(&s_runconfig, &s_ctrl_loop_detail);
    // at the same priority, no cores will get nominal
    for (int i = 0; i < NUM_D_CORES; ++i)
    {
        uint8_t expected_limit;
        if (i == (NUM_D_CORES - 1))
        {
            // highest throttling priority will be max plimit
            expected_limit = MAX_PLIMIT;
        }
        else if (i == (NUM_D_CORES - 2))
        {
            // next highest throttling pri will be nominal + 1
            expected_limit = TEST_NOMINAL_PERF + 1;
        }
        else
        {
            expected_limit = TEST_NOMINAL_PERF;
        }
        assert_int_equal(s_ctrl_loop_detail.cores.core[core_numbers[i]].selected_plimit, expected_limit);
        assert_true(corebits_is_bit_set(&s_ctrl_loop_detail.plimits_pending, core_numbers[i]));
    }
    // expect pstate selections differently in different pri levels (1 selection each)
    assert_int_equal(s_ctrl_loop_detail.plimit.selections[NUM_D_CORES - 1].acc[MAX_PLIMIT], 1);
    assert_int_equal(s_ctrl_loop_detail.plimit.selections[NUM_D_CORES - 2].acc[TEST_NOMINAL_PERF + 1], 1);
    assert_int_equal(s_ctrl_loop_detail.plimit.selections[0].acc[TEST_NOMINAL_PERF], 1);
    assert_true(s_ctrl_loop_detail.throttling);
}

POWER_TEST(distribution_distribute_resources__resources_for_1_turbo, distribution_dist_setup, distribution_dist_teardown)
{
    // use the default setup to ensure all cores get nominal
    setup_single_priority();
    for (int j = 0; j < NUM_D_CORES; ++j)
    {
        uint32_t resource_to_raise_nominal =
            (PLIMIT_TO_RESOURCES(TEST_NOMINAL_PERF - 1) - PLIMIT_TO_RESOURCES(TEST_NOMINAL_PERF));
        s_ctrl_loop_detail.curr_resources += resource_to_raise_nominal;
        uint8_t expected_plimit = TEST_NOMINAL_PERF;
        if (j == (NUM_D_CORES - 1))
        {
            // on
            expected_plimit -= 1;
        }
        distribute_assert_expectations();
        power_distribution_distribute_resources(&s_runconfig, &s_ctrl_loop_detail);
        for (int i = 0; i < NUM_D_CORES; ++i)
        {
            assert_int_equal(s_ctrl_loop_detail.cores.core[core_numbers[i]].selected_plimit, expected_plimit);
            assert_true(corebits_is_bit_set(&s_ctrl_loop_detail.plimits_pending, core_numbers[i]));
        }
    }
    assert_false(s_ctrl_loop_detail.throttling);
}

POWER_TEST(distribution_distribute_resources__resources_for_1_turbo__min_plimit, distribution_dist_setup, distribution_dist_teardown)
{

    // setup cores with min plimit less than where we expect the resource availability to allow us to go, but not at 0
    for (int core = 0; core < NUM_D_CORES; ++core)
    {
        s_ctrl_loop_detail.cores.core[core_numbers[core]].min_plimit = TEST_NOMINAL_PERF - 3;
    }

    // use the default setup to ensure all cores get nominal
    setup_single_priority();
    for (int j = 0; j < NUM_D_CORES; ++j)
    {
        uint32_t resource_to_raise_nominal =
            (PLIMIT_TO_RESOURCES(TEST_NOMINAL_PERF - 1) - PLIMIT_TO_RESOURCES(TEST_NOMINAL_PERF));
        s_ctrl_loop_detail.curr_resources += resource_to_raise_nominal;
        uint8_t expected_plimit = TEST_NOMINAL_PERF;
        if (j == (NUM_D_CORES - 1))
        {
            // on
            expected_plimit -= 1;
        }
        distribute_assert_expectations();
        power_distribution_distribute_resources(&s_runconfig, &s_ctrl_loop_detail);
        for (int i = 0; i < NUM_D_CORES; ++i)
        {
            assert_int_equal(s_ctrl_loop_detail.cores.core[core_numbers[i]].selected_plimit, expected_plimit);
            assert_true(corebits_is_bit_set(&s_ctrl_loop_detail.plimits_pending, core_numbers[i]));
        }
    }
    assert_false(s_ctrl_loop_detail.throttling);
}

POWER_TEST(distribution_distribute_resources__resources_for_max_turbo, distribution_dist_setup, distribution_dist_teardown)
{

    setup_single_priority();
    // setup enough resources for all cores to get max perf
    s_ctrl_loop_detail.curr_resources = PLIMIT_TO_RESOURCES(MAX_PERF) * NUM_D_CORES;
    distribute_assert_expectations();
    power_distribution_distribute_resources(&s_runconfig, &s_ctrl_loop_detail);
    // at the same priority, no cores will get nominal
    for (int i = 0; i < NUM_D_CORES; ++i)
    {
        assert_int_equal(s_ctrl_loop_detail.cores.core[core_numbers[i]].selected_plimit, MAX_PERF);
        assert_true(corebits_is_bit_set(&s_ctrl_loop_detail.plimits_pending, core_numbers[i]));
    }
    // should not be throttling
    assert_false(s_ctrl_loop_detail.throttling);
}

POWER_TEST(distribution_distribute_resources__resources_for_max_turbo__pstate_forced, distribution_dist_setup, distribution_dist_teardown)
{

    setup_single_priority();
    // set forced knob
    s_runconfig.knobs.force_pstate = TEST_NOMINAL_PERF;
    // setup enough resources for all cores to get max perf
    s_ctrl_loop_detail.curr_resources = PLIMIT_TO_RESOURCES(MAX_PERF) * NUM_D_CORES;
    distribute_assert_expectations();
    power_distribution_distribute_resources(&s_runconfig, &s_ctrl_loop_detail);
    // at the same priority, no cores will get nominal
    for (int i = 0; i < NUM_D_CORES; ++i)
    {
        // plimit should not have been touched on forced core
        uint8_t expected_plimit = INVALID_PERF;
        bool expect_bit_set = false;
        assert_int_equal(s_ctrl_loop_detail.cores.core[core_numbers[i]].selected_plimit, expected_plimit);
        assert_int_equal(corebits_is_bit_set(&s_ctrl_loop_detail.plimits_pending, core_numbers[i]), expect_bit_set);
    }
    // should not be throttling
    assert_false(s_ctrl_loop_detail.throttling);
}

POWER_TEST(distribution_distribute_resources__resources_for_max_turbo__cores_with_min_plimit, distribution_dist_setup, distribution_dist_teardown)
{

    // setup enough resources for all cores to get max perf
    s_ctrl_loop_detail.curr_resources = PLIMIT_TO_RESOURCES(MAX_PERF) * NUM_D_CORES;

    // setup two cores with min plimits
    s_ctrl_loop_detail.cores.core[core_numbers[0]].min_plimit = TEST_NOMINAL_PERF;
    // offset resources by amount of min_plimits.
    s_ctrl_loop_detail.curr_resources -= (PLIMIT_TO_RESOURCES(MAX_PERF) - PLIMIT_TO_RESOURCES(TEST_NOMINAL_PERF));

    s_ctrl_loop_detail.cores.core[core_numbers[1]].min_plimit = TEST_NOMINAL_PERF - 1;
    s_ctrl_loop_detail.curr_resources -= (PLIMIT_TO_RESOURCES(MAX_PERF) - PLIMIT_TO_RESOURCES(TEST_NOMINAL_PERF - 1));

    setup_single_priority();
    distribute_assert_expectations();
    power_distribution_distribute_resources(&s_runconfig, &s_ctrl_loop_detail);
    // at the same priority, no cores will get nominal
    for (int i = 0; i < NUM_D_CORES; ++i)
    {
        uint8_t expected_plimit = MAX_PERF;
        if (i == 0)
        {
            expected_plimit = TEST_NOMINAL_PERF;
        }
        else if (i == 1)
        {
            expected_plimit = TEST_NOMINAL_PERF - 1;
        }
        assert_int_equal(s_ctrl_loop_detail.cores.core[core_numbers[i]].selected_plimit, expected_plimit);
        assert_true(corebits_is_bit_set(&s_ctrl_loop_detail.plimits_pending, core_numbers[i]));
    }
    // should not be throttling
    assert_false(s_ctrl_loop_detail.throttling);
}

POWER_TEST(distribution_minimum_plimit_updates__none, distribution_setup, distribution_teardown)
{

    const corebits_t nobits = {0};
    s_runconfig.knobs.minimum_plimit_updates = power_loops_minimum_plimit_t_NONE;
    corebits_clear(&s_ctrl_loop_detail.plimits_pending);
    distribute_assert_expectations();
    power_distribution_minimum_plimit_updates(&s_runconfig, &s_ctrl_loop_detail);
    assert_true(corebits_is_equal(&s_ctrl_loop_detail.plimits_pending, &nobits));
}

POWER_TEST(distribution_minimum_plimit_updates, distribution_setup, distribution_teardown)
{

    corebits_t allbits = {0};
    for (int i = 0; i < COREBITS_MAX_BITS; ++i)
    {
        corebits_set_bit(&allbits, i);
    }

    for (int min_plimit = power_loops_minimum_plimit_t_MIN_1; min_plimit <= power_loops_minimum_plimit_t_MIN_128; ++min_plimit)
    {
        unsigned count_per_iteration = 1 << (min_plimit - 1); // minimum updates count is 2^(minimum-1)
        unsigned necessary_iterations = ((NUM_AP_CORES_PER_DIE + count_per_iteration - 1) / count_per_iteration);

        s_runconfig.knobs.minimum_plimit_updates = (power_loops_minimum_plimit_t)min_plimit;
        corebits_clear(&s_ctrl_loop_detail.plimits_pending);
        printf("All plimits cycled in %d iterations\n", necessary_iterations);
        for (unsigned iterations = 0; iterations < necessary_iterations; iterations++)
        {
            assert_false(corebits_is_equal(&s_ctrl_loop_detail.plimits_pending, &allbits));
            distribute_assert_expectations();
            power_distribution_minimum_plimit_updates(&s_runconfig, &s_ctrl_loop_detail);
        }
        assert_true(corebits_is_equal(&s_ctrl_loop_detail.plimits_pending, &allbits));
    }
}
