//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scf_mhu_device.c
 * SCF MHU is a Kingsgate platform implementation of a sensor fifo.
 * It implements the abstract sensor_fifo_driver_interface_t for this hardware.
 */

/*------------- Includes -----------------*/

#include "scf_mhu_device.h"

#include "device_fifo_id.h"
#include "hw_fifo.h"
#include "sensor_fifo_driver_interface.h"   // for psensor_fifo_device_prop...
#include "sensor_fifo_driver_interface_i.h" // for (anonymous struct)::(ano...
#include "telemetry_config_struct.h"        // for scf_base_config, telem

#include <DfwkHost.h>    // for DfwkDeviceInitialize
#include <bug_check.h>   // for BUG_ASSERT
#include <fpfw_status.h> // for FPFW_STATUS_SUCCESS, fpf...
#include <scf_mhu_regs.h>
#include <sensor_ram_bridge.h>
#include <silibs_platform.h> // for MMIO_READ32, MMIO_WRITE32
#include <stdbool.h>         // for false, true
#include <stddef.h>          // for size_t
#include <stdint.h>          // for UINT32_MAX, uintptr_t
#include <utils.h>           // for sleep_ms

/*-- Symbolic Constant Macros (defines) --*/
// clang-format off

#define SCF_LOSSY_CONFIG                                            \
    {                                                               \
        .disable_lossy_telemetry                = true,             \
        .temperature_expiration_timer           = 6000,             \
        .temperature_expiration_buffer_timer    = 6000,             \
        .voltage_expiration_timer               = 6000,             \
        .voltage_expiration_buffer_timer        = 6000,             \
        .current_expiration_timer               = 6000,             \
        .current_expiration_buffer_timer        = 6000,             \
    }

// start_address_incl has been defined as the physical fifo start location
// the hardware fifo producers require programming the start address incremented by 8 bytes to reserve space for an automated timestamp
// the firmware fifo producers do not require this, but do so anyway to allow for the same underlying fifo management code to be used
#define DEFAULT_TLM_CFG                                                                                                                                                                          \
    {                                                                                                                                                                                            \
        .temp_telem_start_address            = sp_scf_mhu_device->sensor_fifo_device.fifo_property_table[DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD].start_address_incl + QUADWORD_ADDRESS_SIZE,          \
        .temp_buffer_size                    = sp_scf_mhu_device->sensor_fifo_device.fifo_property_table[DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD].entry_count - 1,                                     \
        .temp_telem_end_address              = sp_scf_mhu_device->sensor_fifo_device.fifo_property_table[DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD].end_address_excl - 1,                                \
        .volt_telem_start_address            = sp_scf_mhu_device->sensor_fifo_device.fifo_property_table[DEVICE_FIFO_TILE_VOLT_TLM_HW_PROD].start_address_incl + QUADWORD_ADDRESS_SIZE,          \
        .volt_buffer_size                    = sp_scf_mhu_device->sensor_fifo_device.fifo_property_table[DEVICE_FIFO_TILE_VOLT_TLM_HW_PROD].entry_count - 1,                                     \
        .volt_telem_end_address              = sp_scf_mhu_device->sensor_fifo_device.fifo_property_table[DEVICE_FIFO_TILE_VOLT_TLM_HW_PROD].end_address_excl - 1,                                \
        .current_telem_start_address         = sp_scf_mhu_device->sensor_fifo_device.fifo_property_table[DEVICE_FIFO_CORE_CURRENT_TLM_HW_PROD].start_address_incl + QUADWORD_ADDRESS_SIZE,       \
        .current_buffer_size                 = sp_scf_mhu_device->sensor_fifo_device.fifo_property_table[DEVICE_FIFO_CORE_CURRENT_TLM_HW_PROD].entry_count - 1,                                  \
        .current_telem_end_address           = sp_scf_mhu_device->sensor_fifo_device.fifo_property_table[DEVICE_FIFO_CORE_CURRENT_TLM_HW_PROD].end_address_excl - 1,                             \
        .pstate_telem_fifo_start_address     = sp_scf_mhu_device->sensor_fifo_device.fifo_property_table[DEVICE_FIFO_PSTATE_TLM_HW_PROD].start_address_incl + QUADWORD_ADDRESS_SIZE,             \
        .pstate_fifo_size                    = (sp_scf_mhu_device->sensor_fifo_device.fifo_property_table[DEVICE_FIFO_PSTATE_TLM_HW_PROD].entry_count / 128) - 1,                                \
        .scp_msg_fifo_start_address          = sp_scf_mhu_device->sensor_fifo_device.fifo_property_table[DEVICE_FIFO_SCP_MSG_TLM_HW_PROD].start_address_incl + QUADWORD_ADDRESS_SIZE,            \
        .scp_msg_fifo_size                   = (sp_scf_mhu_device->sensor_fifo_device.fifo_property_table[DEVICE_FIFO_SCP_MSG_TLM_HW_PROD].entry_count / 128) - 1,                               \
        .soc_pvt_temp_telem_start_address    = {sp_scf_mhu_device->sensor_fifo_device.fifo_property_table[DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD].start_address_incl + QUADWORD_ADDRESS_SIZE, 0,0,0},  \
        .soc_pvt_volt_telem_start_address    = {sp_scf_mhu_device->sensor_fifo_device.fifo_property_table[DEVICE_FIFO_PVT_VOLT_TLM_FW_PROD].start_address_incl + QUADWORD_ADDRESS_SIZE, 0,0,0},  \
        .dimm_temp_start_address             = sp_scf_mhu_device->sensor_fifo_device.fifo_property_table[DEVICE_FIFO_DIMM_TEMP_TLM_FW_PROD].start_address_incl + QUADWORD_ADDRESS_SIZE,          \
        .vr_temp_start_address               = sp_scf_mhu_device->sensor_fifo_device.fifo_property_table[DEVICE_FIFO_VR_TEMP_TLM_FW_PROD].start_address_incl + QUADWORD_ADDRESS_SIZE,            \
        .vr_curr_start_address               = sp_scf_mhu_device->sensor_fifo_device.fifo_property_table[DEVICE_FIFO_VR_CURRENT_TLM_FW_PROD].start_address_incl + QUADWORD_ADDRESS_SIZE,         \
        .tile_mask                           = 0,                                                                                                                   \
        .core_mask_lo                        = 0,                                                                                                                   \
        .core_mask_hi                        = 0,                                                                                                                   \
        .lossy_telem_cfg                     = SCF_LOSSY_CONFIG,                                                                                                    \
        .set_sensor_bridge_enable            = true                                                                                                                 \
    }

