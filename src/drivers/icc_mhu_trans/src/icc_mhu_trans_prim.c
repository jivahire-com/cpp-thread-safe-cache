//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file icc_mhu_trans_prim.c
 *  This modules supports icc mhu transport primitives (non dfwk)
 */

/*------------- Includes -----------------*/
#include "icc_mhu_trans_prim_i.h"

#include <FpFwAssert.h>
#include <FpFwCli.h> // for FpFwCliPrint, _FPFW_CLI_STATUS, FPF...
#include <arm_intrinsic.h>
#include <debug.h>
#include <icc_mhu.h>
#include <icc_mhu_cfg.h>
#include <icc_mhu_trans_prim.h>
#include <idhw.h>
#include <kng_icc_shared.h>
#include <kng_scmi_shared.h>
#include <kng_scp_tfa_shared.h>
#include <mhuv3.h>

icc_mhu_properties_t properties = {
    .pIcc_configuration_table = NULL,
    .number_of_channel_cfg = 0,
};

/*------------- Functions Prototypes----------------*/
// TODO - Put notes in this ADO to remove this on the next PR https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1573706
picc_mhu_configuration_t get_configuration_address(uint16_t mhu_interface_id);

/*------------- Functions ----------------*/

//
// PRIVATE HEADER FUNCTIONS
//

size_t icc_mhu_trans_get_buffer_size(uint16_t core_2_core_id)
{
    size_t size = 0;
    uint8_t index = icc_mhu_trans_get_index_from_core_id(core_2_core_id);
    if (index != (uint8_t)ICC_MHU_TRANS_INVALID_INDEX)
    {
        icc_mhu_configuration_t* icc_config = properties.pIcc_configuration_table;
        size = icc_config[index].mailbox_size;
    }

    return size;
}

uint8_t icc_mhu_trans_get_index_from_core_id(uint16_t core_2_core_id)
{
    uint8_t index_return = (uint8_t)ICC_MHU_TRANS_INVALID_INDEX;
    icc_mhu_configuration_t* icc_config = properties.pIcc_configuration_table;
    for (uint8_t index = 0; index < properties.number_of_channel_cfg; index++)
    {
        if (icc_config[index].channel_id == core_2_core_id)
        {
            index_return = index;
            break;
        }
    }

    return index_return;
}

int icc_mhu_trans_set_configuration_table(icc_mhu_configuration_t* pConfig, uint8_t number_of_channel_cfg)
{
    // Check if the channel valid
    if (pConfig == NULL || number_of_channel_cfg == 0)
    {
        return ICC_MHU_INVALID_PARAMETER;
    }

    int status = icc_mhu_set_configuration_table(pConfig, number_of_channel_cfg);

    properties.pIcc_configuration_table = pConfig;
    properties.number_of_channel_cfg = number_of_channel_cfg;
    return status;
}

int icc_mhu_trans_get_message(uint16_t mhu_interface_id, picc_msg_receive_t message)
{
    int status = icc_mhu_get_message(mhu_interface_id, message);
    if (status == ICC_MHU_STATUS_SUCCESS)
    {
        // since this returns indicating a message received, check the protocol_type
        picc_mhu_configuration_t pIcc_cfg = get_configuration_address(mhu_interface_id);
        if (pIcc_cfg->icc_channel_info.protocol_type == ICC_PROTOCOL_TYPE_SCMI_ON_ICC)
        {
            // Since we are receiving a message from an SCMI ICC then we need to set the status SCMI Status flag
            picc_mhu_packet_t packet = (picc_mhu_packet_t)pIcc_cfg->mailbox_address;
            volatile scmi_icc_packet_t* scmi_packet = (scmi_icc_packet_t*)packet->payload;

            // set the status bit so that the status is free
            scmi_packet->smt_header.status = SMT_CHANNEL_FREE;
        }
    }
    return status;
}

//
// PUBLIC FUNCTIONS
//

