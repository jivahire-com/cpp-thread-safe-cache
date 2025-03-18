//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_memory_map_test.cpp
 * DDR Manager memory map tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstddef>         // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep
#include <ddrss_lib.h>
#include <fpfw_cfg_mgr.h>
extern "C" {
#include "ddr_manager_i.h"  // for ddr_create_memory_map
#include "ddr_memory_map.h" // for ddrss_get_memory_map
#include "ddr_mocks.h"
#include "memory_map/ddrss_region_def.h" // for PAS_ROOT, PAS_SECURE, AVAILABLE_SYSMEM

/*-- Symbolic Constant Macros (defines) --*/
#define UNUSED(x)          (void)(x)
#define MAX_REGIONS        12
#define RSVD_REGIONS_COUNT (sizeof(unsort_regions) / sizeof(ddrss_memory_region_t))

extern ddrss_memory_region_t _dram_rsvd_regions[];
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
static ddrss_memory_region_t ddr_test_map[] = {
    {
        .start_address = (uint64_t)0x80000000U,
        .end_address = (uint64_t)0x81800000,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM) | AVAILABLE_SYSMEM,
            },
    },
    {
        .start_address = (uint64_t)0x81800000U,
        .end_address = (uint64_t)0xFF000000U,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_ROOT) | AVAILABLE_SYSMEM,
            },
    },
    {
        .start_address = (uint64_t)0xFF000000U,
        .end_address = 0x100000000U,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_ROOT) | AVAILABLE_SYSMEM,
            },
    },
    {
        .start_address = 0x8080000000U,
        .end_address = 0x8200000000U,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM) | AVAILABLE_SYSMEM,
            },
    },
    {
        .start_address = 0x8400000000U,
        .end_address = 0xB000000000U,
        .attr =
            {
                .as_uint32 = PAS_ROOT | AVAILABLE_SYSMEM,
            },
    },
    {
        .start_address = 0x20000000000U,
        .end_address = 0xF0000000000U,
        .attr =
            {
                .as_uint32 = PAS_ROOT | AVAILABLE_SYSMEM,
            },
    },
};

static ddrss_memory_region_t unsort_regions[] = {
    {
        .start_address = (uint64_t)0x81800000U,
        .end_address = (uint64_t)0xFF000000U,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_ROOT) | AVAILABLE_SYSMEM,
            },
    },
    {
        .start_address = 0x8400000000U,
        .end_address = 0xB000000000U,
        .attr =
            {
                .as_uint32 = PAS_ROOT | AVAILABLE_SYSMEM,
            },
    },
    {
        .start_address = (uint64_t)0xFF000000U,
        .end_address = 0x100000000U,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_ROOT) | AVAILABLE_SYSMEM,
            },
    },
    {
        .start_address = (uint64_t)0x80000000U,
        .end_address = (uint64_t)0x81800000,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM) | AVAILABLE_SYSMEM,
            },
    },
    {
        .start_address = 0x8080000000U,
        .end_address = 0x8200000000U,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM) | AVAILABLE_SYSMEM,
            },
    },
    {
        .start_address = 0,
        .end_address = 0,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM) | AVAILABLE_SYSMEM,
            },
    },
};

static ddrss_memory_region_t ddr_overlap_map[] = {
    {
        .start_address = (uint64_t)0x80000000U,
        .end_address = (uint64_t)0x81800000,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM) | AVAILABLE_SYSMEM,
            },
    },
    {
        .start_address = (uint64_t)0x81800000U,
        .end_address = (uint64_t)0xFF000000U,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_ROOT) | AVAILABLE_SYSMEM,
            },
    },
    {
        .start_address = (uint64_t)0xFF000000U,
        .end_address = 0x8180000000U,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_ROOT) | AVAILABLE_SYSMEM,
            },
    },
    {
        .start_address = 0x8080000000U, // overlap
        .end_address = 0x8200000000U,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM) | AVAILABLE_SYSMEM,
            },
    },
    {
        .start_address = 0,
        .end_address = 0,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM) | AVAILABLE_SYSMEM,
            },
    },
};

static ddrss_sys_mem_region_t ddrss_sys_test_map = {
    .mem_regions =
        {
            ddr_test_map[0],
            ddr_test_map[1],
            ddr_test_map[2],
            ddr_test_map[3],
            ddr_test_map[4],
            ddr_test_map[5],
        },
    .num_regions = 6,
};

