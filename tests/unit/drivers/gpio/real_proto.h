//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file real_proto.h
 * Forward declarations for real functions of mock.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <DfwkAsyncRequest.h>
#include <DfwkInterface.h>
#include <DfwkPtrTypes.h>
#include <DfwkQueue.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
void __real_DfwkDeviceInitialize(PDFWK_DEVICE_HEADER Device, PDFWK_SCHEDULE Schedule);
void __real_DfwkQueueInitialize(PDFWK_QUEUE Queue,
                                PDFWK_DEVICE_HEADER Device,
                                DFWK_ASYNC_REQUEST_DISPATCH DispatchRoutine,
                                void* DispatchContext,
                                DFWK_QUEUE_TYPE QueueType);
void __real_DfwkInterfaceInitialize(PDFWK_INTERFACE_HEADER Interface,
                                    PDFWK_DEVICE_HEADER Device,
                                    PDFWK_QUEUE DispatchQueue,
                                    DFWK_REQUEST_DISPATCH_SYNC DispatchSync);

bool __real_DfwkQueueDequeueRequest(PDFWK_QUEUE Queue, PDFWK_ASYNC_REQUEST_HEADER* Request);
void __real_DfwkQueueEnqueueRequest(PDFWK_QUEUE Queue, PDFWK_ASYNC_REQUEST_HEADER Request);