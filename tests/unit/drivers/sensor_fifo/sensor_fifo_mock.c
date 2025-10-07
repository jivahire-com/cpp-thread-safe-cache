//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sensor_fifo_mock.c
 * Provide mock functions for sensor fifo driver tests
 */

/*------------- Includes -----------------*/

#include "sensor_fifo_mock.h"

#include "hw_fifo.h"

#include <DfwkClient.h>              // for PDFWK_DEVICE_HEADER, PDFWK_INTERFACE_HEADER
#include <FpFwCMocka.h>              // for check_expected_ptr, mock_type, function_called
#include <stdint.h>                  // for uint32_t, uint64_t, int32_t
#include <telemetry_config_struct.h> // for telemetry_common_cfg_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
uint32_t __real_mmio_read32(volatile uint32_t* addr);
void __real_mmio_write32(volatile uint32_t* addr, uint32_t data);
void __real_mmio_update32(volatile uint32_t* addr, uint32_t data, uint32_t mask);
uint64_t __real_mmio_read64(volatile uint64_t* addr);
void __real_mmio_write64(volatile uint64_t* addr, uint64_t data);
void __wrap_init_scf_mhu(uintptr_t scf_mhu_base_addr, telemetry_common_cfg_t* telem_config);

/*-- Declarations (Statics and globals) --*/
bool snsr_fifo_mock_check_mmio_inputs = true;
bool snsr_fifo_mock_use_real_mmio = false;

sensor_fifo_mem_t fifo_mem;

