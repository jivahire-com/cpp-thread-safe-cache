//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_sys_info.c
 * Source file to implement cli for system information
 */

/*-------------------------------- Includes ---------------------------------*/
#include <FpFwCli.h>         // for FpFwCliPrint, FPFW_CLI_COM...
#include <FpFwLinkedList.h>  // for NULL_LIST_ENTRY
#include <FpFwUtils.h>       // for FPFW_ARRAY_SIZE
#include <idsw.h>            // for idsw_get_platform_sdv,
#include <idsw_kng.h>        // for idsw_get_platform_sdv,
#include <silibs_platform.h> // for MMIO_WRITE32, MMIO_READ32
#include <stdint.h>          // for uint32_t, uint8_t
#include <stdlib.h>          // for errno, strtol
#include <system_info.h>     // for system_info_is_hsp_present, system_info_is_warm_start

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/
static FPFW_CLI_STATUS cli_get_platform_id(int argc, const char** argv);

/*------------------- Declarations (Statics and globals) --------------------*/
// clang-format off
static FPFW_CLI_COMMAND s_sys_info_int_commands_table[] = {
    {NULL_LIST_ENTRY, "sys", "plat_id",  cli_get_platform_id,  "Reads platform ID", "Usage: ID"},
};

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/**
 * @brief Routine to read platform ID
 *
 * \b Description:
 * Reads the platform.
 *
 * @retval
 *  On success, CLI_SUCCESS
 *  On failure, CLI_ERROR
 * */
static FPFW_CLI_STATUS cli_get_platform_id(int argc, const char** argv)
{
    FPFW_UNUSED(argc);
    FPFW_UNUSED(argv);

    KNG_PLAT_ID plat_id = system_info_get_platform();

    switch(plat_id) {
        case PLATFORM_FPGA:
            FpFwCliPrint("Platform ID: PLATFORM_FPGA\n");
            break;
        case PLATFORM_FPGA_TINY:
            FpFwCliPrint("Platform ID: PLATFORM_FPGA_TINY\n");
            break;
        case PLATFORM_FPGA_SMALL:
            FpFwCliPrint("Platform ID: PLATFORM_FPGA_SMALL\n");
            break;
        case PLATFORM_FPGA_LARGE:
            FpFwCliPrint("Platform ID: PLATFORM_FPGA_LARGE\n");
            break;
        case PLATFORM_FPGA_LARGE_RVP:
            FpFwCliPrint("Platform ID: PLATFORM_FPGA_LARGE_RVP\n");
            break;
        case PLATFORM_RTL_SIM:
            FpFwCliPrint("Platform ID: PLATFORM_RTL_SIM\n");
            break;
        case PLATFORM_SVP_SIM:
            FpFwCliPrint("Platform ID: PLATFORM_SVP_SIM\n");
            break;
        case PLATFORM_SVP_MIN_CONFIG_SIM:
            FpFwCliPrint("Platform ID: PLATFORM_SVP_MIN_CONFIG_SIM\n");
            break;
        case PLATFORM_QMU_SIM:
            FpFwCliPrint("Platform ID: PLATFORM_QMU_SIM\n");
            break;
        case PLATFORM_OTUT_SIM:
            FpFwCliPrint("Platform ID: PLATFORM_OTUT_SIM\n");
            break;
        case PLATFORM_EMU:
            FpFwCliPrint("Platform ID: PLATFORM_EMU\n");
            break;
        case PLATFORM_EMU_1D:
            FpFwCliPrint("Platform ID: PLATFORM_EMU_1D\n");
            break;
        case PLATFORM_EMU_2D:
            FpFwCliPrint("Platform ID: PLATFORM_EMU_2D\n");
            break;
        case PLATFORM_EMU_1D_8C:
            FpFwCliPrint("Platform ID: PLATFORM_EMU_1D_8C\n");
            break;
        case PLATFORM_EMU_2D_8C:
            FpFwCliPrint("Platform ID: PLATFORM_EMU_2D_8C\n");
            break;
        case PLATFORM_RVP_EVT_SILICON:
            FpFwCliPrint("Platform ID: PLATFORM_RVP_EVT_SILICON\n");
            break;
        default:
            FpFwCliPrint("Platform ID: Unknown (ID: 0x%x)\n", plat_id);
            break;
    }
    FpFwCliPrint(" get_plat_id_comp\n");
    return CLI_SUCCESS;
}

/*----------------------------- Global Functions ----------------------------*/
FPFW_CLI_STATUS cli_sys_info_init(void)
{
    FpFwCliRegisterTable(s_sys_info_int_commands_table, FPFW_ARRAY_SIZE(s_sys_info_int_commands_table));
    FpFwCliPrint("cli_sys_info_init CLI complete\n");

    return CLI_SUCCESS;
}
