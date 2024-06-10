/**
 * @file test_mscp_hspmbx_init.cpp
 * Tests initialization of ICC transports for both SCP/MCP cores
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for TEST_FUNCTION, mock_type, will_return
#include <cstdint>         // for uint32_t

extern "C" {
#include <DfwkHost.h>                // for PDFWK_DEVICE_HEADER, PDFWK_SCHE...
#include <DfwkThreadXHost.h>         // for DFWK_THREADX_HOST
#include <FpFwUtils.h>               // for FPFW_UNUSED
#include <MboxPrimitives.h>          // for FPFW_MBX_REG_CONFIG, HSP_MBX_FI...
#include <fpfw_init.h>               // for fpfw_init_result_t, fpfw_init_c...
#include <fpfw_mbox_icc_transport.h> // for fpfw_mbox_icc_transport_config_t
#include <fpfw_status.h>             // for FPFW_STATUS_SUCCESS, fpfw_status_t

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_hspmbox;
extern fpfw_init_component_t _fpfw_component_icc_mbx;

/*------------- Mock Functions ----------------*/

void __wrap_DfwkDeviceInitialize(PDFWK_DEVICE_HEADER Device, PDFWK_SCHEDULE Schedule)
{
    assert_non_null(Device);
    check_expected_ptr(Schedule);
    function_called();
}

void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    FPFW_UNUSED(id);
    return mock_type(void*);
}

fpfw_status_t __wrap_fpfw_mbox_icc_transport_dfwk_device_init(fpfw_mbox_icc_transport_device_t* dev,
                                                              fpfw_mbox_icc_transport_config_t* cfg)
{
    assert_non_null(cfg);
    assert_non_null(dev);
    assert_int_equal(cfg->mbox_dev_cfg.MbxFifoDepth, HSP_MBX_FIFO_DEPTH);
    assert_int_equal(cfg->mbox_dev_cfg.MbxBaseAddr, 1000);
    assert_int_equal(cfg->mbox_dev_cfg.MbxMesgHandlingType, MBX_MESG_HANDLING_SINGLE_MESG_AT_A_TIME_FIXED_SIZE);
    assert_int_equal(cfg->mbox_dev_cfg.MbxImplementation, MBX_IMPL_POLLING);
    assert_int_equal(cfg->mbox_dev_cfg.MsgSizeBytes, HSP_MBX_FIFO_DEPTH * sizeof(uint32_t));
    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_mbox_icc_transport_dfwk_interface_init(fpfw_mbox_icc_transport_device_t* dev,
                                                                 fpfw_mbox_icc_transport_intrf_t* intrf)
{
    assert_non_null(intrf);
    check_expected_ptr(dev);
    return mock_type(fpfw_status_t);
}

/*------------- Test cases ----------------*/

TEST_FUNCTION(test_icc_transport_mscp_hspmbx_init, nullptr, nullptr)
{
    // Set up expectations
    DFWK_THREADX_HOST test_host = {};

    will_return(__wrap_fpfw_init_get_handle, &test_host);
    expect_value(__wrap_DfwkDeviceInitialize, Schedule, &(test_host.Schedule));
    expect_function_call(__wrap_DfwkDeviceInitialize);
    will_return(__wrap_fpfw_mbox_icc_transport_dfwk_device_init, FPFW_STATUS_SUCCESS);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_hspmbox.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

TEST_FUNCTION(test_icc_mbx_init, nullptr, nullptr)
{
    // Set up expectations
    fpfw_mbox_icc_transport_device_t test_dev = {};
    will_return(__wrap_fpfw_init_get_handle, &test_dev);
    expect_value(__wrap_fpfw_mbox_icc_transport_dfwk_interface_init, dev, &test_dev);
    will_return(__wrap_fpfw_mbox_icc_transport_dfwk_interface_init, FPFW_STATUS_SUCCESS);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_icc_mbx.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

// Add more test cases if needed
}