sensor_fifo_device_properties_t test_fifo_properties[DEVICE_FIFO_MAX_ID] = {
    [DEVICE_FIFO_PSTATE_TLM_HW_PROD] =
        {
            .device_fifo_id = DEVICE_FIFO_PSTATE_TLM_HW_PROD,
            .entry_count = PSTATE_FIFO_NUM_ENTRIES,
            .entry_size_bytes = PSTATE_FIFO_ENTRY_SIZE_BYTES,
            .stride_size_bytes = PSTATE_FIFO_STRIDE_SIZE_BYTES,
            .start_address_incl = (uintptr_t)fifo_mem.pstate_fifo,
            .end_address_excl = (uintptr_t)(fifo_mem.pstate_fifo + sizeof(fifo_mem.pstate_fifo)),
            .name = "PSTATE Fifo",
        },
    [DEVICE_FIFO_SCP_MSG_TLM_HW_PROD] =
        {
            .device_fifo_id = DEVICE_FIFO_SCP_MSG_TLM_HW_PROD,
            .entry_count = SCP_MSG_FIFO_NUM_ENTRIES,
            .entry_size_bytes = SCP_MSG_FIFO_ENTRY_SIZE_BYTES,
            .stride_size_bytes = SCP_MSG_FIFO_STRIDE_SIZE_BYTES,
            .start_address_incl = (uintptr_t)fifo_mem.msg_fifo,
            .end_address_excl = (uintptr_t)(fifo_mem.msg_fifo + sizeof(fifo_mem.msg_fifo)),
            .name = "SCP Msg Fifo",
        },
    [DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD] =
        {
            .device_fifo_id = DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD,
            .entry_count = TILE_TEMP_FIFO_NUM_ENTRIES,
            .entry_size_bytes = TILE_TEMP_FIFO_ENTRY_SIZE_BYTES,
            .stride_size_bytes = TILE_TEMP_FIFO_STRIDE_SIZE_BYTES,
            .start_address_incl = (uintptr_t)fifo_mem.tile_temp_fifo,
            .end_address_excl = (uintptr_t)(fifo_mem.tile_temp_fifo + sizeof(fifo_mem.tile_temp_fifo)),
            .name = "Tile Temperature Fifo",
        },
    [DEVICE_FIFO_TILE_VOLT_TLM_HW_PROD] =
        {
            .device_fifo_id = DEVICE_FIFO_TILE_VOLT_TLM_HW_PROD,
            .entry_count = TILE_VOLT_FIFO_NUM_ENTRIES,
            .entry_size_bytes = TILE_VOLT_FIFO_ENTRY_SIZE_BYTES,
            .stride_size_bytes = TILE_VOLT_FIFO_STRIDE_SIZE_BYTES,
            .start_address_incl = (uintptr_t)fifo_mem.tile_volt_fifo,
            .end_address_excl = (uintptr_t)(fifo_mem.tile_volt_fifo + sizeof(fifo_mem.tile_volt_fifo)),
            .name = "Tile Voltage Fifo",
        },
    [DEVICE_FIFO_CORE_CURRENT_TLM_HW_PROD] =
        {
            .device_fifo_id = DEVICE_FIFO_CORE_CURRENT_TLM_HW_PROD,
            .entry_count = CORE_CURRENT_FIFO_NUM_ENTRIES,
            .entry_size_bytes = CORE_CURRENT_FIFO_ENTRY_SIZE_BYTES,
            .stride_size_bytes = CORE_CURRENT_FIFO_STRIDE_SIZE_BYTES,
            .start_address_incl = (uintptr_t)fifo_mem.core_current_fifo,
            .end_address_excl = (uintptr_t)(fifo_mem.core_current_fifo + sizeof(fifo_mem.core_current_fifo)),
            .name = "Core Current Fifo",
        },
    [DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD] =
        {
            .device_fifo_id = DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD,
            .entry_count = PVT_TEMP_FIFO_NUM_ENTRIES,
            .entry_size_bytes = PVT_TEMP_FIFO_ENTRY_SIZE_BYTES,
            .stride_size_bytes = PVT_TEMP_FIFO_STRIDE_SIZE_BYTES,
            .start_address_incl = (uintptr_t)fifo_mem.pvt_temp_fifo,
            .end_address_excl = (uintptr_t)(fifo_mem.pvt_temp_fifo + sizeof(fifo_mem.pvt_temp_fifo)),
            .name = "PVT Temperature Fifo",
        },

    [DEVICE_FIFO_PVT_VOLT_TLM_FW_PROD] =
        {
            .device_fifo_id = DEVICE_FIFO_PVT_VOLT_TLM_FW_PROD,
            .entry_count = PVT_VOLT_FIFO_NUM_ENTRIES,
            .entry_size_bytes = PVT_VOLT_FIFO_ENTRY_SIZE_BYTES,
            .stride_size_bytes = PVT_VOLT_FIFO_STRIDE_SIZE_BYTES,
            .start_address_incl = (uintptr_t)fifo_mem.pvt_volt_fifo,
            .end_address_excl = (uintptr_t)(fifo_mem.pvt_volt_fifo + sizeof(fifo_mem.pvt_volt_fifo)),
            .name = "PVT Voltage Fifo",
        },
    [DEVICE_FIFO_DIMM_TEMP_TLM_FW_PROD] =
        {
            .device_fifo_id = DEVICE_FIFO_DIMM_TEMP_TLM_FW_PROD,
            .entry_count = DIMM_FIFO_NUM_ENTRIES,
            .entry_size_bytes = DIMM_FIFO_ENTRY_SIZE_BYTES,
            .stride_size_bytes = DIMM_FIFO_STRIDE_SIZE_BYTES,
            .start_address_incl = (uintptr_t)fifo_mem.dimm_temp_fifo,
            .end_address_excl = (uintptr_t)(fifo_mem.dimm_temp_fifo + sizeof(fifo_mem.dimm_temp_fifo)),
            .name = "DIMM Fifo",
        },
    [DEVICE_FIFO_VR_TEMP_TLM_FW_PROD] =
        {
            .device_fifo_id = DEVICE_FIFO_VR_TEMP_TLM_FW_PROD,
            .entry_count = VR_TEMP_FIFO_NUM_ENTRIES,
            .entry_size_bytes = VR_TEMP_FIFO_ENTRY_SIZE_BYTES,
            .stride_size_bytes = VR_TEMP_FIFO_STRIDE_SIZE_BYTES,
            .start_address_incl = (uintptr_t)fifo_mem.vr_temp_fifo,
            .end_address_excl = (uintptr_t)(fifo_mem.vr_temp_fifo + sizeof(fifo_mem.vr_temp_fifo)),
            .name = "VR Temp Fifo",
        },
    [DEVICE_FIFO_VR_CURRENT_TLM_FW_PROD] =
        {
            .device_fifo_id = DEVICE_FIFO_VR_CURRENT_TLM_FW_PROD,
            .entry_count = VR_CURRENT_FIFO_NUM_ENTRIES,
            .entry_size_bytes = VR_CURRENT_FIFO_ENTRY_SIZE_BYTES,
            .stride_size_bytes = VR_CURRENT_FIFO_STRIDE_SIZE_BYTES,
            .start_address_incl = (uintptr_t)fifo_mem.vr_curr_fifo,
            .end_address_excl = (uintptr_t)(fifo_mem.vr_curr_fifo + sizeof(fifo_mem.vr_curr_fifo)),
            .name = "VR Current Fifo",
        },
};