// clang-format on

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static void reset_fifos(void);

/*-- Declarations (Statics and globals) --*/
pscf_mhu_device_config_t sp_scf_mhu_device_cfg;
pscf_mhu_device_t sp_scf_mhu_device;

/**
 * @brief register control data.  NOTE: Although, SCF_MHU_*_ADDRESS defines are listed as an address, they
 * are actually an offset from the start of the scf_mhu_regs location. Therefore the table is initialized
 * here with the offset, but then the address of scf_mhu_regs is added in scf_mhu_device_initialize
 *
 */
static sensor_fifo_control_t hw_fifo_control[] = {

    [DEVICE_FIFO_PSTATE_TLM_HW_PROD] =
        {
            .read_pointer_reg_address = SCF_MHU_PSTATE_READ_PTR_ADDRESS,
            .write_pointer_reg_address = SCF_MHU_PSTATE_WRITE_PTR_ADDRESS,
            .enable_reg_address = SCF_MHU_PSTATE_CONFIG_ADDRESS,
            .latched_write_address = UINT32_MAX,
            .enable_mask = SCF_MHU_PSTATE_CONFIG_BUFFER_EN_MASK,
            .enabled = false,
        },
    [DEVICE_FIFO_SCP_MSG_TLM_HW_PROD] =
        {
            .read_pointer_reg_address = SCF_MHU_MESSAGE_READ_PTR_ADDRESS,
            .write_pointer_reg_address = SCF_MHU_MESSAGE_WRITE_PTR_ADDRESS,
            .enable_reg_address = SCF_MHU_MESSAGE_CONFIG_ADDRESS,
            .latched_write_address = UINT32_MAX,
            .enable_mask = SCF_MHU_MESSAGE_CONFIG_MESSAGE_EN_MASK,
            .enabled = false,
        },
    [DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD] =
        {
            .read_pointer_reg_address = SCF_MHU_CSS_TEMP_READ_PTR_ADDRESS,
            .write_pointer_reg_address = SCF_MHU_CSS_TEMP_WRITE_PTR_ADDRESS,
            .enable_reg_address = SCF_MHU_CSS_TEMP_BUF_CONFIG1_ADDRESS,
            .latched_write_address = UINT32_MAX,
            .enable_mask = SCF_MHU_CSS_TEMP_BUF_CONFIG1_BUFFER_EN_MASK,
            .enabled = false,
        },
    [DEVICE_FIFO_TILE_VOLT_TLM_HW_PROD] =
        {
            .read_pointer_reg_address = SCF_MHU_CSS_VOLT_READ_PTR_ADDRESS,
            .write_pointer_reg_address = SCF_MHU_CSS_VOLT_WRITE_PTR_ADDRESS,
            .enable_reg_address = SCF_MHU_CSS_VOLT_BUF_CONFIG1_ADDRESS,
            .latched_write_address = UINT32_MAX,
            .enable_mask = SCF_MHU_CSS_VOLT_BUF_CONFIG1_BUFFER_EN_MASK,
            .enabled = false,
        },
    [DEVICE_FIFO_CORE_CURRENT_TLM_HW_PROD] =
        {
            .read_pointer_reg_address = SCF_MHU_CORE_CURR_READ_PTR_ADDRESS,
            .write_pointer_reg_address = SCF_MHU_CORE_CURR_WRITE_PTR_ADDRESS,
            .enable_reg_address = SCF_MHU_CORE_CURR_BUF_CONFIG1_ADDRESS,
            .latched_write_address = UINT32_MAX,
            .enable_mask = SCF_MHU_CORE_CURR_BUF_CONFIG1_BUFFER_EN_MASK,
            .enabled = false,
        },

    [DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD] =
        {
            .read_pointer_reg_address = SCF_MHU_SOC_TOP_TEMP_BUF_RD_PTR_ADDRESS,
            .write_pointer_reg_address = SCF_MHU_SOC_TOP_TEMP_BUF_WR_PTR_ADDRESS,
            .enable_reg_address = 0, // NA
            .latched_write_address = UINT32_MAX,
            .enable_mask = 0, // NA
            .enabled = false,
        },
    [DEVICE_FIFO_PVT_VOLT_TLM_FW_PROD] =
        {
            .read_pointer_reg_address = SCF_MHU_SOC_TOP_VOLT_BUF_RD_PTR_ADDRESS,
            .write_pointer_reg_address = SCF_MHU_SOC_TOP_VOLT_BUF_WR_PTR_ADDRESS,
            .enable_reg_address = 0, // NA
            .latched_write_address = UINT32_MAX,
            .enable_mask = 0, // NA
            .enabled = false,
        },
    [DEVICE_FIFO_DIMM_TEMP_TLM_FW_PROD] =
        {
            .read_pointer_reg_address = SCF_MHU_DIMM_TEMP_BUF_RD_PTR_ADDRESS,
            .write_pointer_reg_address = SCF_MHU_DIMM_TEMP_BUF_WR_PTR_ADDRESS,
            .enable_reg_address = 0, // NA
            .latched_write_address = UINT32_MAX,
            .enable_mask = 0, // NA
            .enabled = false,
        },
    [DEVICE_FIFO_VR_TEMP_TLM_FW_PROD] =
        {
            .read_pointer_reg_address = SCF_MHU_VR_TEMP_BUF_RD_PTR_ADDRESS,
            .write_pointer_reg_address = SCF_MHU_VR_TEMP_BUF_WR_PTR_ADDRESS,
            .enable_reg_address = 0, // NA
            .latched_write_address = UINT32_MAX,
            .enable_mask = 0, // NA
            .enabled = false,
        },
    [DEVICE_FIFO_VR_CURRENT_TLM_FW_PROD] =
        {
            .read_pointer_reg_address = SCF_MHU_VR_CURR_BUF_RD_PTR_ADDRESS,
            .write_pointer_reg_address = SCF_MHU_VR_CURR_BUF_WR_PTR_ADDRESS,
            .enable_reg_address = 0, // NA
            .latched_write_address = UINT32_MAX,
            .enable_mask = 0, // NA
            .enabled = false,
        },
};

