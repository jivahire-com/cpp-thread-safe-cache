//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file icc_mhu_trans_dfwk_i.h
 *  ICC MHU primitive support internal.
 */

#pragma once


/**
 * @brief API to open the icc mhu driver interface
 *
 * @param p_interface
 * @return ICC_MHU_TRANS_STATUS
 */
int32_t icc_mhu_transport_dfwk_interface_open(DFWK_INTERFACE_HEADER* p_interface);

/**
 * @brief API to close the icc mhu driver interface
 *
 * @param p_interface
 * @return NONE
 */
void icc_mhu_transport_dfwk_interface_close(DFWK_INTERFACE_HEADER* p_interface);

/**
 * @brief API for all Asynchronous dispatcher requests
 *
 * @param Request An async request
 * @return NONE
 */
void icc_mhu_transport_driver_dispatch_async(PDFWK_ASYNC_REQUEST_HEADER Request, void* Context);

/**
 * @brief API for all Synchronous dispatcher requests
 *
 * @param Request An sync request
 * @return FPFW_ICC_TRANSPORT_STATUS
 */
fpfw_status_t icc_mhu_transport_driver_dispatch_sync(PDFWK_SYNC_REQUEST_HEADER Request);

/**
 * @brief ICC MHU ISR to handle receiving messages
 */
void icc_mhu_isr(void* context);

/**
 * @brief Serialized dispatch routine for async recv requests
 * 
 * @param Request An async request
 * @param Context Dispatch context
 * 
 * @return NONE
 */
void dispatch_async_recv(PDFWK_ASYNC_REQUEST_HEADER Request, void* Context);

/**
 * @brief Serialized dispatch routine for async send requests
 * 
 * @param Request An async request
 * @param Context Dispatch context
 * 
 * @return NONE
 */
void dispatch_async_send(PDFWK_ASYNC_REQUEST_HEADER Request, void* Context);
