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
 * @param icc_ctx The mailbox primitive context.
 */
void mesh_init(uint8_t die_num, fpfw_icc_base_ctx_t* icc_ctx);
