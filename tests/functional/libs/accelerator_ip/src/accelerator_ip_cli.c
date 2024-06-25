//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accelerator_ip_cli.c
 * This file provides CLI interface to test the Accelerator IP test code from SCP
 */


/*----------------------------- Nested includes -----------------------------*/
#include <accelerator_ip_cli.h>
#include <atu_lib.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <silibs_ap_top_regs.h>
#include <stdio.h>
#include <stdlib.h>
#include <vab_sdm_top_regs.h>


/*-------------------- Symbolic Constant Macros (defines) -------------------*/
#define MAX_NUM_OF_DIE (2)
#define MAX_CMDLINE_ARG (2)

// Do not update if _result is already false
#define UPDATE_TEST_RESULT(_cond, _result)     (_result = (_result) ? ((_cond) ? true : false) : false)

/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

static atu_map_entry_t sdm_vab_atu_map_entry[MAX_NUM_OF_DIE] = {
    // D0-SDMSS0
    {
        .ap_base_address = AP_TOP_D0_VAB_SDM_ADDRESS + VAB_SDM_TOP_SDMSS_ADDRESS,
        .mscp_start_address = 0,
        .mscp_end_address = AP_TOP_D0_VAB_SDM_SIZE - 1,
        .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT},
    },
    // D1-SDMSS0
    {
        .ap_base_address = AP_TOP_D1_VAB_SDM_ADDRESS + VAB_SDM_TOP_SDMSS_ADDRESS,
        .mscp_start_address = 0,
        .mscp_end_address = AP_TOP_D1_VAB_SDM_SIZE - 1,
        .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT},
    }
};

/*----------------------------- Static Functions ----------------------------*/

/**
 * Map ATU and return the SCP mapped address
 */
static uintptr_t init_test(uint32_t die_id)
{
    atu_map(ATU_ID_MSCP, &sdm_vab_atu_map_entry[die_id]);

   return sdm_vab_atu_map_entry[die_id].mscp_start_address;
}

/**
 * Destroy ATU mapping
 */
static void clean_test(uint32_t die_id)
{
    atu_unmap(ATU_ID_MSCP, &sdm_vab_atu_map_entry[die_id]);
}

/**
 * Function to read specified register and check with expected value.
 * If result mactch, test pass else fail
 */
static bool check_result(uintptr_t reg_addr, char *reg_name, uint32_t expected_value, const char *fun_name)
{
    uint32_t result;
    uint32_t read_value;

    // Read register value
    read_value = MMIO_READ32(reg_addr);

    // Compare read value with expected value
    result = (read_value == expected_value) ? true : false;

    // TODO : Need to remove printf with CLI/TestFW compatible print function
    printf("Func: %s, Reg: %s, Read value = 0x%x, Expected value = 0x%x, Result = %s\n",
        fun_name, reg_name, (unsigned int)read_value, (unsigned int)expected_value, (result ? "Pass" : "Fail"));

    return result;
}

/**
 * Function to test RCEIP type0 bringup
 */
static bool scp_sdm_rciep_type0_header_test(uint32_t die_id)
{
    uintptr_t base_addr;
    uintptr_t addrblock_base;
    uintptr_t reg_addr;
    int32_t result = true;

    // ATU mapped base address
    base_addr = init_test(die_id);
    addrblock_base = base_addr + SDM_EXT_CFG__ADDRESSBLOCK_0X100000_ADDRESS;

    reg_addr = addrblock_base + _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_PF_TYPE0_SDM_ID_ADDRESS;
    UPDATE_TEST_RESULT(check_result(reg_addr, "BCFG_BOOT_CFG_PF_TYPE0_SDM_ID", SDM_BCFG_BOOT_CFG_PF_TYPE0_SDM_ID_EXPECTED_VALUE, __func__), result);

    reg_addr = addrblock_base + _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_PF_TYPE0_SS_ID_ADDRESS;
    UPDATE_TEST_RESULT(check_result(reg_addr, "BCFG_BOOT_CFG_PF_TYPE0_SS_ID", SDM_BCFG_BOOT_CFG_PF_TYPE0_SS_ID_EXPECTED_VALUE, __func__), result);

    reg_addr = addrblock_base + _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_PF_TYPE0_REV_ADDRESS;
    UPDATE_TEST_RESULT(check_result(reg_addr, "BCFG_BOOT_CFG_PF_TYPE0_REV", SDM_BCFG_BOOT_CFG_PF_TYPE0_REV_EXPECTED_VALUE, __func__), result);

    reg_addr = addrblock_base + _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_PF_TYPE0_CLASS_CODE_ADDRESS;
    UPDATE_TEST_RESULT(check_result(reg_addr, "BCFG_BOOT_CFG_PF_TYPE0_CLASS_CODE", SDM_BCFG_BOOT_CFG_PF_TYPE0_CLASS_CODE_EXPECTED_VALUE, __func__), result);

    reg_addr = addrblock_base + _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_PF_TYPE0_INTR_ADDRESS;
    UPDATE_TEST_RESULT(check_result(reg_addr, "BCFG_BOOT_CFG_PF_TYPE0_INTR", SDM_BCFG_BOOT_CFG_PF_TYPE0_INTR_EXPECTED_VALUE, __func__), result);

    clean_test(die_id);

    return result;
}

