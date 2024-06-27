//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accelerator_ip_bcfg_params.h
 * Accelerator IP Boot config specific.
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/

/*------------------- Symbolic Constant Macros (defines) --------------------*/
#define BCFG_BOOT_CFG_SDM_CTRL_DTI_V3_PROTOCOL         (0x1)
#define BCFG_BOOT_CFG_SDM_CTRL_PCIE_SID_SEGMENT_ID     (0x00)
#define BCFG_BOOT_CFG_SDM_CTRL_PF_CRS_RESP             (0x00)
#define BCFG_BOOT_CFG_SDM_CTRL_VF_CRS_RESP_DATA        (0x00)

#define BCFG_BOOT_CFG_TID_CTRL_SMMU_DTI_TID            (0x00)
#define BCFG_BOOT_CFG_TID_CTRL_GIC_TID                 (0x00)

#define BCFG_BOOT_CFG_PF_TYPE0_CLASS_CODE_PROG_INTF             (0x00)
#define BCFG_BOOT_CFG_PF_TYPE0_CLASS_CODE_SUB_CLASS_CODE        (0x00)
#define BCFG_BOOT_CFG_PF_TYPE0_CLASS_CODE_BASE_CLASS_CODE       (0x12)

#define BCFG_BOOT_CFG_RCEC_TYPE0_CLASS_CODE_PROG_INTF           (0x00)
#define BCFG_BOOT_CFG_RCEC_TYPE0_CLASS_CODE_SUB_CLASS_CODE      (0x07)
#define BCFG_BOOT_CFG_RCEC_TYPE0_CLASS_CODE_BASE_CLASS_CODE     (0x08)

#define BCFG_BOOT_CFG_PF_TYPE0_SDM_ID_VEN_ID                (0x1414)
#define BCFG_BOOT_CFG_PF_TYPE0_SDM_ID_DEV_ID                (0xC010)
#define BCFG_BOOT_CFG_PF_TYPE0_SS_ID_SUBSYS_VENDOR_ID       (0x00)  // TODO (ADO 1728266): value need to be updated once decided
#define BCFG_BOOT_CFG_PF_TYPE0_SS_ID_SUBSYS_ID              (0x00)  // TODO (ADO 1728266): value need to be updated once decided
#define BCFG_BOOT_CFG_PF_TYPE0_REV_REV_ID                   (0x00)  // TODO (ADO 1728266): value need to be updated once decided
#define BCFG_BOOT_CFG_PF_TYPE0_INTR_PIN                     (0x01)

#define BCFG_BOOT_CFG_RCEC_TYPE0_SDM_ID_VEN_ID              (0x1414)
#define BCFG_BOOT_CFG_RCEC_TYPE0_SDM_ID_DEV_ID              (0xC012)
#define BCFG_BOOT_CFG_RCEC_TYPE0_SS_ID_SUBSYS_VENDOR_ID     (0x00)  // TODO (ADO 1728266): value need to be updated once decided
#define BCFG_BOOT_CFG_RCEC_TYPE0_SS_ID_SUBSYS_ID            (0x00)  // TODO (ADO 1728266): value need to be updated once decided
#define BCFG_BOOT_CFG_RCEC_TYPE0_REV_REV_ID                 (0x00)  // TODO (ADO 1728266): value need to be updated once decided
#define BCFG_BOOT_CFG_RCEC_TYPE0_INTR_PIN                   (0x02)

/*
 * Boot vector for emCPU - to be programmed in initvtor prior to M7 reset
 * deassertion.
 */
#define DIE0_SDMSS_INSTANCE0_INT_VECTOR                     (0x80000)
#define DIE0_CDEDSS_INSTANCE0_INT_VECTOR                    (0x80000)

/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/


