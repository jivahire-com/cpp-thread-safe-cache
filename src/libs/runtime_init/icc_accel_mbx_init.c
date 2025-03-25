/**
 * @file icc_accel_mbx_init.c
 * Instantiates icc base ctx for accel mailbox transport interface for SCP platform
 */

/*------------- Includes -----------------*/

#include <DbgPrint.h>
#include <DfwkHost.h>        // for DfwkDeviceInitialize
#include <DfwkThreadXHost.h> // for PDFWK_THREADX_HOST
#include <MboxPrimitives.h>
#include <accel_intr.h>
#include <accel_intr_virt_irq.h>
#include <accelip_id.h>    // for ACCEL_ID_CDED, ACCEL_ID_SDM
#include <atu_init.h>      // for atu_svc_accel_atu_addr
#include <fpfw_icc_base.h> // for fpfw_icc_base_ctx_t
#include <fpfw_icc_base_i.h>
#include <fpfw_init.h>               // for FPFW_INIT_STATUS_E_INVALID_NODE
#include <fpfw_mbox_icc_transport.h> // for ICC_MAX_ASYNC_REQ_TYPE
#include <fpfw_timer.h>              // for fpfw_timer_t
#include <fpfw_timer_port.h>
#include <icc_platform_defines.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <interrupts.h>
#include <kng_soc_constants.h>
#include <sdm_ext_cfg_regs.h>
#include <stddef.h> // for NULL
#include <stdio.h>  // for printf

/*-------------- Macros ------------------*/

/*------------- Typedefs -----------------*/

#define ACCEL_MBOX_OFFSET (SDM_EXT_CFG__ADDRESSBLOCK_0X100000_ADDRESS + ACCEL_MBOX_OFFSET_AFTER_0X100000)

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static struct _fpfw_timer_t accel_mbx_timer[NUM_VALID_ACCEL_ID][ICC_MAX_ASYNC_REQ_TYPE] = {};
static fpfw_mbox_icc_transport_config_t accel_mbx_cfg[NUM_VALID_ACCEL_ID] = {};

static fpfw_mbox_icc_transport_device_t accel_mbx_dev[NUM_VALID_ACCEL_ID] = {};
static fpfw_mbox_icc_transport_intrf_t accel_mbx_intf[NUM_VALID_ACCEL_ID] = {};

static uint8_t s_accel_mbx_dispatch_buff[NUM_VALID_ACCEL_ID][LARGE_FIFO_MBOX_MAX_MESG_SIZE_BYTES] = {};
static fpfw_icc_base_ctx_t s_accel_mbx_icc_base_ctx[NUM_VALID_ACCEL_ID] = {};
static fpfw_icc_base_config s_accel_mbx_icc_cfg[NUM_VALID_ACCEL_ID] = {};

const uint32_t accel_irq_num[NUM_VALID_ACCEL_ID] = {
    [ACCEL_ID_SDM] = GET_VIRTUAL_IRQ(HW_INT_SDM_COMB_INT, SDM_EXT_MBX_I2E_INTR, SDM_DOMAIN),
    [ACCEL_ID_CDED] = GET_VIRTUAL_IRQ(HW_INT_CDED_COMB_INT, SDM_EXT_MBX_I2E_INTR, CDED_SDM_DOMAIN),
};

/*------------- Static Functions ----------------*/

/**
 * @note ICC Support added for Large FIFO Mailbox
 *
 * Add config for each icc transport interface .A single icc base ctx for every
 * transport interface. Transport driver interface is not directly exposed to
 * the users. It's recommended all inter core communication goes thru icc base
 * service
 *
 * Steps to initialize an instance of icc base context:
 * 1. Declare the correct configs required for the specific transport
 * 2. Initialize the transport driver (Device & interface init)
 * 3. Call fpfw_icc_base_init() with icc config, this populates the ctx
 * 4. If the base init is successful, call fpfw_icc_dispatcher_start()
 * 5. Finally return the initialized icc base ctx to the user for invoking icc base APIs
 */
