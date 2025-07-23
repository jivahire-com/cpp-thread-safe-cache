//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file mesh.h
 *  Mesh Definitions
 */

#pragma once

/*--------------- Includes ---------------*/
#include <DbgPrint.h>
#include <cper.h>
#include <numa_config_variable.h>
#include "shared_sds_def.h"

#define MESH_INFO(...) FPFW_DBGPRINT_INFO("[CMN800] " __VA_ARGS__)
#define MESH_DBG(...) FPFW_DBGPRINT_VERBOSE("[CMN800] " __VA_ARGS__)
#define MESH_ERR(...)  FPFW_DBGPRINT_ERROR("[CMN800] " __VA_ARGS__)
#define MESH_CRIT(...) FPFW_DBGPRINT_ALWAYS("[CMN800] " __VA_ARGS__)

#define PRINT64_HEX(a) (uint32_t)((a) >> 32), (uint32_t)((a) & 0xFFFFFFFF)

#define MESH_HNI_CFG_CTL_MAX            6
#define MESH_HNI_AUX_CTL_MAX            6
#define MESH_RNI_CFG_CTL_MAX            8
#define MESH_RNI_AUX_CTL_MAX            8
#define MESH_RND_CFG_CTL_MAX            7
#define MESH_RND_AUX_CTL_MAX            7
#define MESH_RNI_QOS_CFG_MAX            3
#define MESH_RND_QOS_CFG_MAX            3
#define MESH_MXP_QOS_CFG_MAX            3

typedef enum{
    DDRSS_0_MC0 = 0,
    DDRSS_0_MC1 = 1,
    DDRSS_1_MC2 = 2,
    DDRSS_1_MC3 = 3,
    DDRSS_2_MC4 = 4,
    DDRSS_2_MC5 = 5,
    DDRSS_3_MC6 = 6,
    DDRSS_3_MC7 = 7,
    DDRSS_4_MC8 = 8,
    DDRSS_4_MC9 = 9,
    DDRSS_5_MC10 = 10,
    DDRSS_5_MC11 = 11,
    DDRSS_6_MC12 = 12,
    DDRSS_6_MC13 = 13,
    DDRSS_7_MC14 = 14,
    DDRSS_7_MC15 = 15,
    DDRSS_8_MC16 = 16,
    DDRSS_8_MC17 = 17,
    DDRSS_9_MC18 = 18,
    DDRSS_9_MC19 = 19,
    DDRSS_10_MC20 = 20,
    DDRSS_10_MC21 = 21,
    DDRSS_11_MC22 = 22,
    DDRSS_11_MC23 = 23,
    DDRSS_MAX = 24,
} ddrss_mc_id_t;

// 0xFFFFFFFC_03000000_00000000
#define MESH_DEFAULT_HNS_FUSES_31_0  0x00000000
#define MESH_DEFAULT_HNS_FUSES_63_32 0x03000000
#define MESH_DEFAULT_HNS_FUSES_95_64 0xFFFFFFFC
/**
 * @brief Initializes the mesh.
 *
 * This function initializes the mesh with the specified die number and mailbox primitive context.
 *
 * @param die_num The die number.
 * @param icc_ctx The mailbox primitive context.
 */
void mesh_init(uint8_t die_num, fpfw_icc_base_ctx_t* icc_ctx);

/**
 * @brief Initializes the D2D.
 *
 * This function initializes the D2D with the specified die number and mailbox primitive context.
 *
 * @param die_num The die number.
 * @param icc_ctx The mailbox primitive context.
 */
void d2d_init(uint8_t die_num, fpfw_icc_base_ctx_t* icc_ctx);

/**
 * @brief Print the NUMA Data Structure that is filled out
 *
 * @param numa_cfg Pointer to the numa cfg to print
 *
 * @return void
 */
void print_numa_info(NUMA_CFG* numa_cfg);

/*!
 * @brief API to get the HNS Defect Vector from Fuses
 *
 * @retval int Success or Failure
 */
int mesh_get_hns_sds_vector_from_hns_sparring(kng_hns_fuses_t *hns_fuses_sds);
