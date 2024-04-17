//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accelerator_ip_pcie_params.h
 * Accelerator IP pcie params specific.
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/
#include <stdint.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/
typedef struct {
    uint8_t bus;
    uint8_t dev;
    uint8_t func;
} pcie_bdf_t;

typedef struct {
    uint8_t prog_intf;
    uint8_t sub_class_code;
    uint8_t base_class_code; 
} pcie_class_code_t;

typedef struct {
    uint32_t dti_v3_protocol : 1;
    uint32_t pcie_sid_segment_id : 2;
    uint32_t pf_crs_resp_data : 1;
    uint32_t vf_crs_resp_data : 1;
} pcie_ctrl_reg_t;

typedef struct {
    uint8_t smmu_dti_tid;
    uint8_t gic_tid;
} pcie_tid_ctrl_reg_t;

typedef struct {
    uint16_t            vendor_id;
    uint16_t            device_id;
    uint16_t            ss_vendor_id;
    uint16_t            subsystem_id;
    uint8_t             revision_id;
    pcie_class_code_t   *class_code;
    uint8_t             intr_pin;
} pcie_type0_ctxt_t;

typedef struct {
    pcie_type0_ctxt_t   *p_rciep_type0_ctxt;
    pcie_type0_ctxt_t   *p_rcec_type0_ctxt; 
    pcie_ctrl_reg_t     *ctrl_reg;
    pcie_tid_ctrl_reg_t *tid_ctrl;
    pcie_bdf_t          *p_rciep_bdf;
    pcie_bdf_t          *p_rcec_bdf;
} accelip_pcie_ctxt_t;

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/