/**
 * Function to test RCEC type0 bringup
 */
static bool scp_sdm_rcec_type0_header_test(uint32_t die_id)
{
    uintptr_t base_addr;
    uintptr_t addrblock_base;
    uintptr_t reg_addr;
    int32_t result = true;

    base_addr = init_test(die_id);
    addrblock_base = base_addr + SDM_EXT_CFG__ADDRESSBLOCK_0X100000_ADDRESS;

    reg_addr = addrblock_base + _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_RCEC_TYPE0_SDM_ID_ADDRESS;
    UPDATE_TEST_RESULT(check_result(reg_addr, "BCFG_BOOT_CFG_RCEC_TYPE0_SDM_ID", SDM_BCFG_BOOT_CFG_RCEC_TYPE0_SDM_ID_EXPECTED_VALUE, __func__), result);

    reg_addr = addrblock_base + _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_RCEC_TYPE0_SS_ID_ADDRESS;
    UPDATE_TEST_RESULT(check_result(reg_addr, "BCFG_BOOT_CFG_RCEC_TYPE0_SS_ID", SDM_BCFG_BOOT_CFG_RCEC_TYPE0_SS_ID_EXPECTED_VALUE, __func__), result);

    reg_addr = addrblock_base + _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_RCEC_TYPE0_REV_ADDRESS;
    UPDATE_TEST_RESULT(check_result(reg_addr, "BCFG_BOOT_CFG_RCEC_TYPE0_REV", SDM_BCFG_BOOT_CFG_RCEC_TYPE0_REV_EXPECTED_VALUE, __func__), result);

    reg_addr = addrblock_base + _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_RCEC_TYPE0_CLASS_CODE_ADDRESS;
    UPDATE_TEST_RESULT(check_result(reg_addr, "BCFG_BOOT_CFG_RCEC_TYPE0_CLASS_CODE", SDM_BCFG_BOOT_CFG_RCEC_TYPE0_CLASS_CODE_EXPECTED_VALUE, __func__), result);

    reg_addr = addrblock_base + _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_RCEC_TYPE0_INTR_ADDRESS;
    UPDATE_TEST_RESULT(check_result(reg_addr, "BCFG_BOOT_CFG_RCEC_TYPE0_INTR", SDM_BCFG_BOOT_CFG_RCEC_TYPE0_INTR_EXPECTED_VALUE, __func__), result);

    clean_test(die_id);

    return result;
}

/**
 * Function to test SDM ECAM bringup
 */
static bool scp_sdm_ecam_access_test(uint32_t die_id)
{
    uintptr_t base_addr;
    uintptr_t addrblock_base;
    uintptr_t reg_addr;
    int32_t result = true;

    base_addr = init_test(die_id);
    addrblock_base = base_addr + SDM_EXT_CFG__ADDRESSBLOCK_0X100000_ADDRESS;

    reg_addr = addrblock_base + _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_ECAM_ADDRESS;
    UPDATE_TEST_RESULT(check_result(reg_addr, "BCFG_BOOT_CFG_ECAM", SDM_BCFG_BOOT_CFG_ECAM_EXPECTED_VALUE, __func__), result);

    reg_addr = addrblock_base + _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_BDF_ADDRESS;
    UPDATE_TEST_RESULT(check_result(reg_addr, "BCFG_BOOT_CFG_BDF", SDM_BCFG_BOOT_CFG_BDF_EXPECTED_VALUE, __func__), result);

    reg_addr = addrblock_base + _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_RCEC_BDF_ADDRESS;
    UPDATE_TEST_RESULT(check_result(reg_addr, "BCFG_BOOT_CFG_RCEC_BDF", SDM_BCFG_BOOT_CFG_RCEC_BDF_EXPECTED_VALUE, __func__), result);

    reg_addr = addrblock_base + _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_SDM_CTRL_ADDRESS;
    UPDATE_TEST_RESULT(check_result(reg_addr, "BCFG_BOOT_CFG_SDM_CTRL", SDM_BCFG_BOOT_CFG_SDM_CTRL_EXPECTED_VALUE, __func__), result);

    clean_test(die_id);

    return result;
}

