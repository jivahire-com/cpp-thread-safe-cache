//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_mesh_init.cpp
 * Implement unit tests for mesh init
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for CmockaWrapperTest, TEST_FUNCTION, che...

extern "C" {
#include <fpfw_icc_base.h>   // for fpfw_icc_base_init, fpfw_icc_ba...
#include <fpfw_icc_base_i.h> // for fpfw_icc_base_ctx_t
#include <fpfw_init.h>
#include <fpfw_mbox_icc_transport.h> // for ICC_MBX_ASYNC_RECV, ICC_MBX_ASY...
#include <idhw.h>
#include <mesh.h>
#include <stdint.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_mesh;
static FPFW_MBX_PRIMITIVE_CTX test_hsp_mbx_ctx;
static fpfw_mbox_icc_transport_device_t test_hsp_mbx_dev = {
    .mbox_prim_ctx = test_hsp_mbx_ctx,
};

static DFWK_INTERFACE_HEADER test_icc_transport_interface = {
    .OwningDevice = (PDFWK_DEVICE_HEADER)&test_hsp_mbx_dev,
};

static fpfw_icc_base_config test_icc_cfg = {
    .transport_interface = &test_icc_transport_interface,
};

static fpfw_icc_base_ctx_t test_icc_ctx = {
    .icc_cfg = test_icc_cfg,
};

/*------------- Functions ----------------*/
void* __wrap_fpfw_init_get_handle(const char* id)
{
    FPFW_UNUSED(id);
    return &test_icc_ctx;
}

int __wrap_mesh_init(uint8_t die_num, FPFW_MBX_PRIMITIVE_CTX* hsp_mbx_ctx)
{
    check_expected(die_num);
    assert_non_null(hsp_mbx_ctx);

    return SILIBS_SUCCESS;
}

KNG_DIE_ID __wrap_idhw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

TEST_FUNCTION(test_mesh_init, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    will_return_always(__wrap_idhw_get_die_id, test_die);
    expect_value(__wrap_mesh_init, die_num, test_die);
    _fpfw_component_mesh.init_fn();
}
}
