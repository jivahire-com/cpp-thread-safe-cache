//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file fuse_init.h
 * Header containing definitions for initialization of fuse service
 */

#pragma once

/*----------- Nested includes ------------*/
#include <fpfw_icc_base.h>
#include <hsp_firmware_headers.h>
#include <kingsgate_hsp_mailbox_commands.h>
/*-- Symbolic Constant Macros (defines) --*/
#define FUSE_NAME                            "[KNG Fuse] "

/*-------------- Typedefs ----------------*/
enum {
    FUSE_NO_OVERRIDES                             =(-19),
    FUSE_ERROR_IGNORE_VALIDS                      =(-20),
    FUSE_ERROR_WITH_OVERRIDES                     =(-21),
    FUSE_ERROR_GET_EXCLUSION_LIST                 =(-22),
    FUSE_ERROR_DISTRIBUTION_PHASE_MAJOR3_MINOR0   =(-23),
    FUSE_ERROR_DISTRIBUTION_PHASE_MAJOR3_MINOR1   =(-24),
    FUSE_ERROR_DISTRIBUTION_PHASE_MAJOR4_MINOR0   =(-25),
    FUSE_ERROR_DISTRIBUTION_PHASE_MAJOR4_MINOR1   =(-26),
    FUSE_ERROR_DISABLE_KNOB                       =(-27)
};

typedef union _kng_hsp_fuse_mailbox_msg {
	struct kng_hsp_mailbox_cmd_load_fw_req fuse_req;	/**< incoming mailbox message from protocol to handler. */
	struct kng_hsp_mailbox_msg_rsp rsp;	        /**< outgoing mailbox message from handler to protocol. */
    uint32_t as_uint32[4];
} kng_hsp_fuse_mailbox_msg;

typedef enum {
    FUSE_DISTRIBUTION_STAGE_POST_HSP = 0,
    FUSE_DISTRIBUTION_STAGE_POST_HSP_MESH_INIT = 1,
    FUSE_DISTRIBUTION_STAGE_POST_MESH_INIT = 2,
    FUSE_DISTRIBUTION_STAGE_POST_MESH_INIT_BRIDGE_INIT = 3
} fuse_distribution_stage_t;


/*--------- Function Prototypes ----------*/
#ifdef __cplusplus
extern "C" {
#endif

void fuse_init(fpfw_icc_base_ctx_t* icc_base_ctx);
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
 *      stage (0 -> distribute to MSCP 1-> distribute from HNP to the last 2-> All distribute)
 *
 * @return FPFW_INIT_STATUS_SUCCES on success
 *
 */
int platform_fuse_distribution(fuse_distribution_stage_t stage);
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
/**
 * write fuse disable core to AP API
 *
 * Description:
 *      This API will return the value of a fuse based on provided bit offset and width within fuse data
 *
 *
 * @return FPFW_INIT_STATUS_SUCCES on success
 *
 */
int write_fuse_info_to_ap();
#ifdef __cplusplus
}
#endif