//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file mts_platform_mock.c
 * Mocks for the Message Transfer Service platform specific functions
 */

/*------------- Includes -----------------*/
#include "mts_mock.h"

#include <FpFwCMocka.h>    // for check_expected, check_expected_ptr, mock_type
#include <FpFwUtils.h>     // for FPFW_UNUSED
#include <fpfw_icc_base.h> // for fpfw_icc_base_ctx_t
#include <fpfw_status.h>   // for fpfw_status_t
#include <stdint.h>        // for uint8_t
#include <stdio.h>         // for printf
#include <string.h>        // for memset, memcpy
#include <transfer_relay_protocol.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
uint8_t _ProviderMetadata_et_msdata_start; // Pointer to the start of the .ProviderMetadata section
uint8_t _ProviderMetadata_et_msdata_end;   // Pointer to the end   of the .ProviderMetadata section
uint8_t _EventMetadata_et_msdata_start;    // Pointer to the start of the .EventMetadata section
uint8_t _EventMetadata_et_msdata_end;      // Pointer to the end   of the .EventMetadata memory section

/*------------- Functions ----------------*/

uint8_t __wrap_transfer_rly_get_this_die_id(void)
{
    return mock_type(uint8_t);
}

fpfw_status_t __wrap_fpfw_icc_base_send_sync(fpfw_icc_base_ctx_t* icc_ctx, void* payload_buffer, size_t buffer_size)
{
    FPFW_UNUSED(icc_ctx);
    assert_true(buffer_size <= MAX_ICC_BUFFER_SIZE);
    assert_true(buffer_size > 0);

    uint8_t* outgoing_icc_buffer = mock_type(uint8_t*);

    memset(outgoing_icc_buffer, 0, MAX_ICC_BUFFER_SIZE);
    memcpy(outgoing_icc_buffer, payload_buffer, buffer_size);

    return mock_type(fpfw_status_t);
}

bool __wrap_transfer_rly_should_send_dcp_msg(p_trp_msg_t trp_msg, p_trp_endpoint_t trp_endpoint)
{
    FPFW_UNUSED(trp_msg);
    FPFW_UNUSED(trp_endpoint);

    return mock_type(bool);
}

bool __wrap_transfer_rly_should_send_trp_msg(p_trp_msg_t trp_msg, p_trp_endpoint_t trp_endpoint)
{
    FPFW_UNUSED(trp_msg);
    FPFW_UNUSED(trp_endpoint);

    return mock_type(bool);
}

void __wrap_FpFwAssert(int expression)
{
    check_expected(expression);
}

void __wrap_FpFwAssertWithArgs(int expression, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3)
{
    FPFW_UNUSED(expression);
    FPFW_UNUSED(arg0);
    FPFW_UNUSED(arg1);
    FPFW_UNUSED(arg2);
    FPFW_UNUSED(arg3);

    function_called();
}

UINT __wrap__tx_thread_sleep(ULONG timer_ticks)
{
    FPFW_UNUSED(timer_ticks);

    return mock_type(UINT);
}