uint32_t pstate_fifo_ctrl_reg = 0;
uint32_t msg_fifo_ctrl_reg = 0;
uint32_t tile_temp_fifo_ctrl_reg = 0;
uint32_t tile_volt_fifo_ctrl_reg = 0;
uint32_t core_current_fifo_ctrl_reg = 0;

uint32_t pstate_fifo_read_reg = (uint32_t)fifo_mem.pstate_fifo + FIFO_TIMESTAMP_SIZE;
uint32_t msg_fifo_read_reg = (uint32_t)fifo_mem.msg_fifo + FIFO_TIMESTAMP_SIZE;
uint32_t tile_temp_fifo_read_reg = (uint32_t)fifo_mem.tile_temp_fifo + FIFO_TIMESTAMP_SIZE;
uint32_t tile_volt_fifo_read_reg = (uint32_t)fifo_mem.tile_volt_fifo + FIFO_TIMESTAMP_SIZE;
uint32_t core_current_fifo_read_reg = (uint32_t)fifo_mem.core_current_fifo + FIFO_TIMESTAMP_SIZE;
uint32_t pvt_temp_fifo_read_reg = (uint32_t)fifo_mem.pvt_temp_fifo + FIFO_TIMESTAMP_SIZE;
uint32_t pvt_volt_fifo_read_reg = (uint32_t)fifo_mem.pvt_volt_fifo + FIFO_TIMESTAMP_SIZE;
uint32_t dimm_temp_fifo_read_reg = (uint32_t)fifo_mem.dimm_temp_fifo + FIFO_TIMESTAMP_SIZE;
uint32_t vr_temp_fifo_read_reg = (uint32_t)fifo_mem.vr_temp_fifo + FIFO_TIMESTAMP_SIZE;
uint32_t vr_curr_fifo_read_reg = (uint32_t)fifo_mem.vr_curr_fifo + FIFO_TIMESTAMP_SIZE;

uint32_t pstate_fifo_write_reg = (uint32_t)fifo_mem.pstate_fifo + FIFO_TIMESTAMP_SIZE;
uint32_t msg_fifo_write_reg = (uint32_t)fifo_mem.msg_fifo + FIFO_TIMESTAMP_SIZE;
uint32_t tile_temp_fifo_write_reg = (uint32_t)fifo_mem.tile_temp_fifo + FIFO_TIMESTAMP_SIZE;
uint32_t tile_volt_fifo_write_reg = (uint32_t)fifo_mem.tile_volt_fifo + FIFO_TIMESTAMP_SIZE;
uint32_t core_current_fifo_write_reg = (uint32_t)fifo_mem.core_current_fifo + FIFO_TIMESTAMP_SIZE;
uint32_t pvt_temp_fifo_write_reg = (uint32_t)fifo_mem.pvt_temp_fifo + FIFO_TIMESTAMP_SIZE;
uint32_t pvt_volt_fifo_write_reg = (uint32_t)fifo_mem.pvt_volt_fifo + FIFO_TIMESTAMP_SIZE;
uint32_t dimm_temp_fifo_write_reg = (uint32_t)fifo_mem.dimm_temp_fifo + FIFO_TIMESTAMP_SIZE;
uint32_t vr_temp_fifo_write_reg = (uint32_t)fifo_mem.vr_temp_fifo + FIFO_TIMESTAMP_SIZE;
uint32_t vr_curr_fifo_write_reg = (uint32_t)fifo_mem.vr_curr_fifo + FIFO_TIMESTAMP_SIZE;

