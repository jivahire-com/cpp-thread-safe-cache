//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * This section is used to describe the file and any relevant
 * information related to it purpose and how it should be used.
 */

/*------------- Includes -----------------*/
#include "pcie_mocks.h"

#include <DfwkCommon.h>     // for PDFWK_ASYNC_REQUEST_HEADER, DFWK_ASYNC_REQUE...
#include <FpFwCMocka.h>     // for check_expected_ptr, assert_non_null, assert_...
#include <accelerator_ip.h> // For Accel isolation
#include <cmocka.h>
#include <fpfw_status.h>
#include <idsw_kng.h>
#include <pcie_dfwk.h>      // for pcie_async_request_t, pciess_device_t
#include <pcie_ss_common.h> // pciess_entity
#include <scp_pcie_manager.h>
#include <stddef.h> // for size_t
#include <stdint.h>
#include <variable_services.h>
#include <variable_services_mem.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void __wrap_pcie_dfwk_init(pciess_device_t* dev, PDFWK_SCHEDULE schedule)
{
    assert_non_null(dev);
    assert_non_null(schedule);
    check_expected_ptr(schedule);
}

void __wrap_DfwkAsyncRequestInitialize(PDFWK_ASYNC_REQUEST_HEADER Request, size_t RequestSize)
{
    check_expected_ptr(Request);
    assert_int_equal(RequestSize, sizeof(pcie_async_request_t));
    Request->AllocatedSize = RequestSize;
}

void __wrap_DfwkAsyncRequestSetCompletionRoutine(PDFWK_ASYNC_REQUEST_HEADER Request,
                                                 DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine,
                                                 void* CompletionContext)
{
    check_expected_ptr(Request);
    check_expected_ptr(CompletionRoutine);
    check_expected_ptr(CompletionContext);
}

int32_t __wrap_DfwkInterfaceSendSync(PDFWK_INTERFACE_HEADER Interface, PDFWK_SYNC_REQUEST_HEADER Request)
{
    assert_non_null(Interface);
    assert_non_null(Request);
    check_expected(Request->RequestType);

    pcie_sync_request_t* req = (pcie_sync_request_t*)Request;

    void* data = mock_ptr_type(pcie_ss_entity_t*);

    if (Request->RequestType == GET_RPSS_ENTITY_REQUEST)
    {
        assert_non_null(data);
        assert_non_null(req->p_requested_data);
        pcie_ss_entity_t** rpss_ent = (pcie_ss_entity_t**)(req->p_requested_data);
        *rpss_ent = (pcie_ss_entity_t*)data;
    }

    req->status = mock();
    return mock_type(int32_t);
}

void __wrap_DfwkInterfaceSendAsync(PDFWK_INTERFACE_HEADER Interface, PDFWK_ASYNC_REQUEST_HEADER Request)
{
    check_expected_ptr(Interface);
    check_expected_ptr(Request);
}

void __wrap_DfwkAsyncRequestComplete(PDFWK_ASYNC_REQUEST_HEADER Request)
{
    check_expected_ptr(Request);
}

UINT __wrap__txe_event_flags_delete(TX_EVENT_FLAGS_GROUP* group_ptr)
{
    check_expected(group_ptr);
    return mock_type(UINT);
}

KNG_DIE_ID __wrap_idsw_get_die_id(void)
{
    return mock_type(KNG_DIE_ID);
}

pcie_manager_context_t* __wrap_scp_pcie_get_manager_context(uint8_t rpss_idx)
{
    (void)rpss_idx;
    return mock_ptr_type(pcie_manager_context_t*);
}

pcie_ss_entity_t* __wrap_send_sync_get_rpss_entity(uint8_t rpss_idx)
{
    (void)rpss_idx;
    return mock_ptr_type(pcie_ss_entity_t*);
}

/* Calls the actual memcpy when needed outside of tested code*/
void* __real_memcpy(void* __a, const void* __b, size_t __c);

void* __wrap_memcpy(void* __a, const void* __b, size_t __c)
{
    if (memcpy_mock)
    {
        return NULL;
    }
    return __real_memcpy(__a, __b, __c);
}

void* __real_memset(void* str, int c, size_t n);

void* __wrap_memset(void* str, int c, size_t n)
{
    if (memcpy_mock)
    {
        return NULL;
    }
    return __real_memset(str, c, n);
}

/*
void __wrap_end_async_wait_for_event(pcie_manager_context_t* ctx, uint8_t rp_idx, uint8_t num_event)
{
    (void)(rp_idx);
    (void)(num_event);

}*/

void __wrap_variable_serv_copy_to_rmss_ram(volatile uint8_t* target_addr, const void* source_ptr, size_t size)
{
    FPFW_UNUSED(target_addr);
    FPFW_UNUSED(source_ptr);
    FPFW_UNUSED(size);
}

int32_t __wrap_variable_service_async_set_variable(var_service_req_ctx_t* var_serv_ctx,
                                                   var_service_req_params_t* req_params,
                                                   variable_service_req_complete_notify callback,
                                                   void* context)
{
    FPFW_UNUSED(var_serv_ctx);
    FPFW_UNUSED(req_params);
    FPFW_UNUSED(callback);
    FPFW_UNUSED(context);

    var_serv_ctx->async_req_result = mock();

    callback(context, var_serv_ctx, NULL, 0);
    return 0;
}

int32_t __wrap_variable_service_initialize_ctx(var_service_req_ctx_t* var_serv_ctx, var_service_shared_mem_t* mem_ctx)
{
    assert_non_null(var_serv_ctx);
    assert_non_null(mem_ctx);

    uint64_t min = mock();
    uint64_t max = mock();
    assert_in_range(mem_ctx->payload_base, min, max);

    return 0;
}

bool __wrap_accel_is_isolation_enabled(ACCEL_ID accel_type)
{
    FPFW_UNUSED(accel_type);
    return mock_type(bool);
}

void __wrap_pcie_dfwk_interface_init(pciess_device_t* dev, pciess_device_interface_t* iface)
{
    FPFW_UNUSED(dev);
    FPFW_UNUSED(iface);
}

UINT __wrap_pcie_phyfw_wait_load_event(TX_EVENT_FLAGS_GROUP* pcie_phyfw_load_complete)
{
    FPFW_UNUSED(pcie_phyfw_load_complete);
    return TX_SUCCESS;
}

int32_t __wrap_DfwkClientInterfaceOpen(PDFWK_INTERFACE_HEADER Interface)
{
    check_expected_ptr(Interface);
    return mock_type(int32_t);
}

UINT __wrap__tx_thread_sleep(ULONG timer_ticks)
{
    check_expected(timer_ticks);
    return TX_SUCCESS;
}

uint8_t __wrap_system_info_get_board_id()
{
    return mock_type(uint8_t);
}

uint8_t __wrap_config_get_overlake_rpss_index_secondary_soc(void)
{
    return mock_type(uint8_t);
}

uint8_t __wrap_config_get_overlake_rpss_index_primary_soc(void)
{
    return mock_type(uint8_t);
}

uint8_t __wrap_config_get_enable_overlake_sbr_workaround(void)
{
    return mock_type(uint8_t);
}

void __wrap_ras_print_record(ras_error_record_t* record)
{
    assert_non_null(record);
}
