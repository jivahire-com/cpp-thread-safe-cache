//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file scmi_init.h
 *  SCMI initialization APIs
 */

#pragma once

/*--------------- Includes ---------------*/
#include <DfwkCommon.h>
/*--------- Macro Definitions ------------*/


/*--------------- Structures ---------------*/

/**
 * @brief Initialization API to provide an apcore interface to SCMI
 *
 * @param p_interface - apcore dfwk interface
 */
void scmi_set_apcore_interface(DFWK_INTERFACE_HEADER *p_interface);