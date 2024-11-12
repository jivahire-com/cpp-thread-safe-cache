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
#include <FPFwInterrupts.h>
#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwLock.h>
#include <FpFwUtils.h>   // for FPFW_UNUSED
#include <fpfw_status.h> // for fpfw_status_t
#include <icc_mhu.h>
#include <icc_mhu_cfg.h>
#include <stdint.h> // for uint32_t, uint64_t, int32_t
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

uint8_t data[] = {1, 2, 3, 4};

UINT timer_active_status = TX_FALSE;

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
    message->size = sizeof(data);
    message->data = data;
    return mock_type(int);
}

int __wrap_icc_mhu_check_message_received(uint16_t mhu_interface_id)
{
    FPFW_UNUSED(mhu_interface_id);
    return mock_type(int);
}

fpfw_status_t __wrap_fpfw_icc_transport_try_send_sync_req(PDFWK_INTERFACE_HEADER transport_interface, void* send_buffer, size_t buffer_size)
{
    FPFW_UNUSED(transport_interface);
    FPFW_UNUSED(send_buffer);
    FPFW_UNUSED(buffer_size);
    return mock_type(int);
}

fpfw_status_t __wrap_fpfw_icc_transport_try_recv_sync_req(PDFWK_INTERFACE_HEADER transport_interface,
                                                          void* recv_buffer,
                                                          size_t buffer_size,
                                                          size_t* output_recv_bytes)
{
    FPFW_UNUSED(transport_interface);
    FPFW_UNUSED(recv_buffer);
    FPFW_UNUSED(buffer_size);
    FPFW_UNUSED(output_recv_bytes);
    return mock_type(int);
}

uint32_t __wrap_FPFwCoreInterruptRegisterCallback(uint32_t irqnum, FPFwCoreInterruptHandler handler, void* arg)
{
    check_expected(irqnum);
    check_expected(handler);
    check_expected(arg);
    return mock_type(uint32_t);
}

uint32_t __wrap_FPFwCoreInterruptEnableVector(uint32_t irqnum)
{
    check_expected(irqnum);
    return mock_type(uint32_t);
}

uint32_t __wrap_FPFwCoreInterruptDisableVector(uint32_t irqnum)
{
    check_expected(irqnum);
    return mock_type(uint32_t);
}

FPFW_LOCK_STATE __wrap_FpFwLockAcquire(PFPFW_LOCK Lock)
{
    FPFW_UNUSED(Lock);
    return 0;
}

void __wrap_FpFwLockRelease(PFPFW_LOCK Lock, FPFW_LOCK_STATE OldState)
{
    FPFW_UNUSED(Lock);
    FPFW_UNUSED(OldState);
}

UINT __wrap__txe_timer_info_get(TX_TIMER* timer_ptr, CHAR** name, UINT* active, ULONG* remaining_ticks, ULONG* reschedule_ticks, TX_TIMER** next_timer)
{
    FPFW_UNUSED(timer_ptr);
    FPFW_UNUSED(name);

    *active = timer_active_status;

    FPFW_UNUSED(remaining_ticks);
    FPFW_UNUSED(reschedule_ticks);
    FPFW_UNUSED(next_timer);

    return TX_SUCCESS;
}

void __wrap_DfwkAsyncRequestComplete(PDFWK_ASYNC_REQUEST_HEADER Request)
{
    check_expected(Request);
}