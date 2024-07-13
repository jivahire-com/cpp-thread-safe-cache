//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file icc_mhu_mock.c
 * Provide mock functions for icc mhu transport driver tests
 */

/*------------- Includes -----------------*/
#include "icc_mhu_trans_ut.h"

#include <DfwkClient.h> // for PDFWK_DEVICE_HEADER, PDFWK_INTERFACE_HEADER
#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <icc_mhu.h>
#include <icc_mhu_cfg.h>
#include <stdint.h> // for uint32_t, uint64_t, int32_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

int __wrap_icc_mhu_send_message(uint16_t mhu_interface_id, uint32_t command, uint8_t* data, size_t size)
{
    FPFW_UNUSED(mhu_interface_id);
    FPFW_UNUSED(command);
    FPFW_UNUSED(size);
    FPFW_UNUSED(data);
    return mock_type(int);
}

int __wrap_icc_mhu_get_message(uint16_t mhu_interface_id, picc_msg_receive_t message)
{
    FPFW_UNUSED(mhu_interface_id);
    FPFW_UNUSED(message);
    return mock_type(int);
}

void __wrap_mhuv3_clr_mdbcw_status(uintptr_t mbx, uint32_t channel_num, uint32_t flag_num)
{
    FPFW_UNUSED(mbx);
    FPFW_UNUSED(channel_num);
    FPFW_UNUSED(flag_num);
}

int __wrap_icc_mhu_trans_get_message(uint16_t mhu_interface_id, picc_msg_receive_t message)
{
    FPFW_UNUSED(mhu_interface_id);
    message->command = WRAPPER_ICC_COMMAND;
    message->size = 0;
    return mock_type(int);
}