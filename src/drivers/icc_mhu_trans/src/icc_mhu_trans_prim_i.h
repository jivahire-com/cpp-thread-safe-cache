//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file icc_mhu_trans_prim_i.h
 *  ICC MHU primitive support internal. 
 */

#pragma once

#include <icc_mhu.h>
#include <icc_mhu_cfg.h>

#define MODULE_NAME_ICC "[ICC MHU] "
#define NEWLINE     "\n"
#define ICC_MHU_LOG_INFO(fmt, ...) printf(MODULE_NAME_ICC fmt NEWLINE, ##__VA_ARGS__)

/**
 * @brief API to load the configuration table based on the core and platform to use
 *
 * @param pConfig ipointer to the configuration table
 * @return ICC_MHU_TRANS_STATUS
 */
int icc_mhu_trans_set_configuration_table(icc_mhu_configuration_t* pConfig, uint8_t number_of_channel_cfg);

/**
 * @brief API to load enable the interrupt status of each channel
 *
 * @param pConfig ipointer to the configuration table
 * @return ICC_MHU_STATUS_SUCCESS
 */
int icc_mhu_trans_enable_interrupts();

/**
 * @brief API to get message from MHU interface channel id.
 *
 * @param mhu_interface_id interface channel id that specifies the source id of the current core and destination core
 * @param message pointer to the datastructure to return if a message is received, should not pass a NULL
 * @return ICC_MHU_TRANS_STATUS
 */
int icc_mhu_trans_get_message(uint16_t mhu_interface_id, picc_msg_receive_t message);

/**
 * @brief API to provide configuration address and number of configs
 *
 * @param properties
 * @return none
 */
void icc_mhu_trans_get_config_entries(icc_mhu_properties_t* properties);