int icc_mhu_trans_init(void* p_config_table, uint8_t table_size)
{
    // Set the configurations
    icc_mhu_configuration_t* icc_config = (icc_mhu_configuration_t*)p_config_table;
    int status = icc_mhu_trans_set_configuration_table(icc_config, table_size);
    if (status != ICC_MHU_INVALID_PARAMETER)
    {
        // Check for any SCMI receive configuration and make sure to set the channel to be free
        for (uint8_t index = 0; index < table_size; index++)
        {
            // Check if the channel is for SCMI
            if (icc_config[index].icc_channel_info.protocol_type == ICC_PROTOCOL_TYPE_SCMI_ON_ICC &&
                icc_config[index].icc_channel_info.direction == ICC_RECEIVE_DIR)
            {
                // Since we are receiving a message from an SCMI ICC then we need to set the status SCMI Status flag
                picc_mhu_packet_t packet = (picc_mhu_packet_t)icc_config[index].mailbox_address;
                volatile scmi_icc_packet_t* scmi_packet = (scmi_icc_packet_t*)packet->payload;

                // set the status bit so that the status is free
                scmi_packet->smt_header.status = SMT_CHANNEL_FREE;
            }
        }
    }

    return status;
}

int icc_mhu_trans_send_message(uint16_t mhu_interface_id, uint32_t command, uint8_t* data, size_t size)
{
    // convert the index
    int status = icc_mhu_send_message(mhu_interface_id, command, data, size);

    return status;
}

int icc_mhu_trans_get_cmd_msg_from_index(uint16_t core_2_core_id, icc_mhu_request_t* client_msg)
{

    // Create the data structure to obtain
    uint8_t data[SIZE_OF_TEMP_BUFFER];
    icc_msg_receive_t message;
    message.data = data;
    int status = icc_mhu_trans_get_message(core_2_core_id, &message);
    if (status == ICC_MHU_STATUS_SUCCESS)
    {

        // Check if the command matches
        status = ICC_MHU_TRANS_NO_CMD_MESSAGE_AVAILABLE;
        if (client_msg->header.msg_header.command == message.command)
        {
            // Copy the size available provided by the client
            if (client_msg->header.msg_header.payload_size >= message.size)
            {
                if (message.size > SIZE_OF_TEMP_BUFFER)
                {
                    return ICC_MHU_TRANS_NO_CMD_MESSAGE_AVAILABLE;
                }
                memcpy(client_msg->payload, data, message.size);
                status = ICC_MHU_TRANS_CMD_MESSAGE_AVAILABLE;
                client_msg->header.msg_header.payload_size = message.size;
            }
        }
    }

    return status;
}

int icc_mhu_trans_get_msg_from_index(uint16_t core_2_core_id, icc_mhu_request_t* client_msg)
{
    // Create the data structure to obtain
    uint8_t data[SIZE_OF_TEMP_BUFFER] = {0};
    icc_msg_receive_t message;
    message.data = data;
    int status = icc_mhu_trans_get_message(core_2_core_id, &message);
    if (status == ICC_MHU_STATUS_SUCCESS)
    {

        // Check if the command matches
        status = ICC_MHU_TRANS_NO_CMD_MESSAGE_AVAILABLE;

        // Copy the size available provided by the client
        if (client_msg->header.msg_header.payload_size >= message.size)
        {
            if (message.size > SIZE_OF_TEMP_BUFFER)
            {
                return ICC_MHU_TRANS_NO_CMD_MESSAGE_AVAILABLE;
            }
            memcpy(client_msg->payload, data, message.size);
            status = ICC_MHU_TRANS_CMD_MESSAGE_AVAILABLE;
            client_msg->header.msg_header.command = message.command;
            client_msg->header.msg_header.payload_size = message.size;
        }
    }

    return status;
}

/**
 * @brief API to check the status bit field of SCMI based on index provided.
 *
 * @param index of the channel config from the config table
 * @return None
 */
void icc_mhu_trans_check_scmi_status_bit(uint16_t index)
{

    // Since we are receiving a message from an SCMI ICC then we need to set the status SCMI Status flag
    icc_mhu_configuration_t* icc_config = properties.pIcc_configuration_table;
    picc_mhu_packet_t packet = (picc_mhu_packet_t)icc_config[index].mailbox_address;
    volatile scmi_icc_packet_t* scmi_packet = (scmi_icc_packet_t*)packet->payload;

    ICC_MHU_LOG_INFO("  SCMI Status[%d]: %d\n", (int)index, (int)scmi_packet->smt_header.status);
    ICC_MHU_LOG_INFO("          Address: %p\n", &(scmi_packet->smt_header.status));
}

/**
 * @brief API to provide configuration address and number of configs
 *
 * @param properties
 * @return none
 */
icc_mhu_properties_t icc_mhu_trans_get_config_entries(void)
{
    return properties;
}