/*------------- Functions ----------------*/

void scf_mhu_device_initialize(pscf_mhu_device_t scf_mhu_device,
                               DFWK_SCHEDULE* schedule,
                               psensor_fifo_device_properties_t property_table,
                               size_t property_table_array_size,
                               pscf_mhu_device_config_t scf_mhu_device_cfg)
{
    BUG_ASSERT_PARAM(property_table_array_size == DEVICE_FIFO_MAX_ID, property_table_array_size, 0);

    DfwkDeviceInitialize(&(scf_mhu_device->sensor_fifo_device.base_device), schedule);
    scf_mhu_device->sensor_fifo_device.initialized = true;
    scf_mhu_device->sensor_fifo_device.dispatch_sync = scf_mhu_request_dispatch_sync;
    scf_mhu_device->sensor_fifo_device.fifo_property_table = property_table;

    hw_fifo_init(property_table, hw_fifo_control);

    sp_scf_mhu_device = scf_mhu_device;
    sp_scf_mhu_device_cfg = scf_mhu_device_cfg;

    // table was initialized with offsets, add base address to them
    for (DEVICE_FIFO_ID id = DEVICE_FIFO_PSTATE_TLM_HW_PROD; id < DEVICE_FIFO_MAX_ID; id++)
    {
        hw_fifo_control[id].read_pointer_reg_address += sp_scf_mhu_device_cfg->scf_mhu_base_address;
        hw_fifo_control[id].write_pointer_reg_address += sp_scf_mhu_device_cfg->scf_mhu_base_address;
        hw_fifo_control[id].enable_reg_address += sp_scf_mhu_device_cfg->scf_mhu_base_address;
    }

    scf_base_config scf_config;
    scf_config.scf_mhu_base_address = sp_scf_mhu_device_cfg->scf_mhu_base_address;
    scf_config.scf_ram_base_address = sp_scf_mhu_device_cfg->scf_ram_base_address;
    scf_config.scf_ram_buffer_size = sp_scf_mhu_device_cfg->scf_ram_buffer_size;
    scf_set_working_config((uintptr_t)&scf_config);

    reset_fifos();
}

