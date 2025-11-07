//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_payload_ecid.c
 * Registering ECID information data to put in crash dump
 */

/*--------------- Includes ---------------*/
#include "crash_dump_payload.h" // for CD_MSFT_VERSION_INFO, CD_GUID, ...

#include <crash_dump.h>               // for crash_dump_type_context_t
#include <modules/CdDumpDescriptor.h> // for FPFwCdDumpPriority, _FPFwCdDum...
#include <modules/CdDumpManager.h>    // for CdRegisterAddress32, CdRegiste...
#include <string.h>                   // for memcpy
#include <tlm_fuses.h>                // for ecid_t, tlm_fuses_get_ecid

/*-- Symbolic Constant Macros (defines) --*/
/**
 * GUID used to locate the ECID info in Crash dump
 * GUID : 185084EC-E4AD-4DA1-A4DF-8ADCF33DD1B4
 */
#define CD_ECID_INFO_GUID                                  \
    {                                                      \
        0x185084EC, 0xE4AD, 0x4DA1,                        \
        {                                                  \
            0xA4, 0xDF, 0x8A, 0xDC, 0xF3, 0x3D, 0xD1, 0xB4 \
        }                                                  \
    }

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
static void ecid_update(FPFwCrashDumpCtx* ctx, uint64_t dumpOffset, void* userContext, uint32_t payloadSize)
{
    FPFW_UNUSED(userContext);
    FPFW_UNUSED(payloadSize);

    ecid_t cdEcidInfo = {0};
    fpfw_status_t status = tlm_fuses_get_ecid(&cdEcidInfo);

    if (FPFW_STATUS_SUCCEEDED(status))
    {
        ctx->memAPIs.FPFwCDMemcpyLocalToGlobal(&ctx->memAPIs, dumpOffset, &cdEcidInfo, sizeof(ecid_t));
    }
}

void crash_dump_register_ECID(crash_dump_type_context_t* type_context)
{
    CdRegisterCustomChunk(&type_context->crash_dump_ctx,
                          (GUID)CD_ECID_INFO_GUID,
                          NULL,
                          DUMP_ROUND_UP(sizeof(ecid_t), DUMP_NATURAL_ALIGNMENT),
                          NULL,
                          ecid_update,
                          FPFW_CD_DUMP_PRIORITY_CRITICAL);
}
