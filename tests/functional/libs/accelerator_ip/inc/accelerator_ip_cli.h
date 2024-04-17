//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accelerator_ip_cli.h
 * This file provides CLI interface to test the Accelerator IP test code from SCP
 */

#pragma once

#ifndef _ACCELERATOR_IP_CLI_H_
#define _ACCELERATOR_IP_CLI_H_

/*----------------------------- Nested includes -----------------------------*/
#include <stdint.h>

/*-------------------- Symbolic Constant Macros (defines) -------------------*/
/* RCIEP Type0 related */
#define SDM_BCFG_BOOT_CFG_PF_TYPE0_SDM_ID_EXPECTED_VALUE        (0xc0101414) // Ven id & Dev id
#define SDM_BCFG_BOOT_CFG_PF_TYPE0_SS_ID_EXPECTED_VALUE         (0x00000000) // TODO: need to update once value decided
#define SDM_BCFG_BOOT_CFG_PF_TYPE0_REV_EXPECTED_VALUE           (0x00000000) // TODO: need to update once value decided
#define SDM_BCFG_BOOT_CFG_PF_TYPE0_CLASS_CODE_EXPECTED_VALUE    (0x00120000)
#define SDM_BCFG_BOOT_CFG_PF_TYPE0_INTR_EXPECTED_VALUE          (0x00000001)

/* RCEC Type0 related */
#define SDM_BCFG_BOOT_CFG_RCEC_TYPE0_SDM_ID_EXPECTED_VALUE      (0xC0121414) // Ven id & Dev id
#define SDM_BCFG_BOOT_CFG_RCEC_TYPE0_SS_ID_EXPECTED_VALUE       (0x00000000) // TODO: need to update once value decided
#define SDM_BCFG_BOOT_CFG_RCEC_TYPE0_REV_EXPECTED_VALUE         (0x00000000) // TODO: need to update once value decided
#define SDM_BCFG_BOOT_CFG_RCEC_TYPE0_CLASS_CODE_EXPECTED_VALUE  (0x00080700)
#define SDM_BCFG_BOOT_CFG_RCEC_TYPE0_INTR_EXPECTED_VALUE        (0x00000002)

 /* ECAM related */
#define SDM_BCFG_BOOT_CFG_ECAM_EXPECTED_VALUE                   (0x00000400)
#define SDM_BCFG_BOOT_CFG_BDF_EXPECTED_VALUE                    (0x0000F800)
#define SDM_BCFG_BOOT_CFG_RCEC_BDF_EXPECTED_VALUE               (0x0000F900)
#define SDM_BCFG_BOOT_CFG_SDM_CTRL_EXPECTED_VALUE               (0x081FE183)

/* SDM emCPU related */
#define SDM_EMCPU_CFG_INITVTOR_EXPECTED_VALUE                   (0x00000000) // TODO: Need to update once value decided
#define SDM_EMCPU_CFG_RST_CTRL_EXPECTED_VALUE                   (0x00000001) //Fence disabled, nsysreset released
#define SDM_BCFG_BOOT_CFG_MEM_INIT_EXPECTED_VALUE               (0x00000000)
#define SDM_EMCPU_CFG_TCM_CTRL_EXPECTED_VALUE                   (0x00000707)
#define SDM_BCFG_BOOT_CFG_ECC_DIS_EXPECTED_VALUE                (0x00000000)
#define SDM_EMCPU_CFG_CPUWAIT_EXPECTED_VALUE                    (0x00000000)

/*-------------------------------- Typedefs ---------------------------------*/

typedef enum
{
    RCIPE_TYPE0_HEADER_TEST = 0,
    RCEC_TYPE0_HEADER_TEST,
    ECAM_ACCESS_TEST,
    EMCPU_INIT_TEST,
    ALL_TEST,
    MAX_TEST
} eACCELERATOR_IP_BRINGUP_TEST_ID;

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/
/**
 * @brief Accelerator IP Bringup Test
 *
 * \b Description:
 * SCP will do the configure for SDM/CDED before pre-int. This
 * test case will make sure that the configuration has been done
 * as per the requirement.
 *
 * @param argc Total number of cmd line argument to the function.
 * @param argv List of arguments. Expected are: 1st function name,
 *              2nd  eACCELERATOR_IP_BRINGUP_TEST_ID test type
 *
 * @retval true if all tests pass else false.
 */
int32_t accelerator_ip_bringup_test(int32_t argc, char **argv);

#endif /*_ACCELERATOR_IP_CLI_H_ */
