//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file mesh.h
 *  Mesh Definitions
 */

#pragma once

/*--------------- Includes ---------------*/

#define MESH_INFO(...) INFO_PRINT("[CMN800] " __VA_ARGS__)
#define MESH_DBG(...) DEBUG_PRINT("[CMN800] " __VA_ARGS__)
#define MESH_ERR(...)  CRITICAL_PRINT("[CMN800] " __VA_ARGS__)
#define MESH_CRIT(...) CRITICAL_PRINT("[CMN800] " __VA_ARGS__)

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
