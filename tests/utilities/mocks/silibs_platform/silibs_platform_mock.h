//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file silibs_platform_mock.h
 *  ThreadX mocks for unit test
 */

#pragma once

/*-------------------------------- Includes ---------------------------------*/

#include <stdint.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/
void mmio_set_mock_data(uint32_t addr, uint32_t data);
