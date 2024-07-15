//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scmi_drv_mock.c
 * Provide mock functions for scmi driver tests
 */

/*------------- Includes -----------------*/

#include <DfwkClient.h>         // for PDFWK_DEVICE_HEADER, PDFWK_INTERFACE_HEADER
#include <FpFwCMocka.h>         // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>          // for FPFW_UNUSED
#include <ap_core.h>            // for pap_core_asynchronous_request_t
#include <icc_mhu_trans_prim.h> // for icc mhu
#include <kng_scmi_shared.h>    // for SCMI Data structures and functions
#include <scmi_prim.h>
#include <scmi_prim_i.h>
#include <stdint.h> // for uint32_t, uint64_t, int32_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

int __wrap_icc_mhu_trans_get_msg_from_index(uint8_t index, icc_mhu_transport_message_t* client_msg)
{
    FPFW_UNUSED(index);
    FPFW_UNUSED(client_msg);
    return mock_type(int);
}

int __wrap_icc_mhu_trans_send_message_idx(uint16_t index, uint32_t command, uint8_t* data, size_t size)
{
    FPFW_UNUSED(index);
    FPFW_UNUSED(command);
    FPFW_UNUSED(data);
    FPFW_UNUSED(size);
    return mock_type(int);
}

int __wrap_scmi_send_resp(uint8_t protocol_id, uint8_t command, uint8_t* payload, size_t size)
{
    FPFW_UNUSED(protocol_id);
    FPFW_UNUSED(command);
    FPFW_UNUSED(size);
    FPFW_UNUSED(payload);
    return mock_type(int);
}

int __wrap_scmi_check_message(scmi_icc_packet_t* packet)
{
    FPFW_UNUSED(packet);
    return mock_type(int);
}

int __wrap_scmi_power_protocol_cmds(uint8_t cmd_code, uint8_t* payload, size_t size)
{
    FPFW_UNUSED(cmd_code);
    FPFW_UNUSED(payload);
    FPFW_UNUSED(size);
    return mock_type(int);
}

int __wrap_scmi_sys_pwr_protocol_cmds(uint8_t cmd_code, uint8_t* payload, size_t size)
{
    FPFW_UNUSED(cmd_code);
    FPFW_UNUSED(payload);
    FPFW_UNUSED(size);
    return mock_type(int);
}

int __wrap_scmi_ap_core_protocol_cmds(uint8_t cmd_code, uint8_t* payload, size_t size)
{
    FPFW_UNUSED(cmd_code);
    FPFW_UNUSED(payload);
    FPFW_UNUSED(size);
    return mock_type(int);
}

int32_t __wrap_DfwkClientInterfaceOpen(PDFWK_INTERFACE_HEADER Interface)
{
    check_expected_ptr(Interface);
    return mock_type(int32_t);
}

void __wrap_DfwkAsyncRequestInititalize(PDFWK_ASYNC_REQUEST_HEADER Request, size_t RequestSize)
{
    check_expected_ptr(Request);
    check_expected(RequestSize);
    Request->AllocatedSize = RequestSize;
}

void __wrap_ap_core_set_rvbaraddr(PDFWK_INTERFACE_HEADER p_interface,
                                  pap_core_asynchronous_request_t p_request,
                                  uint64_t rvbaraddr,
                                  DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                                  void* p_completion_context)
{
    FPFW_UNUSED(p_interface);
    FPFW_UNUSED(p_request);
    FPFW_UNUSED(completion_routine);
    FPFW_UNUSED(p_completion_context);
    check_expected(rvbaraddr);
}

void __wrap_ap_core_core_power_on(PDFWK_INTERFACE_HEADER p_interface,
                                  pap_core_asynchronous_request_t p_request,
                                  uint32_t core_id,
                                  DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                                  void* p_completion_context)
{
    FPFW_UNUSED(p_interface);
    FPFW_UNUSED(p_request);
    FPFW_UNUSED(completion_routine);
    FPFW_UNUSED(p_completion_context);
    check_expected(core_id);
}

void __wrap_ap_core_core_power_off(PDFWK_INTERFACE_HEADER p_interface,
                                   pap_core_asynchronous_request_t p_request,
                                   uint32_t core_id,
                                   DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                                   void* p_completion_context)
{
    FPFW_UNUSED(p_interface);
    FPFW_UNUSED(p_request);
    FPFW_UNUSED(completion_routine);
    FPFW_UNUSED(p_completion_context);
    check_expected(core_id);
}