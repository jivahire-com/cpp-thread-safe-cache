//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file event_trace_mocks.c
 * Mocks for the event trace
 */

/*------------- Includes -----------------*/

#include <FpFwCMocka.h>
#include <IFpFwEventTracingController.h>
#include <IFpFwEventTracingDefines.h>
#include <IFpFwEventTracingEvents.h>
#include <IFpFwEventTracingStatus.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

uint8_t _data_etproviderfilter_start;      // Pointer to the start of the .ProviderFilter section
uint8_t _data_etproviderfilter_end;        // Pointer to the end   of the .ProviderFilter section
uint8_t _ProviderMetadata_et_msdata_start; // Pointer to the start of the .ProviderMetadata section
uint8_t _ProviderMetadata_et_msdata_end;   // Pointer to the end   of the .ProviderMetadata section
uint8_t _EventMetadata_et_msdata_start;    // Pointer to the start of the .EventMetadata section
uint8_t _EventMetadata_et_msdata_end;      // Pointer to the end   of the .EventMetadata memory section

// For test sections
FPFW_ET_PROVIDER_METADATA_HEADER _ProviderMetadata_Test_et_msdata_start; // Pointer to the start of the .ProviderMetadata section
FPFW_ET_PROVIDER_METADATA_HEADER _ProviderMetadata_Test_et_msdata_end; // Pointer to the end   of the .ProviderMetadata section
FPFW_ET_EVENT_METADATA_HEADER _EventMetadata_Test_et_msdata_start; // Pointer to the start of the .EventMetadata section
FPFW_ET_EVENT_METADATA_HEADER _EventMetadata_Test_et_msdata_end; // Pointer to the end   of the .EventMetadata memory section

/*------------- Functions ----------------*/

FPFW_ET_STATUS __wrap_FPFwETDecoderInit(PFPFW_ET_PROVIDER_METADATA_HEADER pProviderMetadataStart,
                                        PFPFW_ET_PROVIDER_METADATA_HEADER pProviderMetadataEnd,
                                        PFPFW_ET_EVENT_METADATA_HEADER pEventMetadataStart,
                                        PFPFW_ET_EVENT_METADATA_HEADER pEventMetadataEnd)
{
    check_expected_ptr(pProviderMetadataStart);
    check_expected_ptr(pProviderMetadataEnd);
    check_expected_ptr(pEventMetadataStart);
    check_expected_ptr(pEventMetadataEnd);

    return mock_type(FPFW_ET_STATUS);
}

FPFW_ET_STATUS __wrap_FPFwETTraceInitialize(PFPFW_ET_CONTROLLER pController, PFPFW_ET_CONTROLLER_CONFIG pConfig)
{
    check_expected_ptr(pController);
    check_expected_ptr(pConfig);

    return mock_type(FPFW_ET_STATUS);
}

FPFW_ET_STATUS __wrap_FPFwETDecoderPrintEvent(PFPFW_ET_EVENT_DESCRIPTOR pEvent, uint32_t* eventSize)
{
    check_expected_ptr(pEvent);
    check_expected_ptr(eventSize);

    return mock_type(FPFW_ET_STATUS);
}

FPFW_ET_STATUS __wrap_FPFwETControllerRecycleBuffer(PFPFW_ET_CONTROLLER pTraceController, const uint32_t bufIndex)
{
    check_expected_ptr(pTraceController);
    check_expected(bufIndex);

    return mock_type(FPFW_ET_STATUS);
}
