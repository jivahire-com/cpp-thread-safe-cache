//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file utc_sync_manager_mctp_i.h
 * Private Header for the UTC Sync Manager MCTP transport helper library.
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/

// #include <FpFwLinkedList.h>
#include <fpfw_mctp.h>
#include <fpfw_status.h>
#include <stdint.h>
#include <stddef.h>
#include <tx_api.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

// clang-format off

#define FPFW_UTC_SYNC_MANAGER_MCTP_BINDING_MAX_PAYLOAD_SIZE  (1024U)

//
// TODO: Outside of the BMC EID these values are set in stone by the MVDP. Move these 
//       and other MDVP handling to the shared firmware repo. Make a MCTP MVDP lib others
//       can use.
//       ADO: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/3139887
//       ADO: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/3139890
//
#define MCTP_MVDP_COMPLETION_CODE_SUCCESS    (0x00)
#define MCTP_MVDP_MSG_TYPE                   (0x7E)
#define MCTP_MVDP_PCI_VENDOR_ID_0            (0x14)
#define MCTP_MVDP_PCI_VENDOR_ID_1            (0x14)
#define MCTP_MVDP_ESCAPE_SEQ_0               (0x80)
#define MCTP_MVDP_ESCAPE_SEQ_1               (0xFF)
#define MCTP_MVDP_1P_BMC_CMD_SET_ID          (0x04)
#define MCTP_MVDP_1P_BMC_CMD_SET_PROTO_VER_0 (0x00)
#define MCTP_MVDP_1P_BMC_CMD_SET_PROTO_VER_1 (0x00)
#define MCTP_MVDP_1P_BMC_CMD_SET_NTP_CMD_ID  (0x01)

//
// TODO: - Get this from the MCTP libraries once supported, hardcode until then
//         ADO: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/3139896
//
#define MCTP_BMC_EID (0x0A)

// clang-format on

/*-------------------------------- Typedefs ---------------------------------*/

//
// Details can be found in the MCTP Vendor Defined Protocol specification
// https://microsoft.sharepoint.com/:w:/t/CSIFirmwareDev/IQCfgQ0gzR-OT6Th9dntuDvkAegnBzqpBSXVhjqi0JtT0ps?e=SivjUY
//
typedef struct
{
    uint8_t msg_type;
    uint8_t pci_vendor_id[2]; 
    uint8_t escape_seq1;
    uint8_t escape_seq2;
    uint8_t command_set;
    uint8_t protocol_version[2];
    uint8_t command;
    /* TODO: ADO: 3087952 [Kingsgate] Add CRC MVDP Messages
     * https://azurecsi.visualstudio.com/Dev/_workitems/edit/3087952 */
} __attribute__((packed)) mctp_mvdp_msg_header_t;

//
// Requests and Responses both use the same base header. However Responses have
// an additional field.
//

typedef struct _UTC_SYNC_MANAGER_MCTP_MESSAGE_SEND_T
{
    mctp_mvdp_msg_header_t base_header;
    uint64_t t0; // Timestamp when the UTC Sync Manager sends the request to the BMC
} __attribute__((packed)) utc_sync_manager_mctp_message_send_t;

// Response Header with additional Completion Code field
typedef struct
{
    mctp_mvdp_msg_header_t base_header;
    uint8_t completion_code;
} __attribute__((packed)) mctp_mvdp_resp_msg_header_t;

typedef struct _UTC_SYNC_MANAGER_MCTP_MESSAGE_RECEIVE_T
{
    mctp_mvdp_resp_msg_header_t header;
    uint64_t t0; // Timestamp when the UTC Sync Manager sends the request to the BMC
    uint64_t t1; // Timestamp when the BMC receives the request
    uint64_t t2; // Timestamp when the BMC responds to the request
} __attribute__((packed)) utc_sync_manager_mctp_message_receive_t;

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/**
 * @brief Callback function to handle received MCTP messages.
 *
 * @param[in] recv_msg_info Structure to hold received message information
 * @param[in] status        Status of the received message
 */
void on_mctp_msg_receive(fpfw_mctp_recv_msg_info* recv_msg_info, fpfw_status_t status);

/**
 * @brief Callback function to handle MCTP message send completion.
 *
 * @param[in] send_msg_info Structure to hold sent message information
 * @param[in] status        Status of the sent message
 * 
 * @return None
 */
void on_mctp_send_msg_complete(fpfw_mctp_send_msg_info* send_msg_info, fpfw_status_t status);

/**
 * @brief Send a MCTP service message using the UTC Sync Manager.
 *
 * @param[in] p_send_msg_info Pointer to the structure containing the message information to be sent.
 *
 * @return fpfw_status_t Status code indicating the result of the send operation.
 */
fpfw_status_t utc_sync_manager_mctp_send(fpfw_mctp_send_msg_info* p_send_msg_info);

/**
 * @brief Pends a MCTP read with the MCTP service for the MVDP Message Type.
 *
 * @param[in] p_recv_msg_info Pointer to the structure holding the received message information.
 *
 * @return fpfw_status_t Status code indicating the result of the read operation.
 */
fpfw_status_t utc_sync_manager_mctp_read(fpfw_mctp_recv_msg_info* p_recv_msg_info);