sensor_fifo_control_t test_hw_fifo_control[] = {

    [DEVICE_FIFO_PSTATE_TLM_HW_PROD] =
        {
            .read_pointer_reg_address = (uintptr_t)&pstate_fifo_read_reg,
            .write_pointer_reg_address = (uintptr_t)&pstate_fifo_write_reg,
            .enable_reg_address = (uintptr_t)&pstate_fifo_ctrl_reg,
            .latched_write_address = UINT32_MAX,
            .enable_mask = PSTATE_FIFO_CTRL_EN_MASK,
            .enabled = false,
        },
    [DEVICE_FIFO_SCP_MSG_TLM_HW_PROD] =
        {
            .read_pointer_reg_address = (uintptr_t)&msg_fifo_read_reg,
            .write_pointer_reg_address = (uintptr_t)&msg_fifo_write_reg,
            .enable_reg_address = (uintptr_t)&msg_fifo_ctrl_reg,
            .latched_write_address = UINT32_MAX,
            .enable_mask = SCP_MSG_FIFO_CTRL_EN_MASK,
            .enabled = false,
        },
    [DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD] =
        {
            .read_pointer_reg_address = (uintptr_t)&tile_temp_fifo_read_reg,
            .write_pointer_reg_address = (uintptr_t)&tile_temp_fifo_write_reg,
            .enable_reg_address = (uintptr_t)&tile_temp_fifo_ctrl_reg,
            .latched_write_address = UINT32_MAX,
            .enable_mask = TILE_TEMP_FIFO_CTRL_EN_MASK,
            .enabled = false,
        },
    [DEVICE_FIFO_TILE_VOLT_TLM_HW_PROD] =
        {
            .read_pointer_reg_address = (uintptr_t)&tile_volt_fifo_read_reg,
            .write_pointer_reg_address = (uintptr_t)&tile_volt_fifo_write_reg,
            .enable_reg_address = (uintptr_t)&tile_volt_fifo_ctrl_reg,
            .latched_write_address = UINT32_MAX,
            .enable_mask = TILE_VOLT_FIFO_CTRL_EN_MASK,
            .enabled = false,
        },
    [DEVICE_FIFO_CORE_CURRENT_TLM_HW_PROD] =
        {
            .read_pointer_reg_address = (uintptr_t)&core_current_fifo_read_reg,
            .write_pointer_reg_address = (uintptr_t)&core_current_fifo_write_reg,
            .enable_reg_address = (uintptr_t)&core_current_fifo_ctrl_reg,
            .latched_write_address = UINT32_MAX,
            .enable_mask = CORE_CURRENT_FIFO_CTRL_EN_MASK,
            .enabled = false,
        },
    [DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD] =
        {
            .read_pointer_reg_address = (uintptr_t)&pvt_temp_fifo_read_reg,
            .write_pointer_reg_address = (uintptr_t)&pvt_temp_fifo_write_reg,
            .enable_reg_address = 0,
            .latched_write_address = UINT32_MAX,
            .enable_mask = PVT_TEMP_FIFO_CTRL_EN_MASK,
            .enabled = false,
        },
    [DEVICE_FIFO_PVT_VOLT_TLM_FW_PROD] =
        {
            .read_pointer_reg_address = (uintptr_t)&pvt_volt_fifo_read_reg,
            .write_pointer_reg_address = (uintptr_t)&pvt_volt_fifo_write_reg,
            .enable_reg_address = 0,
            .latched_write_address = UINT32_MAX,
            .enable_mask = PVT_VOLT_FIFO_CTRL_EN_MASK,
            .enabled = false,
        },
    [DEVICE_FIFO_DIMM_TEMP_TLM_FW_PROD] =
        {
            .read_pointer_reg_address = (uintptr_t)&dimm_temp_fifo_read_reg,
            .write_pointer_reg_address = (uintptr_t)&dimm_temp_fifo_write_reg,
            .enable_reg_address = 0,
            .latched_write_address = UINT32_MAX,
            .enable_mask = DIMM_FIFO_CTRL_EN_MASK,
            .enabled = false,
        },
    [DEVICE_FIFO_VR_TEMP_TLM_FW_PROD] =
        {
            .read_pointer_reg_address = (uintptr_t)&vr_temp_fifo_read_reg,
            .write_pointer_reg_address = (uintptr_t)&vr_temp_fifo_write_reg,
            .enable_reg_address = 0,
            .latched_write_address = UINT32_MAX,
            .enable_mask = VR_TEMP_FIFO_CTRL_EN_MASK,
            .enabled = false,
        },
    [DEVICE_FIFO_VR_CURRENT_TLM_FW_PROD] =
        {
            .read_pointer_reg_address = (uintptr_t)&vr_curr_fifo_read_reg,
            .write_pointer_reg_address = (uintptr_t)&vr_curr_fifo_write_reg,
            .enable_reg_address = 0,
            .latched_write_address = UINT32_MAX,
            .enable_mask = VR_CURRENT_FIFO_CTRL_EN_MASK,
            .enabled = false,
        },
};

