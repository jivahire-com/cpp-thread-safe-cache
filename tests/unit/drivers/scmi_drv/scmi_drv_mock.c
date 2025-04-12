//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scmi_drv_mock.c
 * Provide mock functions for scmi driver tests
 */

/*------------- Includes -----------------*/

//
// extern C helper macros
//
#ifdef __cplusplus
    #define BEGIN_EXTERN_C extern "C" {
    #define END_EXTERN_C   }
#else // __cplusplus
    #define BEGIN_EXTERN_C
    #define END_EXTERN_C
#endif // __cplusplus

#include <DfwkClient.h> // for PDFWK_DEVICE_HEADER, PDFWK_INTERFACE_HEADER
#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <ap_core.h>    // for pap_core_asynchronous_request_t
#include <fpfw_init.h>
#include <fpfw_status.h>     // for fpfw_status_t
#include <kng_scmi_shared.h> // for SCMI Data structures and functions
#include <mhu_icc_transport.h>
#include <scmi_prim.h>
#include <scmi_prim_i.h>
#include <startup_shutdown.h>
#include <stdint.h> // for uint32_t, uint64_t, int32_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

fpfw_status_t __wrap_fpfw_icc_transport_recv_async_req(PDFWK_INTERFACE_HEADER transport_interface,
                                                       PDFWK_ASYNC_REQUEST_HEADER request,
                                                       void* recv_buffer,
                                                       size_t buffer_size,
                                                       DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE comp_routine,
                                                       void* comp_ctx)
{
    FPFW_UNUSED(transport_interface);
    FPFW_UNUSED(request);
    FPFW_UNUSED(recv_buffer);
    FPFW_UNUSED(buffer_size);
    FPFW_UNUSED(comp_routine);
    FPFW_UNUSED(comp_ctx);
    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_icc_transport_try_send_sync_req(PDFWK_INTERFACE_HEADER transport_interface, void* send_buffer, size_t buffer_size)
{
    FPFW_UNUSED(transport_interface);
    FPFW_UNUSED(send_buffer);
    FPFW_UNUSED(buffer_size);
    return mock_type(fpfw_status_t);
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

void __wrap_scmi_cold_boot_completion(PDFWK_ASYNC_REQUEST_HEADER request, void* p_completion_context)
{
    FPFW_UNUSED(request);
    FPFW_UNUSED(p_completion_context);
}

void __wrap_scmi_shutdown_completion(PDFWK_ASYNC_REQUEST_HEADER request, void* p_completion_context)
{
    FPFW_UNUSED(request);
    FPFW_UNUSED(p_completion_context);
}

void __wrap_scmi_warm_boot_completion(PDFWK_ASYNC_REQUEST_HEADER request, void* p_completion_context)
{
    FPFW_UNUSED(request);
    FPFW_UNUSED(p_completion_context);
}

int32_t __wrap_DfwkClientInterfaceOpen(PDFWK_INTERFACE_HEADER Interface)
{
    check_expected_ptr(Interface);
    return mock_type(int32_t);
}

void __wrap_DfwkAsyncRequestInitialize(PDFWK_ASYNC_REQUEST_HEADER Request, size_t RequestSize)
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

void __wrap_ap_core_power_completion(PDFWK_ASYNC_REQUEST_HEADER request, void* p_completion_context)
{
    FPFW_UNUSED(request);
    FPFW_UNUSED(p_completion_context);
}

void __wrap_ap_core_reset_addr_completion(PDFWK_ASYNC_REQUEST_HEADER request, void* p_completion_context)
{
    FPFW_UNUSED(request);
    FPFW_UNUSED(p_completion_context);
}

void __wrap_sos_shutdown(PDFWK_INTERFACE_HEADER p_interface,
                         pstartup_shutdown_request_t p_request,
                         ssi_shutdown_type_t shutdown_type,
                         DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                         void* p_completion_context)
{
    FPFW_UNUSED(p_interface);
    FPFW_UNUSED(p_request);
    FPFW_UNUSED(shutdown_type);
    FPFW_UNUSED(completion_routine);
    FPFW_UNUSED(p_completion_context);
}