static fpfw_status_t accel_mbox_init(ACCEL_ID accel_type)
{
    uint32_t mbox_base_addr = atu_svc_accel_atu_addr(accel_type);

    accel_mbx_cfg[accel_type].mbox_dev_cfg.MbxFifoDepth = LARGE_MBX_FIFO_DEPTH;
    accel_mbx_cfg[accel_type].mbox_dev_cfg.MbxMesgHandlingType = MBX_MESG_HANDLING_SINGLE_MESG_AT_A_TIME;
    accel_mbx_cfg[accel_type].mbox_dev_cfg.MbxImplementation = MBX_IMPL_INTERRUPT;
    accel_mbx_cfg[accel_type].mbox_dev_cfg.MsgSizeBytes = LARGE_FIFO_MBOX_MAX_MESG_SIZE_BYTES;
    accel_mbx_cfg[accel_type].mbox_dev_cfg.MbxBaseAddr = mbox_base_addr + ACCEL_MBOX_OFFSET;
    accel_mbx_cfg[accel_type].timer_period = KG_LARGE_FIFO_MBOX_POLL_INTERVAL_NS;
    accel_mbx_cfg[accel_type].timer_handle[ICC_MBX_ASYNC_SEND] = &accel_mbx_timer[accel_type][ICC_MBX_ASYNC_SEND];
    accel_mbx_cfg[accel_type].timer_handle[ICC_MBX_ASYNC_RECV] = &accel_mbx_timer[accel_type][ICC_MBX_ASYNC_RECV];
    accel_mbx_cfg[accel_type].mbx_irq_num = accel_irq_num[accel_type];

    s_accel_mbx_icc_cfg[accel_type].transport_interface = &accel_mbx_intf[accel_type].header;
    s_accel_mbx_icc_cfg[accel_type].dispatch_cfg.transport_interface = &accel_mbx_intf[accel_type];
    s_accel_mbx_icc_cfg[accel_type].dispatch_cfg.dispatcher_buffer = &s_accel_mbx_dispatch_buff[accel_type];
    s_accel_mbx_icc_cfg[accel_type].dispatch_cfg.dispatcher_buffer_size = sizeof(s_accel_mbx_dispatch_buff[accel_type]);
    s_accel_mbx_icc_cfg[accel_type].dispatch_cfg.strategy.cmd_code.is_used = true;
    s_accel_mbx_icc_cfg[accel_type].dispatch_cfg.strategy.cmd_code.start_pos = LARGE_FIFO_MBOX_CMD_CODE_START_POS;
    s_accel_mbx_icc_cfg[accel_type].dispatch_cfg.strategy.cmd_code.size_bits = LARGE_FIFO_MBOX_CMD_CODE_SIZE;
    s_accel_mbx_icc_cfg[accel_type].dispatch_cfg.strategy.cmd_code.valid_max = LARGE_FIFO_MBOX_MAX_CMD_CODE;
    s_accel_mbx_icc_cfg[accel_type].dispatch_cfg.strategy.cmd_code.valid_min = LARGE_FIFO_MBOX_MIN_CMD_CODE;
    s_accel_mbx_icc_cfg[accel_type].dispatch_cfg.strategy.seq_num.is_used = true;
    s_accel_mbx_icc_cfg[accel_type].dispatch_cfg.strategy.seq_num.start_pos = LARGE_FIFO_MBOX_SEQ_NUM_START_POS;
    s_accel_mbx_icc_cfg[accel_type].dispatch_cfg.strategy.seq_num.size_bits = LARGE_FIFO_MBOX_SEQ_NUM_SIZE;
    s_accel_mbx_icc_cfg[accel_type].dispatch_cfg.strategy.seq_num.valid_max = LARGE_FIFO_MBOX_MAX_SEQ_NUM;
    s_accel_mbx_icc_cfg[accel_type].dispatch_cfg.strategy.seq_num.valid_min = LARGE_FIFO_MBOX_MIN_SEQ_NUM;
    s_accel_mbx_icc_cfg[accel_type].dispatch_cfg.match_strategy_cb = NULL;
    s_accel_mbx_icc_cfg[accel_type].dispatch_cfg.match_strategy_ctx = NULL;
    s_accel_mbx_icc_cfg[accel_type].ctx = NULL;

    //! Initialize large fifo mailbox transport driver
    fpfw_status_t status =
        fpfw_mbox_icc_transport_dfwk_device_init(&accel_mbx_dev[accel_type], &accel_mbx_cfg[accel_type]);
    if (status != DFWK_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR("fpfw_mbox_icc_transport_dfwk_device_init failed for accel %d with status %ld\n",
                            accel_type,
                            (long int)status);
        return status;
    }

    DfwkDeviceInitialize(&accel_mbx_dev[accel_type].header,
                         &((PDFWK_THREADX_HOST)fpfw_init_get_handle("dfwk"))->Schedule);
    status = fpfw_mbox_icc_transport_dfwk_interface_init(&accel_mbx_dev[accel_type], &accel_mbx_intf[accel_type]);
    if (status != DFWK_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR(
            "fpfw_mbox_icc_transport_dfwk_interface_init failed for accel %d with status %ld\n",
            accel_type,
            (long int)status);
        return status;
    }

    //! Initialize ICC base ctx for large fifo mailbox transport driver
    status = fpfw_icc_base_init(&s_accel_mbx_icc_base_ctx[accel_type], &s_accel_mbx_icc_cfg[accel_type]);
    if (status != FPFW_STATUS_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR("fpfw_icc_base_init failed for accel %d with status %ld\n", accel_type, (long int)status);
        return status;
    }

    //! start the dispatcher to receive data over large fifo mailbox
    status = fpfw_icc_dispatcher_start(&s_accel_mbx_icc_base_ctx[accel_type].dispatch_ctx);
    if (status != FPFW_ICC_DISPATCH_STATUS_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR("fpfw_icc_dispatcher_start failed for accel %d with status %ld\n", accel_type, (long int)status);
        return status;
    }

    return FPFW_INIT_STATUS_SUCCESS;
}

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(icc_sdm_mbx, FPFW_INIT_DEPENDENCIES("dfwk", "hw_ver", "accel_atu", "debug_print", "virt_irq"))
{
    ACCEL_ID accel_type = ACCEL_ID_SDM;

    fpfw_status_t status = accel_mbox_init(accel_type);
    if (status != FPFW_INIT_STATUS_SUCCESS)
    {
        return (fpfw_init_result_t){status, NULL};
    }

    //! pass in status & ref to ctx to the clients
    return (fpfw_init_result_t){status, &s_accel_mbx_icc_base_ctx[accel_type]};
}

FPFW_INIT_COMPONENT(icc_cded_mbx, FPFW_INIT_DEPENDENCIES("dfwk", "hw_ver", "accel_atu", "debug_print", "virt_irq"))
{
    ACCEL_ID accel_type = ACCEL_ID_CDED;

    fpfw_status_t status = accel_mbox_init(accel_type);
    if (status != FPFW_INIT_STATUS_SUCCESS)
    {
        return (fpfw_init_result_t){status, NULL};
    }

    //! pass in status & ref to ctx to the clients
    return (fpfw_init_result_t){status, &s_accel_mbx_icc_base_ctx[accel_type]};
}
