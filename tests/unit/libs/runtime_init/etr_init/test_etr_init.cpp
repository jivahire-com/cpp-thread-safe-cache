//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_etr_init.cpp
 * ETR init test.
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:etr_init

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {
#include <FpFwUtils.h>
#include <event_trace_relay.h>
#include <fpfw_init.h>
#include <fpfw_status.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <string.h>
#include <tlm_fuses.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

static ecid_t s_test_ecid = {
    .wafer_lot_num = "12345678",
    .wafer_num = 0x1F,
    .x_coord = 0x7F,
    .y_coord = 0x3F,
    .parity_bits = 0xF,
};

extern fpfw_init_component_t _fpfw_component_etr;

/*------------- Functions ----------------*/

//
// Mocks
//

fpfw_status_t __wrap_tlm_fuses_get_ecid(ecid_t* ecid)
{
    *ecid = s_test_ecid;
    return mock_type(fpfw_status_t);
}

void __wrap_etr_initialize(etr_service_context_t* p_context, const etr_service_config_t* p_config)
{
    FPFW_UNUSED(p_context);

    // Validate the soc id matches the ecid when unpacked
    ecid_t ecid_from_config = {};

    // Extract the ECID fields from the packed soc_id
    // Based on the bit layout from etr_init.c:
    // Parity Bits (5 bits):       bits 0-4
    // Y Coordinate (7 bits):      bits 5-11
    // X Coordinate (7 bits):      bits 12-18
    // Wafer Number (5 bits):      bits 19-23
    // Wafer Lot Number (72 bits): bits 24-95
    // Reserved (32 bits):         bits 96-127

    const uint8_t* soc_id = p_config->soc_info.soc_id;

    // Extract Parity Bits (5 bits): bits 0-4
    ecid_from_config.parity_bits = soc_id[0] & 0x1F;

    // Extract Y Coordinate (7 bits): bits 5-11
    // First 3 bits from soc_id[0] (bits 5-7) and last 4 bits from soc_id[1] (bits 8-11)
    ecid_from_config.y_coord = ((soc_id[0] >> 5) & 0x07) | ((soc_id[1] & 0x0F) << 3);

    // Extract X Coordinate (7 bits): bits 12-18
    // First 4 bits from soc_id[1] (bits 12-15) and last 3 bits from soc_id[2] (bits 16-18)
    ecid_from_config.x_coord = ((soc_id[1] >> 4) & 0x0F) | ((soc_id[2] & 0x07) << 4);

    // Extract Wafer Number (5 bits): bits 19-23
    ecid_from_config.wafer_num = (soc_id[2] >> 3) & 0x1F;

    // Extract Wafer Lot Number (72 bits): bits 24-95
    // Copy 9 bytes starting from soc_id[3]
    memcpy(ecid_from_config.wafer_lot_num, &soc_id[3], ECID_WAFER_LOT_NUMBER_CHAR_SIZE);

    // Validate that the unpacked ECID matches the expected test ECID
    assert_memory_equal(&ecid_from_config, &s_test_ecid, sizeof(ecid_t));
}

idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return mock_type(idsw_die_id_t);
}

//
// Tests
//

TEST_FUNCTION(test_etc_init, nullptr, nullptr)
{
    // Set up expectations
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_tlm_fuses_get_ecid, FPFW_STATUS_SUCCESS);

    // Call API under test
    _fpfw_component_etr.init_fn();
}
}