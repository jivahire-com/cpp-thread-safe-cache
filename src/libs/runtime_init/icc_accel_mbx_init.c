/**
 * @file icc_accel_mbx_init.c
 * Instantiates icc base ctx for accel mailbox transport interface for SCP platform
 */

/*------------- Includes -----------------*/

#include <DfwkHost.h>                   // for DfwkDeviceInitialize
#include <DfwkThreadXHost.h>            // for PDFWK_THREADX_HOST
#include <MboxPrimitives.h>
#include <accel_intr.h>
#include <accelerator_ip.h>
#include <fpfw_icc_base.h>              // for fpfw_icc_base_ctx_t
#include <fpfw_icc_base_i.h>
#include <fpfw_init.h>                  // for FPFW_INIT_STATUS_E_INVALID_NODE
#include <fpfw_mbox_icc_transport.h>    // for ICC_MAX_ASYNC_REQ_TYPE               
#include <fpfw_timer.h>                 // for fpfw_timer_t
#include <fpfw_timer_port.h>
#include <icc_platform_defines.h>
#include <idsw.h>
#include <idsw_kng.h>   
#include <interrupts.h>
#include <sdm_ext_cfg_regs.h>  
#include <stddef.h>                     // for NULL


/*-------------- Macros ------------------*/

/*------------- Typedefs -----------------*/

#define SDM_MBOX_OFFSET      (SDM_EXT_CFG__ADDRESSBLOCK_0X100000_ADDRESS + SDM_MBOX_OFFSET_AFTER_0X100000)

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

/**
 * @note ICC Support added for Large FIFO Mailbox
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
FPFW_INIT_COMPONENT(icc_sdm_mbx, FPFW_INIT_DEPENDENCIES("dfwk", "hw_ver", "accel"))
{
    //! Statics declarations required for mailbox primitive
    static struct _fpfw_timer_t sdm_scp_mbx_timer[ICC_MAX_ASYNC_REQ_TYPE] = {};

    static fpfw_mbox_icc_transport_config_t sdm_scp_mbx_cfg = {
        .mbox_dev_cfg = {
            .MbxFifoDepth = LARGE_MBX_FIFO_DEPTH,
            .MbxMesgHandlingType = MBX_MESG_HANDLING_SINGLE_MESG_AT_A_TIME,
            .MbxImplementation = MBX_IMPL_INTERRUPT,
            .MsgSizeBytes = LARGE_FIFO_MBOX_MAX_MESG_SIZE_BYTES,
            /* This is populated later in the code since it depends of ATU mapping base address */
            .MbxBaseAddr = 0,
        },
        .timer_period = KG_HSP_MBOX_POLL_INTERVAL_NS,
        .timer_handle = {
            &sdm_scp_mbx_timer[ICC_MBX_ASYNC_SEND],
            &sdm_scp_mbx_timer[ICC_MBX_ASYNC_RECV]
        },
        .mbx_irq_num = HW_INT_SDM_MBOX_INT,
    };

    //! Statics declarations required for mailbox transport driver
    static fpfw_mbox_icc_transport_device_t sdm_scp_mbx_dev = {};
    static fpfw_mbox_icc_transport_intrf_t sdm_scp_mbx_intf = {};

    //! Statics declarations required for icc base service
    static uint8_t s_sdm_scp_mbx_dispatch_buff[LARGE_FIFO_MBOX_MAX_MESG_SIZE_BYTES] = {0};
    static fpfw_icc_base_ctx_t s_sdm_scp_mbx_icc_base_ctx;
    static fpfw_icc_base_config s_sdm_scp_mbx_icc_cfg = {
        .transport_interface = &sdm_scp_mbx_intf.header,
        .dispatch_cfg = {
            .transport_interface = &sdm_scp_mbx_intf,
            .dispatcher_buffer = &s_sdm_scp_mbx_dispatch_buff,
            .dispatcher_buffer_size = sizeof(s_sdm_scp_mbx_dispatch_buff),
            .strategy = {
                .cmd_code = {
                    .is_used = true,
                    .start_pos = HSP_MBOX_CMD_CODE_START_POS,
                    .size_bits = HSP_MBOX_CMD_CODE_SIZE,
                    .valid_max = HSP_MBOX_MAX_CMD_CODE,
                    .valid_min = HSP_MBOX_MIN_CMD_CODE,
                },
                .seq_num = {
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

    sdm_scp_mbx_cfg.mbox_dev_cfg.MbxBaseAddr = accelerator_ip_get_atu_mapped_cfg_address(ACCELERATOR_SDMSS) + SDM_MBOX_OFFSET;
    //! Initialize hsp mailbox transport driver
    fpfw_status_t status = fpfw_mbox_icc_transport_dfwk_device_init(&sdm_scp_mbx_dev, &sdm_scp_mbx_cfg);
    if (status != DFWK_SUCCESS)
    {
        return (fpfw_init_result_t){status, NULL};
    }
    DfwkDeviceInitialize(&sdm_scp_mbx_dev.header, &((PDFWK_THREADX_HOST)fpfw_init_get_handle("dfwk"))->Schedule);
    status = fpfw_mbox_icc_transport_dfwk_interface_init(&sdm_scp_mbx_dev, &sdm_scp_mbx_intf);
    if (status != DFWK_SUCCESS)
    {
        return (fpfw_init_result_t){status, NULL};
    }

    //! Initialize ICC base ctx for hsp mailbox transport driver
    status = fpfw_icc_base_init(&s_sdm_scp_mbx_icc_base_ctx, &s_sdm_scp_mbx_icc_cfg);
    if (status == FPFW_STATUS_SUCCESS)
    {
        //! start the dispatcher to receive data over hsp mailbox
        status = fpfw_icc_dispatcher_start(&s_sdm_scp_mbx_icc_base_ctx.dispatch_ctx);
        if (status != FPFW_ICC_DISPATCH_STATUS_SUCCESS)
        {
            return (fpfw_init_result_t){status, NULL};
        }
    } else {
        return (fpfw_init_result_t){status, NULL};
    }


    /**
     * Setup callback context for this mailbox.
     * This context will be passed ICC stack interrupt handler
     */
    accel_intr_set_mbx_ctx(ACCELERATOR_SDMSS, &sdm_scp_mbx_dev);
    /**
     * ICC stack register's its own CB on the shared NVIC interrupt line
     * Overwrite that CB with accel_intr lib CB so that interrupt comes to
     * accel_intr lib and later it can be routed to ICC stack
     */
    status = accel_intr_init(ACCELERATOR_SDMSS);
    if (status)
    {
        return (fpfw_init_result_t){status, NULL};
    }

    //! pass in status & ref to ctx to the clients
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &s_sdm_scp_mbx_icc_base_ctx};
}
