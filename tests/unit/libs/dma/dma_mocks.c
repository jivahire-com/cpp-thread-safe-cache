//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file dma_mocks.c
 * Mock functions for DMA driver tests
 */

/*------------- Includes -----------------*/
#include <DfwkCommon.h>              // for PDFWK_DEVICE_HEADER, DFWK_ASYNC_REQUEST_HEADER, PDFWK_SCHEDULE
#include <DfwkStatus.h>              // for DFWK_SUCCESS
#include <FpFwCMocka.h>              // IWYU pragma: keep
#include <FpFwUtils.h>
#include <atu_lib.h>
#include <cmocka.h>                  // IWYU pragma: keep
#include <dmac.h>                    // for dmac_cfg_t
#include <fpfw_init.h>               // for fpfw_init_component_t, fpfw_init_get_handle
#include <idsw_kng.h>                // for idsw_get_cpu_type
#include <nvic.h>
#include <silibs_status.h>
#include <stddef.h>
#include <tx_api.h>                  // for UINT, ULONG, TX_QUEUE, TX_SUCCESS


/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
uint32_t g_intu_sts;
bool g_mmio_read32_mocktype;
bool g_mmio_read64_mocktype;
bool g_FpFwLockAcquire_check_signature;

DFWK_ASYNC_REQUEST_DISPATCH s_dispatch_routine = NULL;
DFWK_REQUEST_DISPATCH_SYNC s_dispatch_routine_sync = NULL;

/*------------- Functions ----------------*/

//
// Mocks
//
int __wrap_atu_translate_address(atu_id_t atu_id, uint64_t ap_addr, uint32_t *mscp_addr)
{
    FPFW_UNUSED(atu_id);
    FPFW_UNUSED(ap_addr);

    *mscp_addr = mock_type(uint32_t);
    return (mock_type(int));
}

int __wrap_atu_map(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    if (atu_id >= ATU_ID_MAX || atu_map_entry == NULL)
    {
        return SILIBS_E_PARAM;
    }
    // Keep mscp base non-zero to allow checking base address in UTs
    atu_map_entry->mscp_start_address = 0xffffffff;

    return mock_type(int);
}

int __wrap_atu_unmap(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    if (atu_id >= ATU_ID_MAX || atu_map_entry == NULL)
    {
        return SILIBS_E_PARAM;
    }

    return mock_type(int);
}

