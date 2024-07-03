//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file fuse_init.h
 * Header containing definitions for initialization of fuse service
 */

#pragma once

/*----------- Nested includes ------------*/

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
#ifdef __cplusplus
extern "C" {
#endif

void fuse_init();
/**
 * Read fuse API
 *
 * Description:
 *      This API will return the value of the fuse override result.
 *
 *
 * @return FPFW_INIT_STATUS_SUCCES on success
 *
 */
int platform_fuse_override(void);
/**
 * Read fuse API
 *
 * Description:
 *      This API will return the value of the fuse override result.
 *
 *
 * @return FPFW_INIT_STATUS_SUCCES on success
 *
 */
int platform_fuse_distribution(void);
/**
 * Read fuse API
 *
 * Description:
 *      This API will return the value of a fuse based on provided bit offset and width within fuse data
 *
 * @param[in] fuse_store_address - Address to store fuse value, location size must be greater than or equal to fuse_bit_width 
 * @param[in] fuse_bit_offset - Bit offset of fuse (from header)
 * @param[in] fuse_bit_width - Bit width of fuse (from header, expect max 64)
 *
 * @return FPFW_INIT_STATUS_SUCCES on success
 *
 */
int platform_read_for_fuse(const uintptr_t , const uint64_t , const uint32_t );
#ifdef __cplusplus
}
#endif