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

/**
 * @brief Initializes the mesh.
 *
 * This function initializes the mesh with the specified die number and mailbox primitive context.
 *
 * @param die_num The die number.
 * @param icc_ctx The mailbox primitive context.
 */
void mesh_init(uint8_t die_num, fpfw_icc_base_ctx_t* icc_ctx);