nvic_status_t __wrap_nvic_irq_set_isr_with_param(uint32_t irq_num, isr_callback_fn_with_params_t isr, void* parameter)
{
    check_expected(irq_num);
    check_expected(isr);
    assert_non_null(parameter);
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

uint32_t __wrap_mmio_read32(volatile uint32_t* addr)
{
    FPFW_UNUSED(addr);
    if (g_mmio_read32_mocktype)
    {
        return (mock_type(uint32_t));
    }
    else
    {
        return (0);
    }
}

void __wrap_mmio_write32(volatile uint32_t* addr, uint32_t data)
{
    FPFW_UNUSED(addr);
    FPFW_UNUSED(data);
    return;
}

uint32_t __wrap_mmio_read64(volatile uint32_t* addr)
{
    FPFW_UNUSED(addr);
    if (g_mmio_read64_mocktype)
    {
        return (mock_type(uint64_t));
    }
    else
    {
        return (0);
    }
}

void __wrap_mmio_write64(volatile uint32_t* addr, uint64_t data)
{
    FPFW_UNUSED(addr);
    FPFW_UNUSED(data);
    return;
}

// This is the crux of the DDR ISR testing - it will set which interrupt is 'triggered'
int __wrap_intu_get_interrupt_status(uintptr_t intu_base_addr, uint32_t* intr)
{
    FPFW_UNUSED(intu_base_addr);
    *intr = g_intu_sts;
    return (SILIBS_SUCCESS);
}

/**
 * @brief Mock function for DfwkDeviceInitialize
 *
 * @param[in] Device : Device passed from caller function
 * @param[in] Schedule : Schedule passed from caller function
 *
 */
void __wrap_DfwkDeviceInitialize(PDFWK_DEVICE_HEADER Device, PDFWK_SCHEDULE Schedule)
{
    check_expected_ptr(Device);
    check_expected_ptr(Schedule);
}

/**
 * @brief Mock function for DfwkAsyncRequestComplete
 *
 * @param[in] Request : Request header sent by caller function
 *
 */
void __wrap_DfwkAsyncRequestComplete(PDFWK_ASYNC_REQUEST_HEADER Request)
{
    check_expected_ptr(Request);
}

/**
 * @brief Mock function for FpFwAssert
 *
 * @param[in] expression : Expression to validate
 *
 */
void __wrap_FpFwAssert(int expression)
{
    check_expected(expression);
}

idsw_cpu_type_t __wrap_idsw_get_cpu_type(void)
{
    return mock_type(idsw_cpu_type_t);
}

UINT __wrap__txe_timer_create(TX_TIMER* timer_ptr,
                              CHAR* name_ptr,
                              VOID (*expiration_function)(ULONG id),
                              ULONG expiration_input,
                              ULONG initial_ticks,
                              ULONG reschedule_ticks,
                              UINT auto_activate,
                              UINT timer_control_block_size)
{
    FPFW_UNUSED(timer_ptr);
    FPFW_UNUSED(name_ptr);
    FPFW_UNUSED(expiration_function);
    FPFW_UNUSED(expiration_input);
    FPFW_UNUSED(initial_ticks);
    FPFW_UNUSED(reschedule_ticks);
    FPFW_UNUSED(auto_activate);
    FPFW_UNUSED(timer_control_block_size);
    return mock_type(UINT);
}

UINT  __wrap__txe_timer_activate(TX_TIMER *timer_ptr)
{
    FPFW_UNUSED(timer_ptr);
    return mock_type(UINT);
}

uint64_t __wrap_get_dmac_id_reg(uintptr_t dmac_base_addr)
{
    FPFW_UNUSED(dmac_base_addr);
    return (uint64_t)0;
}

uint64_t __wrap_get_dmac_compver_reg(uintptr_t dmac_base_addr)
{
    FPFW_UNUSED(dmac_base_addr);
    return (uint64_t)0;
}

void __wrap_set_dmac_reset_reg(uintptr_t dmac_base_addr, uint64_t val)
{
    FPFW_UNUSED(dmac_base_addr);
    FPFW_UNUSED(val);
    return;
}

uint64_t __wrap_get_dmac_reset_reg(uintptr_t dmac_base_addr)
{
    FPFW_UNUSED(dmac_base_addr);
    return (uint64_t)0;
}

uint64_t __wrap_get_dmac_cfg_reg(uintptr_t dmac_base_addr)
{
    FPFW_UNUSED(dmac_base_addr);
    return (uint64_t)0;
}

void __wrap_set_dmac_cfg_reg(uintptr_t dmac_base_addr, uint64_t val)
{
    FPFW_UNUSED(dmac_base_addr);
    FPFW_UNUSED(val);
    return;
}

FPFW_LOCK_STATE __wrap_FpFwLockAcquire(PFPFW_LOCK Lock)
{
    if(g_FpFwLockAcquire_check_signature)
    {
        check_expected(Lock->Signature);
    }
    return ((FPFW_LOCK_STATE)0);
}

void __wrap_FpFwLockRelease(PFPFW_LOCK Lock, FPFW_LOCK_STATE OldState)
{
    FPFW_UNUSED(Lock);
    FPFW_UNUSED(OldState);
}

void __wrap_DfwkQueueEnqueueRequest(PDFWK_QUEUE Queue, PDFWK_ASYNC_REQUEST_HEADER Request)
{
    check_expected_ptr(Queue);
    check_expected_ptr(Request);
}

void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    FPFW_UNUSED(id);
    return mock_type(void*);
}

silibs_status_t __wrap_dmac_start_single_block_transfer(uintptr_t dmac_base_addr, dmac_cfg_t *cfg)
{
    check_expected(dmac_base_addr);
    check_expected(cfg->dmac_dest_tr_width);
    check_expected(cfg->dmac_src_tr_width);
    check_expected(cfg->max_block_ts_bytes);
    check_expected(cfg->dmac_transfer_cfg->channel_id);
    check_expected(cfg->dmac_transfer_cfg->dmac_dest_addr);
    check_expected(cfg->dmac_transfer_cfg->dmac_src_addr);
    check_expected(cfg->dmac_transfer_cfg->transfer_size);
    return SILIBS_SUCCESS;
}

uint64_t __wrap_dmac_get_ch_interrupt_status(uintptr_t dmac_base_addr, DMAC_CHANNEL channel_id)
{
    FPFW_UNUSED(dmac_base_addr);
    FPFW_UNUSED(channel_id);
    return (mock_type(uint64_t));
}