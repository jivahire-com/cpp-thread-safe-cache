//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_mscp_hspmbx_init.cpp
 * Tests initialization of ICC transports for both SCP/MCP cores
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:icc_init

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for TEST_FUNCTION, mock_type, will_return
#include <cstdint>         // for uint32_t

extern "C" {
#include <DfwkHost.h>        // for PDFWK_DEVICE_HEADER, PDFWK_SCHE...
#include <DfwkThreadXHost.h> // for DFWK_THREADX_HOST
#include <FpFwUtils.h>       // for FPFW_UNUSED
#include <MboxPrimitives.h>  // for FPFW_MBX_FIFO_DEPTH, FPFW_MBX_I...
#include <accel_intr.h>
#include <accelip_id.h>
#include <fpfw_icc_base.h>           // for fpfw_icc_base_config, fpfw_icc_...
#include <fpfw_icc_dispatcher.h>     // for fpfw_icc_dispatch_ctx
#include <fpfw_init.h>               // for fpfw_init_component_id_t, fpfw_...
#include <fpfw_mbox_icc_transport.h> // for fpfw_mbox_icc_transport_device_t
#include <fpfw_status.h>             // for fpfw_status_t, FPFW_STATUS_SUCCESS
#include <icc_mhu.h>
#include <icc_mhu_cfg.h>
#include <icc_platform_defines.h>
#include <idsw.h>     // for KNG_DIE_ID, KNG_PLAT_ID
#include <idsw_kng.h> // for KNG_DIE_ID, KNG_PLAT_ID
#include <limits.h>
#include <mhu_icc_transport.h>
#include <mscp_exp_rmss_memory_map.h>
#include <mscp_uefi_shared_ddrss.h>

/*---------------Macros-------------------*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_icc_hspmbx;
extern fpfw_init_component_t _fpfw_component_icc_d2dmbx;
extern fpfw_init_component_t _fpfw_component_icc_sdm_mbx;
extern fpfw_init_component_t _fpfw_component_icc_cded_mbx;
extern fpfw_init_component_t _fpfw_component_icc_mscp2tfa_if;
extern fpfw_init_component_t _fpfw_component_icc_mscp2mscp;
extern fpfw_init_component_t _fpfw_component_icc_mscp2apns;
extern fpfw_init_component_t _fpfw_component_icc_die2die;

/*------------- Mock Functions ----------------*/

void __wrap_DfwkDeviceInitialize(PDFWK_DEVICE_HEADER Device, PDFWK_SCHEDULE Schedule)
{
    assert_non_null(Device);
    assert_non_null(Schedule);
    function_called();
}

void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    FPFW_UNUSED(id);
    return mock_type(void*);
}

KNG_DIE_ID __wrap_idhw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

KNG_PLAT_ID __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(KNG_PLAT_ID);
}

idsw_cpu_type_t __wrap_idsw_get_cpu_type(void)
{
    return mock_type(idsw_cpu_type_t);
}

int __wrap_spi_controller_init(uintptr_t spi_master_reg, uint16_t clkDiv)
{
    FPFW_UNUSED(spi_master_reg);
    FPFW_UNUSED(clkDiv);
    return mock_type(int);
}

int __wrap_spi_bridge_init(uintptr_t spi_bridge_reg, uint16_t clkDiv)
{
    FPFW_UNUSED(spi_bridge_reg);
    FPFW_UNUSED(clkDiv);
    return mock_type(int);
}

int __wrap_spi_controller_check_errors(uintptr_t spi_master_reg)
{
    FPFW_UNUSED(spi_master_reg);
    return mock_type(int);
}

int __wrap_spi_bridge_check_errors(uintptr_t spi_bridge_reg)
{
    FPFW_UNUSED(spi_bridge_reg);
    return mock_type(int);
}

int __wrap_spi_bridge_clear_error_interrupts(uintptr_t spi_bridge_reg)
{
    FPFW_UNUSED(spi_bridge_reg);
    return mock_type(int);
}

