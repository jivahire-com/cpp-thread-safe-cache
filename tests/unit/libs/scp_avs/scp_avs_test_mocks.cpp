//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scp_avs_test_mocks.cpp
 * Mocks for scp_avs
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {
#include <DfwkClient.h>
#include <avs_lib.h>
#include <nvic.h>
#include <stdint.h>
}

extern "C" {

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*----------- Mock Functions -------------*/

/*------------- Functions ----------------*/
void __wrap_DfwkInterfaceSendAsync(PDFWK_INTERFACE_HEADER Interface, PDFWK_ASYNC_REQUEST_HEADER Request)
{
    check_expected(Interface);
    check_expected(Request);
}
void __wrap_DfwkAsyncRequestSetCompletionRoutine(PDFWK_ASYNC_REQUEST_HEADER Request,
                                                 DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine,
                                                 void* CompletionContext)
{
    check_expected(Request);
    check_expected(CompletionRoutine);
    check_expected(CompletionContext);
}

nvic_status_t __wrap_nvic_irq_set_isr_with_param(uint32_t irq_num, isr_callback_fn_with_params_t isr, void* parameter)
{
    check_expected(irq_num);
    check_expected(isr);
    check_expected(parameter);
    return (NVIC_STATUS_SUCCESS);
}

nvic_status_t __wrap_nvic_irq_clear_pending(uint32_t irq_num)
{
    check_expected(irq_num);
    return (NVIC_STATUS_SUCCESS);
}

nvic_status_t __wrap_nvic_irq_enable(uint32_t irq_num)
{
    check_expected(irq_num);
    return (NVIC_STATUS_SUCCESS);
}

void __wrap_avs_enable_interrupt(uint32_t avs_id, uint32_t intr)
{
    check_expected(avs_id);
    check_expected(intr);
}

void __wrap_avs_init(uintptr_t avs_base_addr)
{
    check_expected(avs_base_addr);
}
int __wrap_avs_send_cmd_frame(uint32_t avs_id, uint32_t cmd_num, avs_master_command_t* cmd_mem)
{
    check_expected(avs_id);
    check_expected(cmd_num);
    check_expected(cmd_mem);
    return (SILIBS_SUCCESS);
}
}