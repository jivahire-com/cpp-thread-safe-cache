//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_dfwk.h
 * Header containing definitions for the dfwk portion of power service
 */

#pragma once

/*----------- Nested includes ------------*/
#include <DfwkDriver.h>
#include <stdint.h>

/*-------------- Typedefs ----------------*/
// struct for the power service/device
typedef struct {
    DFWK_DEVICE_HEADER header;
    DFWK_QUEUE default_queue;
} power_service_t, *ppower_service_t;

// struct for an interface to the power service
typedef struct  {
    DFWK_INTERFACE_HEADER header;
    ppower_service_t p_device;
} power_service_interface_t, *ppower_service_interface_t;

/*--------- Function Prototypes ----------*/
#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif