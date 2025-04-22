//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file boot_status.h
 *  APIs for clients to raise boot status send requests to HSP
 */

#pragma once

/*--------------- Includes ---------------*/
#include <fpfw_icc_base.h> // for fpfw_icc_base_send_recv_req_t, fpfw...
#include <hsp_firmware_headers.h>

/*-- Symbolic Constant Macros (defines) --*/
//! These values will be programmed into boot status extended field of 
//! `kng_hsp_mailbox_boot_status_extd_notify` struct
//! Generate status to display on LED, Bits 24-31 are programmed into LED
#define GEN_BOOT_STATUS_EX_LED_CODE(group, subgroup, instance, status) \
(((uint32_t)(group) & 0xFFU) | \
(((uint32_t)(subgroup) & 0xFFU) << 8U) | \
(((uint32_t)(instance) & 0xFFU) << 16U) | \
(((uint32_t)(status) & 0xFFU) << 24U))

//! Generate regular status, Bits 24-31 are set to 0
#define GEN_BOOT_STATUS_EX_GENERIC_CODE(group, subgroup, instance) \
(((uint32_t)(group) & 0xFFU) | \
(((uint32_t)(subgroup) & 0xFFU) << 8U) | \
(((uint32_t)(instance) & 0xFFU) << 16U) & 0x00FFFFFFU)

/*-------------- Typedefs ----------------*/
/**
 * @brief Optional cb to be called when the boot status send request is completed.
 * Notify the client that the send request is completed & the boot_status_req_t
 * mem can be reused.
 */
typedef void (*boot_status_send_complete_notify)(void* context);

/**
 * @brief Enum for types of accel cores
 * that send their boot status to scp
 */
typedef enum _boot_status_accel_core_type
{
    BOOT_STATUS_SDM,
    BOOT_STATUS_CDED,
    BOOT_STATUS_ACCEL_MAX,
} boot_status_accel_core_type;

/**
 * @brief Memory required to raise a sync/async boot status
 * notify request to HSP.
 * Only a single request is supported at a time, so no need
 * to maintain a list of requests
 */
typedef struct _boot_status_req_t
{
  kng_hsp_mailbox_msg msg;                  //! actual mbox payload
  fpfw_icc_base_send_req_t hsp_send_params; //! async message send params
  boot_status_send_complete_notify cb; //! Optional,client provided send complete cb
  void* cb_ctx;                           //!Optional, client provided ctx to be passed to cb
} boot_status_req_t;

/**
 * @brief Memory required to raise a async boot status
 * Populated in boot status init.
 */
typedef struct _boot_status_icc_ctx_t
{
    fpfw_icc_base_ctx_t* hsp_mbx_ctx; //! store icc ctx for HSP mbox
    //! The following is only applicable for SCP core
    struct{
      fpfw_icc_base_ctx_t* lfifo_mbx_ctx; //! store icc ctx for large fifo mbox
      fpfw_icc_base_send_req_t* send_params;//! memory for send req for large fifo mbox
      fpfw_icc_base_recv_req_t* recv_params; //! memory for recv req for large fifo mbox
    } accel_icc_ctx[BOOT_STATUS_ACCEL_MAX]; //! store icc ctx for large fifo mbox for each die
}boot_status_icc_ctx_t;

/**
 * @brief As per FW architecture, Values agreed with HSP, 
 * The component groups refers to individual domain reporting the status code
 * Ones supported by MSCP FW are listed here.
 * @note Do not change the values of the enum, as they are used in the HSP code.
 */
typedef enum _boot_status_component_group_t
{
  COMPONENT_GROUP_HSP = 0x00, // Do not use
  COMPONENT_GROUP_SCP = 0x01,
  COMPONENT_GROUP_MCP = 0x02,
  COMPONENT_GROUP_KMP = 0x02, // Do not use
  COMPONENT_GROUP_CDED = 0x04,
  COMPONENT_GROUP_SDM = 0x05,
  COMPONENT_GROUP_AP = 0x06,
} boot_status_component_group_t;

/**
 * @brief Refers to the sub-group within the domain reporting the status code.
 * These can be modules, services etc. Added some examples here to start off.
 */
typedef enum _boot_status_component_subgroup_t
{
  MSCP_GENERIC = 0x00,
  MSCP_BOOTLOADER,
  MSCP_MESH,
  MSCP_TOWER,
  MSCP_ACCEL,
  MSCP_DDR,
  MSCP_CRASH_DUMP,
  MSCP_TELEMETRY,
  MSCP_POWER,
  MSCP_HEALTH_MONITOR,
  MSCP_SUBGROUP_MAX,
} boot_status_component_subgroup_t;

/**
 * @brief Refers to the instance of the group/sub group reporting the status code.
 */
typedef enum _boot_status_component_instance_t
{
  SCP_PRIMARY = 0x00,
  SCP_SECONDARY,
  MCP_PRIMARY,
  MCP_SECONDARY,
  CDED_PRIMARY,
  CDED_SECONDARY,
  SDM_PRIMARY,
  SDM_SECONDARY,
  MAX_COMPONENT_INSTANCE,
} boot_status_component_instance_t;

/**
 * @brief Add local MSCP status codes here
 * These will be sent to HSP but not used to generate LED status codes.
 * Not to be confused with boot status codes given in boot_status_codes.h from HSP artifacts
 */
