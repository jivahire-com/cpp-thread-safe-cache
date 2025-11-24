//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file build_info_cli.c
 * Source file to implement cli for build_info
 */

/*-------------------------------- Includes ---------------------------------*/
#include <FpFwCli.h>        // for FPFW_CLI_STATUS, FpFwCliRegister, CLI_SU...
#include <FpFwLinkedList.h> // for NULL_LIST_ENTRY
#include <FpFwUtils.h>      // for FPFW_UNUSED
#include <build_data.h>     // for BUILD_PC, BUILD_TIMESTAMP, GIT_BRANCH
#include <idhw.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <inttypes.h>
#include <stdio.h>
#include <system_info.h>
#include <utils.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/
static FPFW_CLI_STATUS whoami(int Argc, const char** Argv);

/*------------------- Declarations (Statics and globals) --------------------*/
static FPFW_CLI_COMMAND s_build_info_commands = {NULL_LIST_ENTRY, "build_info", "whoami", whoami, "show build metadata", "Usage: whoami"};

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/
static PLACED_CODE FPFW_CLI_STATUS whoami(int Argc, const char** Argv)
{
    FPFW_UNUSED(Argc);
    FPFW_UNUSED(Argv);

    printf("----------------------------------------------------------------\n");
    printf("Build Version: %d.%d.%d\n", (int)MAJOR_VERSION, (int)MINOR_VERSION, (int)PATCH_VERSION);
    printf("Branch: %s\n", GIT_BRANCH);
    printf("Commit: %s\n", GIT_COMMIT_SHA);
    printf("Build Timestamp: %s\n", BUILD_TIMESTAMP);
    printf("Build PC: %s\n", BUILD_PC);

    /* MSCP Build Info */
    printf("MSCP tag " SCP_DESCRIBE "\n");
    printf("MSCP Commit " GIT_COMMIT_SHA "\n");

    /* Silibs Info */
    printf("Silibs tag " SILIBS_DESCRIBE "\n");
    printf("Silibs Commit " SILIBS_LAST_COMMIT_DESCRIBE "\n");

    /* SDM Info */
    printf("SDM tag " SDM_DESCRIBE "\n");
    printf("SDM Commit " SDM_LAST_COMMIT_DESCRIBE "\n");

    /* SoC Die Info */
    printf("SoC CPU Type: ");
    switch ((uint8_t)idsw_get_cpu_type())
    {
    case CPU_AP:
        printf("AP\n");
        break;
    case CPU_SCP:
        printf("SCP\n");
        break;
    case CPU_MCP:
        printf("MCP\n");
        break;
    case CPU_HSP:
        printf("HSP\n");
        break;
    case CPU_TBP:
        printf("TBP\n");
        break;
    case CPU_SDM:
        printf("SDM\n");
        break;
    case CPU_CDED_SDM:
        printf("CDED_SDM\n");
        break;
    case CPU_CDED_KMP:
        printf("CDED_KMP\n");
        break;
    default:
        printf("Unknown\n");
        break;
    }
    printf("SoC Platform: ");
    switch (idsw_get_platform_sdv())
    {
    case PLATFORM_FPGA:
        printf("FPGA\n");
        break;
    case PLATFORM_FPGA_LARGE:
        printf("FPGA_LARGE\n");
        break;
    case PLATFORM_FPGA_LARGE_RVP:
        printf("FPGA_LARGE_RVP\n");
        break;
    case PLATFORM_SVP_SIM:
        printf("SVP_SIM\n");
        break;
    case PLATFORM_SVP_MIN_CONFIG_SIM:
        printf("SVP_MIN_CONFIG_SIM\n");
        break;
    case PLATFORM_RVP_EVT_SILICON:
        printf("RVP_EVT_SILICON\n");
        break;
    default:
        printf("Unknown\n");
        break;
    }

    // Print SoC Die Info
    printf("SoC Die ID: %d\n", (uint8_t)idsw_get_die_id());
    printf("SoC Single Die Boot Enable: %s\n", idhw_is_single_die_boot_en() ? "true" : "false");

    // Print SoC secure state
    printf("SoC Secure State: [%d]\n", system_info_get_security_state());
    printf("SoC Mission Mode: %s\n", system_info_get_mission_mode() ? "true" : "false");

    const NOTE_GNU_BUILD_ID* p_metadata = &g_note_gnu_build_id;

    /* Print the build GUID in standard format 4Bytes-2Bytes-2Bytes-2Bytes-6Bytes */
    printf("Build GUID: ");
    uint32_t build_id_0_4_bytes = (p_metadata->BuildId[0] << 24) | (p_metadata->BuildId[1] << 16) |
                                  (p_metadata->BuildId[2] << 8) | (p_metadata->BuildId[3]);
    uint16_t build_id_1_2_bytes = (p_metadata->BuildId[4] << 8) | (p_metadata->BuildId[5]);
    uint16_t build_id_2_2_bytes = (p_metadata->BuildId[6] << 8) | (p_metadata->BuildId[7]);
    uint16_t build_id_3_2_bytes = (p_metadata->BuildId[8] << 8) | (p_metadata->BuildId[9]);
    uint64_t build_id_4_6_bytes =
        ((uint64_t)p_metadata->BuildId[10] << 40) | ((uint64_t)p_metadata->BuildId[11] << 32) |
        ((uint64_t)p_metadata->BuildId[12] << 24) | ((uint64_t)p_metadata->BuildId[13] << 16) |
        ((uint64_t)p_metadata->BuildId[14] << 8) | ((uint64_t)p_metadata->BuildId[15]);

    printf("%08" PRIx32 "-%04" PRIx16 "-%04" PRIx16 "-%04" PRIx16 "-%012" PRIx64 "\n",
           build_id_0_4_bytes,
           build_id_1_2_bytes,
           build_id_2_2_bytes,
           build_id_3_2_bytes,
           build_id_4_6_bytes);

    printf("----------------------------------------------------------------\n");

    return CLI_SUCCESS;
}

/*----------------------------- Global Functions ----------------------------*/
FPFW_CLI_STATUS build_info_cli_init(void)
{
    FpFwCliRegister(&s_build_info_commands);

    return CLI_SUCCESS;
}
