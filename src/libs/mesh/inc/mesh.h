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
#include <numa_config_variable.h>

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
 * @brief Print the NUMA Data Structure that is filled out
 *
 * @param numa_cfg Pointer to the numa cfg to print
 *
 * @return void
 */
void print_numa_info(NUMA_CFG* numa_cfg);
