//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * This section is used to describe the file and any relevant
 * information related to it purpose and how it should be used.
 */

/*------------- Includes -----------------*/
#include <DfwkCommon.h> // for PDFWK_ASYNC_REQUEST_HEADER, DFWK_ASYNC_REQUE...
#include <FpFwCMocka.h> // for check_expected_ptr, assert_non_null, assert_...
#include <idsw_kng.h>
#include <pcie_dfwk.h>      // for pcie_async_request_t, pciess_device_t
#include <pcie_ss_common.h> // pciess_entity
#include <scp_pcie_manager.h>
#include <stddef.h> // for size_t

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

void __wrap_DfwkInterfaceSendAsync(PDFWK_INTERFACE_HEADER Interface, PDFWK_ASYNC_REQUEST_HEADER Request)
{
    check_expected_ptr(Interface);
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

/*
void __wrap_end_async_wait_for_event(pcie_manager_context_t* ctx, uint8_t rp_idx, uint8_t num_event)
{
    (void)(rp_idx);
    (void)(num_event);

}*/
