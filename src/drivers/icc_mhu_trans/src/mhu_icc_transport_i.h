//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file mhu_icc_transport_i.h
 *  
 *  @brief Private header for the MHU ICC Transport Driver.
 */

#pragma once

#include <DfwkInterface.h>
#include <DfwkPtrTypes.h>
#include <fpfw_status.h>

//
// Interface Functions
//

/**
 * @brief Open the mhu icc transport interface
 *
 * @note Not ISR Safe
 * @note Not Reentrant Safe
 * 
 * @param[in] p_interface Pointer to the interface to open
 * 
 * @return FPFW_ICC_TRANSPORT_STATUS_*
 */
fpfw_status_t mhu_icc_transport_interface_open(DFWK_INTERFACE_HEADER* p_interface);

/**
 * @brief Close the mhu icc transport interface
 *
 * @note Not ISR Safe
 * @note Not Reentrant Safe
 * 
 * @param[in] p_interface Pointer to the interface to close
 * 
 * @return NONE
 */
void mhu_icc_transport_interface_close(DFWK_INTERFACE_HEADER* p_interface);

//
// Request Handling Functions
//

/**
 * @brief Top level dispatch routine for all sync requests
 *
 * @note Not ISR Safe
 * @note Not Reentrant Safe
 * 
 * @param[in] Request An sync request
 * 
 * @return FPFW_ICC_TRANSPORT_STATUS_*
 */
fpfw_status_t mhu_icc_transport_dispatch_sync(PDFWK_SYNC_REQUEST_HEADER Request);

/**
 * @brief Top level dispatch routine for all async requests
 *
 * @note Not ISR Safe
 * @note Not Reentrant Safe
 * 
 * @param[in] Request An async request
 * @param[in] Context Dispatch context
 * 
 * @return NONE
 */
void mhu_icc_transport_dispatch_async(PDFWK_ASYNC_REQUEST_HEADER Request, void* Context);

/**
 * @brief Serialized dispatch routine for async recv requests
 * 
 * @param[in] Request An async request
 * @param[in] Context Dispatch context
 * 
 * @return NONE
 */
void mhu_icc_transport_dispatch_async_recv(PDFWK_ASYNC_REQUEST_HEADER Request, void* Context);

/**
 * @brief Serialized dispatch routine for async send requests
 * 
 * @param[in] Request An async request
 * @param[in] Context Dispatch context
 * 
 * @return NONE
 */
void mhu_icc_transport_dispatch_async_send(PDFWK_ASYNC_REQUEST_HEADER Request, void* Context);

/**
 * @brief ISR to handle receiving packets. Will complete the active async recv request
 *        if there is one.
 * 
 * @param[in] context Pointer to context from ISR registration
 * 
 * @return NONE
 */
void mhu_icc_transport_isr(void* context);