typedef enum _mscp_boot_status_codes_t
{
    MSCP_BOOT_STATUS_CODE_UNUSED = 0x0,
    //! progress codes for SCP
    MSCP_BOOT_STATUS_CODE_SCP_OK,
    MSCP_BOOT_STATUS_CODE_SCP_START,
    MSCP_BOOT_STATUS_CODE_SCP_COLD_BOOT,
    MSCP_BOOT_STATUS_CODE_SCP_WARM_BOOT,
    MSCP_BOOT_STATUS_CODE_SCP_IRQ_DISABLED,
    MSCP_BOOT_STATUS_CODE_SCP_MESH_INIT_END,
    MSCP_BOOT_STATUS_CODE_SCP_TOWER_INIT_END,
    MSCP_BOOT_STATUS_CODE_SCP_ACCEL_INIT_END,
    MSCP_BOOT_STATUS_CODE_SCP_DDR_INIT_END,
    //! Error codes for SCP
    MSCP_BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG,
    //! Keep last for SCP, do not use as post code
    MSCP_BOOT_STATUS_CODE_SCP_MAX,

    //! progress codes for MCP
    MSCP_BOOT_STATUS_CODE_MCP_OK,
    MSCP_BOOT_STATUS_CODE_MCP_START,
    MSCP_BOOT_STATUS_CODE_MCP_COLD_BOOT,
    MSCP_BOOT_STATUS_CODE_MCP_WARM_BOOT,
    MSCP_BOOT_STATUS_CODE_MCP_IRQ_DISABLED,
    //! Error codes for MCP, do not use as post code
    MSCP_BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG,
    //! Keep last for MCP
    MSCP_BOOT_STATUS_CODE_MCP_MAX,

    //! Keep last entry in this enum, do not use as post code
    MSCP_BOOT_STATUS_CODE_MAX,

} mscp_boot_status_code_t;

/**
 * @brief Led status codes for the platform agnostic to die id.
 * These are evaluated to appropriate boot status code that is 
 * used to program the LED status
 */
typedef enum _led_status_codes_t
{
  LED_STATUS_CODE_SCP_OK = 0x0,
	LED_STATUS_CODE_SCP_MESH_INIT_START,
	LED_STATUS_CODE_SCP_TOWER_INIT_START,
	LED_STATUS_CODE_SCP_ACCEL_INIT_START,
	LED_STATUS_CODE_SCP_DDR_INIT_START,
	LED_STATUS_CODE_SCP_BOOT_COMPLETE,
  //! Led status applicable for MCP
	LED_STATUS_CODE_MCP_OK,
	LED_STATUS_CODE_MCP_BOOT_COMPLETE,
  //! end of led status for MCP
	LED_STATUS_CODE_SDM_OK,
	LED_STATUS_CODE_SDM_BOOT_COMPLETE,
	LED_STATUS_CODE_CDED_OK,
	LED_STATUS_CODE_CDED_BOOT_COMPLETE,
	LED_STATUS_CODE_SCP_ACCEL_FAILED = 0x60,
	LED_STATUS_CODE_SCP_E_DDR0_TRAINING, //! die 0 -> DDR0, die 1 -> DDR6
	LED_STATUS_CODE_SCP_E_DDR1_TRAINING, //! die 0 -> DDR1, die 1 -> DDR7
	LED_STATUS_CODE_SCP_E_DDR2_TRAINING, //! die 0 -> DDR2, die 1 -> DDR8
	LED_STATUS_CODE_SCP_E_DDR3_TRAINING, //! die 0 -> DDR3, die 1 -> DDR9
	LED_STATUS_CODE_SCP_E_DDR4_TRAINING, //! die 0 -> DDR4, die 1 -> DDR10
	LED_STATUS_CODE_SCP_E_DDR5_TRAINING, //! die 0 -> DDR5, die 1 -> DDR11
	LED_STATUS_CODE_SDM_HW_FAULT,
	LED_STATUS_CODE_CDED_HW_FAULT,
  LED_STATUS_CODE_MAX,
}led_status_codes_t;

/*--------- Function Prototypes ----------*/
/**
 * @brief Called in runtime init to cache the icc base contexts.
 * 
 * @param boot_status_ctx Pointer to the boot status context to be initialized.
 */
void boot_status_init(boot_status_icc_ctx_t* boot_status_ctx);
  
/**
 * @brief Raw API to send boot status notification to HSP. 
 * Programs the params into the boot_status & boot_status_ex fields of the
 * `kng_hsp_mailbox_boot_status_extd_notify` struct and sends it to HSP.
 * 
 * @param p_req_mem Pointer to the request memory. Client must provide this memory. No need to populate
 * This memory is used to send the request to HSP. The client must ensure that this memory is not used
 * until the request is completed. The memory is freed by the client after the request is completed.
 * 
 * @param boot_status MSCP FW status to send to HSP. This status can be
 * local to the platform, for eg given by the enum mscp_boot_status_code_t. HSP doesn't 
 * care about this field, it will buffer it however.
 * 
 * @param boot_status_ex status generated via the above macros. The last 8 bits
 * of this field (status field) are used to program the LED status code if non zero. The rest of the bit fields are 
 * stored in the history buffer. 
 * Generate using `GEN_BOOT_STATUS_EX_LED_CODE` or Generate using `GEN_BOOT_STATUS_EX_GENERIC_CODE`
 * When using GEN_BOOT_STATUS_EX_LED_CODE, for status bit field use values enum given 
 * in boot_status_codes.h from hsp artifacts
 */
void boot_status_notify_extd(boot_status_req_t *p_req_mem, uint32_t boot_status, uint32_t boot_status_ex);

/**
 * @brief Simplistic API to send status to HSP to be displayed on leds.
 * @param p_req_mem Pointer to the request memory. Client must provide this memory. No need to populate.
 * @param status must be one of the status codes defined in the enum
 * given in led_status_codes_t.
 */
void post_led_status(boot_status_req_t *p_req_mem, led_status_codes_t status);

/**
 *  @brief Reset the boot status notification state.
 *  Used for Testing.
 */
void boot_status_reset(void);