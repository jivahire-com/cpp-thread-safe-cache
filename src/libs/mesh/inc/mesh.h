//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file mesh.h
 *  Calls the Mesh Sequence Library for KNG SOC 
 */

#pragma once

/*--------------- Includes ---------------*/

/**
 * @brief Initializes the mesh.
 *
 * This function initializes the mesh with the specified die number and mailbox primitive context.
 *
 * @param die_num The die number.
 * @param hsp_mbx_ctx The mailbox primitive context.
 */
void mesh_init(uint8_t die_num, FPFW_MBX_PRIMITIVE_CTX* hsp_mbx_ctx);