fpfw_status_t __wrap_fpfw_mbox_icc_transport_dfwk_device_init(fpfw_mbox_icc_transport_device_t* dev,
                                                              fpfw_mbox_icc_transport_config_t* cfg)
{
    assert_non_null(cfg);
    assert_non_null(dev);

    assert_int_equal(cfg->mbox_dev_cfg.MbxMesgHandlingType, MBX_MESG_HANDLING_SINGLE_MESG_AT_A_TIME);
    assert_int_equal(cfg->mbox_dev_cfg.MbxImplementation, MBX_IMPL_INTERRUPT);

    if (cfg->mbox_dev_cfg.MbxFifoDepth == HSP_MBX_FIFO_DEPTH)
    {
        //! The base address is compiler defined in Cmake for this test
        assert_int_equal(cfg->mbox_dev_cfg.MbxBaseAddr, 1000);
        assert_int_equal(cfg->mbox_dev_cfg.MsgSizeBytes, HSP_MBOX_MAX_MESG_SIZE_BYTES);
        assert_int_equal(cfg->mbox_dev_cfg.MbxSendBaseAddr, 0);
        assert_int_equal(cfg->mbox_dev_cfg.MbxRecvBaseAddr, 0);
        assert_null(cfg->mbox_dev_cfg.RemoteRegRead32);
        assert_null(cfg->mbox_dev_cfg.RemoteRegWrite32);
    }
    else if (cfg->mbox_dev_cfg.MbxFifoDepth == D2D_MBX_FIFO_DEPTH)
    {
        //! The base address is compiler defined in Cmake for this test
        assert_int_equal(cfg->mbox_dev_cfg.MbxBaseAddr, 2000);
        assert_int_equal(cfg->mbox_dev_cfg.MsgSizeBytes, D2D_FIFO_MBOX_MAX_MESG_SIZE_BYTES);
        assert_int_equal(cfg->mbox_dev_cfg.MbxSendBaseAddr, 2000);
        assert_int_equal(cfg->mbox_dev_cfg.MbxRecvBaseAddr, 2000);
        assert_non_null(cfg->mbox_dev_cfg.RemoteRegRead32);
        assert_non_null(cfg->mbox_dev_cfg.RemoteRegWrite32);
    }
    else if (cfg->mbox_dev_cfg.MbxFifoDepth == LARGE_MBX_FIFO_DEPTH)
    {
        assert_int_equal(cfg->mbox_dev_cfg.MsgSizeBytes, LARGE_FIFO_MBOX_MAX_MESG_SIZE_BYTES);
        assert_int_equal(cfg->mbox_dev_cfg.MbxSendBaseAddr, 0);
        assert_int_equal(cfg->mbox_dev_cfg.MbxRecvBaseAddr, 0);
        assert_null(cfg->mbox_dev_cfg.RemoteRegRead32);
        assert_null(cfg->mbox_dev_cfg.RemoteRegWrite32);
    }
    else
    {
        assert_false(0);
    }

    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_mbox_icc_transport_dfwk_interface_init(fpfw_mbox_icc_transport_device_t* dev,
                                                                 fpfw_mbox_icc_transport_intrf_t* intrf)
{
    assert_non_null(intrf);
    assert_non_null(dev);
    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_icc_base_init(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_config* icc_cfg)
{
    assert_non_null(icc_ctx);
    assert_non_null(icc_cfg);
    assert_non_null(icc_cfg->transport_interface);
    assert_non_null(icc_cfg->dispatch_cfg.transport_interface);
    assert_non_null(icc_cfg->dispatch_cfg.dispatcher_buffer);
    assert_true(icc_cfg->dispatch_cfg.strategy.cmd_code.is_used);

    check_expected(icc_cfg->dispatch_cfg.strategy.seq_num.is_used);
    check_expected(icc_cfg->dispatch_cfg.dispatcher_buffer_size);
    check_expected(icc_cfg->dispatch_cfg.strategy.cmd_code.size_bits);
    check_expected(icc_cfg->dispatch_cfg.strategy.seq_num.size_bits);
    check_expected(icc_cfg->dispatch_cfg.strategy.cmd_code.start_pos);
    check_expected(icc_cfg->dispatch_cfg.strategy.seq_num.start_pos);
    check_expected(icc_cfg->dispatch_cfg.strategy.cmd_code.valid_max);
    check_expected(icc_cfg->dispatch_cfg.strategy.seq_num.valid_max);
    check_expected(icc_cfg->dispatch_cfg.strategy.cmd_code.valid_min);
    check_expected(icc_cfg->dispatch_cfg.strategy.seq_num.valid_min);
    check_expected(icc_cfg->dispatch_cfg.match_strategy_cb);
    check_expected(icc_cfg->dispatch_cfg.match_strategy_ctx);

    assert_null(icc_cfg->ctx);
    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_icc_dispatcher_start(fpfw_icc_dispatch_ctx* p_dispatch_ctx)
{
    FPFW_UNUSED(p_dispatch_ctx);
    return mock_type(fpfw_status_t);
}

uint32_t __wrap_accel_intr_init(ACCEL_ID accel_type)
{
    FPFW_UNUSED(accel_type);
    return mock_type(uint32_t);
}

fpfw_status_t __wrap_mhu_icc_transport_device_init(mhu_icc_transport_device_t* dev,
                                                   DFWK_SCHEDULE* schedule,
                                                   mhu_icc_transport_device_config_t* config)
{
    FPFW_UNUSED(dev);
    FPFW_UNUSED(schedule);
    FPFW_UNUSED(config);
    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_mhu_icc_transport_interface_init(mhu_icc_transport_device_t* dev, mhu_icc_transport_intrf_t* p_interface)
{
    FPFW_UNUSED(dev);
    FPFW_UNUSED(p_interface);
    return mock_type(fpfw_status_t);
}

idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return mock_type(idsw_die_id_t);
}

bool __wrap_idhw_is_single_die_boot_en()
{
    return mock_type(bool);
}

bool __wrap_accel_is_isolation_enabled(ACCEL_ID accel_type)
{
    check_expected(accel_type);

    return mock_type(bool);
}

/*------------- Test cases ----------------*/

TEST_FUNCTION(test_icc_hspmbx_init, nullptr, nullptr)
{
    // Set up expectations
    DFWK_THREADX_HOST test_host = {};

    //! Verify wrapped APIs are invoked in order
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    expect_function_call(__wrap_DfwkDeviceInitialize);
    will_return(__wrap_fpfw_mbox_icc_transport_dfwk_device_init, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_mbox_icc_transport_dfwk_interface_init, FPFW_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.is_used, true);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.dispatcher_buffer_size, HSP_MBOX_MAX_MESG_SIZE_BYTES);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.size_bits, HSP_MBOX_CMD_CODE_SIZE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.size_bits, HSP_MBOX_SEQ_NUM_SIZE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.start_pos, HSP_MBOX_CMD_CODE_START_POS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.start_pos, HSP_MBOX_SEQ_NUM_START_POS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_max, HSP_MBOX_MAX_CMD_CODE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_max, HSP_MBOX_MAX_SEQ_NUM);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_min, HSP_MBOX_MIN_CMD_CODE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_min, HSP_MBOX_MIN_SEQ_NUM);
    will_return(__wrap_fpfw_icc_base_init, FPFW_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_cb, NULL);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_ctx, NULL);
    will_return(__wrap_fpfw_icc_dispatcher_start, FPFW_ICC_DISPATCH_STATUS_SUCCESS);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_hspmbx.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_d2dmbx_init, nullptr, nullptr)
{
    // Set up expectations
    DFWK_THREADX_HOST test_host = {};

    //! Verify wrapped APIs are invoked in order
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    expect_function_call(__wrap_DfwkDeviceInitialize);
    will_return(__wrap_fpfw_mbox_icc_transport_dfwk_device_init, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_mbox_icc_transport_dfwk_interface_init, FPFW_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.is_used, true);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.dispatcher_buffer_size, D2D_FIFO_MBOX_MAX_MESG_SIZE_BYTES);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.size_bits, D2D_MBOX_CMD_CODE_SIZE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.size_bits, D2D_MBOX_SEQ_NUM_SIZE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.start_pos, D2D_MBOX_CMD_CODE_START_POS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.start_pos, D2D_MBOX_SEQ_NUM_START_POS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_max, D2D_MBOX_MAX_CMD_CODE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_max, D2D_MBOX_MAX_SEQ_NUM);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_min, D2D_MBOX_MIN_CMD_CODE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_min, D2D_MBOX_MIN_SEQ_NUM);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_cb, NULL);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_ctx, NULL);
    will_return(__wrap_fpfw_icc_base_init, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_dispatcher_start, FPFW_ICC_DISPATCH_STATUS_SUCCESS);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_d2dmbx.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_sdm_mbx_init, nullptr, nullptr)
{
    // Set up expectations
    DFWK_THREADX_HOST test_host = {};

    // SDM isolation
    expect_value(__wrap_accel_is_isolation_enabled, accel_type, ACCEL_ID_SDM);
    will_return(__wrap_accel_is_isolation_enabled, false);

    //! Verify wrapped APIs are invoked in order
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    expect_function_call(__wrap_DfwkDeviceInitialize);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_fpfw_mbox_icc_transport_dfwk_device_init, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_mbox_icc_transport_dfwk_interface_init, FPFW_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.is_used, true);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.dispatcher_buffer_size, LARGE_FIFO_MBOX_MAX_MESG_SIZE_BYTES);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.size_bits, LARGE_FIFO_MBOX_CMD_CODE_SIZE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.size_bits, LARGE_FIFO_MBOX_SEQ_NUM_SIZE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.start_pos, LARGE_FIFO_MBOX_CMD_CODE_START_POS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.start_pos, LARGE_FIFO_MBOX_SEQ_NUM_START_POS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_max, LARGE_FIFO_MBOX_MAX_CMD_CODE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_max, LARGE_FIFO_MBOX_MAX_SEQ_NUM);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_min, LARGE_FIFO_MBOX_MIN_CMD_CODE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_min, LARGE_FIFO_MBOX_MIN_SEQ_NUM);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_cb, NULL);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_ctx, NULL);
    will_return(__wrap_fpfw_icc_base_init, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_dispatcher_start, FPFW_ICC_DISPATCH_STATUS_SUCCESS);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_sdm_mbx.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_sdm_mbx_init_isolation, nullptr, nullptr)
{
    // SDM isolation
    expect_value(__wrap_accel_is_isolation_enabled, accel_type, ACCEL_ID_SDM);
    will_return(__wrap_accel_is_isolation_enabled, true);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_sdm_mbx.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_sdm_mbx_init__fail1, nullptr, nullptr)
{
    // SDM isolation
    expect_value(__wrap_accel_is_isolation_enabled, accel_type, ACCEL_ID_SDM);
    will_return(__wrap_accel_is_isolation_enabled, false);

    //! Verify wrapped APIs are invoked in order
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_fpfw_mbox_icc_transport_dfwk_device_init, FPFW_STATUS_FAIL);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_sdm_mbx.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status != FPFW_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_sdm_mbx_init__fail2, nullptr, nullptr)
{
    // Set up expectations
    DFWK_THREADX_HOST test_host = {};

    // SDM isolation
    expect_value(__wrap_accel_is_isolation_enabled, accel_type, ACCEL_ID_SDM);
    will_return(__wrap_accel_is_isolation_enabled, false);

    //! Verify wrapped APIs are invoked in order
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    expect_function_call(__wrap_DfwkDeviceInitialize);
    will_return(__wrap_fpfw_mbox_icc_transport_dfwk_device_init, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_mbox_icc_transport_dfwk_interface_init, FPFW_STATUS_FAIL);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_sdm_mbx.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status != FPFW_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_sdm_mbx_init__fail3, nullptr, nullptr)
{
    // Set up expectations
    DFWK_THREADX_HOST test_host = {};

    // SDM isolation
    expect_value(__wrap_accel_is_isolation_enabled, accel_type, ACCEL_ID_SDM);
    will_return(__wrap_accel_is_isolation_enabled, false);

    //! Verify wrapped APIs are invoked in order
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    expect_function_call(__wrap_DfwkDeviceInitialize);
    will_return(__wrap_fpfw_mbox_icc_transport_dfwk_device_init, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_mbox_icc_transport_dfwk_interface_init, FPFW_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.is_used, true);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.dispatcher_buffer_size, LARGE_FIFO_MBOX_MAX_MESG_SIZE_BYTES);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.size_bits, LARGE_FIFO_MBOX_CMD_CODE_SIZE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.size_bits, LARGE_FIFO_MBOX_SEQ_NUM_SIZE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.start_pos, LARGE_FIFO_MBOX_CMD_CODE_START_POS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.start_pos, LARGE_FIFO_MBOX_SEQ_NUM_START_POS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_max, LARGE_FIFO_MBOX_MAX_CMD_CODE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_max, LARGE_FIFO_MBOX_MAX_SEQ_NUM);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_min, LARGE_FIFO_MBOX_MIN_CMD_CODE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_min, LARGE_FIFO_MBOX_MIN_SEQ_NUM);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_cb, NULL);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_ctx, NULL);
    will_return(__wrap_fpfw_icc_base_init, FPFW_STATUS_FAIL);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_sdm_mbx.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status != FPFW_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_sdm_mbx_init__fail4, nullptr, nullptr)
{
    // Set up expectations
    DFWK_THREADX_HOST test_host = {};

    // SDM isolation
    expect_value(__wrap_accel_is_isolation_enabled, accel_type, ACCEL_ID_SDM);
    will_return(__wrap_accel_is_isolation_enabled, false);

    //! Verify wrapped APIs are invoked in order
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    expect_function_call(__wrap_DfwkDeviceInitialize);
    will_return(__wrap_fpfw_mbox_icc_transport_dfwk_device_init, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_mbox_icc_transport_dfwk_interface_init, FPFW_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.is_used, true);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.dispatcher_buffer_size, LARGE_FIFO_MBOX_MAX_MESG_SIZE_BYTES);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.size_bits, LARGE_FIFO_MBOX_CMD_CODE_SIZE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.size_bits, LARGE_FIFO_MBOX_SEQ_NUM_SIZE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.start_pos, LARGE_FIFO_MBOX_CMD_CODE_START_POS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.start_pos, LARGE_FIFO_MBOX_SEQ_NUM_START_POS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_max, LARGE_FIFO_MBOX_MAX_CMD_CODE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_max, LARGE_FIFO_MBOX_MAX_SEQ_NUM);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_min, LARGE_FIFO_MBOX_MIN_CMD_CODE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_min, LARGE_FIFO_MBOX_MIN_SEQ_NUM);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_cb, NULL);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_ctx, NULL);
    will_return(__wrap_fpfw_icc_base_init, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_dispatcher_start, FPFW_STATUS_FAIL);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_sdm_mbx.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status != FPFW_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_cded_mbx_init, nullptr, nullptr)
{
    // Set up expectations
    DFWK_THREADX_HOST test_host = {};

    // CDED isolation
    expect_value(__wrap_accel_is_isolation_enabled, accel_type, ACCEL_ID_CDED);
    will_return(__wrap_accel_is_isolation_enabled, false);

    //! Verify wrapped APIs are invoked in order
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    expect_function_call(__wrap_DfwkDeviceInitialize);
    will_return(__wrap_fpfw_mbox_icc_transport_dfwk_device_init, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_mbox_icc_transport_dfwk_interface_init, FPFW_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.is_used, true);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.dispatcher_buffer_size, LARGE_FIFO_MBOX_MAX_MESG_SIZE_BYTES);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.size_bits, LARGE_FIFO_MBOX_CMD_CODE_SIZE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.size_bits, LARGE_FIFO_MBOX_SEQ_NUM_SIZE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.start_pos, LARGE_FIFO_MBOX_CMD_CODE_START_POS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.start_pos, LARGE_FIFO_MBOX_SEQ_NUM_START_POS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_max, LARGE_FIFO_MBOX_MAX_CMD_CODE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_max, LARGE_FIFO_MBOX_MAX_SEQ_NUM);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_min, LARGE_FIFO_MBOX_MIN_CMD_CODE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_min, LARGE_FIFO_MBOX_MIN_SEQ_NUM);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_cb, NULL);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_ctx, NULL);
    will_return(__wrap_fpfw_icc_base_init, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_dispatcher_start, FPFW_ICC_DISPATCH_STATUS_SUCCESS);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_cded_mbx.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_cded_mbx_init_isolation, nullptr, nullptr)
{
    // CDED isolation
    expect_value(__wrap_accel_is_isolation_enabled, accel_type, ACCEL_ID_CDED);
    will_return(__wrap_accel_is_isolation_enabled, true);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_cded_mbx.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_cded_mbx_init__fail1, nullptr, nullptr)
{
    // CDED isolation
    expect_value(__wrap_accel_is_isolation_enabled, accel_type, ACCEL_ID_CDED);
    will_return(__wrap_accel_is_isolation_enabled, false);

    //! Verify wrapped APIs are invoked in order
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_fpfw_mbox_icc_transport_dfwk_device_init, FPFW_STATUS_FAIL);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_cded_mbx.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status != FPFW_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_cded_mbx_init__fail2, nullptr, nullptr)
{
    // Set up expectations
    DFWK_THREADX_HOST test_host = {};

    // CDED isolation
    expect_value(__wrap_accel_is_isolation_enabled, accel_type, ACCEL_ID_CDED);
    will_return(__wrap_accel_is_isolation_enabled, false);

    //! Verify wrapped APIs are invoked in order
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    expect_function_call(__wrap_DfwkDeviceInitialize);
    will_return(__wrap_fpfw_mbox_icc_transport_dfwk_device_init, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_mbox_icc_transport_dfwk_interface_init, FPFW_STATUS_FAIL);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_cded_mbx.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status != FPFW_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_cded_mbx_init__fail3, nullptr, nullptr)
{
    // Set up expectations
    DFWK_THREADX_HOST test_host = {};

    // CDED isolation
    expect_value(__wrap_accel_is_isolation_enabled, accel_type, ACCEL_ID_CDED);
    will_return(__wrap_accel_is_isolation_enabled, false);

    //! Verify wrapped APIs are invoked in order
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    expect_function_call(__wrap_DfwkDeviceInitialize);
    will_return(__wrap_fpfw_mbox_icc_transport_dfwk_device_init, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_mbox_icc_transport_dfwk_interface_init, FPFW_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.is_used, true);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.dispatcher_buffer_size, LARGE_FIFO_MBOX_MAX_MESG_SIZE_BYTES);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.size_bits, LARGE_FIFO_MBOX_CMD_CODE_SIZE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.size_bits, LARGE_FIFO_MBOX_SEQ_NUM_SIZE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.start_pos, LARGE_FIFO_MBOX_CMD_CODE_START_POS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.start_pos, LARGE_FIFO_MBOX_SEQ_NUM_START_POS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_max, LARGE_FIFO_MBOX_MAX_CMD_CODE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_max, LARGE_FIFO_MBOX_MAX_SEQ_NUM);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_min, LARGE_FIFO_MBOX_MIN_CMD_CODE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_min, LARGE_FIFO_MBOX_MIN_SEQ_NUM);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_cb, NULL);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_ctx, NULL);
    will_return(__wrap_fpfw_icc_base_init, FPFW_STATUS_FAIL);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_cded_mbx.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status != FPFW_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_cded_mbx_init__fail4, nullptr, nullptr)
{
    // Set up expectations
    DFWK_THREADX_HOST test_host = {};

    // CDED isolation
    expect_value(__wrap_accel_is_isolation_enabled, accel_type, ACCEL_ID_CDED);
    will_return(__wrap_accel_is_isolation_enabled, false);

    //! Verify wrapped APIs are invoked in order
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    expect_function_call(__wrap_DfwkDeviceInitialize);
    will_return(__wrap_fpfw_mbox_icc_transport_dfwk_device_init, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_mbox_icc_transport_dfwk_interface_init, FPFW_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.is_used, true);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.dispatcher_buffer_size, LARGE_FIFO_MBOX_MAX_MESG_SIZE_BYTES);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.size_bits, LARGE_FIFO_MBOX_CMD_CODE_SIZE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.size_bits, LARGE_FIFO_MBOX_SEQ_NUM_SIZE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.start_pos, LARGE_FIFO_MBOX_CMD_CODE_START_POS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.start_pos, LARGE_FIFO_MBOX_SEQ_NUM_START_POS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_max, LARGE_FIFO_MBOX_MAX_CMD_CODE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_max, LARGE_FIFO_MBOX_MAX_SEQ_NUM);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_min, LARGE_FIFO_MBOX_MIN_CMD_CODE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_min, LARGE_FIFO_MBOX_MIN_SEQ_NUM);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_cb, NULL);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_ctx, NULL);
    will_return(__wrap_fpfw_icc_base_init, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_dispatcher_start, FPFW_STATUS_FAIL);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_cded_mbx.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status != FPFW_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_mhu_mscp2tfa_if_mcp, nullptr, nullptr)
{
    // Setup expectations
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_mscp2tfa_if.init_fn();
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_mhu_mscp2tfa_if_scp_failed_device, nullptr, nullptr)
{
    // Setup expectations
    DFWK_THREADX_HOST test_host = {};

    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    will_return(__wrap_mhu_icc_transport_device_init, FPFW_ICC_TRANSPORT_STATUS_FAILED_ERR);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_mscp2tfa_if.init_fn();
    assert_true(result.status != FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_mhu_mscp2tfa_if_scp_failed_interface, nullptr, nullptr)
{
    // Setup expectations
    DFWK_THREADX_HOST test_host = {};

    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    will_return(__wrap_mhu_icc_transport_device_init, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    will_return(__wrap_mhu_icc_transport_interface_init, FPFW_ICC_TRANSPORT_STATUS_FAILED_ERR);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_mscp2tfa_if.init_fn();
    assert_true(result.status != FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_mhu_mscp2tfa_if_scp_success, nullptr, nullptr)
{
    // Setup expectations
    DFWK_THREADX_HOST test_host = {};

    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    will_return(__wrap_mhu_icc_transport_device_init, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    will_return(__wrap_mhu_icc_transport_interface_init, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_mscp2tfa_if.init_fn();
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_mhu_mscp2mscp_failed_device, nullptr, nullptr)
{
    // Setup expectations
    DFWK_THREADX_HOST test_host = {};

    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    will_return(__wrap_mhu_icc_transport_device_init, FPFW_ICC_TRANSPORT_STATUS_FAILED_ERR);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_mscp2mscp.init_fn();
    assert_true(result.status != FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_mhu_mscp2mscp_failed_interface, nullptr, nullptr)
{
    // Setup expectations
    DFWK_THREADX_HOST test_host = {};

    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    will_return(__wrap_mhu_icc_transport_device_init, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    will_return(__wrap_mhu_icc_transport_interface_init, FPFW_ICC_TRANSPORT_STATUS_FAILED_ERR);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_mscp2mscp.init_fn();
    assert_true(result.status != FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_mhu_mscp2mscp_success, nullptr, nullptr)
{
    // Setup expectations
    DFWK_THREADX_HOST test_host = {};

    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    will_return(__wrap_mhu_icc_transport_device_init, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    will_return(__wrap_mhu_icc_transport_interface_init, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.is_used, true);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.dispatcher_buffer_size, SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_RECEIVE_SIZE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.size_bits, ICC_MHU_CMD_SIZE_BITS - 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.size_bits, ICC_MHU_TOKEN_SIZE_BITS - 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.start_pos, ICC_MHU_CMD_BIT_OFFSET);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.start_pos, ICC_MHU_TOKEN_BIT_OFFSET);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_max, UINT32_MAX / 2);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_max, UINT32_MAX / 2);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_min, 0);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_min, 0);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_cb, mhu_icc_transport_dispatcher_match_cb);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_ctx, NULL);
    will_return(__wrap_fpfw_icc_base_init, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_dispatcher_start, FPFW_ICC_DISPATCH_STATUS_SUCCESS);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_mscp2mscp.init_fn();
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_mhu_mscp2apns_failed_device, nullptr, nullptr)
{
    // Setup expectations
    DFWK_THREADX_HOST test_host = {};

    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    will_return(__wrap_mhu_icc_transport_device_init, FPFW_ICC_TRANSPORT_STATUS_FAILED_ERR);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_mscp2apns.init_fn();
    assert_true(result.status != FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_mhu_mscp2apns_failed_interface, nullptr, nullptr)
{
    // Setup expectations
    DFWK_THREADX_HOST test_host = {};

    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    will_return(__wrap_mhu_icc_transport_device_init, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    will_return(__wrap_mhu_icc_transport_interface_init, FPFW_ICC_TRANSPORT_STATUS_FAILED_ERR);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_mscp2apns.init_fn();
    assert_true(result.status != FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_mhu_mscp2apns_scp_d0_success, nullptr, nullptr)
{
    // Setup expectations
    DFWK_THREADX_HOST test_host = {};

    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    will_return(__wrap_mhu_icc_transport_device_init, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    will_return(__wrap_mhu_icc_transport_interface_init, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.is_used, true);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.dispatcher_buffer_size, ICC_MHU_DDR_PAYLOAD_SIZE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.size_bits, ICC_MHU_CMD_SIZE_BITS - 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.size_bits, ICC_MHU_TOKEN_SIZE_BITS - 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.start_pos, ICC_MHU_CMD_BIT_OFFSET);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.start_pos, ICC_MHU_TOKEN_BIT_OFFSET);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_max, UINT32_MAX >> 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_max, UINT32_MAX >> 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_min, 0);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_min, 0);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_cb, mhu_icc_transport_dispatcher_match_cb);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_ctx, NULL);
    will_return(__wrap_fpfw_icc_base_init, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_dispatcher_start, FPFW_ICC_DISPATCH_STATUS_SUCCESS);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_mscp2apns.init_fn();
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_mhu_mscp2apns_scp_d1_success, nullptr, nullptr)
{
    // Setup expectations
    DFWK_THREADX_HOST test_host = {};

    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_die_id, DIE_1);
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    will_return(__wrap_mhu_icc_transport_device_init, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    will_return(__wrap_mhu_icc_transport_interface_init, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.is_used, true);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.dispatcher_buffer_size, ICC_MHU_DDR_PAYLOAD_SIZE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.size_bits, ICC_MHU_CMD_SIZE_BITS - 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.size_bits, ICC_MHU_TOKEN_SIZE_BITS - 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.start_pos, ICC_MHU_CMD_BIT_OFFSET);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.start_pos, ICC_MHU_TOKEN_BIT_OFFSET);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_max, UINT32_MAX >> 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_max, UINT32_MAX >> 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_min, 0);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_min, 0);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_cb, mhu_icc_transport_dispatcher_match_cb);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_ctx, NULL);
    will_return(__wrap_fpfw_icc_base_init, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_dispatcher_start, FPFW_ICC_DISPATCH_STATUS_SUCCESS);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_mscp2apns.init_fn();
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_mhu_mscp2apns_mcp_d0_success, nullptr, nullptr)
{
    // Setup expectations
    DFWK_THREADX_HOST test_host = {};

    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    will_return(__wrap_mhu_icc_transport_device_init, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    will_return(__wrap_mhu_icc_transport_interface_init, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.is_used, true);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.dispatcher_buffer_size, ICC_MHU_DDR_PAYLOAD_SIZE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.size_bits, ICC_MHU_CMD_SIZE_BITS - 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.size_bits, ICC_MHU_TOKEN_SIZE_BITS - 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.start_pos, ICC_MHU_CMD_BIT_OFFSET);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.start_pos, ICC_MHU_TOKEN_BIT_OFFSET);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_max, UINT32_MAX >> 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_max, UINT32_MAX >> 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_min, 0);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_min, 0);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_cb, mhu_icc_transport_dispatcher_match_cb);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_ctx, NULL);
    will_return(__wrap_fpfw_icc_base_init, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_dispatcher_start, FPFW_ICC_DISPATCH_STATUS_SUCCESS);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_mscp2apns.init_fn();
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_mhu_mscp2apns_mcp_d1_success, nullptr, nullptr)
{
    // Setup expectations
    DFWK_THREADX_HOST test_host = {};

    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_die_id, DIE_1);
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    will_return(__wrap_mhu_icc_transport_device_init, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    will_return(__wrap_mhu_icc_transport_interface_init, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.is_used, true);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.dispatcher_buffer_size, ICC_MHU_DDR_PAYLOAD_SIZE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.size_bits, ICC_MHU_CMD_SIZE_BITS - 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.size_bits, ICC_MHU_TOKEN_SIZE_BITS - 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.start_pos, ICC_MHU_CMD_BIT_OFFSET);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.start_pos, ICC_MHU_TOKEN_BIT_OFFSET);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_max, UINT32_MAX >> 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_max, UINT32_MAX >> 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_min, 0);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_min, 0);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_cb, mhu_icc_transport_dispatcher_match_cb);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_ctx, NULL);
    will_return(__wrap_fpfw_icc_base_init, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_dispatcher_start, FPFW_ICC_DISPATCH_STATUS_SUCCESS);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_mscp2apns.init_fn();
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_mhu_d2d_single_die, nullptr, nullptr)
{

    will_return(__wrap_idhw_is_single_die_boot_en, true);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_die2die.init_fn();
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_mhu_d2d_failed_device, nullptr, nullptr)
{
    // Setup expectations
    DFWK_THREADX_HOST test_host = {};

    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    will_return(__wrap_mhu_icc_transport_device_init, FPFW_ICC_TRANSPORT_STATUS_FAILED_ERR);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_die2die.init_fn();
    assert_true(result.status != FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_mhu_d2d_failed_interface, nullptr, nullptr)
{
    // Setup expectations
    DFWK_THREADX_HOST test_host = {};

    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    will_return(__wrap_mhu_icc_transport_device_init, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    will_return(__wrap_mhu_icc_transport_interface_init, FPFW_ICC_TRANSPORT_STATUS_FAILED_ERR);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_die2die.init_fn();
    assert_true(result.status != FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_mhu_d2d_scp_d0_success, nullptr, nullptr)
{
    // Setup expectations
    DFWK_THREADX_HOST test_host = {};

    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    will_return(__wrap_mhu_icc_transport_device_init, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    will_return(__wrap_mhu_icc_transport_interface_init, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.is_used, true);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.dispatcher_buffer_size, ICC_MHU_DDR_PAYLOAD_SIZE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.size_bits, ICC_MHU_CMD_SIZE_BITS - 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.size_bits, ICC_MHU_TOKEN_SIZE_BITS - 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.start_pos, ICC_MHU_CMD_BIT_OFFSET);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.start_pos, ICC_MHU_TOKEN_BIT_OFFSET);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_max, UINT32_MAX >> 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_max, UINT32_MAX >> 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_min, 0);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_min, 0);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_cb, mhu_icc_transport_dispatcher_match_cb);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_ctx, NULL);
    will_return(__wrap_fpfw_icc_base_init, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_dispatcher_start, FPFW_ICC_DISPATCH_STATUS_SUCCESS);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_die2die.init_fn();
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_mhu_d2d_scp_d1_success, nullptr, nullptr)
{
    // Setup expectations
    DFWK_THREADX_HOST test_host = {};

    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_die_id, DIE_1);
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    will_return(__wrap_mhu_icc_transport_device_init, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    will_return(__wrap_mhu_icc_transport_interface_init, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.is_used, true);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.dispatcher_buffer_size, ICC_MHU_DDR_PAYLOAD_SIZE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.size_bits, ICC_MHU_CMD_SIZE_BITS - 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.size_bits, ICC_MHU_TOKEN_SIZE_BITS - 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.start_pos, ICC_MHU_CMD_BIT_OFFSET);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.start_pos, ICC_MHU_TOKEN_BIT_OFFSET);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_max, UINT32_MAX >> 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_max, UINT32_MAX >> 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_min, 0);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_min, 0);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_cb, mhu_icc_transport_dispatcher_match_cb);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_ctx, NULL);
    will_return(__wrap_fpfw_icc_base_init, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_dispatcher_start, FPFW_ICC_DISPATCH_STATUS_SUCCESS);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_die2die.init_fn();
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_mhu_d2d_mcp_d0_success, nullptr, nullptr)
{
    // Setup expectations
    DFWK_THREADX_HOST test_host = {};

    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    will_return(__wrap_mhu_icc_transport_device_init, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    will_return(__wrap_mhu_icc_transport_interface_init, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.is_used, true);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.dispatcher_buffer_size, ICC_MHU_DDR_PAYLOAD_SIZE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.size_bits, ICC_MHU_CMD_SIZE_BITS - 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.size_bits, ICC_MHU_TOKEN_SIZE_BITS - 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.start_pos, ICC_MHU_CMD_BIT_OFFSET);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.start_pos, ICC_MHU_TOKEN_BIT_OFFSET);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_max, UINT32_MAX >> 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_max, UINT32_MAX >> 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_min, 0);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_min, 0);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_cb, mhu_icc_transport_dispatcher_match_cb);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_ctx, NULL);
    will_return(__wrap_fpfw_icc_base_init, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_dispatcher_start, FPFW_ICC_DISPATCH_STATUS_SUCCESS);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_die2die.init_fn();
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_mhu_d2d_mcp_d1_success, nullptr, nullptr)
{
    // Setup expectations
    DFWK_THREADX_HOST test_host = {};

    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_die_id, DIE_1);
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    will_return(__wrap_mhu_icc_transport_device_init, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    will_return(__wrap_mhu_icc_transport_interface_init, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.is_used, true);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.dispatcher_buffer_size, ICC_MHU_DDR_PAYLOAD_SIZE);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.size_bits, ICC_MHU_CMD_SIZE_BITS - 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.size_bits, ICC_MHU_TOKEN_SIZE_BITS - 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.start_pos, ICC_MHU_CMD_BIT_OFFSET);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.start_pos, ICC_MHU_TOKEN_BIT_OFFSET);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_max, UINT32_MAX >> 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_max, UINT32_MAX >> 1);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.cmd_code.valid_min, 0);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.strategy.seq_num.valid_min, 0);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_cb, mhu_icc_transport_dispatcher_match_cb);
    expect_value(__wrap_fpfw_icc_base_init, icc_cfg->dispatch_cfg.match_strategy_ctx, NULL);
    will_return(__wrap_fpfw_icc_base_init, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_dispatcher_start, FPFW_ICC_DISPATCH_STATUS_SUCCESS);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_die2die.init_fn();
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}
}