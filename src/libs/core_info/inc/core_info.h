//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 *  @file core_info.h
 *  APIs for clients to get system information
 */
#pragma once
/*--------------- Includes ---------------*/
#include <corebits.h>
#include <fpfw_icc_base.h> // for fpfw_icc_base_send_recv_req_t, fpfw...
#include <idsw.h>          // for idsw_get_platform_sdv,
#include <idsw_kng.h>      // for idsw_get_platform_sdv,
#include <stdbool.h>
/*-------------- Typedefs ----------------*/

/**
 * @brief Combined core identifier containing die ID and core ID.
 *        - Bits 31:16: die ID
 *        - Bits 15:0:  core ID
 */
typedef union {
    struct {
        uint16_t core_id;  // bits 15:0 (LSB)
        uint16_t die_id;   // bits 31:16 (MSB)
    };
    uint32_t combined_id;
} etc_core_identifier_t;

/*--------- Function Prototypes ----------*/
/**
  * @brief Retrieves the disable corebits The disable corebits is to show the core0 
  *     .  to core67 enable or disable condition
  *
  * @return The corebits .
  */
 void core_info_get_platform_disable_cores();

 corebits_t *core_info_get_enable_cores_result();
 void core_info_init();

 /**
  * @brief Retrieves the combined core identifier which includes both die ID and core ID. The format of the combined ID is as follows:
  * - Bits 31:16 represent the die ID
  * - Bits 15:0 represent the core ID
  * This combined core identifier provides a unique identifier for each core in the system, allowing clients to easily determine both the die and core information from a single value.
  * 
  * @return The combined core identifier, with bits 31:16 representing the die ID and bits 15:0 representing the core ID.
  */
 uint32_t core_info_get_combined_core_id();