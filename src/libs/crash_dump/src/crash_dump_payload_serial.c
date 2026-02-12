//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_payload_serial.c
 * Registering serial output data to put in crash dump
 */

/*--------------- Includes ---------------*/
#include "crash_dump_payload.h" // for CD_MSFT_VERSION_INFO, CD_GUID, ...

#include <crash_dump.h>               // for crash_dump_type_context_t
#include <modules/CdDumpDescriptor.h> // for FPFwCdDumpPriority, _FPFwCdDum...
#include <modules/CdDumpManager.h>    // for CdRegisterAddress32, CdRegiste...
#include <string.h>                   // for memcpy

/*-- Symbolic Constant Macros (defines) --*/
/**
 * GUID used to locate the Serial output info in Crash dump
 * GUID : FB05C11B-2980-4FB6-A6BE-64FDB1E88C50
 */
#define CD_SERIAL_OUTPUT_INFO_GUID                         \
    {                                                      \
        0xFB05C11B, 0x2980, 0x4FB6,                        \
        {                                                  \
            0xA6, 0xBE, 0x64, 0xFD, 0xB1, 0xE8, 0x8C, 0x50 \
        }                                                  \
    }

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
static cd_serial_output_buffer_t cd_serial_output_buffer = {0};

/*------------- Functions ----------------*/
void crash_dump_write_serial_byte(uint8_t byte)
{
    cd_serial_output_buffer.buffer[cd_serial_output_buffer.tail] = byte;
    cd_serial_output_buffer.tail = (cd_serial_output_buffer.tail + 1) % MAX_CD_SERIAL_OUTPUT_BUFFER_SIZE;
    if (cd_serial_output_buffer.tail == cd_serial_output_buffer.head)
    {
        // Buffer is full, move head to the next position to make room for new data
        cd_serial_output_buffer.head = (cd_serial_output_buffer.head + 1) % MAX_CD_SERIAL_OUTPUT_BUFFER_SIZE;
    }
}

static void serial_output_update(FPFwCrashDumpCtx* ctx, uint64_t dumpOffset, void* userContext, uint32_t payloadSize)
{
    FPFW_UNUSED(userContext);
    FPFW_UNUSED(payloadSize);

    ctx->memAPIs.FPFwCDMemcpyLocalToGlobal(&ctx->memAPIs, dumpOffset, &cd_serial_output_buffer, sizeof(cd_serial_output_buffer_t));
}

void crash_dump_register_serial_output(crash_dump_type_context_t* type_context)
{
    CdRegisterCustomChunk(&type_context->crash_dump_ctx,
                          (GUID)CD_SERIAL_OUTPUT_INFO_GUID,
                          NULL,
                          DUMP_ROUND_UP(sizeof(cd_serial_output_buffer_t), DUMP_NATURAL_ALIGNMENT),
                          NULL,
                          serial_output_update,
                          FPFW_CD_DUMP_PRIORITY_OPPORTUNISTIC);
}