/**
 * Function to test SDM emCPU bringup
 */
static bool scp_sdm_emcpu_init_test(uint32_t die_id)
{
    uintptr_t base_addr;
    uintptr_t addrblock_base;
    uintptr_t reg_addr;
    int32_t result = true;

    base_addr = init_test(die_id);
    addrblock_base = base_addr + SDM_EXT_CFG__ADDRESSBLOCK_0X100000_ADDRESS;

    reg_addr = addrblock_base + _ADDRESSBLOCK_0X100000_EMCPU_CFG_INITVTOR_ADDRESS;
    UPDATE_TEST_RESULT(check_result(reg_addr, "EMCPU_CFG_INITVTOR", SDM_EMCPU_CFG_INITVTOR_EXPECTED_VALUE, __func__), result);

    reg_addr = addrblock_base + _ADDRESSBLOCK_0X100000_EMCPU_CFG_RST_CTRL_ADDRESS;
    UPDATE_TEST_RESULT(check_result(reg_addr, "EMCPU_CFG_RST_CTRL", SDM_EMCPU_CFG_RST_CTRL_EXPECTED_VALUE, __func__), result);

    reg_addr = addrblock_base + _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_MEM_INIT_ADDRESS;
    UPDATE_TEST_RESULT(check_result(reg_addr, "BCFG_BOOT_CFG_MEM_INIT", SDM_BCFG_BOOT_CFG_MEM_INIT_EXPECTED_VALUE, __func__), result);

    reg_addr = addrblock_base + _ADDRESSBLOCK_0X100000_EMCPU_CFG_TCM_CTRL_ADDRESS;
    UPDATE_TEST_RESULT(check_result(reg_addr, "EMCPU_CFG_TCM_CTRL", SDM_EMCPU_CFG_TCM_CTRL_EXPECTED_VALUE, __func__), result);

    reg_addr = addrblock_base + _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_ECC_DIS_ADDRESS;
    UPDATE_TEST_RESULT(check_result(reg_addr, "BCFG_BOOT_CFG_ECC_DIS", SDM_BCFG_BOOT_CFG_ECC_DIS_EXPECTED_VALUE, __func__), result);

    reg_addr = addrblock_base + _ADDRESSBLOCK_0X100000_EMCPU_CFG_CPUWAIT_ADDRESS;
    UPDATE_TEST_RESULT(check_result(reg_addr, "EMCPU_CFG_CPUWAIT", SDM_EMCPU_CFG_CPUWAIT_EXPECTED_VALUE, __func__), result);

    clean_test(die_id);

    return result;
}

/*----------------------------- Global Functions ----------------------------*/

/**
 * This API will be connected with CLI interface for
 * Accelerator IP Bringup testing
 *
 */
int32_t accelerator_ip_bringup_test(int32_t argc, char **argv)
{
    uint32_t result = true;
    KNG_DIE_ID die_id = idsw_get_die_id();

    // 1st argument fn name, 2nd test type
    if (argc != MAX_CMDLINE_ARG)
    {
        // TODO: Display CLI cmd help (when CLI is integrated)
        return false;
    }

    switch((eACCELERATOR_IP_BRINGUP_TEST_ID)atoi(argv[1]))
    {
        case RCIPE_TYPE0_HEADER_TEST:
            UPDATE_TEST_RESULT(scp_sdm_rciep_type0_header_test(die_id), result);
            break;
        case RCEC_TYPE0_HEADER_TEST:
            UPDATE_TEST_RESULT(scp_sdm_rcec_type0_header_test(die_id), result);
            break;
        case ECAM_ACCESS_TEST:
            UPDATE_TEST_RESULT(scp_sdm_ecam_access_test(die_id), result);
            break;
        case EMCPU_INIT_TEST:
            UPDATE_TEST_RESULT(scp_sdm_emcpu_init_test(die_id), result);
            break;
        case ALL_TEST:
            UPDATE_TEST_RESULT(scp_sdm_rciep_type0_header_test(die_id), result);
            UPDATE_TEST_RESULT(scp_sdm_rcec_type0_header_test(die_id), result);
            UPDATE_TEST_RESULT(scp_sdm_ecam_access_test(die_id), result);
            UPDATE_TEST_RESULT(scp_sdm_emcpu_init_test(die_id), result);
            break;
        default:
            /* should not come here */
            result = false;
    }

    return result;
}