static ddrss_memory_region_t ddr_inclusive_map[] = {
    {
        .start_address = (uint64_t)0x80000000U,
        .end_address = (uint64_t)0xFFFFFFFFU,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM) | AVAILABLE_SYSMEM,
            },
    },
    {
        .start_address = (uint64_t)0x20080000000U,
        .end_address = (uint64_t)0x25FFFFFFFFFU,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_ROOT) | AVAILABLE_SYSMEM,
            },
    },
    {
        .start_address = (uint64_t)0x40000000000U,
        .end_address = 0x45FFFFFFFFFU,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_ROOT) | AVAILABLE_SYSMEM,
            },
    },
    {
        .start_address = 0,
        .end_address = 0,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM) | AVAILABLE_SYSMEM,
            },
    },
    {
        .start_address = 0,
        .end_address = 0,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM) | AVAILABLE_SYSMEM,
            },
    },
};

static ddrss_sys_mem_region_t ddrss_sys_inclusive_test_map = {
    .mem_regions =
        {
            ddr_inclusive_map[0],
            ddr_inclusive_map[1],
            ddr_inclusive_map[2],
            ddr_inclusive_map[3],
            ddr_inclusive_map[4],
        },
    .num_regions = 5,
};

const ddrss_sys_mem_region_t* p_inclusive_test_map = &ddrss_sys_inclusive_test_map;

// (I=incoming range, R=Reserved range, OI=Expected output- incoming, OR=Expected output - reserved)
//    I    R    OR
//    I    R    OR
//    I    R    OR
//         R
static ddrss_memory_region_t test_map_reservations_aligned_start_exceeds_range[] = {
    {
        .start_address = 0x8080000000U,
        .end_address = 0x8400000000U,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM) | AVAILABLE_SYSMEM,
            }, // No longer used when adding reservation- keep for testing
    },
    {
        .start_address = 0,
        .end_address = 0,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM) | AVAILABLE_SYSMEM,
            },
    },
};

// (I=incoming range, R=Reserved range, OI=Expected output- incoming, OR=Expected output - reserved)
//    I         OI
//    I    R    OR
//    I    R    OR
//    I    R    OR
//    I         OI
static ddrss_memory_region_t test_map_reservations_three_contiguous[] = {
    {
        .start_address = 0x8084000000U,
        .end_address = 0x8086000000U,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM) | NOT_AVAILABLE_SYSMEM,
            },
    },
    {
        .start_address = 0x8086000000U,
        .end_address = 0x8086600000U,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM) | NOT_AVAILABLE_SYSMEM,
            },
    },
    {
        .start_address = 0x8086600000U,
        .end_address = 0x8088000000U,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM) | NOT_AVAILABLE_SYSMEM,
            },
    },
    {
        .start_address = 0,
        .end_address = 0,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM) | NOT_AVAILABLE_SYSMEM,
            },
    },
};

// (I=incoming range, R=Reserved range, OI=Expected output- incoming, OR=Expected output - reserved)
//    I         OI
//    I    R    OR
//    I         OI
//    I    R    OR
static ddrss_memory_region_t test_map_reservations_noncontiguous_non_aligned[] = {
    {
        .start_address = 0x8082000000U,
        .end_address = 0x8084000000U,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM) | NOT_AVAILABLE_SYSMEM,
            },
    },
    {
        .start_address = 0x9FFFFFF000U,
        .end_address = 0xA000000000U,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM) | NOT_AVAILABLE_SYSMEM,
            },
    },
    {
        .start_address = 0,
        .end_address = 0,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM) | NOT_AVAILABLE_SYSMEM,
            },
    },
};

// (I=incoming range, R=Reserved range, OI=Expected output- incoming, OR=Expected output - reserved)
//         R
//    I    R    OR
//    I    R    OR
//         R
static ddrss_memory_region_t test_map_reservations_starts_ends_out_of_range[] = {
    {
        .start_address = 0x8060000000U,
        .end_address = 0x8300000000U,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM) | NOT_AVAILABLE_SYSMEM,
            },
    },
    {
        .start_address = 0,
        .end_address = 0,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM) | NOT_AVAILABLE_SYSMEM,
            },
    },
};

// (I=incoming range, R=Reserved range, OI=Expected output- incoming, OR=Expected output - reserved)
//    I        OI
//    I        OI
//    I        OI
static ddrss_memory_region_t test_map_reservations_none[] = {
    {
        .start_address = 0,
        .end_address = 0,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM) | NOT_AVAILABLE_SYSMEM,
            },
    },
};

