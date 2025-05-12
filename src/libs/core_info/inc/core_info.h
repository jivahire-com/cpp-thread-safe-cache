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