/*------------- Functions ----------------*/

void initialize_mock_fifos(void)
{
    pstate_fifo_ctrl_reg = 0;
    msg_fifo_ctrl_reg = 0;
    tile_temp_fifo_ctrl_reg = 0;
    tile_volt_fifo_ctrl_reg = 0;
    core_current_fifo_ctrl_reg = 0;

    pstate_fifo_read_reg = (uint32_t)fifo_mem.pstate_fifo + FIFO_TIMESTAMP_SIZE;
    msg_fifo_read_reg = (uint32_t)fifo_mem.msg_fifo + FIFO_TIMESTAMP_SIZE;
    tile_temp_fifo_read_reg = (uint32_t)fifo_mem.tile_temp_fifo + FIFO_TIMESTAMP_SIZE;
    tile_volt_fifo_read_reg = (uint32_t)fifo_mem.tile_volt_fifo + FIFO_TIMESTAMP_SIZE;
    core_current_fifo_read_reg = (uint32_t)fifo_mem.core_current_fifo + FIFO_TIMESTAMP_SIZE;
    pvt_temp_fifo_read_reg = (uint32_t)fifo_mem.pvt_temp_fifo + FIFO_TIMESTAMP_SIZE;
    pvt_volt_fifo_read_reg = (uint32_t)fifo_mem.pvt_volt_fifo + FIFO_TIMESTAMP_SIZE;
    dimm_temp_fifo_read_reg = (uint32_t)fifo_mem.dimm_temp_fifo + FIFO_TIMESTAMP_SIZE;
    vr_temp_fifo_read_reg = (uint32_t)fifo_mem.vr_temp_fifo + FIFO_TIMESTAMP_SIZE;
    vr_curr_fifo_read_reg = (uint32_t)fifo_mem.vr_curr_fifo + FIFO_TIMESTAMP_SIZE;

    pstate_fifo_write_reg = (uint32_t)fifo_mem.pstate_fifo + FIFO_TIMESTAMP_SIZE;
    msg_fifo_write_reg = (uint32_t)fifo_mem.msg_fifo + FIFO_TIMESTAMP_SIZE;
    tile_temp_fifo_write_reg = (uint32_t)fifo_mem.tile_temp_fifo + FIFO_TIMESTAMP_SIZE;
    tile_volt_fifo_write_reg = (uint32_t)fifo_mem.tile_volt_fifo + FIFO_TIMESTAMP_SIZE;
    core_current_fifo_write_reg = (uint32_t)fifo_mem.core_current_fifo + FIFO_TIMESTAMP_SIZE;
    pvt_temp_fifo_write_reg = (uint32_t)fifo_mem.pvt_temp_fifo + FIFO_TIMESTAMP_SIZE;
    pvt_volt_fifo_write_reg = (uint32_t)fifo_mem.pvt_volt_fifo + FIFO_TIMESTAMP_SIZE;
    dimm_temp_fifo_write_reg = (uint32_t)fifo_mem.dimm_temp_fifo + FIFO_TIMESTAMP_SIZE;
    vr_temp_fifo_write_reg = (uint32_t)fifo_mem.vr_temp_fifo + FIFO_TIMESTAMP_SIZE;
    vr_curr_fifo_write_reg = (uint32_t)fifo_mem.vr_curr_fifo + FIFO_TIMESTAMP_SIZE;

    hw_fifo_init(test_fifo_properties, test_hw_fifo_control);

    for (uint32_t fifo_id = DEVICE_FIFO_PSTATE_TLM_HW_PROD; fifo_id < DEVICE_FIFO_MAX_ID; fifo_id++)
    {
        hw_fifo_disable((DEVICE_FIFO_ID)fifo_id);

        test_hw_fifo_control[fifo_id].overflow_count = 0;
        test_hw_fifo_control[fifo_id].write_errors = 0;
        test_hw_fifo_control[fifo_id].read_errors = 0;
    }
}

void __wrap_DfwkAsyncRequestComplete(PDFWK_ASYNC_REQUEST_HEADER Request)
{
    check_expected_ptr(Request);
}

