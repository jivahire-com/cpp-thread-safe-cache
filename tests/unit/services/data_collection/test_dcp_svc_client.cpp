//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_dcp_svc_client.cpp
 *
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:data_collection

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <data_collection_protocol.h>
#include <data_collection_service.h>
#include <dcp_service_client_i.h>
#include <fpfw_status.h> // for FPFW_STATUS_SUCCESS, FPFW_...
#include <stdint.h>      // for uint8_t
#include <telemetry_relay_protocol.h>
}

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);

    return 0;
}

//
// Tests
//
TEST_FUNCTION(test_dcp_svc_client_handle_trp_msg, test_setup, nullptr)
{
    trp_msg_t trp_msg;
    dcp_svc_client_handle_trp_msg(&trp_msg);
}