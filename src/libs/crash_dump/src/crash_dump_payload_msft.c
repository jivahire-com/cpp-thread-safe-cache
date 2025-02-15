//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_payload_msft.c
 * Registering MSFT build information data to put in crash dump
 */

/*--------------- Includes ---------------*/
#include "crash_dump_payload.h" // for CD_MSFT_VERSION_INFO, CD_GUID, ...

#include <build_data.h>               // for BUILD_ELF_SECTION_BINARY_METADATA
#include <crash_dump.h>               // for crash_dump_type_context_t
#include <modules/CdDumpDescriptor.h> // for FPFwCdDumpPriority, _FPFwCdDum...
#include <modules/CdDumpManager.h>    // for CdRegisterAddress32, CdRegiste...
#include <string.h>                   // for memcpy

/*-- Symbolic Constant Macros (defines) --*/
/**
 * GUID used to locate the Msft Version info in Crash dump
 * GUID : da166264-6f19-4647-bf3d-4f9b523f5d08
 */
#define CD_MSFT_VERSION_INFO_GUID                          \
    {                                                      \
        0xda166264, 0x6f19, 0x4647,                        \
        {                                                  \
            0xbf, 0x3d, 0x4f, 0x9b, 0x52, 0x3f, 0x5d, 0x08 \
        }                                                  \
    }

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
CD_MSFT_VERSION_INFO cdMsftVersionInfo;

/*------------- Functions ----------------*/
void crash_dump_register_standard_info(crash_dump_type_context_t* type_context)
{
    cdMsftVersionInfo.guid = (CD_GUID)CD_MSFT_VERSION_INFO_GUID;
    cdMsftVersionInfo.versionInfo.Major = g_BuildMetadata.Major;
    cdMsftVersionInfo.versionInfo.Minor = g_BuildMetadata.Minor;
    cdMsftVersionInfo.versionInfo.Revision = g_BuildMetadata.Revision;
    cdMsftVersionInfo.versionInfo.Build = g_BuildMetadata.Build;
    memcpy((void*)(cdMsftVersionInfo.versionInfo.String), // Can be modified to have Symbol and Product information.
           (void*)(g_BuildMetadata.String),
           BUILD_ELF_BINARY_METADATA_STR_SIZE);
    memcpy((void*)(cdMsftVersionInfo.elf_build_id), (void*)(g_note_gnu_build_id.BuildId), SHA1_GNU_ELF_BUILD_ID_SIZE__bytes);

    CdRegisterAddress32(&type_context->crash_dump_ctx, (void*)&cdMsftVersionInfo, sizeof(cdMsftVersionInfo), FPFW_CD_DUMP_PRIORITY_CRITICAL);
}