void __wrap_DfwkInterfaceInitialize(PDFWK_INTERFACE_HEADER Interface,
                                    PDFWK_DEVICE_HEADER Device,
                                    PDFWK_QUEUE DispatchQueue,
                                    DFWK_REQUEST_DISPATCH_SYNC DispatchSync)
{

    check_expected_ptr(Interface);
    check_expected_ptr(Device);
    check_expected_ptr(DispatchQueue);
    check_expected_ptr(DispatchSync);
}

// wrapper function that checks expected values for the two incoming parameters
void __wrap_DfwkDeviceInitialize(PDFWK_DEVICE_HEADER Device, PDFWK_SCHEDULE Schedule)
{
    check_expected_ptr(Device);
    check_expected_ptr(Schedule);
}

void __wrap_DfwkQueueInitialize(PDFWK_QUEUE Queue,
                                PDFWK_DEVICE_HEADER Device,
                                DFWK_ASYNC_REQUEST_DISPATCH DispatchRoutine,
                                void* DispatchContext,
                                DFWK_QUEUE_TYPE QueueType)
{
    check_expected_ptr(Queue);
    check_expected_ptr(Device);
    check_expected(DispatchRoutine);
    check_expected_ptr(DispatchContext);
    check_expected(QueueType);
}

int32_t __wrap_DfwkInterfaceSendSync(PDFWK_INTERFACE_HEADER Interface, PDFWK_SYNC_REQUEST_HEADER Request)
{
    check_expected_ptr(Interface);
    check_expected_ptr(Request);
    return mock_type(int32_t);
}

void __wrap_FpFwAssert(int expression)
{
    check_expected(expression);
}

uint32_t __wrap_mmio_read32(volatile uint32_t* addr)
{
    assert_non_null(addr);

    if (snsr_fifo_mock_use_real_mmio)
    {
        return __real_mmio_read32(addr);
    }
    else
    {
        if (snsr_fifo_mock_check_mmio_inputs)
        {
            check_expected_ptr(addr);
        }

        return mock_type(uint32_t);
    }
}

void __wrap_mmio_write32(volatile uint32_t* addr, uint32_t data)
{
    assert_non_null(addr);
    if (snsr_fifo_mock_use_real_mmio)
    {
        __real_mmio_write32(addr, data);
    }
    else
    {
        if (snsr_fifo_mock_check_mmio_inputs)
        {
            check_expected_ptr(addr);
            check_expected(data);
        }
    }
}

void __wrap_mmio_update32(volatile uint32_t* addr, uint32_t data, uint32_t mask)
{
    assert_non_null(addr);

    if (snsr_fifo_mock_use_real_mmio)
    {
        __real_mmio_update32(addr, data, mask);
    }
    else
    {
        if (snsr_fifo_mock_check_mmio_inputs)
        {
            check_expected_ptr(addr);
            check_expected(data);
            check_expected(mask);
        }
    }
}

uint64_t __wrap_mmio_read64(volatile uint64_t* addr)
{
    assert_non_null(addr);
    if (snsr_fifo_mock_use_real_mmio)
    {
        return __real_mmio_read64(addr);
    }
    else
    {
        if (snsr_fifo_mock_check_mmio_inputs)
        {
            check_expected_ptr(addr);
        }
        return mock_type(uint64_t);
    }
}

void __wrap_mmio_write64(volatile uint64_t* addr, uint64_t data)
{
    assert_non_null(addr);

    if (snsr_fifo_mock_use_real_mmio)
    {
        __real_mmio_write64(addr, data);
    }
    else
    {
        if (snsr_fifo_mock_check_mmio_inputs)
        {
            check_expected_ptr(addr);
            check_expected(data);
        }
    }
}

void __wrap_scf_trigger_start(uintptr_t scp_exp_csr_base_address)
{
    check_expected(scp_exp_csr_base_address);
}

void __wrap_scf_trigger_stop(uintptr_t scp_exp_csr_base_address)
{
    check_expected(scp_exp_csr_base_address);
}

void __wrap_init_scf_mhu(uintptr_t scf_mhu_base_addr, telemetry_common_cfg_t* telem_config)
{
    assert_non_null(scf_mhu_base_addr);
    assert_non_null(telem_config);

    function_called();
}