// (I=incoming range, R=Reserved range, OI=Expected output- incoming, OR=Expected output - reserved)
//    I        OI
//    I   R    OR
//    I        OI
static ddrss_memory_region_t test_map_reservations_verify_security_flag_per_requirement[] = {
    {
        .start_address = 0x81800100U,
        .end_address = 0x81800400U,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM) | NOT_AVAILABLE_SYSMEM,
            }, // Incoming map has this flag set PAS_SECURE
    },
    {
        .start_address = 0,
        .end_address = 0,
        .attr =
            {
                .as_uint32 = (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM) | NOT_AVAILABLE_SYSMEM,
            },
    }};
} // extern "C"

//
// Tests
//
TEST_FUNCTION(test_ddrss_get_memory_map, NULL, NULL)
{
    ddrss_sys_mem_region_t all_mem = {{}, 0};
    const ddrss_sys_mem_region_t* pAllMemRegions = &all_mem;

    will_return(__wrap_ddrss_get_system_mem_region, &ddrss_sys_test_map);

    ddrss_get_memory_map(&pAllMemRegions);
}

TEST_FUNCTION(test_ddr_create_memory_map_no_borgens, NULL, NULL)
{
    will_return(__wrap_config_get_borgens_1gb_ddr_reserve_enable, true);
    will_return(__wrap_ddrss_get_system_mem_region, &ddrss_sys_test_map);

    ddr_create_memory_map();
}

TEST_FUNCTION(test_ddr_make_incoming_map_exclusive, NULL, NULL)
{
    ddrss_memory_region_t* mmap = (ddrss_memory_region_t*)ddrss_sys_inclusive_test_map.mem_regions;
    uint32_t idx = 0;

    // Check preconditions
    while (idx < ddrss_sys_inclusive_test_map.num_regions)
    {
        if (mmap[idx].start_address == 0 && mmap[idx].end_address == 0)
        {
            break;
        }
        assert_int_equal(mmap[idx].end_address & 0xFFFF, 0xFFFF);
        idx++;
    }

    // Change incoming memory map to use exclusive addressing.
    //  Ex: 0x80000000 - 0x90000000 instead of 0x80000000 - 0x8FFFFFFF
    reformat_incoming_memory_map(&p_inclusive_test_map);

    // Check postconditions
    idx = 0;
    while (idx < ddrss_sys_inclusive_test_map.num_regions)
    {
        if (mmap[idx].start_address == 0 && mmap[idx].end_address == 0)
        {
            break;
        }

        assert_int_equal((mmap[idx].end_address & 0xFFFF) & 0xFFFF, 0);
        idx++;
    }
}
TEST_FUNCTION(test_sort_reserved_regions, NULL, NULL)
{
    ddrss_memory_region_t sorted_reservations[RSVD_REGIONS_COUNT] = {};

    sort_reserved_regions(unsort_regions, RSVD_REGIONS_COUNT, sorted_reservations);

    assert_int_equal(sorted_reservations[0].start_address, 0x80000000);
    assert_int_equal(sorted_reservations[0].end_address, 0x81800000);
    assert_int_equal(sorted_reservations[0].attr.available_sysmem, 1);
    assert_int_equal(sorted_reservations[0].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM));

    assert_int_equal(sorted_reservations[1].start_address, 0x81800000);
    assert_int_equal(sorted_reservations[1].end_address, 0xFF000000);
    assert_int_equal(sorted_reservations[1].attr.available_sysmem, 1);
    assert_int_equal(sorted_reservations[1].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_ROOT));

    assert_int_equal(sorted_reservations[2].start_address, 0xFF000000);
    assert_int_equal(sorted_reservations[2].end_address, 0x100000000);
    assert_int_equal(sorted_reservations[2].attr.available_sysmem, 1);
    assert_int_equal(sorted_reservations[2].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_ROOT));

    assert_int_equal(sorted_reservations[3].start_address, 0x8080000000);
    assert_int_equal(sorted_reservations[3].end_address, 0x8200000000);
    assert_int_equal(sorted_reservations[3].attr.available_sysmem, 1);
    assert_int_equal(sorted_reservations[3].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM));

    assert_int_equal(sorted_reservations[4].start_address, 0x8400000000);
    assert_int_equal(sorted_reservations[4].end_address, 0xB000000000);
    assert_int_equal(sorted_reservations[4].attr.available_sysmem, 1);
    assert_int_equal(sorted_reservations[4].attr.as_uint32 & 0xf, PAS_ROOT);
}

