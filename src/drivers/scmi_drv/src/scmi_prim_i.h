//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file scmi_prim_i.h
 *  SCMI primitive support internal header file. 
 */

#pragma once

/*--------------- Includes ---------------*/
#include <DbgPrint.h>
#include <silibs_platform.h>
#include <kng_scmi_shared.h>
#include <stdio.h>

/*--------- Macro Definitions ------------*/
#define MODULE_NAME_SCMI "[SCMI] "
#define NEWLINE     "\n"
#define SCMI_LOG_INFO(fmt, ...) FPFW_DBGPRINT_INFO(MODULE_NAME_SCMI fmt NEWLINE, ##__VA_ARGS__)
#define SCMI_LOG_ERR(fmt, ...) FPFW_DBGPRINT_ERROR(MODULE_NAME_SCMI fmt NEWLINE, ##__VA_ARGS__)

/**
 * @brief API to process and check the message.
 *
 * @param packet - SCMI ICC Formated packet
 * @return SCMI_PROTOCOL_CMD_SUCCESS - when command with parameters are successfully processed
 */
int scmi_check_message(scmi_icc_packet_t* packet);