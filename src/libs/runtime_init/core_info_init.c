//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ap_core_init.c
 * This file contains the config/initialization for the APcore service
 */

/*------------- Includes -----------------*/

#include <DbgPrint.h>
#include <atu_api.h>
#include <atu_lib.h>
#include <bug_check.h> // for BUG_ASSERT_PARAM, BUG_ASSERT
#include <core_info.h>
#include <fpfw_init.h>
#if defined(SCP_RUNTIME_INIT)
    #include <fuse_client.h>
    #include <fuse_init.h>
#endif
#include <fuse_utils.h>
#include <idhw.h>
#include <idsw.h>
#include <stdio.h> // for NULL, printf
/*-------------- Functions ---------------*/
#if defined(SCP_RUNTIME_INIT)
FPFW_INIT_COMPONENT(core_info, FPFW_INIT_DEPENDENCIES("cfg_mgr", "fuse_post_mesh"))
#elif defined(MCP_RUNTIME_INIT)
FPFW_INIT_COMPONENT(core_info, FPFW_INIT_DEPENDENCIES("cfg_mgr", "fuse_en"))
#endif
{
    core_info_init();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}