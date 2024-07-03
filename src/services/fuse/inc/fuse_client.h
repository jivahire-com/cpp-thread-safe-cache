//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @brief FUSE Module public header
 *
 */

#pragma once

/*----------- Nested includes ------------*/
#include <FpFwCli.h>        // for FpFwCliInitialize, FpFwCliStart, FPFW_CLI_...
#include <stdbool.h>

/*--------- Function Prototypes ----------*/
FPFW_CLI_STATUS platform_fuse_init_cli(void);