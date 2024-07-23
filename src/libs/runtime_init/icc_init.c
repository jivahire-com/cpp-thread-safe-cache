/**
 * @file icc_init.c
 * Instantiates icc base ctx for each transport interface available on the platform
 */

/*------------- Includes -----------------*/
#include <DfwkHost.h>                // for DfwkDeviceInitialize
#include <DfwkStatus.h>              // for DFWK_SUCCESS
#include <DfwkThreadXHost.h>         // for PDFWK_THREADX_HOST
#include <MboxPrimitives.h>          // for FPFW_MBX_FIFO_DEPTH, FPFW_MBX_I...
#include <fpfw_icc_base.h>           // for fpfw_icc_base_init, fpfw_icc_ba...
#include <fpfw_icc_base_i.h>         // for fpfw_icc_base_ctx_t
#include <fpfw_icc_dispatcher.h>     // for fpfw_icc_dispatcher_start
#include <fpfw_init.h>               // for fpfw_init_result_t, fpfw_init_g...
#include <fpfw_mbox_icc_transport.h> // for ICC_MBX_ASYNC_RECV, ICC_MBX_ASY...
#include <fpfw_status.h>             // for FPFW_STATUS_SUCCESS, fpfw_status_t
#include <fpfw_timer_port.h>         // for _fpfw_timer_t
#include <icc_platform_defines.h>
#include <interrupts.h>
#include <silibs_mcp_top_regs.h>
#include <silibs_scp_top_regs.h>
#include <stdbool.h> // for true
#include <stddef.h>  // for NULL
#include <stdint.h>  // for uint32_t, uint8_t

/*-------------- Macros ------------------*/
/**
 * HSP Mailbox Message Format (16 bytes):
 * 0    flag(4 bits)   ctx(4 bits)  seq(8 bits)  cmd(16 bits)
 * 1    data(32 bits)
 * 2    data(32 bits)
 * 3    data(32 bits)
 */
#define KG_HSP_MBOX_POLL_INTERVAL_NS (20000000ULL)
#define HSP_MBOX_MAX_CMD_CODE        (0xFFFFU)
#define HSP_MBOX_MIN_CMD_CODE        (0x1U)
#define HSP_MBOX_CMD_CODE_SIZE       (16U)
#define HSP_MBOX_CMD_CODE_START_POS  (0U) //! 16th bit from lsb
#define HSP_MBOX_MAX_SEQ_NUM         (0xFFU)
#define HSP_MBOX_MIN_SEQ_NUM         (0x0U)
#define HSP_MBOX_SEQ_NUM_SIZE        (8U)
#define HSP_MBOX_SEQ_NUM_START_POS   (16U) //! 24th bit from lsb

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
/**
 * @note ICC Support added for HSP Mailbox
 *
 * Similarly add config for each icc transport interface
 * A single icc base ctx for every transport interface.
 * Transport driver interface is not directly exposed to the users
 * It's recommended all inter core communication goes thru icc base service
 *
 * Steps to initialize an instance of icc base context:
 * 1. Declare the correct configs required for the specific transport
 * 2. Initialize the transport driver (Device & interface init)
 * 3. Call fpfw_icc_base_init() with icc config, this populates the ctx
 * 4. If the base init is successful, call fpfw_icc_dispatcher_start()
 * 5. Finally return the initialized icc base ctx to the user for invoking icc base APIs
 */
FPFW_INIT_COMPONENT(icc_hspmbx, FPFW_INIT_DEPENDENCIES("dfwk"))
{
    //! Statics declarations required for mailbox primitive
    static struct _fpfw_timer_t timer[ICC_MAX_ASYNC_REQ_TYPE] = {};
    static fpfw_mbox_icc_transport_config_t cfg = {
        .mbox_dev_cfg = {.MbxFifoDepth = HSP_MBX_FIFO_DEPTH,
                         .MbxMesgHandlingType = MBX_MESG_HANDLING_SINGLE_MESG_AT_A_TIME,
                         .MbxImplementation = MBX_IMPL_INTERRUPT,
                         .MsgSizeBytes = (HSP_MBX_FIFO_DEPTH * sizeof(uint32_t)),
                         .MbxBaseAddr = MSCP2HSP_MAILBOX_BASE_ADDRESS},
        .timer_period = KG_HSP_MBOX_POLL_INTERVAL_NS,
        .timer_handle = {&timer[ICC_MBX_ASYNC_SEND], &timer[ICC_MBX_ASYNC_RECV]},
        .mbx_irq_num = HW_INT_HSP2MSCP_MBOX_INT,
    };

    //! Statics declarations required for mailbox transport driver
    static fpfw_mbox_icc_transport_device_t mscp_mbx_dev = {};
    static fpfw_mbox_icc_transport_intrf_t mscp_mbx_intf = {};

    //! Statics declarations required for icc base service
    static uint8_t s_dispatch_buff[HSP_MBOX_MAX_MESG_SIZE_BYTES] = {0};
    static fpfw_icc_base_ctx_t s_icc_base_ctx;
    static fpfw_icc_base_config s_icc_cfg = {
        .transport_interface = &mscp_mbx_intf.header,
        .dispatch_cfg =
            {
                .transport_interface = &mscp_mbx_intf,
                .dispatcher_buffer = &s_dispatch_buff,
                .dispatcher_buffer_size = sizeof(s_dispatch_buff),
                .strategy =
                    {
                        .cmd_code =
                            {
                                .is_used = true,
                                .start_pos = HSP_MBOX_CMD_CODE_START_POS,
                                .size_bits = HSP_MBOX_CMD_CODE_SIZE,
                                .valid_max = HSP_MBOX_MAX_CMD_CODE,
                                .valid_min = HSP_MBOX_MIN_CMD_CODE,
                            },
                        .seq_num =
                            {
                                .is_used = true,
                                .start_pos = HSP_MBOX_SEQ_NUM_START_POS,
                                .size_bits = HSP_MBOX_SEQ_NUM_SIZE,
                                .valid_max = HSP_MBOX_MAX_SEQ_NUM,
                                .valid_min = HSP_MBOX_MIN_SEQ_NUM,
                            },
                    },
                .match_strategy_cb = NULL,
                .match_strategy_ctx = NULL,
            },
        .ctx = NULL,
    };

    //! Initialize hsp mailbox transport driver
    fpfw_status_t status = fpfw_mbox_icc_transport_dfwk_device_init(&mscp_mbx_dev, &cfg);
    if (status != DFWK_SUCCESS)
    {
        return (fpfw_init_result_t){status, NULL};
    }
    DfwkDeviceInitialize(&mscp_mbx_dev.header, &((PDFWK_THREADX_HOST)fpfw_init_get_handle("dfwk"))->Schedule);
    status = fpfw_mbox_icc_transport_dfwk_interface_init(&mscp_mbx_dev, &mscp_mbx_intf);
    if (status != DFWK_SUCCESS)
    {
        return (fpfw_init_result_t){status, NULL};
    }

    //! Initialize ICC base ctx for hsp mailbox transport driver
    status = fpfw_icc_base_init(&s_icc_base_ctx, &s_icc_cfg);
    if (status == FPFW_STATUS_SUCCESS)
    {
        //! start the dispatcher to receive data over hsp mailbox
        status = fpfw_icc_dispatcher_start(&s_icc_base_ctx.dispatch_ctx);
        if (status != FPFW_ICC_DISPATCH_STATUS_SUCCESS)
        {
            return (fpfw_init_result_t){status, NULL};
        }
    }
    //! pass in status & ref to ctx to the clients
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &s_icc_base_ctx};
}