static void reset_fifos(void)
{
    if (sp_scf_mhu_device_cfg->is_scp)
    {
        // turn off global enable
        scf_trigger_stop(sp_scf_mhu_device_cfg->scf_exp_csr_base_address);

        sleep_ms(1); // wait for telemetry to flush, 1ms per RMSS HAS

        clear_scf_ram();

        telemetry_common_cfg_t telem_config = DEFAULT_TLM_CFG;

        telem_config.core_mask_hi = sp_scf_mhu_device_cfg->core_mask_hi;
        telem_config.core_mask_lo = sp_scf_mhu_device_cfg->core_mask_lo;
        telem_config.tile_mask = sp_scf_mhu_device_cfg->tile_mask;

        init_scf_mhu(sp_scf_mhu_device_cfg->scf_mhu_base_address, &telem_config);

        // init_scf_mhu() enables the hw fifo's, override and disable, let app enable
        for (DEVICE_FIFO_ID id = DEVICE_FIFO_PSTATE_TLM_HW_PROD; id <= LAST_HW_PROD_FIFO_ID; id++)
        {
            hw_fifo_disable(id);

            // hw leaves write pointer at zero until the first hw producer actually adds data and then updates the write pointer
            // this handles the case when the test mode is entered without enabling the hw producers
            uint32_t read_ptr = MMIO_READ32(hw_fifo_control[id].read_pointer_reg_address);
            MMIO_WRITE32(hw_fifo_control[id].write_pointer_reg_address, read_ptr);
        }
    }
    else // MCP
    {
        hw_fifo_get_enabled_from_hw();
    }
}

