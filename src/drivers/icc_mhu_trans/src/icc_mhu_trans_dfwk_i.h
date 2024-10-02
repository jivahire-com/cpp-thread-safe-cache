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
 * @param interface
 * @return ICC_MHU_TRANS_STATUS
 */
int32_t icc_mhu_transport_dfwk_interface_open(DFWK_INTERFACE_HEADER* interface);

/**
 * @brief API to close the icc mhu driver interface
 *
 * @param interface
 * @return NONE
 */
void icc_mhu_transport_dfwk_interface_close(DFWK_INTERFACE_HEADER* interface);

/**
 * @brief API for all Asynchronous dispatcher requests
 *
 * @param Request
 * @return NONE
 */
void icc_mhu_transport_driver_dispatch_async(PDFWK_ASYNC_REQUEST_HEADER Request, void* Context);
fpfw_status_t icc_mhu_transport_driver_dispatch_sync(PDFWK_SYNC_REQUEST_HEADER Request);
