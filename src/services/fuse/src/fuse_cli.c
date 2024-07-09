/*
 * Copyright (c) Microsoft Corporation.
 *
 * Description:
 *     CLI support for fuse module
 */

/*------------- Includes -----------------*/
#include <FpFwAssert.h>     // for FPFW_RUNTIME_ASSERT
#include <FpFwCli.h>        // for FpFwCliInitialize, FpFwCliStart, FPFW_CLI_...
#include <fpfw_init.h>      // for fpfw_init_get_handle, FPFW_INIT_C...
#include <fuse.h>           // fuse library functions
#include <fuse_defines.h>   // Test revision get
#include <fuse_dma.h>       // apply copy fuse to ram
#include <fuse_events.h>    // apply event trace for fuse
#include <fuse_init.h>      // fuse service API interface
#include <fuses_top_regs.h> // efuse registry address
#include <idsw.h>           // SW platform id
#include <idsw_kng.h>
#include <stdbool.h> // for false, true
#include <stdint.h>  // for uintptr_t
#include <stdio.h>   // for NULL, printf
#include <utils.h>   // for sleep_ms()...

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static FPFW_CLI_STATUS fuse_sample_detail(int argc, const char** argv);

/*-- Declarations (Statics and globals) --*/
FPFW_CLI_COMMAND cli_fuse_commands[] = {
    {NULL_LIST_ENTRY, "fuse", "sample_detail", fuse_sample_detail, "Display fuse sample detail", "Usage: fuse_sample_detail (no arguments)"}};

// return 8bit sample detail fuse
static uint8_t get_sample_fuse(const unsigned fuse_bit_offset, const unsigned fuse_bit_size)
{

    KNG_PLAT_ID platform_id = idsw_get_platform_sdv();

    // only read fuses on silicon platform
    if (platform_id == PLATFORM_RVP_EVT_SILICON)
    {
        uint64_t fuse_data = 0;

        // read fuse at index into temp fuse data
        int status = platform_read_for_fuse((uintptr_t)&fuse_data, fuse_bit_offset, fuse_bit_size);
        if (status == FPFW_INIT_STATUS_SUCCESS)
        {
            return (fuse_data & 0xFF);
        }
    }
    return 0;
}

static FPFW_CLI_STATUS fuse_sample_detail(int argc, const char** pp_argv)
{
    UNUSED(argc);
    UNUSED(pp_argv);

    uint8_t sample_major_rev =
        get_sample_fuse(GENERAL_SAMPLE_INFO_SAMPLE_MAJOR_REV_BIT_OFFSET, GENERAL_SAMPLE_INFO_SAMPLE_MAJOR_REV_WIDTH);
    uint8_t customer_sample_milestone = get_sample_fuse(CUSTOMER_SAMPLE_INFO_CUSTOMER_SAMPLE_MILESTONE_BIT_OFFSET,
                                                        CUSTOMER_SAMPLE_INFO_CUSTOMER_SAMPLE_MILESTONE_WIDTH);
    uint8_t customer_sample_minor_rev = get_sample_fuse(CUSTOMER_SAMPLE_INFO_CUSTOMER_SAMPLE_MINOR_REV_BIT_OFFSET,
                                                        CUSTOMER_SAMPLE_INFO_CUSTOMER_SAMPLE_MINOR_REV_WIDTH);
    uint8_t developer_sample_milestone =
        get_sample_fuse(DEVELOPMENT_SAMPLE_INFO_DEVELOPMENT_SAMPLE_MILESTONE_BIT_OFFSET,
                        DEVELOPMENT_SAMPLE_INFO_DEVELOPMENT_SAMPLE_MILESTONE_WIDTH);
    uint8_t developer_sample_minor_rev =
        get_sample_fuse(DEVELOPMENT_SAMPLE_INFO_DEVELOPMENT_SAMPLE_MINOR_REV_BIT_OFFSET,
                        DEVELOPMENT_SAMPLE_INFO_DEVELOPMENT_SAMPLE_MINOR_REV_WIDTH);

    const char* customer_milestones[] = {"undefined", "ES", "PC", "PR", "TBD"};
    const char* developer_milestones[] = {"undefined", "DS", "TBD"};

    FpFwCliPrint("\n");
    FpFwCliPrint("Customer sample : %s%d.%d (%d.%d.%d)\n",
                 customer_milestones[FPFW_MIN(customer_sample_milestone, FPFW_ARRAY_SIZE(customer_milestones) - 1)],
                 sample_major_rev,
                 customer_sample_minor_rev,
                 customer_sample_milestone,
                 sample_major_rev,
                 customer_sample_minor_rev);
    FpFwCliPrint("Developer sample: %s%d.%d (%d.%d.%d)\n",
                 developer_milestones[FPFW_MIN(developer_sample_milestone, FPFW_ARRAY_SIZE(developer_milestones) - 1)],
                 sample_major_rev,
                 developer_sample_minor_rev,
                 developer_sample_milestone,
                 sample_major_rev,
                 developer_sample_minor_rev);
    FpFwCliPrint("\n");

    return CLI_SUCCESS;
}

/**
 *  API for registering the CLI commands for this module
 *
 *  @return
 *      none
 */
FPFW_CLI_STATUS platform_fuse_init_cli(void)
{
    FpFwCliRegisterTable(&cli_fuse_commands[0], FPFW_ARRAY_SIZE(cli_fuse_commands));
    return CLI_SUCCESS;
}