fpfw_status_t scf_mhu_request_dispatch_sync(PDFWK_SYNC_REQUEST_HEADER request)
{
    fpfw_status_t status = FPFW_STATUS_SUCCESS;
    switch (request->RequestType)
    {
    case SENSOR_FIFO_SYNC_REQUEST_READ_REG: {
        psensor_fifo_drv_inf_read_reg_request read_req = (psensor_fifo_drv_inf_read_reg_request)request;
        read_req->output.value = MMIO_READ32(read_req->input.address);
        break;
    }

    case SENSOR_FIFO_SYNC_REQUEST_WRITE_REG: {
        psensor_fifo_drv_inf_write_reg_request write_req = (psensor_fifo_drv_inf_write_reg_request)request;
        MMIO_WRITE32(write_req->input.address, write_req->input.value);
        break;
    }

    case SENSOR_FIFO_SYNC_SET_GLOBAL_HW_ENABLE: {
        psensor_fifo_drv_inf_global_enable global_en_req = (psensor_fifo_drv_inf_global_enable)request;
        if (global_en_req->input.enable)
        {
            scf_trigger_start(sp_scf_mhu_device_cfg->scf_exp_csr_base_address);
        }
        else
        {
            scf_trigger_stop(sp_scf_mhu_device_cfg->scf_exp_csr_base_address);
        }
        break;
    }

    case SENSOR_FIFO_SYNC_WRITE_ENTRY: {
        psensor_fifo_drv_inf_write_entry write_entry_req = (psensor_fifo_drv_inf_write_entry)request;
        BUG_ASSERT_PARAM(write_entry_req->input.fifo_id < DEVICE_FIFO_MAX_ID, write_entry_req->input.fifo_id, 0); // check here on entry, not necessary to repeat on static functions
        status = hw_fifo_write_entry(write_entry_req->input.fifo_id,
                                     write_entry_req->input.src_data,
                                     write_entry_req->input.entry_size,
                                     write_entry_req->input.num_entries,
                                     write_entry_req->input.stride_index);
        break;
    }

    case SENSOR_FIFO_SYNC_READ_ENTRY: {
        psensor_fifo_drv_inf_read_entry read_entry_req = (psensor_fifo_drv_inf_read_entry)request;
        BUG_ASSERT_PARAM(read_entry_req->input.fifo_id < DEVICE_FIFO_MAX_ID, read_entry_req->input.fifo_id, 0); // check here on entry, not necessary to repeat on static functions
        status = hw_fifo_read_entry(read_entry_req->input.fifo_id,
                                    read_entry_req->input.entry_size,
                                    read_entry_req->output.dest_data,
                                    read_entry_req->output.num_entries_read,
                                    read_entry_req->output.num_entries_remaining,
                                    read_entry_req->output.stride_index);
        break;
    }

    case SENSOR_FIFO_SYNC_SET_FIFO_ENABLE: {
        psensor_fifo_drv_inf_fifo_enable fifo_enable_req = (psensor_fifo_drv_inf_fifo_enable)request;
        BUG_ASSERT_PARAM(fifo_enable_req->input.fifo_id < DEVICE_FIFO_MAX_ID, fifo_enable_req->input.fifo_id, 0); // check here on entry, not necessary to repeat on static functions
        if (fifo_enable_req->input.enable)
        {
            hw_fifo_enable(fifo_enable_req->input.fifo_id);
        }
        else
        {
            hw_fifo_disable(fifo_enable_req->input.fifo_id);
        }

        // return the current status up the stack
        for (DEVICE_FIFO_ID fifo_id = 0; fifo_id < DEVICE_FIFO_MAX_ID; fifo_id++)
        {
            (*fifo_enable_req->output.is_enabled)[fifo_id] = hw_fifo_is_enabled(fifo_id);
        }
        break;
    }

    case SENSOR_FIFO_SYNC_SYNCHRONIZE_FIFO_ENABLES: {
        psensor_fifo_drv_inf_sync_fifo_enables fifo_enable_req = (psensor_fifo_drv_inf_sync_fifo_enables)request;

        // the hardware fifo's enabled are controlled by hardware, just update the shadow copy from the registers directly
        hw_fifo_get_enabled_from_hw();

        for (DEVICE_FIFO_ID fifo_id = FIRST_FW_PROD_FIFO_ID; fifo_id < DEVICE_FIFO_MAX_ID; fifo_id++)
        {
            if ((*fifo_enable_req->input.is_enabled)[fifo_id])
            {
                hw_fifo_enable(fifo_id);
            }
            else
            {
                hw_fifo_disable(fifo_id);
            }
        }

        break;
    }

    case SENSOR_FIFO_SYNC_UPDATE_WRITE_PTR: {
        psensor_fifo_drv_inf_update_write_stride update_stride_req = (psensor_fifo_drv_inf_update_write_stride)request;
        BUG_ASSERT_PARAM(update_stride_req->input.fifo_id < DEVICE_FIFO_MAX_ID,
                         update_stride_req->input.fifo_id,
                         0); // check here on entry, not necessary to repeat on static functions
        hw_fifo_update_write_ptr_by_stride_size(update_stride_req->input.fifo_id);
        break;
    }

    case SENSOR_FIFO_SYNC_QUERY_IS_ENABLED: {
        psensor_fifo_drv_inf_fifo_is_enabled is_enabled_req = (psensor_fifo_drv_inf_fifo_is_enabled)request;
        hw_fifo_get_enabled_from_hw();
        for (DEVICE_FIFO_ID fifo_id = 0; fifo_id < DEVICE_FIFO_MAX_ID; fifo_id++)
        {
            (*is_enabled_req->output.is_enabled)[fifo_id] = hw_fifo_is_enabled(fifo_id);
        }
        break;
    }

    case SENSOR_FIFO_SYNC_QUERY_IS_EMPTY: {
        psensor_fifo_drv_inf_fifo_is_empty is_empty_req = (psensor_fifo_drv_inf_fifo_is_empty)request;

        for (DEVICE_FIFO_ID fifo_id = 0; fifo_id < DEVICE_FIFO_MAX_ID; fifo_id++)
        {
            (*is_empty_req->output.is_empty)[fifo_id] = hw_fifo_is_empty(fifo_id);
        }
        break;
    }

    case SENSOR_FIFO_SYNC_RESET: {
        reset_fifos();
        break;
    }

    default:
        // No other types of requests are supported
        status = FPFW_STATUS_INVALID_ARGS;
        BUG_ASSERT(false);
        break;
    }
    return status;
}