TEST_FUNCTION(test_reservation_order, NULL, NULL)
{
    int status;
    /* test unsorted regions*/
    status = check_reservation_order(unsort_regions);
    assert_int_equal(status, SILIBS_E_DATA);

    /* test overlap regions*/
    status = check_reservation_order(ddr_overlap_map);
    assert_int_equal(status, SILIBS_E_DATA);

    /* test normal regions*/
    status = check_reservation_order(test_map_reservations_three_contiguous);
    assert_int_equal(status, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_ddrmap_add_reservations_happy_path, NULL, NULL)
{
    ddrss_memory_region_t result_map_64[MAX_REGIONS] = {};
    const ddrss_sys_mem_region_t* test_map = &ddrss_sys_test_map;

    ddrmap_add_reservations((*test_map).mem_regions, test_map_reservations_aligned_start_exceeds_range, result_map_64);

    /**** Test 1 - Reserved range aligns with boundary of existing range ****/
    assert_int_equal(result_map_64[0].start_address, 0x80000000);
    assert_int_equal(result_map_64[0].end_address, 0x81800000);
    assert_int_equal(result_map_64[0].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[0].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM));

    assert_int_equal(result_map_64[1].start_address, 0x81800000);
    assert_int_equal(result_map_64[1].end_address, 0xFF000000);
    assert_int_equal(result_map_64[1].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[1].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_ROOT));

    assert_int_equal(result_map_64[2].start_address, 0xFF000000);
    assert_int_equal(result_map_64[2].end_address, 0x100000000);
    assert_int_equal(result_map_64[2].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[2].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_ROOT));

    // Inserted range ---------------------------------------------
    assert_int_equal(result_map_64[3].start_address, 0x8080000000);
    assert_int_equal(result_map_64[3].end_address, 0x8200000000);
    assert_int_equal(result_map_64[3].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[3].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM));
    // ------------------------------------------------------------
    assert_int_equal(result_map_64[4].start_address, 0x8400000000);
    assert_int_equal(result_map_64[4].end_address, 0xB000000000);
    assert_int_equal(result_map_64[4].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[4].attr.as_uint32 & 0xf, PAS_ROOT);

    /**** Test 2 - Reserved range does not align with boundary of existing range ****/
    ddrmap_add_reservations((*test_map).mem_regions, test_map_reservations_three_contiguous, result_map_64);

    assert_int_equal(result_map_64[0].start_address, 0x80000000);
    assert_int_equal(result_map_64[0].end_address, 0x81800000);
    assert_int_equal(result_map_64[0].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[0].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM));

    assert_int_equal(result_map_64[1].start_address, 0x81800000);
    assert_int_equal(result_map_64[1].end_address, 0xFF000000);
    assert_int_equal(result_map_64[1].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[1].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_ROOT));

    assert_int_equal(result_map_64[2].start_address, 0xFF000000);
    assert_int_equal(result_map_64[2].end_address, 0x100000000);
    assert_int_equal(result_map_64[2].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[2].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_ROOT));

    // Inserted ranges --------------------------------------------
    assert_int_equal(result_map_64[3].start_address, 0x8080000000);
    assert_int_equal(result_map_64[3].end_address, 0x8084000000);
    assert_int_equal(result_map_64[3].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[3].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM));

    assert_int_equal(result_map_64[4].start_address, 0x8084000000);
    assert_int_equal(result_map_64[4].end_address, 0x8086000000);
    assert_int_equal(result_map_64[4].attr.available_sysmem, 0);
    assert_int_equal(result_map_64[4].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM));

    assert_int_equal(result_map_64[5].start_address, 0x8086000000);
    assert_int_equal(result_map_64[5].end_address, 0x8086600000);
    assert_int_equal(result_map_64[5].attr.available_sysmem, 0);
    assert_int_equal(result_map_64[5].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM));

    assert_int_equal(result_map_64[6].start_address, 0x8086600000);
    assert_int_equal(result_map_64[6].end_address, 0x8088000000);
    assert_int_equal(result_map_64[6].attr.available_sysmem, 0);
    assert_int_equal(result_map_64[6].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM));

    assert_int_equal(result_map_64[7].start_address, 0x8088000000);
    assert_int_equal(result_map_64[7].end_address, 0x8200000000);
    assert_int_equal(result_map_64[7].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[7].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM));
    // ------------------------------------------------------------

    assert_int_equal(result_map_64[8].start_address, 0x8400000000);
    assert_int_equal(result_map_64[8].end_address, 0xB000000000);
    assert_int_equal(result_map_64[8].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[8].attr.as_uint32 & 0xf, PAS_ROOT);

    assert_int_equal(result_map_64[9].start_address, 0x20000000000);
    assert_int_equal(result_map_64[9].end_address, 0xF0000000000);
    assert_int_equal(result_map_64[9].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[9].attr.as_uint32 & 0xf, PAS_ROOT);

    assert_int_equal(result_map_64[10].start_address, 0);
    assert_int_equal(result_map_64[10].end_address, 0);
    assert_int_equal(result_map_64[10].attr.available_sysmem, 0);
    assert_int_equal(result_map_64[10].attr.as_uint32 & 0xf, 0);

    /**** Test 3 - test_map_reservations_noncontiguous_non_aligned ****/
    ddrmap_add_reservations((*test_map).mem_regions, test_map_reservations_noncontiguous_non_aligned, result_map_64);

    assert_int_equal(result_map_64[0].start_address, 0x80000000);
    assert_int_equal(result_map_64[0].end_address, 0x81800000);
    assert_int_equal(result_map_64[0].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[0].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM));

    assert_int_equal(result_map_64[1].start_address, 0x81800000);
    assert_int_equal(result_map_64[1].end_address, 0xFF000000);
    assert_int_equal(result_map_64[1].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[1].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_ROOT));

    assert_int_equal(result_map_64[2].start_address, 0xFF000000);
    assert_int_equal(result_map_64[2].end_address, 0x100000000);
    assert_int_equal(result_map_64[2].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[2].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_ROOT));

    // Inserted ranges --------------------------------------------
    assert_int_equal(result_map_64[3].start_address, 0x8080000000);
    assert_int_equal(result_map_64[3].end_address, 0x8082000000);
    assert_int_equal(result_map_64[3].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[3].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM));

    assert_int_equal(result_map_64[4].start_address, 0x8082000000);
    assert_int_equal(result_map_64[4].end_address, 0x8084000000);
    assert_int_equal(result_map_64[4].attr.available_sysmem, 0);
    assert_int_equal(result_map_64[4].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM));

    assert_int_equal(result_map_64[5].start_address, 0x8084000000);
    assert_int_equal(result_map_64[5].end_address, 0x8200000000);
    assert_int_equal(result_map_64[5].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[5].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM));

    assert_int_equal(result_map_64[6].start_address, 0x8400000000);
    assert_int_equal(result_map_64[6].end_address, 0x9FFFFFF000);
    assert_int_equal(result_map_64[6].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[6].attr.as_uint32 & 0xf, PAS_ROOT);

    assert_int_equal(result_map_64[7].start_address, 0x9FFFFFF000);
    assert_int_equal(result_map_64[7].end_address, 0xA000000000);
    assert_int_equal(result_map_64[7].attr.available_sysmem, 0);
    assert_int_equal(result_map_64[7].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM));

    assert_int_equal(result_map_64[8].start_address, 0xA000000000);
    assert_int_equal(result_map_64[8].end_address, 0xB000000000);
    assert_int_equal(result_map_64[8].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[8].attr.as_uint32 & 0xf, PAS_ROOT);
    // ------------------------------------------------------------

    assert_int_equal(result_map_64[9].start_address, 0x20000000000);
    assert_int_equal(result_map_64[9].end_address, 0xF0000000000);
    assert_int_equal(result_map_64[9].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[9].attr.as_uint32 & 0xf, PAS_ROOT);

    assert_int_equal(result_map_64[10].start_address, 0);
    assert_int_equal(result_map_64[10].end_address, 0);
    assert_int_equal(result_map_64[10].attr.available_sysmem, 0);
    assert_int_equal(result_map_64[10].attr.as_uint32 & 0xf, 0);

    /**** test_map_reservations_starts_ends_out_of_range ****/
    ddrmap_add_reservations((*test_map).mem_regions, test_map_reservations_starts_ends_out_of_range, result_map_64);
    // Inserted ranges --------------------------------------------
    assert_int_equal(result_map_64[3].start_address, 0x8080000000);
    assert_int_equal(result_map_64[3].end_address, 0x8200000000);
    assert_int_equal(result_map_64[3].attr.available_sysmem, 0);
    assert_int_equal(result_map_64[3].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM));

    assert_int_equal(result_map_64[4].start_address, 0x8400000000);
    assert_int_equal(result_map_64[4].end_address, 0xB000000000);
    assert_int_equal(result_map_64[4].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[4].attr.as_uint32 & 0xf, PAS_ROOT);

    /**** test_map_reservations_none ****/
    ddrmap_add_reservations((*test_map).mem_regions, test_map_reservations_none, result_map_64);

    assert_int_equal(result_map_64[0].start_address, 0x80000000);
    assert_int_equal(result_map_64[0].end_address, 0x81800000);
    assert_int_equal(result_map_64[0].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[0].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM));

    assert_int_equal(result_map_64[1].start_address, 0x81800000);
    assert_int_equal(result_map_64[1].end_address, 0xFF000000);
    assert_int_equal(result_map_64[1].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[1].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_ROOT));

    assert_int_equal(result_map_64[2].start_address, 0xFF000000);
    assert_int_equal(result_map_64[2].end_address, 0x100000000);
    assert_int_equal(result_map_64[2].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[2].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_ROOT));

    assert_int_equal(result_map_64[3].start_address, 0x8080000000);
    assert_int_equal(result_map_64[3].end_address, 0x8200000000);
    assert_int_equal(result_map_64[3].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[3].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM));

    assert_int_equal(result_map_64[4].start_address, 0x8400000000);
    assert_int_equal(result_map_64[4].end_address, 0xB000000000);
    assert_int_equal(result_map_64[4].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[4].attr.as_uint32 & 0xf, PAS_ROOT);

    assert_int_equal(result_map_64[5].start_address, 0x20000000000);
    assert_int_equal(result_map_64[5].end_address, 0xF0000000000);
    assert_int_equal(result_map_64[5].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[5].attr.as_uint32 & 0xf, PAS_ROOT);

    assert_int_equal(result_map_64[6].start_address, 0);
    assert_int_equal(result_map_64[6].end_address, 0);
    assert_int_equal(result_map_64[6].attr.available_sysmem, 0);
    assert_int_equal(result_map_64[6].attr.as_uint32 & 0xf, 0);

    /**** test_map_reservations_verify_security_flag_per_requirement ****/
    ddrmap_add_reservations((*test_map).mem_regions, test_map_reservations_verify_security_flag_per_requirement, result_map_64);

    assert_int_equal(result_map_64[0].start_address, 0x80000000);
    assert_int_equal(result_map_64[0].end_address, 0x81800000);
    assert_int_equal(result_map_64[0].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[0].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM));

    // Test that is_security_region == 1 -------------------------
    assert_int_equal(result_map_64[1].start_address, 0x81800000);
    assert_int_equal(result_map_64[1].end_address, 0x81800100);
    assert_int_equal(result_map_64[1].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[1].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_ROOT));

    assert_int_equal(result_map_64[2].start_address, 0x81800100);
    assert_int_equal(result_map_64[2].end_address, 0x81800400);
    assert_int_equal(result_map_64[2].attr.available_sysmem, 0);
    assert_int_equal(result_map_64[2].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM));

    assert_int_equal(result_map_64[3].start_address, 0x81800400);
    assert_int_equal(result_map_64[3].end_address, 0xFF000000);
    assert_int_equal(result_map_64[3].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[3].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_ROOT));
    ///////////////////////////////////////////////////////////////

    assert_int_equal(result_map_64[4].start_address, 0xFF000000);
    assert_int_equal(result_map_64[4].end_address, 0x100000000);
    assert_int_equal(result_map_64[4].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[4].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_ROOT));

    assert_int_equal(result_map_64[5].start_address, 0x8080000000);
    assert_int_equal(result_map_64[5].end_address, 0x8200000000);
    assert_int_equal(result_map_64[5].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[5].attr.as_uint32 & 0xf, (PAS_SECURE | PAS_NON_SECURE | PAS_ROOT | PAS_REALM));

    assert_int_equal(result_map_64[6].start_address, 0x8400000000);
    assert_int_equal(result_map_64[6].end_address, 0xB000000000);
    assert_int_equal(result_map_64[6].attr.available_sysmem, 1);
    assert_int_equal(result_map_64[6].attr.as_uint32 & 0xf, PAS_ROOT);
}

// TEST_FUNCTION(test_ddrmap_add_reservations_happy_path, NULL, NULL)
// {
//     will_return(__wrap_config_get_borgens_1gb_ddr_reserve_enable, false);

//     ddrss_memory_region_t result_map_64[MAX_REGIONS] = {};
//     const ddrss_sys_mem_region_t* test_map = &ddrss_sys_test_map;
